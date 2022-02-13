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
#ifndef PROJECT_H
#define PROJECT_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <memory>
#include "projectoptions.h"
#include "utils.h"

class Project;
class Editor;
class CppParser;


enum ProjectSpecialFolderNode {
    HEADERS,
    SOURCES,
    OTHERS,
    NonSpecial,
    NotFolder
};

struct ProjectModelNode;
using PProjectModelNode = std::shared_ptr<ProjectModelNode>;
struct ProjectModelNode {
    QString text;
    std::weak_ptr<ProjectModelNode> parent;
    int unitIndex;
    int priority;
    QList<PProjectModelNode>  children;
    ProjectSpecialFolderNode folderNodeType;
    int level;
};

class ProjectUnit {

public:
    explicit ProjectUnit(Project* parent);
    Project* parent() const;
    void setParent(Project* newParent);
    Editor *editor() const;
    void setEditor(Editor *newEditor);
    const QString &fileName() const;
    void setFileName(QString newFileName);
    bool isNew() const;
    void setNew(bool newNew);
    const QString &folder() const;
    void setFolder(const QString &newFolder);
    bool compile() const;
    void setCompile(bool newCompile);
    bool compileCpp() const;
    void setCompileCpp(bool newCompileCpp);
    bool overrideBuildCmd() const;
    void setOverrideBuildCmd(bool newOverrideBuildCmd);
    const QString &buildCmd() const;
    void setBuildCmd(const QString &newBuildCmd);
    bool link() const;
    void setLink(bool newLink);
    int priority() const;
    void setPriority(int newPriority);
    const QByteArray &encoding() const;
    void setEncoding(const QByteArray &newEncoding);
    bool modified() const;
    void setModified(bool value);
    bool save();

    PProjectModelNode &node();
    void setNode(const PProjectModelNode &newNode);

private:
    Project* mParent;
    Editor* mEditor;
    QString mFileName;
    bool mNew;
    QString mFolder;
    bool mCompile;
    bool mCompileCpp;
    bool mOverrideBuildCmd;
    QString mBuildCmd;
    bool mLink;
    int mPriority;
    QByteArray mEncoding;
    PProjectModelNode mNode;
};

using PProjectUnit = std::shared_ptr<ProjectUnit>;

class GitManager;
class ProjectModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit ProjectModel(Project* project, QObject* parent=nullptr);
    ~ProjectModel();
    void beginUpdate();
    void endUpdate();
private:
    Project* mProject;
    GitManager *mVCSManager;
    int mUpdateCount;


    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
    QModelIndex getParentIndex(ProjectModelNode * node) const;
    // QAbstractItemModel interface
public:
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Project *project() const;
};

class ProjectModelSortFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ProjectModelSortFilterProxy(QObject *parent = nullptr);
    // QSortFilterProxyModel interface
protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
};

class ProjectTemplate;
class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(const QString& filename, const QString& name,QObject *parent = nullptr);
    ~Project();
    QString directory() const;
    QString executable() const;
    QString makeFileName();
    bool modified() const;
    void setFileName(QString value);
    void setModified(bool value);

    void addFolder(const QString& s);
    PProjectUnit addUnit(const QString& inFileName,
                PProjectModelNode parentNode,
                bool rebuild);
    QString folder();
    void buildPrivateResource(bool forceSave=false);
    void closeUnit(int index);
    void doAutoOpen();
    bool fileAlreadyExists(const QString& s);
    char getCompilerOption(const QString& optionString);
    QString getFolderPath(PProjectModelNode node);
    int getUnitFromString(const QString& s);
    void incrementBuildNumber();
    int indexInUnits(const QString& fileName) const;
    int indexInUnits(const Editor* editor) const;
    PProjectUnit  newUnit(PProjectModelNode parentNode,
                 const QString& customFileName="");
    Editor* openUnit(int index);
    PProjectModelNode pointerToNode(ProjectModelNode * p, PProjectModelNode parent=PProjectModelNode());
    void rebuildNodes();
    bool removeUnit(int index, bool doClose, bool removeFile = false);
    bool removeFolder(PProjectModelNode node);
    void resetParserProjectFiles();
    void saveAll(); // save [Project] and  all [UnitX]
    void saveLayout(); // save all [UnitX]
    void saveOptions();
    void saveUnitAs(int i, const QString& sFileName, bool syncEditor = true); // save single [UnitX]
    bool saveUnits();
    PProjectUnit findUnitByFilename(const QString& filename);
    void associateEditor(Editor* editor);
    void associateEditorToUnit(Editor* editor, PProjectUnit unit);
    void setCompilerOption(const QString& optionString, char value);
    void updateFolders();
    void updateNodeIndexes();
    void setCompilerSet(int compilerSetIndex);

    //void showOptions();
    bool assignTemplate(const std::shared_ptr<ProjectTemplate> aTemplate, bool useCpp);
    //void saveToLog();

    std::shared_ptr<CppParser> cppParser();
    const QString &filename() const;

    const QString &name() const;
    void setName(const QString &newName);

    const PProjectModelNode &rootNode() const;

    ProjectOptions &options();

    ProjectModel* model() ;

    const QList<PProjectUnit> &units() const;

    ProjectModelType modelType() const;
    void setModelType(ProjectModelType type);

signals:
    void nodesChanged();
    void modifyChanged(bool value);
private:
    void checkProjectFileForUpdate(SimpleIni& ini);
    void createFolderNodes();
    void createFileSystemFolderNodes();
    void createFileSystemFolderNode(ProjectSpecialFolderNode folderType, const QString& folderName, PProjectModelNode parent);
    PProjectModelNode getParentFolderNode(const QString& filename);
    PProjectModelNode findFolderNode(const QString& folderPath, ProjectSpecialFolderNode nodeType);
    PProjectModelNode folderNodeFromName(const QString& name);
    void loadOptions(SimpleIni& ini);
    void loadLayout(); // load all [UnitX]
    void loadUnitLayout(Editor *e, int index); // load single [UnitX] cursor positions

    PProjectModelNode makeNewFileNode(const QString& s, bool isFolder, PProjectModelNode newParent);
    PProjectModelNode makeProjectNode();
    void open();
    void removeFolderRecurse(PProjectModelNode node);
    void saveUnitLayout(Editor* e, int index); // save single [UnitX] cursor positions
    void updateFolderNode(PProjectModelNode node);
    void updateCompilerSetType();

private:
    QList<PProjectUnit> mUnits;
    ProjectOptions mOptions;
    QString mFilename;
    QString mName;
    bool mModified;
    QStringList mFolders;
    std::shared_ptr<CppParser> mParser;
    QList<PProjectModelNode> mFolderNodes;
    PProjectModelNode mRootNode;
    QHash<ProjectSpecialFolderNode, PProjectModelNode> mSpecialNodes;
    QHash<QString, PProjectModelNode> mFileSystemFolderNodes;
    ProjectModel mModel;
};

#endif // PROJECT_H
