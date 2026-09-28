#include "Sync/HttpSyncClient.h"

// windows.h antes que winhttp.h, y con las macros de min/max desactivadas
// para no chocar con <algorithm>: es el orden que exige el propio SDK.
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <nlohmann/json.hpp>

namespace noctis::sync {

namespace {

std::wstring toWide(const std::string& text) {
    if (text.empty()) return {};
    int size =
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(),
                         size);
    return result;
}

// RAII para HINTERNET: WinHTTP no libera nada solo, y con excepciones a
// mitad de una petición es fácil dejar handles abiertos si se hace a mano.
class WinHttpHandle {
public:
    WinHttpHandle() = default;
    explicit WinHttpHandle(HINTERNET handle) : handle_(handle) {}
    ~WinHttpHandle() {
        if (handle_) WinHttpCloseHandle(handle_);
    }

    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;

    WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    HINTERNET get() const { return handle_; }
    explicit operator bool() const { return handle_ != nullptr; }

private:
    HINTERNET handle_ = nullptr;
};

} // namespace

HttpSyncClient::HttpSyncClient(std::string baseUrl) : baseUrl_(std::move(baseUrl)) {
    std::wstring wideUrl = toWide(baseUrl_);

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);

    wchar_t hostBuffer[256]{};
    wchar_t pathBuffer[1024]{};
    components.lpszHostName = hostBuffer;
    components.dwHostNameLength = ARRAYSIZE(hostBuffer);
    components.lpszUrlPath = pathBuffer;
    components.dwUrlPathLength = ARRAYSIZE(pathBuffer);

    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &components)) {
        throw core::SyncError("URL de servidor inválida: " + baseUrl_);
    }

    host_ = hostBuffer;
    port_ = components.nPort;
    useTls_ = components.nScheme == INTERNET_SCHEME_HTTPS;

    pathPrefix_ = pathBuffer;
    if (!pathPrefix_.empty() && pathPrefix_.back() == L'/') {
        pathPrefix_.pop_back();
    }
}

void HttpSyncClient::setAuthToken(const std::string& token) {
    token_ = token;
}

HttpSyncClient::HttpResponse HttpSyncClient::request(const std::wstring& method,
                                                       const std::wstring& pathAndQuery,
                                                       const std::string& jsonBody) const {
    WinHttpHandle session(WinHttpOpen(L"Noctis/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        throw core::SyncError("No se pudo iniciar la conexión HTTP");
    }

    WinHttpHandle connection(WinHttpConnect(session.get(), host_.c_str(), port_, 0));
    if (!connection) {
        throw core::SyncError("No se pudo conectar a " + baseUrl_);
    }

    DWORD flags = useTls_ ? WINHTTP_FLAG_SECURE : 0;
    std::wstring fullPath = pathPrefix_ + pathAndQuery;

    WinHttpHandle requestHandle(WinHttpOpenRequest(connection.get(), method.c_str(),
                                                     fullPath.c_str(), nullptr, WINHTTP_NO_REFERER,
                                                     WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!requestHandle) {
        throw core::SyncError("No se pudo preparar la petición HTTP");
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!token_.empty()) {
        headers += L"Authorization: Bearer " + toWide(token_) + L"\r\n";
    }

    LPVOID body = jsonBody.empty() ? WINHTTP_NO_REQUEST_DATA
                                    : static_cast<LPVOID>(const_cast<char*>(jsonBody.data()));
    DWORD bodyLength = static_cast<DWORD>(jsonBody.size());

    BOOL sent = WinHttpSendRequest(requestHandle.get(), headers.c_str(),
                                    static_cast<DWORD>(headers.size()), body, bodyLength,
                                    bodyLength, 0);
    if (!sent) {
        // Motivo típico: no hay red, o el host no resuelve. El código exacto
        // de GetLastError() no le sirve al usuario; el mensaje sí.
        throw core::SyncError("No se pudo enviar la petición: revisa tu conexión a internet");
    }

    if (!WinHttpReceiveResponse(requestHandle.get(), nullptr)) {
        throw core::SyncError("El servidor no respondió");
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(requestHandle.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                         WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                         WINHTTP_NO_HEADER_INDEX);

    std::string responseBody;
    DWORD available = 0;
    do {
        available = 0;
        if (!WinHttpQueryDataAvailable(requestHandle.get(), &available)) break;
        if (available == 0) break;

        std::string chunk(available, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(requestHandle.get(), chunk.data(), available, &read)) break;
        chunk.resize(read);
        responseBody += chunk;
    } while (available > 0);

    return HttpResponse{static_cast<int>(statusCode), responseBody};
}

namespace {

nlohmann::json parseJsonOrThrow(int status, const std::string& body) {
    nlohmann::json parsed = nlohmann::json::object();
    if (!body.empty()) {
        try {
            parsed = nlohmann::json::parse(body);
        } catch (const nlohmann::json::parse_error&) {
            throw core::SyncError("Respuesta inválida del servidor (HTTP " +
                                   std::to_string(status) + ")");
        }
    }

    if (status < 200 || status >= 300) {
        std::string message = parsed.is_object() ? parsed.value("error", std::string("Error del servidor"))
                                                   : std::string("Error del servidor");
        throw core::SyncError(message + " (HTTP " + std::to_string(status) + ")");
    }

    return parsed;
}

// Lee un campo opcional que en JSON puede ser string o null: refleja
// exactamente el color/offset_hint que devuelve la API de anotaciones.
template <typename T>
std::optional<T> optionalField(const nlohmann::json& item, const char* key) {
    if (!item.contains(key) || item.at(key).is_null()) return std::nullopt;
    return item.at(key).get<T>();
}

} // namespace

core::SyncSession HttpSyncClient::login(const std::string& email, const std::string& password,
                                         const std::string& deviceName) {
    nlohmann::json payload = {
        {"email", email},
        {"password", password},
        {"device_name", deviceName},
    };

    HttpResponse response = request(L"POST", L"/auth/login", payload.dump());
    nlohmann::json parsed = parseJsonOrThrow(response.status, response.body);

    core::SyncSession session;
    session.token = parsed.at("token").get<std::string>();
    session.deviceId = parsed.at("device_id").get<std::string>();
    session.userId = parsed.at("user").at("id").get<std::string>();
    session.email = parsed.at("user").at("email").get<std::string>();
    return session;
}

core::PullResult HttpSyncClient::pull(core::SyncRevision since) {
    HttpResponse response =
        request(L"GET", L"/sync/pull?since=" + std::to_wstring(since), "");
    nlohmann::json parsed = parseJsonOrThrow(response.status, response.body);

    core::PullResult result;

    for (const auto& item : parsed.at("notes")) {
        core::RemoteNote note;
        note.id = item.at("id").get<std::string>();
        note.path = item.at("path").get<std::string>();
        note.title = item.at("title").get<std::string>();
        note.content = item.value("content", std::string{});
        note.rev = item.at("rev").get<core::SyncRevision>();
        note.deleted = item.value("deleted", false);
        result.notes.push_back(std::move(note));
    }

    for (const auto& item : parsed.at("annotations")) {
        core::RemoteAnnotation annotation;
        annotation.id = item.at("id").get<std::string>();
        annotation.noteId = item.at("note_id").get<std::string>();
        annotation.body = item.value("body", std::string{});
        annotation.quote = item.value("quote", std::string{});
        annotation.prefix = item.value("prefix", std::string{});
        annotation.suffix = item.value("suffix", std::string{});
        annotation.offsetHint = optionalField<int>(item, "offset_hint");
        annotation.contentChecksum = item.value("content_checksum", std::string{});
        annotation.color = optionalField<std::string>(item, "color");
        annotation.rev = item.at("rev").get<core::SyncRevision>();
        annotation.deleted = item.value("deleted", false);
        result.annotations.push_back(std::move(annotation));
    }

    result.rev = parsed.at("rev").get<core::SyncRevision>();
    result.hasMore = parsed.value("has_more", false);
    return result;
}

core::PushResult HttpSyncClient::push(const std::vector<core::NoteChange>& changes) {
    nlohmann::json notesJson = nlohmann::json::array();
    for (const core::NoteChange& change : changes) {
        nlohmann::json item = {
            {"id", change.id},
            {"path", change.path},
            {"title", change.title},
            {"base_rev", change.baseRev},
        };
        if (change.deleted) {
            item["deleted"] = true;
        } else {
            item["content"] = change.content;
        }
        notesJson.push_back(std::move(item));
    }

    nlohmann::json payload = {{"notes", notesJson}};
    HttpResponse response = request(L"POST", L"/sync/push", payload.dump());
    nlohmann::json parsed = parseJsonOrThrow(response.status, response.body);

    core::PushResult result;

    for (const auto& item : parsed.at("applied")) {
        result.applied.push_back(core::AppliedChange{
            item.at("id").get<std::string>(),
            item.at("rev").get<core::SyncRevision>(),
        });
    }

    for (const auto& item : parsed.at("conflicts")) {
        result.conflicts.push_back(core::PushConflict{
            item.at("id").get<std::string>(),
            item.at("server_rev").get<core::SyncRevision>(),
            item.at("base_rev").get<core::SyncRevision>(),
        });
    }

    result.rev = parsed.at("rev").get<core::SyncRevision>();
    return result;
}

} // namespace noctis::sync
