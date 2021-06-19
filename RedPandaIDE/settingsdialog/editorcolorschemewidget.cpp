#include "editorcolorschemewidget.h"
#include "ui_editorcolorschemewidget.h"
#include "../settings.h"
#include "../colorscheme.h"

EditorColorSchemeWidget::EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorColorSchemeWidget)
{
    ui->setupUi(this);

    for (QString schemeName: pColorManager->getSchemes()) {
        ui->cbScheme->addItem(schemeName);
    }
    ui->treeItems->setModel(&mDefinesModel);
    mDefinesModel.setHorizontalHeaderLabels(QStringList());
    for (QString defineName : pColorManager->getDefines()) {
        addDefine(defineName, pColorManager->getDefine(defineName));
    }
    ui->treeItems->expandAll();
    connect(ui->treeItems->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EditorColorSchemeWidget::onItemSelectionChanged);
    connect(this, &SettingsWidget::settingsChanged,this,
            &EditorColorSchemeWidget::onSettingChanged);
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
    QModelIndex groupIndex = mDefinesModel.index(0,0);
    QModelIndex index = mDefinesModel.index(0,0,groupIndex);
    ui->treeItems->setCurrentIndex(index);
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
    // not found
    ui->widgetSchemeItem->setEnabled(found);
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
}

void EditorColorSchemeWidget::doLoad()
{

}

void EditorColorSchemeWidget::doSave()
{

}
