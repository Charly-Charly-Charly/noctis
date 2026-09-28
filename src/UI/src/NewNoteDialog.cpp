#include "UI/NewNoteDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <filesystem>

namespace noctis::ui {

namespace {
constexpr const char* kLastFolderKey = "newNoteDialog.lastFolder";
constexpr const char* kLastSubfolderKey = "newNoteDialog.lastSubfolder";
} // namespace

NewNoteDialog::NewNoteDialog(core::INoteRepository& repository, core::FolderService& folderService,
                              core::ISettingsStore& settings, core::Folder rootFolder,
                              QWidget* parent)
    : QDialog(parent),
      repository_(repository),
      folderService_(folderService),
      settings_(settings),
      rootFolder_(std::move(rootFolder)) {
    setWindowTitle(tr("Nueva nota"));

    titleEdit_ = new QLineEdit(this);
    folderCombo_ = new QComboBox(this);
    subfolderCombo_ = new QComboBox(this);
    pathPreviewLabel_ = new QLabel(this);
    pathPreviewLabel_->setWordWrap(true);

    auto* newFolderButton = new QPushButton(tr("Nueva carpeta…"), this);
    auto* newSubfolderButton = new QPushButton(tr("Nueva subcarpeta…"), this);

    auto* folderRow = new QHBoxLayout();
    folderRow->addWidget(folderCombo_);
    folderRow->addWidget(newFolderButton);

    auto* subfolderRow = new QHBoxLayout();
    subfolderRow->addWidget(subfolderCombo_);
    subfolderRow->addWidget(newSubfolderButton);

    auto* form = new QFormLayout();
    form->addRow(tr("Título"), titleEdit_);
    form->addRow(tr("Carpeta"), folderRow);
    form->addRow(tr("Subcarpeta"), subfolderRow);
    form->addRow(tr("Ruta final"), pathPreviewLabel_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(folderCombo_, &QComboBox::currentTextChanged, this,
            &NewNoteDialog::handleFolderChanged);
    connect(subfolderCombo_, &QComboBox::currentTextChanged, this,
            &NewNoteDialog::updatePathPreview);
    connect(titleEdit_, &QLineEdit::textChanged, this, &NewNoteDialog::updatePathPreview);
    connect(newFolderButton, &QPushButton::clicked, this, &NewNoteDialog::handleCreateFolder);
    connect(newSubfolderButton, &QPushButton::clicked, this,
            &NewNoteDialog::handleCreateSubfolder);

    std::string preferredFolder;
    if (auto saved = settings_.getString(kLastFolderKey)) preferredFolder = *saved;
    reloadTopFolders(preferredFolder);
}

void NewNoteDialog::preselectFolder(const core::Folder& folder) {
    std::filesystem::path relative = std::filesystem::relative(folder.path, rootFolder_.path);
    auto it = relative.begin();
    if (it == relative.end() || it->empty()) return;

    int topIndex = folderCombo_->findText(QString::fromStdString(it->string()));
    if (topIndex < 0) return;
    folderCombo_->setCurrentIndex(topIndex);

    ++it;
    if (it != relative.end() && !it->empty()) {
        int subIndex = subfolderCombo_->findText(QString::fromStdString(it->string()));
        if (subIndex >= 0) subfolderCombo_->setCurrentIndex(subIndex);
    }
}

void NewNoteDialog::reloadTopFolders(const std::string& preferredName) {
    folderCombo_->blockSignals(true);
    folderCombo_->clear();
    for (const core::Folder& folder : repository_.listSubfolders(rootFolder_)) {
        folderCombo_->addItem(QString::fromStdString(folder.name));
    }
    folderCombo_->blockSignals(false);

    if (!preferredName.empty()) {
        int index = folderCombo_->findText(QString::fromStdString(preferredName));
        if (index >= 0) folderCombo_->setCurrentIndex(index);
    }

    handleFolderChanged();
}

core::Folder NewNoteDialog::currentParentFolder() const {
    core::Folder folder;
    folder.name = folderCombo_->currentText().toStdString();
    folder.path = rootFolder_.path / folder.name;
    return folder;
}

void NewNoteDialog::handleFolderChanged() {
    std::string preferredSubfolder;
    if (auto saved = settings_.getString(kLastSubfolderKey)) preferredSubfolder = *saved;
    reloadSubfolders(preferredSubfolder);
}

void NewNoteDialog::reloadSubfolders(const std::string& preferredName) {
    subfolderCombo_->blockSignals(true);
    subfolderCombo_->clear();

    if (folderCombo_->count() > 0) {
        for (const core::Folder& subfolder : repository_.listSubfolders(currentParentFolder())) {
            subfolderCombo_->addItem(QString::fromStdString(subfolder.name));
        }
    }

    subfolderCombo_->blockSignals(false);
    subfolderCombo_->setEnabled(subfolderCombo_->count() > 0);

    if (!preferredName.empty()) {
        int index = subfolderCombo_->findText(QString::fromStdString(preferredName));
        if (index >= 0) subfolderCombo_->setCurrentIndex(index);
    }

    updatePathPreview();
}

void NewNoteDialog::handleCreateFolder() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nueva carpeta"), tr("Nombre:"),
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    folderService_.createSubfolder(rootFolder_, name.trimmed().toStdString());
    reloadTopFolders(name.trimmed().toStdString());
}

void NewNoteDialog::handleCreateSubfolder() {
    if (folderCombo_->count() == 0) return;

    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nueva subcarpeta"), tr("Nombre:"),
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    folderService_.createSubfolder(currentParentFolder(), name.trimmed().toStdString());
    reloadSubfolders(name.trimmed().toStdString());
}

void NewNoteDialog::updatePathPreview() {
    QString path = folderCombo_->currentText();
    if (subfolderCombo_->isEnabled() && !subfolderCombo_->currentText().isEmpty()) {
        path += "/" + subfolderCombo_->currentText();
    }
    path += "/" + (titleEdit_->text().isEmpty() ? tr("(sin título)") : titleEdit_->text()) + ".md";
    pathPreviewLabel_->setText(path);
}

std::string NewNoteDialog::title() const {
    return titleEdit_->text().trimmed().toStdString();
}

core::Folder NewNoteDialog::targetFolder() const {
    core::Folder folder = currentParentFolder();
    if (subfolderCombo_->isEnabled() && !subfolderCombo_->currentText().isEmpty()) {
        std::string subfolderName = subfolderCombo_->currentText().toStdString();
        folder.path = folder.path / subfolderName;
        folder.name = subfolderName;
    }
    return folder;
}

void NewNoteDialog::accept() {
    if (titleEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Falta el título"), tr("Escribe un título para la nota."));
        return;
    }
    if (folderCombo_->count() == 0) {
        QMessageBox::warning(this, tr("Falta una carpeta"),
                              tr("Crea una carpeta antes de continuar."));
        return;
    }

    settings_.setString(kLastFolderKey, folderCombo_->currentText().toStdString());
    if (subfolderCombo_->isEnabled()) {
        settings_.setString(kLastSubfolderKey, subfolderCombo_->currentText().toStdString());
    }

    QDialog::accept();
}

} // namespace noctis::ui
