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
#include <QHash>
#include <QSet>
#include <QSortFilterProxyModel>
#include <memory>
#include "projectoptions.h"
#include "utils.h"

class Project;
class Editor;
class CppParser;
class EditorList;
class QFileSystemWatcher;

enum ProjectModelNodeType {
    DUMMY_HEADERS_FOLDER,
    DUMMY_SOURCES_FOLDER,
    DUMMY_OTHERS_FOLDER,
    Folder,
    File
};

struct ProjectModelItemRecord {
    ProjectModelNodeType type;
    QString fullPath;
};

class ProjectUnit;

struct ProjectModelNode;
using PProjectModelNode = std::shared_ptr<ProjectModelNode>;
struct ProjectModelNode {
    QString text;
    std::weak_ptr<ProjectModelNode> parent;
    bool isUnit;
    std::weak_ptr<ProjectUnit> pUnit;
    int priority;
    QList<PProjectModelNode>  children;
    ProjectModelNodeType folderNodeType;
    int level;
};

struct ProjectEditorLayout {
    QString filename;
    int top;
    int left;
    int caretX;
    int caretY;
    int order;
    bool isFocused;
    bool isOpen;
};

using PProjectEditorLayout = std::shared_ptr<ProjectEditorLayout>;

class ProjectUnit {

public:
    explicit ProjectUnit(Project* parent);
    Project* parent() const;
    const QString &fileName() const;
    void setFileName(QString newFileName);
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

    PProjectModelNode &node();
    void setNode(const PProjectModelNode &newNode);

//    bool FileMissing() const;
//    void setFileMissing(bool newDontSave);

    void setNew(bool newNew);

    const QByteArray &realEncoding() const;
    void setRealEncoding(const QByteArray &newRealEncoding);

private:
    Project* mParent;
    QString mFileName;
    QString mFolder;
    bool mNew;
    bool mCompile;
    bool mCompileCpp;
    bool mOverrideBuildCmd;
    QString mBuildCmd;
    bool mLink;
    int mPriority;
    QByteArray mEncoding;
    QByteArray mRealEncoding;
    PProjectModelNode mNode;
//    bool mFileMissing;
};

using PProjectUnit = std::shared_ptr<ProjectUnit>;

class GitRepository;
class CustomFileIconProvider;
class ProjectModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit ProjectModel(Project* project, QObject* parent=nullptr);
    ~ProjectModel();
    void beginUpdate();
    void endUpdate();
private:
    Project* mProject;
    int mUpdateCount;
    CustomFileIconProvider* mIconProvider;


    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    void refreshIcon(const QModelIndex& index, bool update=true);
    void refreshIcon(const QString& filename);
    void refreshIcons();
    void refreshNodeIconRecursive(PProjectModelNode node);

    QModelIndex getNodeIndex(ProjectModelNode *node) const;
    QModelIndex getParentIndex(ProjectModelNode * node) const;

    QModelIndex rootIndex() const;

private:

    // QAbstractItemModel interface
public:
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Project *project() const;
    CustomFileIconProvider *iconProvider() const;

    // QAbstractItemModel interface
public:
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;
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
    explicit Project(const QString& filename, const QString& name,
                     EditorList* editorList,
                     QFileSystemWatcher* fileSystemWatcher,
                     QObject *parent = nullptr);

    static std::shared_ptr<Project> load(const QString& filename,
                                    EditorList* editorList,
                                    QFileSystemWatcher* fileSystemWatcher,
                                    QObject *parent = nullptr);
    static std::shared_ptr<Project> create(const QString& filename,
                                           const QString& name,
                                           EditorList* editorList,
                                           QFileSystemWatcher* fileSystemWatcher,
                                           const std::shared_ptr<ProjectTemplate> pTemplate,
                                           bool useCpp,
                                           QObject *parent = nullptr);

    ~Project();
    QString directory() const;
    QString outputFilename() const;
    QString makeFileName();
    QString xmakeFileName();
    bool unitsModifiedSince(const QDateTime& time);
    bool modified() const;
    bool modifiedSince(const QDateTime& time);
//    void setFileName(QString value);
    void setModified(bool value);

    PProjectModelNode addFolder(PProjectModelNode parentFolder, const QString& s);
    PProjectUnit  newUnit(PProjectModelNode parentNode,
                 const QString& customFileName="");
    PProjectUnit addUnit(const QString& inFileName,
                PProjectModelNode parentNode);
    QString folder();
    void buildPrivateResource();
    void closeUnit(PProjectUnit& unit);
    PProjectUnit doAutoOpen();
    bool fileAlreadyExists(const QString& s);

    QString getNodePath(PProjectModelNode node);
    void incrementBuildNumber();

    Editor* openUnit(PProjectUnit& unit, bool forceOpen=true);
    Editor* openUnit(PProjectUnit& unit, const PProjectEditorLayout& layout);
    Editor* unitEditor(const PProjectUnit& unit) const;
    Editor* unitEditor(const ProjectUnit* unit) const;

    QList<PProjectUnit> unitList();
    QStringList unitFiles();

    PProjectModelNode pointerToNode(ProjectModelNode * p, PProjectModelNode parent=PProjectModelNode());
    void rebuildNodes();
    bool removeUnit(PProjectUnit& unit, bool doClose, bool removeFile = false);
    bool removeFolder(PProjectModelNode node);
    void resetParserProjectFiles();
    void saveAll(); // save [Project] and  all [UnitX]
    void saveLayout(); // save all [UnitX]
    void saveOptions();
    void renameUnit(PProjectUnit& unit, const QString& sFileName);
    bool saveUnits();

    PProjectUnit findUnit(const QString& filename);
    PProjectUnit findUnit(const Editor* editor);

    void associateEditor(Editor* editor);
    void associateEditorToUnit(Editor* editor, PProjectUnit unit);
    bool setCompileOption(const QString &key, const QString &value);
    QString getCompileOption(const QString &key) const;

    void updateFolders();
    void setCompilerSet(int compilerSetIndex);

    bool saveAsTemplate(const QString& templateFolder,
                        const QString& name,
                        const QString& description,
                        const QString& category);

    void setEncoding(const QByteArray& encoding);

    std::shared_ptr<CppParser> cppParser();
    const QString &filename() const;

    const QString &name() const;
    void setName(const QString &newName);

    const PProjectModelNode &rootNode() const;

    ProjectOptions &options();

    ProjectModel* model() ;

    ProjectModelType modelType() const;
    void setModelType(ProjectModelType type);

    EditorList *editorList() const;

    QFileSystemWatcher *fileSystemWatcher() const;

    QString fileSystemNodeFolderPath(const PProjectModelNode& node);

    QStringList binDirs();

    void renameFolderNode(PProjectModelNode node, const QString newName);
    void loadUnitLayout(Editor *e);
signals:
    void unitRemoved(const QString& fileName);
    void unitAdded(const QString& fileName);
    void unitRenamed(const QString& oldFileName, const QString& newFileName);
    void nodeRenamed();
    void modifyChanged(bool value);

private:
    QString relativePath(const QString& filename);
    QStringList relativePaths(const QStringList& files);
    QString absolutePath(const QString& filename);
    QStringList absolutePaths(const QStringList& files);

    bool internalRemoveUnit(PProjectUnit& unit, bool doClose, bool removeFile);
    PProjectUnit internalAddUnit(const QString& inFileName,
                PProjectModelNode parentNode);

    bool assignTemplate(const std::shared_ptr<ProjectTemplate> aTemplate, bool useCpp);
    void checkProjectFileForUpdate(SimpleIni& ini);
    void createFolderNodes();
    void createFileSystemFolderNodes();
    void createFileSystemFolderNode(ProjectModelNodeType folderType, const QString& folderName, PProjectModelNode parent, const QSet<QString>& validFolders);
    PProjectModelNode getParentFileSystemFolderNode(const QString& filename);
    PProjectModelNode findFileSystemFolderNode(const QString& folderPath, ProjectModelNodeType nodeType);
    PProjectModelNode getCustomeFolderNodeFromName(const QString& name);
    void loadOptions(SimpleIni& ini);
    //PProjectUnit
    QHash<QString, PProjectEditorLayout> loadLayout();

    PProjectModelNode makeNewFolderNode(
            const QString& folderName,
            PProjectModelNode newParent,
            ProjectModelNodeType nodeType=ProjectModelNodeType::Folder,
            int priority=0);
    PProjectModelNode makeNewFileNode(
            //const QString& fileName,
            PProjectUnit unit,
            int priority,
            PProjectModelNode newParent);
    PProjectModelNode makeProjectNode();
    void open();
    void removeFolderRecurse(PProjectModelNode node);
    void updateFolderNode(PProjectModelNode node);
    void updateCompilerSetting();

private:
    QHash<QString,PProjectUnit> mUnits;
    ProjectOptions mOptions;
    QString mFilename;
    QString mName;
    bool mModified;
    QStringList mFolders;
    std::shared_ptr<CppParser> mParser;
    PProjectModelNode mRootNode;

    QHash<ProjectModelNodeType, PProjectModelNode> mSpecialNodes;
    QHash<QString, PProjectModelNode> mFileSystemFolderNodes;

    QList<PProjectModelNode> mCustomFolderNodes;
    ProjectModel mModel;
    EditorList *mEditorList;
    QFileSystemWatcher* mFileSystemWatcher;
};

#endif // PROJECT_H
