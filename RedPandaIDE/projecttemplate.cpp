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
#include "projecttemplate.h"
#include <QFile>
#include <QMessageBox>
#include "mainwindow.h"
#include "settings.h"

ProjectTemplate::ProjectTemplate(QObject *parent) : QObject(parent)
{

}

int ProjectTemplate::unitCount()
{
    if (!mIni || mVersion<=0)
        return 0;
    return mIni->GetLongValue("Project","UnitCount",0);
}

PTemplateUnit ProjectTemplate::unit(int index)
{
    if (!mIni || mVersion<=0)
        return PTemplateUnit();
    QString section = QString("Unit%1").arg(index);
    if (mIni->GetSectionSize(toByteArray(section))<0) return PTemplateUnit();
    PTemplateUnit unit = std::make_shared<TemplateUnit>();
    QString lang = pSettings->environment().language();
    if (!lang.isEmpty()) {
        unit->Source = fromByteArray(mIni->GetValue(toByteArray(section), QString("Source[%1]").arg(lang).toUtf8(), ""));
        unit->CText = fromByteArray(mIni->GetValue(toByteArray(section), QString("C[%1]").arg(lang).toUtf8(), ""));
        unit->CppText = fromByteArray(mIni->GetValue(toByteArray(section), QString("Cpp[%1]").arg(lang).toUtf8(), ""));
    }
    if (unit->Source.isEmpty())
        unit->Source = fromByteArray(mIni->GetValue(toByteArray(section), "Source", ""));
    if (unit->CText.isEmpty())
        unit->CText = fromByteArray(mIni->GetValue(toByteArray(section), "C", ""));
    if (unit->CppText.isEmpty())
        unit->CppText = fromByteArray(mIni->GetValue(toByteArray(section), "Cpp", ""));
    if (unit->CppText.isEmpty())
        unit->CppText = unit->CText;
    unit->CName = fromByteArray(mIni->GetValue(toByteArray(section), "CName", ""));
    unit->CppName = fromByteArray(mIni->GetValue(toByteArray(section), "CppName", ""));
    if (unit->CppName.isEmpty())
        unit->CppName = unit->CName;
    unit->Target = fromByteArray(mIni->GetValue(toByteArray(section), "Target", ""));
    unit->overwrite = mIni->GetBoolValue(toByteArray(section), "Overwrite", true);
    return unit;
}

void ProjectTemplate::readTemplateFile(const QString &fileName)
{
    if (mIni)
        mIni=nullptr;
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        mFileName = fileName;
        mIni = std::make_shared<SimpleIni>();
        QByteArray data = file.readAll();
        if (mIni->LoadData(data.toStdString()) != SI_OK) {
            QMessageBox::critical(pMainWindow,
                                  tr("Read failed."),
                                  tr("Can't read template file '%1'.").arg(fileName),
                                  QMessageBox::Ok);
            return;
        }
    } else {
        QMessageBox::critical(pMainWindow,
                              tr("Can't Open Template"),
                              tr("Can't open template file '%1' for read.").arg(fileName),
                              QMessageBox::Ok);
        return;
    }

    mVersion = mIni->GetLongValue("Template", "Ver", 0);
    if (mVersion<=0) {
        QMessageBox::critical(pMainWindow,
                              tr("Old version template"),
                              tr("Template file '%1' has version '%2', which is unsupported.")
                              .arg(fileName)
                              .arg(mVersion),
                              QMessageBox::Ok);
        return;
    }

    QString lang = pSettings->environment().language();
    // template info
    mIcon = fromByteArray(mIni->GetValue("Template", "Icon", ""));
    if (!lang.isEmpty()) {
        mCategory = fromByteArray(mIni->GetValue("Template", QString("Category[%1]").arg(lang).toUtf8(), ""));
        mName = fromByteArray(mIni->GetValue("Template", QString("Name[%1]").arg(lang).toUtf8(), ""));
        mDescription = fromByteArray(mIni->GetValue("Template", QString("Description[%1]").arg(lang).toUtf8(), ""));
    }
    if (mCategory.isEmpty())
        mCategory = fromByteArray(mIni->GetValue("Template", "Category", ""));
    if (mName.isEmpty())
        mName = fromByteArray(mIni->GetValue("Template", "Name", ""));
    if (mDescription.isEmpty())
        mDescription = fromByteArray(mIni->GetValue("Template", "Description", ""));
    mIconInfo=fromByteArray(mIni->GetValue("Template", "IconInfo", ""));

    mOptions.icon = mIni->GetValue("Project", "Icon", "");
    mOptions.type = static_cast<ProjectType>(mIni->GetLongValue("Project", "Type", 0)); // default = gui
    mOptions.includeDirs = fromByteArray(mIni->GetValue("Project", "Includes", "")).split(";", Qt::SkipEmptyParts);
    mOptions.binDirs = fromByteArray(mIni->GetValue("Project", "Bins", "")).split(";", Qt::SkipEmptyParts);

    mOptions.libDirs = fromByteArray(mIni->GetValue("Project", "Libs", "")).split(";", Qt::SkipEmptyParts);

    mOptions.resourceIncludes = fromByteArray(mIni->GetValue("Project", "ResourceIncludes", "")).split(";", Qt::SkipEmptyParts);
    mOptions.compilerCmd = fromByteArray(mIni->GetValue("Project", "Compiler", ""));
    mOptions.cppCompilerCmd = fromByteArray(mIni->GetValue("Project", "CppCompiler", ""));
    mOptions.linkerCmd = fromByteArray(mIni->GetValue("Project", "Linker",""));
    mOptions.resourceCmd = fromByteArray(mIni->GetValue("Project", "ResourceCommand", ""));
    mOptions.isCpp = mIni->GetBoolValue("Project", "IsCpp", false);
    mOptions.includeVersionInfo = mIni->GetBoolValue("Project", "IncludeVersionInfo", false);
    mOptions.supportXPThemes = mIni->GetBoolValue("Project", "SupportXPThemes", false);
    mOptions.folderForOutput = fromByteArray(mIni->GetValue("Project", "ExeOutput", ""));
    mOptions.folderForObjFiles = fromByteArray(mIni->GetValue("Project", "ObjectOutput", ""));
    mOptions.logFilename = fromByteArray(mIni->GetValue("Project", "LogOutput", ""));
    mOptions.execEncoding = mIni->GetValue("Project","ExecEncoding", ENCODING_SYSTEM_DEFAULT);

    mOptions.staticLink  = mIni->GetBoolValue("Project", "StaticLink",true);
    mOptions.addCharset  = mIni->GetBoolValue("Project", "AddCharset",true);
    bool useUTF8 = mIni->GetBoolValue("Project", "UseUTF8", false);
    if (useUTF8) {
        mOptions.encoding = mIni->GetValue("Project","Encoding", ENCODING_UTF8);
    } else {
        mOptions.encoding = mIni->GetValue("Project","Encoding", pSettings->editor().defaultEncoding());
    }
    if (mOptions.encoding == ENCODING_AUTO_DETECT)
        mOptions.encoding = ENCODING_SYSTEM_DEFAULT;
    mOptions.modelType = (ProjectModelType)mIni->GetLongValue("Project", "ModelType", (int)ProjectModelType::FileSystem);
    mOptions.classBrowserType = (ProjectClassBrowserType)mIni->GetLongValue("Project", "ClassBrowserType", (int)ProjectClassBrowserType::CurrentFile);

}

bool ProjectTemplate::save()
{
  if (mIni) {
      return mIni->SaveFile(toByteArray(mFileName)) == SI_OK ;
  }
  return false;
}

const QString &ProjectTemplate::category() const
{
    return mCategory;
}

void ProjectTemplate::setCategory(const QString &newCategory)
{
    mCategory = newCategory;
}

const QString &ProjectTemplate::description() const
{
    return mDescription;
}

void ProjectTemplate::setDescription(const QString &newDescription)
{
    mDescription = newDescription;
}

const QString &ProjectTemplate::fileName() const
{
    return mFileName;
}

void ProjectTemplate::setFileName(const QString &newFileName)
{
    mFileName = newFileName;
}

const QString ProjectTemplate::folder() const
{
    return extractFileDir(mFileName);
}

const QString &ProjectTemplate::icon() const
{
    return mIcon;
}

void ProjectTemplate::setIcon(const QString &newIcon)
{
    mIcon = newIcon;
}

const QString &ProjectTemplate::name() const
{
    return mName;
}

void ProjectTemplate::setName(const QString &newName)
{
    mName = newName;
}

const ProjectOptions &ProjectTemplate::options() const
{
    return mOptions;
}

void ProjectTemplate::setOptions(const ProjectOptions &newOptions)
{
    mOptions = newOptions;
}

int ProjectTemplate::version() const
{
    return mVersion;
}

void ProjectTemplate::setVersion(int newVersion)
{
    mVersion = newVersion;
}

QString ProjectTemplate::iconInfo() const
{
    return mIconInfo;
}

void ProjectTemplate::setIconInfo(const QString &newIconInfo)
{
    mIconInfo = newIconInfo;
}

