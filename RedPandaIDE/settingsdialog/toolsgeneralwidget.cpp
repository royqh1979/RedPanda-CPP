#include "toolsgeneralwidget.h"
#include "ui_toolsgeneralwidget.h"
#include "../mainwindow.h"
#include "../settings.h"

#include <QFileDialog>
#include <QMessageBox>

ToolsGeneralWidget::ToolsGeneralWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ToolsGeneralWidget)
{
    ui->setupUi(this);
    ui->cbMacros->setModel(&mMacroInfoModel);
    ui->lstTools->setModel(&mToolsModel);
    mEditType = EditType::None;
    finishEditing(false);
    connect(ui->lstTools->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this,&ToolsGeneralWidget::onToolsCurrentChanged);
    connect(ui->txtProgram,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::updateDemo);
    connect(ui->txtParameters,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::updateDemo);
}

ToolsGeneralWidget::~ToolsGeneralWidget()
{
    delete ui;
}

void ToolsGeneralWidget::onToolsCurrentChanged()
{
    if (mEditType != EditType::None) {
        finishEditing(true);
    }
    QModelIndex index = ui->lstTools->currentIndex();
    if (!index.isValid())
        return;
    PToolItem item = mToolsModel.getTool(index.row());
    if (item) {
        mEditType = EditType::Edit;
        ui->txtDirectory->setText(item->workingDirectory);
        ui->txtParameters->setText(item->parameters);
        ui->txtProgram->setText(item->program);
        ui->txtTitle->setText(item->title);
        ui->chkPauseConsole->setChecked(item->pauseAfterExit);
        ui->panelEdit->setVisible(true);
    }
}

void ToolsGeneralWidget::finishEditing(bool askSave)
{
    if (mEditType == EditType::None) {
        ui->panelEdit->setVisible(false);
        return;
    }
    if (askSave && QMessageBox::question(this,
                          tr("Save Changes?"),
                          tr("Do you want to save changes to the current tool?"),
                          QMessageBox::Yes | QMessageBox::No,
                          QMessageBox::Yes) != QMessageBox::Yes) {
        ui->panelEdit->setVisible(false);
        return;
    }
    ui->panelEdit->setVisible(false);
    if (mEditType == EditType::Add) {
        mEditType = EditType::None;
        PToolItem item = std::make_shared<ToolItem>();
        item->title = ui->txtTitle->text();
        item->program = ui->txtProgram->text();
        item->workingDirectory = ui->txtDirectory->text();
        item->parameters = ui->txtParameters->text();
        item->pauseAfterExit = ui->chkPauseConsole->isChecked();
        mToolsModel.addTool(item);
    } else {
        mEditType = EditType::None;
        QModelIndex index = ui->lstTools->currentIndex();
        if (!index.isValid())
            return;
        PToolItem item = mToolsModel.getTool(index.row());
        item->workingDirectory = ui->txtDirectory->text();
        item->parameters = ui->txtParameters->text();
        item->program = ui->txtProgram->text();
        item->title = ui->txtTitle->text();
        item->pauseAfterExit = ui->chkPauseConsole->isChecked();
    }
}

void ToolsGeneralWidget::prepareEdit()
{
    ui->txtDirectory->setText("");
    ui->txtParameters->setText("");
    ui->txtProgram->setText("");
    ui->txtTitle->setText("");
    ui->chkPauseConsole->setChecked(false);
    ui->panelEdit->setVisible(true);
}

void ToolsGeneralWidget::updateDemo()
{
    ui->txtDemo->setText(
                parseMacros(ui->txtProgram->text())+ " " +
                parseMacros(ui->txtParameters->text()));
}

ToolsModel::ToolsModel(QObject *parent):QAbstractListModel(parent)
{

}

const QList<PToolItem> &ToolsModel::tools() const
{
    return mTools;
}

void ToolsModel::setTools(const QList<PToolItem> &newTools)
{
    beginResetModel();
    mTools = newTools;
    endResetModel();
}

void ToolsModel::addTool(PToolItem item)
{
    beginInsertRows(QModelIndex(),mTools.count(),mTools.count());
    mTools.append(item);
    endInsertRows();
}

PToolItem ToolsModel::getTool(int index)
{
    return mTools[index];
}

void ToolsModel::removeTool(int index)
{
    mTools.removeAt(index);
}

int ToolsModel::rowCount(const QModelIndex &) const
{
    return mTools.count();
}

QVariant ToolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role==Qt::DisplayRole) {
        PToolItem item = mTools[index.row()];
        return item->title;
    }
    return QVariant();
}

void ToolsGeneralWidget::on_btnAdd_clicked()
{
    ui->lstTools->setCurrentIndex(QModelIndex());
    prepareEdit();
    mEditType = EditType::Add;
}


void ToolsGeneralWidget::on_btnEditOk_clicked()
{
    finishEditing(false);
}


void ToolsGeneralWidget::on_btnEditCancel_clicked()
{
    mEditType = EditType::None;
    ui->panelEdit->setVisible(false);
}

void ToolsGeneralWidget::doLoad()
{
    mToolsModel.setTools(pMainWindow->toolsManager()->tools());
}

void ToolsGeneralWidget::doSave()
{
    finishEditing(true);
    pMainWindow->toolsManager()->setTools(mToolsModel.tools());
    pMainWindow->toolsManager()->save();
    pMainWindow->updateTools();
}


void ToolsGeneralWidget::on_btnRemove_clicked()
{
    mEditType = EditType::None;
    finishEditing(false);
    QModelIndex index = ui->lstTools->currentIndex();
    if (index.isValid()) {
        mToolsModel.removeTool(index.row());
    }
}


void ToolsGeneralWidget::on_btnInsertMacro_clicked()
{
    ui->txtParameters->setText(
                ui->txtParameters->text() +
                ui->cbMacros->currentData(Qt::UserRole).toString());
}

void ToolsGeneralWidget::on_btnBrowseWorkingDirectory_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this,tr("Choose Folder"));
    if (!folder.isEmpty()) {
        ui->txtDirectory->setText(folder);
    }
}


void ToolsGeneralWidget::on_btnBrowseProgram_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Select program"),
                pSettings->dirs().app(),
                tr("Executable files (*.exe)"));
    if (!fileName.isEmpty() ) {
        ui->txtProgram->setText(fileName);
    }
}

