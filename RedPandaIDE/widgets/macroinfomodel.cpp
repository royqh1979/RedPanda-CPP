#include "macroinfomodel.h"

MacroInfoModel::MacroInfoModel(QObject *parent) : QAbstractListModel(parent)
{
    addMacroInfo("<DEFAULT>", tr("The default directory"));
    addMacroInfo("<DEVCPP>", tr("Path to the Red Panda C++'s executable file."));
    addMacroInfo("<DEVCPPVERSION>", tr("Version of the Red Panda C++"));
    addMacroInfo("<EXECPATH>", tr("PATH to the Red Panda C++'s installation folder."));
    addMacroInfo("<DATE>", tr("Current date"));
    addMacroInfo("<DATETIME>", tr("Current date and time"));
    addMacroInfo("<INCLUDE>", tr("The first include directory of the working compiler set."));
    addMacroInfo("<LIB>", tr("The first lib directory of the working compiler set."));
    addMacroInfo("<EXENAME>", tr("The compiled filename"));
    addMacroInfo("<EXEFILE>", tr("Full path to the compiled file"));
    addMacroInfo("<SOURCENAME>", tr("Filename of the current source file"));
    addMacroInfo("<SOURCEFILE>", tr("Full path to the current source file"));
    addMacroInfo("<SOURCEPATH>", tr("Path to the current source file's parent folder"));
    addMacroInfo("<WORDXY>", tr("Word at the cursor in the active editor"));
    addMacroInfo("<PROJECTNAME>", tr("Name of the current project"));
    addMacroInfo("<PROJECTFILE>", tr("Full path to the current project file"));
    addMacroInfo("<PROJECTFILENAME>", tr("Name of the current project file"));
    addMacroInfo("<PROJECTPATH>", tr("Path to the current project's folder"));
}

int MacroInfoModel::rowCount(const QModelIndex &parent) const
{
    return mMacroInfos.count();
}

QVariant MacroInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        PMacroInfo info = mMacroInfos[index.row()];
        return info->description;
    } else if (role == Qt::UserRole) {
        PMacroInfo info = mMacroInfos[index.row()];
        return info->macro;
    }
    return QVariant();
}

PMacroInfo MacroInfoModel::getInfo(const QModelIndex &index) const
{
    if (!index.isValid())
        return PMacroInfo();
    return mMacroInfos[index.row()];
}

void MacroInfoModel::addMacroInfo(const QString& macro, const QString& description) {
    PMacroInfo info = std::make_shared<MacroInfo>();
    info->macro = macro;
    info->description = description;
    mMacroInfos.append(info);
}


