#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "Updates/UpdateChecker.h"

using namespace noctis::updates;

namespace {

QCoreApplication& app() {
    static int argc = 1;
    static char name[] = "noctis_updates_tests";
    static char* argv[] = {name, nullptr};
    static QCoreApplication application(argc, argv);
    // Los servidores de prueba son locales: sin esto, la primera petición
    // espera varios segundos a que Windows resuelva el proxy del sistema.
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    return application;
}

// Servidor HTTP mínimo en localhost que responde siempre lo mismo.
class FakeServer : public QObject {
public:
    FakeServer(int status, QByteArray body) : status_(status), body_(std::move(body)) {
        app(); // el servidor necesita el despachador de eventos de la aplicación
        connect(&server_, &QTcpServer::newConnection, this, [this] {
            QTcpSocket* socket = server_.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                request_ += socket->readAll();
                if (!request_.contains("\r\n\r\n")) return;
                const QByteArray response =
                    "HTTP/1.1 " + QByteArray::number(status_) +
                    " X\r\nContent-Type: application/json\r\nContent-Length: " +
                    QByteArray::number(body_.size()) + "\r\nConnection: close\r\n\r\n" + body_;
                socket->write(response);
                socket->disconnectFromHost();
            });
        });
        EXPECT_TRUE(server_.listen(QHostAddress::LocalHost));
    }

    QUrl url() const { return QUrl(QString("http://127.0.0.1:%1/latest").arg(server_.serverPort())); }
    const QByteArray& request() const { return request_; }

private:
    QTcpServer server_;
    int status_;
    QByteArray body_;
    QByteArray request_;
};

enum class Outcome { None, Available, UpToDate, Failed };

struct Result {
    Outcome outcome = Outcome::None;
    QString version;
    QUrl url;
};

// Lanza la consulta y espera (con tope) a que termine con alguna señal.
Result runCheck(const QString& current, const QUrl& endpoint) {
    app();
    UpdateChecker checker(current, endpoint);
    Result result;
    QEventLoop loop;
    QObject::connect(&checker, &UpdateChecker::updateAvailable, &loop,
                     [&](const QString& v, const QUrl& u) {
                         result = {Outcome::Available, v, u};
                         loop.quit();
                     });
    QObject::connect(&checker, &UpdateChecker::upToDate, &loop, [&] {
        result.outcome = Outcome::UpToDate;
        loop.quit();
    });
    QObject::connect(&checker, &UpdateChecker::failed, &loop, [&](const QString&) {
        result.outcome = Outcome::Failed;
        loop.quit();
    });
    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    checker.check();
    loop.exec();
    return result;
}

QByteArray releaseJson(const QString& tag, const QString& url = "https://github.com/x/y/releases/tag/v9",
                       bool prerelease = false, bool draft = false) {
    return QString(R"({"tag_name":"%1","html_url":"%2","prerelease":%3,"draft":%4})")
        .arg(tag, url, prerelease ? "true" : "false", draft ? "true" : "false")
        .toUtf8();
}

TEST(IsNewerVersion, ComparesNumerically) {
    EXPECT_TRUE(isNewerVersion("0.3.0", "0.2.0"));
    EXPECT_TRUE(isNewerVersion("0.10.0", "0.9.0")); // no alfabético
    EXPECT_TRUE(isNewerVersion("1.0.0", "0.99.99"));
    EXPECT_TRUE(isNewerVersion("0.2.1", "0.2.0"));
    EXPECT_FALSE(isNewerVersion("0.2.0", "0.2.0"));
    EXPECT_FALSE(isNewerVersion("0.1.9", "0.2.0"));
}

TEST(IsNewerVersion, IgnoresPrefixAndSuffixAndRejectsGarbage) {
    EXPECT_TRUE(isNewerVersion("v0.3.0", "0.2.0"));
    EXPECT_TRUE(isNewerVersion("0.3.0-beta", "0.2.0"));
    EXPECT_FALSE(isNewerVersion("latest", "0.2.0"));
    EXPECT_FALSE(isNewerVersion("0.3.0", "dev"));
    EXPECT_FALSE(isNewerVersion("", ""));
}

TEST(ParseLatestRelease, ReadsVersionAndUrl) {
    const auto info = parseLatestRelease(releaseJson("v0.3.0"));
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->version, "0.3.0");
    EXPECT_EQ(info->url.toString(), "https://github.com/x/y/releases/tag/v9");
}

TEST(ParseLatestRelease, RejectsInvalidDraftPrereleaseAndNonHttps) {
    EXPECT_FALSE(parseLatestRelease("no es json"));
    EXPECT_FALSE(parseLatestRelease("[]"));
    EXPECT_FALSE(parseLatestRelease(releaseJson("v0.3.0", "https://x.y/z", true)));
    EXPECT_FALSE(parseLatestRelease(releaseJson("v0.3.0", "https://x.y/z", false, true)));
    EXPECT_FALSE(parseLatestRelease(releaseJson("nightly")));
    EXPECT_FALSE(parseLatestRelease(releaseJson("v0.3.0", "http://x.y/z")));
    EXPECT_FALSE(parseLatestRelease(releaseJson("v0.3.0", "file:///C:/malware.exe")));
}

TEST(UpdateChecker, ReportsNewerRelease) {
    FakeServer server(200, releaseJson("v0.3.0"));
    const Result result = runCheck("0.2.0", server.url());
    EXPECT_EQ(result.outcome, Outcome::Available);
    EXPECT_EQ(result.version, "0.3.0");
    EXPECT_EQ(result.url.scheme(), "https");
    // Se identifica con la versión y pide el formato JSON de GitHub.
    // Los nombres de cabecera no distinguen mayúsculas y según la versión de Qt salen en minúscula.
    const QByteArray headers = server.request().toLower();
    EXPECT_TRUE(headers.contains("user-agent: noctis/0.2.0"));
    EXPECT_TRUE(headers.contains("application/vnd.github+json"));
}

TEST(UpdateChecker, ReportsUpToDateForSameOrOlderRelease) {
    FakeServer same(200, releaseJson("v0.2.0"));
    EXPECT_EQ(runCheck("0.2.0", same.url()).outcome, Outcome::UpToDate);
    FakeServer older(200, releaseJson("v0.1.0"));
    EXPECT_EQ(runCheck("0.2.0", older.url()).outcome, Outcome::UpToDate);
}

TEST(UpdateChecker, FailsOnServerErrorMalformedBodyAndNoConnection) {
    FakeServer broken(500, "{}");
    EXPECT_EQ(runCheck("0.2.0", broken.url()).outcome, Outcome::Failed);

    FakeServer garbage(200, "<html>no json</html>");
    EXPECT_EQ(runCheck("0.2.0", garbage.url()).outcome, Outcome::Failed);

    // Puerto cerrado: se levanta un servidor y se apaga para obtener uno libre.
    QUrl closed;
    {
        FakeServer temporary(200, "{}");
        closed = temporary.url();
    }
    EXPECT_EQ(runCheck("0.2.0", closed).outcome, Outcome::Failed);
}

TEST(UpdateChecker, SecondCheckWhileRunningIsIgnored) {
    app();
    FakeServer server(200, releaseJson("v0.3.0"));
    UpdateChecker checker("0.2.0", server.url());
    int emitted = 0;
    QEventLoop loop;
    QObject::connect(&checker, &UpdateChecker::updateAvailable, &loop, [&] {
        ++emitted;
        loop.quit();
    });
    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    checker.check();
    checker.check();
    loop.exec();
    QCoreApplication::processEvents();
    EXPECT_EQ(emitted, 1);
}


// Contra GitHub de verdad (red + TLS). Solo con NOCTIS_LIVE_TESTS=1: los tests
// normales no deben depender de internet.
TEST(UpdateChecker, LiveGitHubEndpoint) {
    if (qEnvironmentVariable("NOCTIS_LIVE_TESTS") != "1") GTEST_SKIP() << "NOCTIS_LIVE_TESTS != 1";
    app();
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));
    // La v0.1.0 ya existe: cualquiera posterior a 0.0.1 debe verse como nueva.
    const Result result = runCheck("0.0.1", UpdateChecker::defaultEndpoint());
    EXPECT_EQ(result.outcome, Outcome::Available);
    EXPECT_TRUE(isNewerVersion(result.version, "0.0.1"));
    EXPECT_EQ(result.url.host(), "github.com");
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
