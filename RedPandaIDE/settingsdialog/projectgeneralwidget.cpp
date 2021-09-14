#include "projectgeneralwidget.h"
#include "ui_projectgeneralwidget.h"
#include "../project.h"
#include "../mainwindow.h"
#include "settings.h"
#include "../systemconsts.h"

#include <QFileDialog>
#include <QIcon>
#include <QTextCodec>

ProjectGeneralWidget::ProjectGeneralWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectGeneralWidget)
{
    ui->setupUi(this);
}

ProjectGeneralWidget::~ProjectGeneralWidget()
{
    delete ui;
}

void ProjectGeneralWidget::refreshIcon()
{
    QPixmap icon(mIconPath);
    ui->lblICon->setPixmap(icon);
}

void ProjectGeneralWidget::doLoad()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    if (!project)
        return;
    ui->txtName->setText(project->name());
    ui->txtFileName->setText(project->filename());
    ui->txtOutputFile->setText(project->executable());

    int srcCount=0,headerCount=0,resCount=0,otherCount=0, totalCount=0;
    foreach (const PProjectUnit& unit, project->units()) {
        switch(getFileType(unit->fileName())) {
        case FileType::CSource:
        case FileType::CppSource:
            srcCount++;
            break;
        case FileType::CppHeader:
        case FileType::CHeader:
            headerCount++;
            break;
        case FileType::WindowsResourceSource:
            resCount++;
            break;
        default:
            otherCount++;
        }
        totalCount++;
    }
    ui->lblFiles->setText(tr("%1 files [ %2 sources, %3 headers, %4 resources, %5 other files ]")
                          .arg(totalCount).arg(srcCount).arg(headerCount)
                          .arg(resCount).arg(otherCount));

    ui->cbDefaultEncoding->addItems(pSystemConsts->codecNames());
    ui->cbDefaultEncoding->setCurrentText(project->options().encoding);

    ui->lstType->setCurrentRow( static_cast<int>(project->options().type));

    ui->cbDefaultCpp->setChecked(project->options().useGPP);
    ui->cbSupportXPTheme->setChecked(project->options().supportXPThemes);
    mIconPath = project->options().icon;
    QPixmap icon(mIconPath);
    refreshIcon();
}

void ProjectGeneralWidget::doSave()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    if (!project)
        return;
    project->setName(ui->txtName->text().trimmed());

    project->options().encoding = ui->cbDefaultEncoding->currentText();

    int row = std::max(0,ui->lstType->currentRow());
    project->options().type = static_cast<ProjectType>(row);

    project->options().useGPP = ui->cbDefaultCpp->isChecked();
    project->options().supportXPThemes = ui->cbSupportXPTheme->isChecked();
    if (mIconPath.isEmpty()
            || ui->lblICon->pixmap(Qt::ReturnByValue).isNull()) {
        project->options().icon = "";
    } else {
        QString iconPath = changeFileExt(project->filename(),"ico");
        QFile::copy(mIconPath, iconPath);
        project->options().icon = iconPath;
        mIconPath = iconPath;
        refreshIcon();
    }

    project->saveOptions();
}

void ProjectGeneralWidget::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select icon file"),
                                                    pSettings->dirs().app(),
                                                    tr("Icon Files (*.ico)"));
    if (!fileName.isEmpty()) {
        mIconPath = fileName;
        QPixmap icon(mIconPath);
        refreshIcon();
    }
    ui->btnRemove->setEnabled(!mIconPath.isEmpty());
}


void ProjectGeneralWidget::on_btnRemove_clicked()
{
    mIconPath = "";
    ui->lblICon->setPixmap(QPixmap());
    ui->btnRemove->setEnabled(!mIconPath.isEmpty());
}

