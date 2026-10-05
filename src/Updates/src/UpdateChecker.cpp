#include "Updates/UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

#include <array>

namespace noctis::updates {

namespace {

constexpr int kTimeoutMs = 10000;

std::optional<std::array<int, 3>> parseVersion(const QString& text) {
    static const QRegularExpression pattern(QStringLiteral(R"(^\s*[vV]?(\d+)\.(\d+)\.(\d+))"));
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch()) return std::nullopt;
    return std::array<int, 3>{match.captured(1).toInt(), match.captured(2).toInt(),
                              match.captured(3).toInt()};
}

} // namespace

bool isNewerVersion(const QString& candidate, const QString& current) {
    const auto candidateParts = parseVersion(candidate);
    const auto currentParts = parseVersion(current);
    if (!candidateParts || !currentParts) return false;
    return *candidateParts > *currentParts;
}

std::optional<ReleaseInfo> parseLatestRelease(const QByteArray& json) {
    const QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject()) return std::nullopt;
    const QJsonObject release = document.object();

    if (release.value("draft").toBool() || release.value("prerelease").toBool()) {
        return std::nullopt;
    }

    const QString tag = release.value("tag_name").toString();
    if (!parseVersion(tag)) return std::nullopt;

    const QUrl url(release.value("html_url").toString());
    if (!url.isValid() || url.scheme() != QLatin1String("https")) return std::nullopt;

    QString version = tag.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) version.remove(0, 1);
    return ReleaseInfo{version, url};
}

QUrl UpdateChecker::defaultEndpoint() {
    return QUrl(QStringLiteral(
        "https://api.github.com/repos/Charly-Charly-Charly/noctis/releases/latest"));
}

UpdateChecker::UpdateChecker(QString currentVersion, QUrl endpoint, QObject* parent)
    : QObject(parent),
      currentVersion_(std::move(currentVersion)),
      endpoint_(std::move(endpoint)),
      network_(new QNetworkAccessManager(this)) {}

void UpdateChecker::check() {
    if (reply_) return;

    QNetworkRequest request(endpoint_);
    // GitHub rechaza las peticiones sin User-Agent. No se envía nada más que
    // el nombre y la versión de la app.
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Noctis/%1").arg(currentVersion_));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(kTimeoutMs);

    reply_ = network_->get(request);
    connect(reply_, &QNetworkReply::finished, this, &UpdateChecker::handleFinished);
}

void UpdateChecker::handleFinished() {
    QNetworkReply* reply = reply_;
    reply_ = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit failed(reply->errorString());
        return;
    }

    const auto release = parseLatestRelease(reply->readAll());
    if (!release) {
        emit failed(tr("Respuesta inesperada del servidor de actualizaciones."));
        return;
    }

    if (isNewerVersion(release->version, currentVersion_)) {
        emit updateAvailable(release->version, release->url);
    } else {
        emit upToDate();
    }
}

} // namespace noctis::updates
