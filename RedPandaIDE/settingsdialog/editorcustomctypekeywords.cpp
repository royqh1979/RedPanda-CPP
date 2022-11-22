#include "editorcustomctypekeywords.h"
#include "ui_editorcustomctypekeywords.h"
#include "../settings.h"
#include "../iconsmanager.h"

EditorCustomCTypeKeywordsWidget::EditorCustomCTypeKeywordsWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::editorcustomctypekeywords)
{
    ui->setupUi(this);
}

EditorCustomCTypeKeywordsWidget::~EditorCustomCTypeKeywordsWidget()
{
    delete ui;
}

void EditorCustomCTypeKeywordsWidget::doLoad()
{
    ui->grpEnableCustomKeywords->setChecked(pSettings->editor().enableCustomCTypeKeywords());
    ui->lstKeywords->clear();
    foreach(const QString& s, pSettings->editor().customCTypeKeywords())
        addKeyword(s);
}

void EditorCustomCTypeKeywordsWidget::doSave()
{
    pSettings->editor().setEnableCustomCTypeKeywords(ui->grpEnableCustomKeywords->isChecked());
    QStringList lst;
    QSet<QString> added;
    for(int i=0;i<ui->lstKeywords->count();i++) {
        QString t=ui->lstKeywords->item(i)->text().trimmed();
        if (!t.isEmpty() && !added.contains(t)) {
            lst.append(t);
            added.insert(t);
        }
    }
    pSettings->editor().setCustomCTypeKeywords(lst);
    pSettings->editor().save();
    doLoad();
}

void EditorCustomCTypeKeywordsWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnAdd, IconsManager::ACTION_MISC_ADD);
    pIconsManager->setIcon(ui->btnRemove, IconsManager::ACTION_MISC_REMOVE);
    pIconsManager->setIcon(ui->btnRemoveAll, IconsManager::ACTION_MISC_CLEAN);
}

QListWidgetItem * EditorCustomCTypeKeywordsWidget::addKeyword(const QString &keyword)
{
    QListWidgetItem * item = new QListWidgetItem(keyword,ui->lstKeywords);
    item->setFlags(Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsEnabled);
    ui->lstKeywords->addItem(item);
    return item;
}

void EditorCustomCTypeKeywordsWidget::on_btnAdd_clicked()
{
    QListWidgetItem *item=addKeyword("");
    ui->lstKeywords->editItem(item);
}


void EditorCustomCTypeKeywordsWidget::on_btnRemove_clicked()
{
    int row = ui->lstKeywords->currentRow();
    if (row>=0 && row<ui->lstKeywords->count()) {
        QListWidgetItem * item = ui->lstKeywords->takeItem(row);
        delete item;
    }
}


void EditorCustomCTypeKeywordsWidget::on_btnRemoveAll_clicked()
{
    ui->lstKeywords->clear();
}

