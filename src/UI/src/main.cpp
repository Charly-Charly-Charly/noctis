#include <QApplication>
#include <QIcon>

#include <cstdlib>

#include "Core/Services/ExportService.h"
#include "Core/Services/FavoritesService.h"
#include "Core/Services/FolderService.h"
#include "Core/Services/NoteService.h"
#include "Core/Services/RecentsService.h"
#include "Core/Services/SearchService.h"
#include "Database/SqliteConnection.h"
#include "Database/SqliteSearchIndex.h"
#include "Export/HtmlExporter.h"
#include "Filesystem/FilesystemNoteRepository.h"
#include "Filesystem/LegacyIdMigration.h"
#include "Settings/JsonSettingsStore.h"
#include "UI/AppContext.h"
#include "UI/MainWindow.h"
#include "UI/Theme.h"
#include "Utilities/Paths.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setFont(noctis::ui::theme::applicationFont());
    app.setStyleSheet(noctis::ui::theme::stylesheet(/*dark=*/false));
    // El .ico embebido en el .exe (ver src/UI/resources/noctis.rc) ya cubre
    // el ícono del Explorador y la barra de tareas; esto además asegura que
    // toda ventana/diálogo de la app (título, Alt+Tab) lo tenga en tiempo
    // de ejecución, sin depender de que Windows resuelva bien el recurso.
    app.setWindowIcon(QIcon(":/logo.png"));

    noctis::settings::JsonSettingsStore settings(noctis::paths::configDirectory() /
                                                  "settings.json");

    const char* userProfile = std::getenv("USERPROFILE");
    std::filesystem::path defaultNotesRoot =
        std::filesystem::path(userProfile ? userProfile : ".") / "Notas";

    // Configurable desde Ajustes ("Cambiar carpeta…"); el cambio recién surte
    // efecto en el próximo arranque. Se persiste el default la primera vez
    // para que Ajustes muestre siempre la carpeta realmente en uso.
    std::filesystem::path notesRoot = defaultNotesRoot;
    if (auto stored = settings.getString("notes.rootPath"); stored && !stored->empty()) {
        notesRoot = std::filesystem::path(*stored);
    } else {
        settings.setString("notes.rootPath", notesRoot.string());
    }
    std::filesystem::create_directories(notesRoot);

    noctis::fs::FilesystemNoteRepository repository(notesRoot);

    noctis::db::SqliteConnection connection(noctis::paths::indexDatabasePath());
    noctis::db::SqliteSearchIndex searchIndex(connection);

    // Favoritos y recientes guardados antes de los UUID apuntan a rutas.
    noctis::fs::LegacyIdMigration(repository, settings).run();

    noctis::exportmod::HtmlExporter htmlExporter;

    noctis::core::NoteService noteService(repository, searchIndex);
    noctis::core::FolderService folderService(repository);
    noctis::core::FavoritesService favoritesService(settings);
    noctis::core::RecentsService recentsService(settings);
    noctis::core::SearchService searchService(searchIndex);
    noctis::core::ExportService exportService(htmlExporter);
    noctis::core::TagService tagService;

    noctis::core::Folder rootFolder{"Notas", notesRoot};

    noctis::ui::AppContext context{noteService,  folderService, favoritesService, recentsService,
                                    searchService, exportService, tagService,
                                    repository,   settings,      rootFolder};

    noctis::ui::MainWindow window(context);
    window.resize(1100, 700);
    window.show();

    return app.exec();
}
