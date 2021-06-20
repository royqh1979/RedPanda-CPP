#include "editorcolorschemewidget.h"
#include "ui_editorcolorschemewidget.h"
#include "../settings.h"
#include "../colorscheme.h"
#include "../mainwindow.h"

#include <QAction>
#include <QMessageBox>

EditorColorSchemeWidget::EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorColorSchemeWidget)
{
    ui->setupUi(this);

    mDefaultSchemeComboFont = ui->cbScheme->font();
    mModifiedSchemeComboFont = mDefaultSchemeComboFont;
    mModifiedSchemeComboFont.setBold(true);
    int schemeCount=0;
    for (QString schemeName: pColorManager->getSchemes()) {
        PColorScheme scheme = pColorManager->get(schemeName);
        if (!scheme)
            return;
        ui->cbScheme->addItem(schemeName);
        if (scheme->customed())
            ui->cbScheme->setItemData(schemeCount,mModifiedSchemeComboFont,Qt::FontRole);
        schemeCount++;
    }
    ui->treeItems->setModel(&mDefinesModel);
    mDefinesModel.setHorizontalHeaderLabels(QStringList());
    for (QString defineName : pColorManager->getDefines()) {
        addDefine(defineName, pColorManager->getDefine(defineName));
    }
    ui->treeItems->expandAll();
    QModelIndex groupIndex = mDefinesModel.index(0,0);
    QModelIndex index = mDefinesModel.index(0,0,groupIndex);
    ui->treeItems->setCurrentIndex(index);
    connect(ui->treeItems->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EditorColorSchemeWidget::onItemSelectionChanged);
    connect(ui->cbScheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditorColorSchemeWidget::changeSchemeComboFont);
    connect(ui->cbScheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditorColorSchemeWidget::onItemSelectionChanged);
    connect(this, &SettingsWidget::settingsChanged,this,
            &EditorColorSchemeWidget::onSettingChanged);
    ui->editDemo->lines()->setText(
            "#include <iostream>\n"
            "#include <conio.h>\n"
            "\n"
            "int x=10;\n"
            "\n"
            "int main(int argc, char **argv)\n"
            "{\n"
            "    int numbers[20];\n"
            "    float average, total; //breakpoint\n"
            "    for (int i = 0; i <= 19; i++)\n"
            "    { // active breakpoint\n"
            "        numbers[i] = i+x;\n"
            "        Total += i; // error line\n"
            "    }\n"
            "    average = total / 20; // comment\n"
            "    cout << \"total: \" << total << \"\nAverage: \" << average;\n"
            "    getch();\n"
            "}\n"
                );
    ui->editDemo->setReadOnly(true);
    onItemSelectionChanged();
}

void EditorColorSchemeWidget::addDefine(const QString& name, PColorSchemeItemDefine define)
{
    QList<QStandardItem*> items = mDefinesModel.findItems(define->group());
    QStandardItem* pGroupItem;
    if (items.count() == 0 ) {
        pGroupItem = new QStandardItem(define->group());
        pGroupItem->setData("", NameRole);
        mDefinesModel.appendRow(pGroupItem);
    } else {
        pGroupItem = items[0];
    }
    QStandardItem* pWidgetItem = new QStandardItem(define->displayName());
    pWidgetItem->setData(name, NameRole);
    pGroupItem->appendRow(pWidgetItem);
}

PColorSchemeItem EditorColorSchemeWidget::getCurrentItem()
{
    QItemSelectionModel * selectionModel = ui->treeItems->selectionModel();
    QString name =mDefinesModel.data(selectionModel->currentIndex(),NameRole).toString();
    if (name.isEmpty())
        return PColorSchemeItem();
    return pColorManager->getItem(ui->cbScheme->currentText(), name);
}

PColorScheme EditorColorSchemeWidget::getCurrentScheme()
{
    return pColorManager->get(ui->cbScheme->currentText());
}

void EditorColorSchemeWidget::connectModificationSlots()
{
    connect(ui->cbBackground,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onBackgroundChanged);
    connect(ui->colorBackground,&ColorEdit::colorChanged,
            this, &EditorColorSchemeWidget::onBackgroundChanged);
    connect(ui->cbForeground,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onForegroundChanged);
    connect(ui->colorForeground,&ColorEdit::colorChanged,
            this, &EditorColorSchemeWidget::onForegroundChanged);
    connect(ui->cbBold,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
    connect(ui->cbItalic,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
    connect(ui->cbStrikeout,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
    connect(ui->cbUnderlined,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
}

void EditorColorSchemeWidget::disconnectModificationSlots()
{
    disconnect(ui->cbBackground,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onBackgroundChanged);
    disconnect(ui->colorBackground,&ColorEdit::colorChanged,
            this, &EditorColorSchemeWidget::onBackgroundChanged);
    disconnect(ui->cbForeground,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onForegroundChanged);
    disconnect(ui->colorForeground,&ColorEdit::colorChanged,
            this, &EditorColorSchemeWidget::onForegroundChanged);
    disconnect(ui->cbBold,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
    disconnect(ui->cbItalic,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
    disconnect(ui->cbStrikeout,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
    disconnect(ui->cbUnderlined,&QCheckBox::stateChanged,
            this, &EditorColorSchemeWidget::onFontStyleChanged);
}

void EditorColorSchemeWidget::setCurrentSchemeModified()
{
    PColorScheme scheme = getCurrentScheme();
    if (scheme) {
        scheme->setCustomed(true);
    }
    if (mModifiedSchemes.contains(ui->cbScheme->currentText()))
        return;
    mModifiedSchemes.insert(ui->cbScheme->currentText());
    ui->cbScheme->setItemData(ui->cbScheme->currentIndex(),
                              mModifiedSchemeComboFont,Qt::FontRole);
    ui->cbScheme->setFont(mModifiedSchemeComboFont);
    ui->cbScheme->view()->setFont(mDefaultSchemeComboFont);
    //we must reset the editor here, because this slot is processed after the onSettingChanged
    onSettingChanged();
}

EditorColorSchemeWidget::~EditorColorSchemeWidget()
{
    delete ui;
}

static void setColorProp(ColorEdit* ce, QCheckBox* cb, const QColor& color) {
    if (color.isValid()) {
        cb->setChecked(true);
        ce->setColor(color);
        ce->setVisible(true);
    } else {
        cb->setChecked(false);
        ce->setVisible(false);
    }
}

void EditorColorSchemeWidget::onItemSelectionChanged()
{
    disconnectModificationSlots();
    QItemSelectionModel * selectionModel = ui->treeItems->selectionModel();
    QString name =mDefinesModel.data(selectionModel->currentIndex(),NameRole).toString();
    bool found = false;
    if (!name.isEmpty()) {
        PColorSchemeItemDefine define = pColorManager->getDefine(name);
        if (define) {
            found = true;
            ui->cbBackground->setEnabled(define->hasBackground());
            ui->colorBackground->setEnabled(define->hasBackground());
            ui->cbForeground->setEnabled(define->hasForeground());
            ui->colorForeground->setEnabled(define->hasForeground());
            ui->grpFontStyles->setEnabled(define->hasFontStyle());
            PColorSchemeItem item = pColorManager->getItem(ui->cbScheme->currentText(), name);
            if (item) {
                if (define->hasBackground()) {
                    setColorProp(ui->colorBackground, ui->cbBackground,item->background());
                } else {
                    setColorProp(ui->colorBackground, ui->cbBackground,QColor());
                }
                if (define->hasForeground()) {
                    setColorProp(ui->colorForeground, ui->cbForeground,item->foreground());
                } else {
                    setColorProp(ui->colorForeground, ui->cbForeground,QColor());
                }
                if (define->hasFontStyle()) {
                    ui->cbBold->setChecked(item->bold());
                    ui->cbItalic->setChecked(item->italic());
                    ui->cbUnderlined->setChecked(item->underlined());
                    ui->cbStrikeout->setChecked(item->strikeout());
                } else {
                    ui->cbBold->setChecked(false);
                    ui->cbItalic->setChecked(false);
                    ui->cbUnderlined->setChecked(false);
                    ui->cbStrikeout->setChecked(false);
                }
            }
        }
    }

    ui->widgetSchemeItem->setEnabled(found);
    connectModificationSlots();
}

void EditorColorSchemeWidget::onSettingChanged()
{
    ui->editDemo->applyColorScheme(ui->cbScheme->currentText());
}

void EditorColorSchemeWidget::onForegroundChanged()
{
    PColorSchemeItem item = getCurrentItem();
    if (!item)
        return;
    if (ui->cbForeground->isChecked()) {
        item->setForeground(ui->colorForeground->color());
    } else {
        item->setForeground(QColor());
    }
    setCurrentSchemeModified();
}

void EditorColorSchemeWidget::onBackgroundChanged()
{
    PColorSchemeItem item = getCurrentItem();
    if (!item)
        return;
    if (ui->cbBackground->isChecked()) {
        item->setBackground(ui->colorBackground->color());
    } else {
        item->setBackground(QColor());
    }
    setCurrentSchemeModified();
}

void EditorColorSchemeWidget::onFontStyleChanged()
{
    PColorSchemeItem item = getCurrentItem();
    if (!item)
        return;
    item->setBold(ui->cbBold->isChecked());
    item->setItalic(ui->cbItalic->isChecked());
    item->setStrikeout(ui->cbStrikeout->isChecked());
    item->setUnderlined(ui->cbUnderlined->isChecked());
    setCurrentSchemeModified();
}

void EditorColorSchemeWidget::changeSchemeComboFont()
{
    QString name = ui->cbScheme->currentText();
    PColorScheme scheme = pColorManager->get(name);
    if (scheme && scheme->customed()) {
        ui->cbScheme->setFont(mModifiedSchemeComboFont);
    } else {
        ui->cbScheme->setFont(mDefaultSchemeComboFont);
    }
    ui->cbScheme->view()->setFont(mDefaultSchemeComboFont);
}

void EditorColorSchemeWidget::doLoad()
{
    ui->cbScheme->setCurrentText(pSettings->editor().colorScheme());
}

void EditorColorSchemeWidget::doSave()
{
    try {
        for (QString name:mModifiedSchemes) {
            pColorManager->saveScheme(name);
        }
        pSettings->editor().setColorScheme(ui->cbScheme->currentText());
        pSettings->editor().save();
        pMainWindow->updateEditorColorSchemes();
    } catch (FileError e) {
        QMessageBox::information(this,tr("Error"),e.reason());
    }
}

void EditorColorSchemeWidget::on_actionCopy_Scheme_triggered()
{

}

void EditorColorSchemeWidget::on_btnSchemeMenu_pressed()
{
    QMenu menu;

    PColorScheme scheme = pColorManager->get(ui->cbScheme->currentText());
    if (scheme) {
        if (scheme->customed()) {
            menu.addAction(ui->actionReset_Scheme);
        }
        if (!scheme->bundled()) {
            menu.addAction(ui->actionRename_Scheme);
            menu.addAction(ui->actionDelete_Scheme);
        }
        menu.addAction(ui->actionCopy_Scheme);
        menu.addAction(ui->actionExport_Scheme);
        menu.addSeparator();
    }
    menu.addAction(ui->actionImport_Scheme);
    QPoint p;
    p.setX(0);
    p.setY(ui->btnSchemeMenu->height()+2);
    QAction* action = menu.exec(ui->btnSchemeMenu->mapToGlobal(p));
    if (action)
        action->trigger();
}
