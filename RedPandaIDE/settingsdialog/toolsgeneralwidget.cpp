/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "toolsgeneralwidget.h"
#include "ui_toolsgeneralwidget.h"
#include "../mainwindow.h"
#include "../settings.h"
#include "../iconsmanager.h"
#include "utils.h"
#include "utils/escape.h"
#include "utils/parsearg.h"
#include "../systemconsts.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QUuid>

ToolsGeneralWidget::ToolsGeneralWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ToolsGeneralWidget)
{
    ui->setupUi(this);
    ui->cbInput->addItems(
                {
                    tr("None"),
                    tr("Current Selection"),
                    tr("Whole Document"),
                });
    ui->cbOutput->addItems(
                {
                    tr("None"),
                    tr("Tools Output"),
                    tr("Replace Current Selection"),
                    tr("Repalce Whole Document"),
                });

    ui->cbMacros->setModel(&mMacroInfoModel);
    QItemSelectionModel *m=ui->lstTools->selectionModel();
    ui->lstTools->setModel(&mToolsModel);
    delete m;
    mCurrentEditingRow = -1;
    showEditPanel(false);
    connect(ui->lstTools, &QAbstractItemView::doubleClicked,
            this, &ToolsGeneralWidget::editTool);
    connect(ui->txtProgram,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::updateDemo);
    connect(ui->txtParameters,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::updateDemo);

    connect(ui->txtTitle,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::onEdited);
    connect(ui->txtProgram,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::onEdited);
    connect(ui->txtParameters,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::onEdited);
    connect(ui->txtDirectory,&QLineEdit::textChanged,
            this, &ToolsGeneralWidget::onEdited);
    connect(ui->cbInput, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ToolsGeneralWidget::onEdited);
    connect(ui->cbOutput, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ToolsGeneralWidget::onEdited);
    connect(ui->chkUTF8, &QCheckBox::stateChanged,
            this, &ToolsGeneralWidget::onEdited);
}

ToolsGeneralWidget::~ToolsGeneralWidget()
{
    delete ui;
}

void ToolsGeneralWidget::editTool(const QModelIndex &index)
{
    if (mCurrentEditingRow>=0)
        return;
    if (!index.isValid())
        return;
    prepareEdit(index.row());
}

void ToolsGeneralWidget::finishEditing(bool askSave)
{
    auto action = finally([this]{
        cleanEditor();
    });
    if (mCurrentEditingRow == -1)
        return;
    if (!mEdited)
        return;
    if (ui->txtTitle->text().isEmpty()) {
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("Title shouldn't be empty!"));
        return;
    }
    if (askSave && QMessageBox::question(this,
                          tr("Save Changes?"),
                          tr("Do you want to save changes to \"%1\"?").arg(ui->txtTitle->text()),
                          QMessageBox::Yes | QMessageBox::No,
                          QMessageBox::Yes) != QMessageBox::Yes) {
        return;
    }
    PToolItem item = mToolsModel.getTool(mCurrentEditingRow);
    item->workingDirectory = ui->txtDirectory->text();
    item->parameters = ui->txtParameters->text();
    item->program = ui->txtProgram->text();
    item->title = ui->txtTitle->text();
    item->inputOrigin = static_cast<ToolItemInputOrigin>(ui->cbInput->currentIndex());
    item->outputTarget = static_cast<ToolItemOutputTarget>(ui->cbOutput->currentIndex());
    item->isUTF8 = ui->chkUTF8->isChecked();
    mToolsModel.updateTool(mCurrentEditingRow, item);
}

void ToolsGeneralWidget::cleanEditor()
{
    mEdited = false;
    mCurrentEditingRow = -1;
    showEditPanel(false);
}

void ToolsGeneralWidget::prepareEdit(int row)
{
    mCurrentEditingRow = row;
    PToolItem item = mToolsModel.getTool(row);
    ui->txtDirectory->setText(item->workingDirectory);
    ui->txtParameters->setText(item->parameters);
    ui->txtProgram->setText(item->program);
    ui->txtTitle->setText(item->title);
    ui->cbInput->setCurrentIndex(static_cast<int>(item->inputOrigin));
    ui->cbOutput->setCurrentIndex(static_cast<int>(item->outputTarget));
    ui->chkUTF8->setChecked(item->isUTF8);
    showEditPanel(true);
    ui->txtTitle->setFocus();
    mEdited = false;
}

void ToolsGeneralWidget::showEditPanel(bool isShow)
{
    ui->panelEdit->setVisible(isShow);
    ui->panelEditButtons->setVisible(!isShow);
    ui->lstTools->setEnabled(!isShow);
}

void ToolsGeneralWidget::onEdited()
{
    mEdited=true;
}

void ToolsGeneralWidget::updateDemo()
{
    QMap<QString,QString> macros = devCppMacroVariables();
    ui->txtDemo->setText(escapeCommandForPlatformShell(
                parseMacros(ui->txtProgram->text(), macros),
                parseArguments(ui->txtParameters->text(), macros, true)
    ));
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

PToolItem ToolsModel::getTool(int row)
{
    return mTools[row];
}

void ToolsModel::updateTool(int row, PToolItem item)
{
    mTools[row] = item;
    QModelIndex index=createIndex(row, 0);
    emit dataChanged(index, index);
}

void ToolsModel::removeTool(int row)
{
    beginRemoveRows(QModelIndex(),row,row);
    mTools.removeAt(row);
    endRemoveRows();
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

Qt::ItemFlags ToolsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
    } else if (index.row() == -1) {
        // -1 means it's a drop target?
        flags = Qt::ItemIsDropEnabled;
    }
    return flags ;
}

Qt::DropActions ToolsModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool ToolsModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);
    mMoveTargetRow=row;
    if (mMoveTargetRow==-1)
        mMoveTargetRow=mTools.length();
    return  QAbstractListModel::dropMimeData(data,action,row,0,parent);
}

bool ToolsModel::insertRows(int /* row */, int /*count*/, const QModelIndex &/*parent*/)
{
    return true;
}

bool ToolsModel::removeRows(int row, int count, const QModelIndex &/*parent*/)
{
    int sourceRow = row;
    int destinationChild = mMoveTargetRow;
    mMoveTargetRow=-1;
    if (sourceRow < 0
        || sourceRow + count - 1 >= mTools.count()
        || destinationChild < 0
        || destinationChild > mTools.count()
        || sourceRow == destinationChild
        || count <= 0) {
        return false;
    }
    if (!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;

    int fromRow = sourceRow;
    if (destinationChild < sourceRow)
        fromRow += count - 1;
    else
        destinationChild--;
    while (count--)
        mTools.move(fromRow, destinationChild);
    endMoveRows();
    return true;
}


void ToolsGeneralWidget::on_btnAdd_clicked()
{
    ui->lstTools->setCurrentIndex(QModelIndex());
    PToolItem item = std::make_shared<ToolItem>();
    item->id=QUuid::createUuid().toString();
    item->title = tr("untitled");
    item->inputOrigin = ToolItemInputOrigin::None;
    item->outputTarget = ToolItemOutputTarget::RedirectToToolsOutputPanel;
    item->isUTF8 = false;
    mToolsModel.addTool(item);
    int row = mToolsModel.tools().count() - 1;
    QModelIndex index=mToolsModel.index(row);
    ui->lstTools->setCurrentIndex(index);
    prepareEdit(row);
}


void ToolsGeneralWidget::on_btnEditOk_clicked()
{
    finishEditing(false);
}


void ToolsGeneralWidget::on_btnEditCancel_clicked()
{
    cleanEditor();
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

void ToolsGeneralWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnAdd,IconsManager::ACTION_MISC_ADD);
    pIconsManager->setIcon(ui->btnEdit, IconsManager::ACTION_MISC_RENAME);
    pIconsManager->setIcon(ui->btnRemove,IconsManager::ACTION_MISC_REMOVE);
    pIconsManager->setIcon(ui->btnBrowseProgram,IconsManager::ACTION_FILE_LOCATE);
    pIconsManager->setIcon(ui->btnBrowseWorkingDirectory,IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void ToolsGeneralWidget::on_btnRemove_clicked()
{
    cleanEditor();
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
        pSettings->dirs().appDir(),
        pSystemConsts->executableFileFilter());
    if (!fileName.isEmpty() ) {
        QString appPath = includeTrailingPathDelimiter(pSettings->dirs().appDir());
        if (fileName.startsWith(appPath))
            fileName = QString("<EXECPATH>") + QDir::separator() + fileName.mid(appPath.length());
        ui->txtProgram->setText(fileName);
    }
}



void ToolsGeneralWidget::on_btnEdit_clicked()
{
    const QModelIndex& index = ui->lstTools->currentIndex();
    if (index.isValid())
        editTool(index);
}

