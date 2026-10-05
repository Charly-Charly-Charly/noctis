#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <optional>

class QNetworkAccessManager;
class QNetworkReply;

namespace noctis::updates {

struct ReleaseInfo {
    QString version; // sin el prefijo "v": "0.3.0"
    QUrl url;        // página del release, para descargar el zip
};

// true si `candidate` es una versión estrictamente posterior a `current`.
// Compara MAJOR.MINOR.PATCH numéricamente (0.10.0 > 0.9.0) e ignora el
// prefijo "v" y cualquier sufijo ("-beta"). Una versión ilegible nunca es
// "más nueva", para no ofrecer una actualización por un error de formato.
bool isNewerVersion(const QString& candidate, const QString& current);

// Extrae la versión y la URL de la respuesta de GitHub (releases/latest).
// nullopt si no es JSON válido, es borrador/prerelease, el tag no es una
// versión, o la URL no es https.
std::optional<ReleaseInfo> parseLatestRelease(const QByteArray& json);

// Pregunta a GitHub cuál es el último release publicado. Asíncrono: no
// bloquea la ventana. Solo informa; descargar/instalar lo hace el usuario.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    static QUrl defaultEndpoint();

    // `endpoint` se puede cambiar para probar contra un servidor local.
    explicit UpdateChecker(QString currentVersion, QUrl endpoint = defaultEndpoint(),
                           QObject* parent = nullptr);

    const QString& currentVersion() const { return currentVersion_; }

    // Lanza la consulta; si ya hay una en curso no hace nada.
    void check();

signals:
    void updateAvailable(const QString& version, const QUrl& url);
    void upToDate();
    void failed(const QString& reason);

private:
    void handleFinished();

    QString currentVersion_;
    QUrl endpoint_;
    QNetworkAccessManager* network_;
    QNetworkReply* reply_ = nullptr;
};

} // namespace noctis::updates
