#include "compilersetoptionwidget.h"
#include "ui_compilersetoptionwidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "compilersetdirectorieswidget.h"
#include <QMessageBox>
#include "../utils.h"
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>

CompilerSetOptionWidget::CompilerSetOptionWidget(const QString& name, const QString& group, QWidget* parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::CompilerSetOptionWidget)
{
    ui->setupUi(this);

    mBinDirWidget = new CompilerSetDirectoriesWidget();
    ui->dirTabs->addTab(mBinDirWidget,QObject::tr("Binaries"));
    mLibDirWidget = new CompilerSetDirectoriesWidget();
    ui->dirTabs->addTab(mLibDirWidget,QObject::tr("Libraries"));
    mCIncludeDirWidget = new CompilerSetDirectoriesWidget();
    ui->dirTabs->addTab(mCIncludeDirWidget,QObject::tr("C Includes"));
    mCppIncludeDirWidget = new CompilerSetDirectoriesWidget();
    ui->dirTabs->addTab(mCppIncludeDirWidget,QObject::tr("C++ Includes"));

    connect(ui->chkUseCustomCompilerParams, &QCheckBox::stateChanged,
             ui->txtCustomCompileParams, &QPlainTextEdit::setEnabled);
    connect(ui->chkUseCustomLinkParams, &QCheckBox::stateChanged,
             ui->txtCustomLinkParams, &QPlainTextEdit::setEnabled);
}

CompilerSetOptionWidget::~CompilerSetOptionWidget()
{
    delete ui;
}

void resetOptionTabs(Settings::PCompilerSet pSet,QTabWidget* pTab)
{
    while (pTab->count()>0) {
        QWidget* p=pTab->widget(0);
        if (p!=nullptr) {
            pTab->removeTab(0);
            p->setParent(nullptr);
            delete p;
        }
    }

    for (PCompilerOption pOption: pSet->options()) {
        QWidget* pWidget = nullptr;
        for (int i=0;i<pTab->count();i++) {
            if (pOption->section == pTab->tabText(i)) {
                pWidget = pTab->widget(i);
                break;
            }
        }
        if (pWidget == nullptr) {
            pWidget = new QWidget();
            pTab->addTab(pWidget,pOption->section);
            pWidget->setLayout(new QGridLayout());
        }
        QGridLayout *pLayout = (QGridLayout*)pWidget->layout();
        int row = pLayout->rowCount();
        pLayout->addWidget(new QLabel(pOption->name),row,0);
        QComboBox* pCombo = new QComboBox();
        if (pOption->choices.count()>0) {
            for (int i=0;i<pOption->choices.count();i++) {
                QString choice = pOption->choices[i];
                QStringList valueName=choice.split("=");
                if (valueName.length()<2) {
                    pCombo->addItem("");
                } else {
                    pCombo->addItem(valueName[0]);
                }
            }
        } else {
            pCombo->addItem(QObject::tr("No"));
            pCombo->addItem(QObject::tr("Yes"));
        }
        pCombo->setCurrentIndex(pOption->value);
        pLayout->addWidget(pCombo,row,1);
    }
    for (int i=0;i<pTab->count();i++) {
        QWidget* pWidget = pTab->widget(i);
        QGridLayout *pLayout = (QGridLayout*)pWidget->layout();
        int row = pLayout->rowCount();
        QSpacerItem* horizontalSpacer = new QSpacerItem(10, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);
        pLayout->addItem(horizontalSpacer,row,0);
    }
}

static void loadCompilerSetSettings(Settings::PCompilerSet pSet, Ui::CompilerSetOptionWidget* ui) {
    ui->chkUseCustomCompilerParams->setChecked(pSet->useCustomCompileParams());
    ui->txtCustomCompileParams->setPlainText(pSet->customCompileParams());
    ui->txtCustomCompileParams->setEnabled(pSet->useCustomCompileParams());
    ui->chkUseCustomLinkParams->setChecked(pSet->useCustomLinkParams());
    ui->txtCustomLinkParams->setPlainText(pSet->customLinkParams());
    ui->txtCustomLinkParams->setEnabled(pSet->useCustomLinkParams());
    ui->chkStaticLink->setChecked(pSet->staticLink());
    ui->chkAutoAddCharset->setChecked(pSet->autoAddCharsetParams());

    //rest tabs in the options widget
    resetOptionTabs(pSet,ui->optionTabs);

    ui->txtCCompiler->setText(pSet->CCompiler());
    ui->txtCppCompiler->setText(pSet->cppCompiler());
    ui->txtMake->setText(pSet->make());
    ui->txtDebugger->setText(pSet->debugger());
    ui->txtResourceCompiler->setText(pSet->resourceCompiler());
    ui->txtProfiler->setText(pSet->profiler());
}

void CompilerSetOptionWidget::doLoad()
{
    ui->cbCompilerSet->clear();
    int index=pSettings->compilerSets().defaultIndex();
    for (int i=0;i<pSettings->compilerSets().list().size();i++) {
        ui->cbCompilerSet->addItem(pSettings->compilerSets().list()[i]->name());
    }
    ui->cbCompilerSet->setCurrentIndex(index);

    //reloadCurrentCompilerSet();
}

void CompilerSetOptionWidget::doSave()
{
    pSettings->compilerSets().saveSets();
}

void CompilerSetOptionWidget::on_cbCompilerSet_currentIndexChanged(int index)
{
    if (index<0)
        return;
    pSettings->compilerSets().setDefaultIndex(index);
    reloadCurrentCompilerSet();
}

void CompilerSetOptionWidget::reloadCurrentCompilerSet()
{
    Settings::PCompilerSet pSet = pSettings->compilerSets().defaultSet();
    loadCompilerSetSettings(pSet, ui);

    mBinDirWidget->setDirList(pSet->binDirs());
    mLibDirWidget->setDirList(pSet->libDirs());
    mCIncludeDirWidget->setDirList(pSet->CIncludeDirs());
    mCppIncludeDirWidget->setDirList(pSet->CppIncludeDirs());

    connectInputs();
}

void CompilerSetOptionWidget::on_btnFindCompilers_pressed()
{
    if (QMessageBox::warning(this,tr("Confirm"),
           tr("Red Panda C++ will clear current compiler list and search"
                      " for compilers in the following locations:\n '%1'\n'%2'\nAre you really want to continue?")
                                 .arg(includeTrailingPathDelimiter(pSettings->dirs().app()) + "MinGW32")
                                 .arg(includeTrailingPathDelimiter(pSettings->dirs().app()) + "MinGW64"),
                                 QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok )
        return;
    pSettings->compilerSets().clearSets();
    pSettings->compilerSets().findSets();
    doLoad();
}

void CompilerSetOptionWidget::on_btnAddBlankCompilerSet_pressed()
{
    QString name = QInputDialog::getText(this,tr("Compiler Set Name"),tr("Name"));
    pSettings->compilerSets().addSet();
    pSettings->compilerSets().setDefaultIndex(pSettings->compilerSets().list().size()-1);
    pSettings->compilerSets().defaultSet()->setName(name);
    doLoad();
}

void CompilerSetOptionWidget::on_btnAddCompilerSetByFolder_pressed()
{
    QString folder = QFileDialog::getExistingDirectory(this, tr("Compiler Set Folder"));
    pSettings->compilerSets().addSets(folder);
    doLoad();
}

void CompilerSetOptionWidget::on_btnRenameCompilerSet_pressed()
{
    QString name = QInputDialog::getText(this,tr("Compiler Set Name"),tr("New name"));
    if (!name.isEmpty())
        pSettings->compilerSets().defaultSet()->setName(name);
    doLoad();
}
