#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"
#include "settings.h"
#include "systemconsts.h"

#include <QDir>
#include <QFile>

NewProjectDialog::NewProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{
    ui->setupUi(this);
    mTemplatesTabBar = new QTabBar(this);
    ui->verticalLayout->insertWidget(0,mTemplatesTabBar);
}

NewProjectDialog::~NewProjectDialog()
{
    delete ui;
}

void NewProjectDialog::addTemplate(const QString &filename)
{
    if (!QFile(filename).exists())
        return;
    PProjectTemplate t = std::make_shared<ProjectTemplate>();
    t->readTemplateFile(filename);
    mTemplates.append(t);
}

void NewProjectDialog::readTemplateDir()
{
    QString templateExt(".");
    templateExt += TEMPLATE_EXT;
    QDir dir(pSettings->dirs().templateDir());
    foreach (const QFileInfo& fileInfo,dir.entryInfoList()) {
        if (fileInfo.isFile()
                && fileInfo.fileName().endsWith(templateExt)) {
            addTemplate(fileInfo.absoluteFilePath());
        }
    }
    updateView();
}

void NewProjectDialog::updateView()
{
    while (mTemplatesTabBar->count()>0) {
        mTemplatesTabBar->removeTab(0);
    }
    QMap<QString,int> categories;
    foreach (const PProjectTemplate& t, mTemplates) {
        QString category = t->category();
        if (category.isEmpty())
            category = tr("Default");
        // Add a page for each unique category
        int tabIndex = categories.value(category,-1);
        if (tabIndex<0) {
            tabIndex = mTemplatesTabBar->addTab(category);
            categories.insert(category,tabIndex);
        }
    }


        // Only add if we're viewing this category
        if SameText(TemplateItem.Category, TabsMain.Tabs[TabsMain.TabIndex]) then begin
          ListItem := ProjView.Items.Add;
          ListItem.Caption := TemplateItem.Name;
          ListItem.Data := pointer(I);
          IconFileName := ValidateFile(TemplateItem.Icon, '', true);
          if IconFileName <> '' then begin

            // Add icon to central dump and tell ListItem to use it
            IconItem := TIcon.Create;
            try
              IconItem.LoadFromFile(IconFileName); // ValidateFile prepends path
              ListItem.ImageIndex := ImageList.AddIcon(IconItem);
              if ListItem.ImageIndex = -1 then
                ListItem.ImageIndex := 0;
            finally
              IconItem.Free;
            end;
          end else
            ListItem.ImageIndex := 0; // don't use an icon
        end;
    }
}
