#pragma once

#include <string>
#include <vector>

#include "Core/Ports/ISyncClient.h"

namespace noctis::sync {

// Implementación real de ISyncClient contra la API descrita en
// server/README.md, usando WinHTTP: no agrega ninguna dependencia externa
// más allá de lo que ya trae Windows.
//
// baseUrl no debe terminar en "/", p. ej. "https://notas.tudominio.com" para
// el servidor real o "http://localhost:8089" para pruebas locales.
class HttpSyncClient : public core::ISyncClient {
public:
    explicit HttpSyncClient(std::string baseUrl);

    core::SyncSession login(const std::string& email, const std::string& password,
                             const std::string& deviceName) override;
    void setAuthToken(const std::string& token) override;
    core::PullResult pull(core::SyncRevision since) override;
    core::PushResult push(const std::vector<core::NoteChange>& changes) override;

private:
    struct HttpResponse {
        int status = 0;
        std::string body;
    };

    // method y path van en ASCII/UTF-8; internamente se convierten a UTF-16
    // porque esa es la unidad de trabajo de WinHTTP.
    HttpResponse request(const std::wstring& method, const std::wstring& pathAndQuery,
                          const std::string& jsonBody) const;

    std::string baseUrl_; // solo para mensajes de error legibles
    std::wstring host_;
    std::wstring pathPrefix_;
    unsigned short port_ = 0;
    bool useTls_ = true;
    std::string token_;
};

} // namespace noctis::sync
