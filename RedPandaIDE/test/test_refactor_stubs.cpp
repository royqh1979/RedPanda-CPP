/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#include "src/mainwindow.h"
#include "src/editormanager.h"
#include "src/widgets/searchresultview.h"

IconsManager *MainWindow::iconsManager() const
{
    return nullptr;
}

void MainWindow::updateEditorColorSchemes()
{
}

std::shared_ptr<Project> MainWindow::project()
{
    return nullptr;
}

EditorManager *MainWindow::editorManager() const
{
    return nullptr;
}

SearchResultModel *MainWindow::searchResultModel()
{
    return nullptr;
}

bool EditorManager::getContentFromOpenedEditor(const QString &, QStringList &) const
{
    return false;
}

Editor *EditorManager::getOpenedEditor(const QString &) const
{
    return nullptr;
}

void EditorManager::beginUpdate()
{
}

void EditorManager::endUpdate()
{
}

void EditorManager::forceCloseEditor(Editor *)
{
}

void EditorManager::activeEditor(Editor *, bool)
{
}

Editor *EditorManager::newEditor(const QString &, const QByteArray &, FileType,
                                 const QString &, bool, QTabWidget *)
{
    return nullptr;
}

bool EditorManager::closeEditor(Editor *, bool, bool)
{
    return true;
}

int EditorManager::pageCount() const
{
    return 0;
}

Editor *EditorManager::operator[](int)
{
    return nullptr;
}

void EditorManager::getVisibleEditors(Editor *&left, Editor *&right) const
{
    left = nullptr;
    right = nullptr;
}

void reconfigureCppParser(std::shared_ptr<CppParser>, int)
{
}
