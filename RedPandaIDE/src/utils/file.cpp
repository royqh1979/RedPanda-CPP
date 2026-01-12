/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#include "file.h"
#include "../systemconsts.h"

#include <QFileInfo>
#include <QDir>
#include <QMap>

FileType getFileType(const QString &filename)
{
    if (filename.isEmpty())
        return FileType::None;
    if (filename.startsWith("makefile", PATH_SENSITIVITY)) {
        return FileType::MakeFile;
    }
    if (filename.endsWith(".s",PATH_SENSITIVITY)) {
        return FileType::GAS;
    }
    if (filename.endsWith(".S",PATH_SENSITIVITY)) {
        return FileType::GAS;
    }
    if (filename.endsWith(".asm",PATH_SENSITIVITY)) {
        return FileType::NASM;
    }
    if (filename.endsWith(".dev",PATH_SENSITIVITY)) {
        return FileType::Project;
    }
    if (filename.endsWith(".C")) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".CPP")) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".c",PATH_SENSITIVITY)) {
        return FileType::CSource;
    }
    if (filename.endsWith(".cpp",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".cc",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".cxx",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".c++",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".H")) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".h",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".hpp",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".hh",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".hxx",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".tcc",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".inl",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".gimple",PATH_SENSITIVITY)) {
        return FileType::GIMPLE;
    }
    if (filename.endsWith(".p",PATH_SENSITIVITY)) {
        return FileType::PreprocessedSource;
    }
    if (filename.endsWith(".rc",PATH_SENSITIVITY)) {
        return FileType::WindowsResourceSource;
    }
    if (filename.endsWith(".in",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".out",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".txt",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".md",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".info",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".dat",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".lua",PATH_SENSITIVITY)) {
        return FileType::LUA;
    }
    if (filename.endsWith(".fs",PATH_SENSITIVITY)) {
        return FileType::FragmentShader;
    }
    if (filename.endsWith(".vs",PATH_SENSITIVITY)) {
        return FileType::VerticeShader;
    }
    if (filename.endsWith(".def",PATH_SENSITIVITY)) {
        return FileType::ModuleDef;
    }
    QFileInfo info(filename);
    if (info.suffix().isEmpty()) {
        return FileType::Other;
    }
    return FileType::Other;
}

static const QMap<QString,FileType> FileTypeMapping{
    {"None", FileType::None},
    {"ATTASM", FileType::GAS},
    {"INTELASM", FileType::GAS},
    {"LUA", FileType::LUA},
    {"CSource", FileType::CSource}, // c source file (.c)
    {"CppSource", FileType::CppSource}, // c++ source file (.cpp)
    {"CCppHeader", FileType::CCppHeader}, // c header (.h)
    {"WindowsResourceSource", FileType::WindowsResourceSource}, // resource source (.res)
    {"Project", FileType::Project}, //Red Panda C++ Project (.dev)
    {"Text", FileType::Text}, // text file
    {"FragmentShader", FileType::FragmentShader},
    {"VerticeShader", FileType::VerticeShader},
    {"ModuleDef", FileType::ModuleDef}, // Windows Module Definition
    {"MakeFile", FileType::MakeFile},
    {"Other", FileType::Other},  // Any others
    {"NASM", FileType::NASM},  // Any others
    {"GAS", FileType::GAS},  // Any others
};

QString fileTypeToName(FileType fileType)
{
    for(auto i=FileTypeMapping.constBegin();i!=FileTypeMapping.constEnd();++i) {
        if (i.value()==fileType)
            return i.key();
    }
    return "None";
}

FileType nameToFileType(const QString &name)
{
    return FileTypeMapping.value(name, FileType::None);
}

NonExclusiveTemporaryFileOwner::NonExclusiveTemporaryFileOwner(std::unique_ptr<QTemporaryFile> &tempFile) :
    filename(tempFile ? tempFile->fileName() : QString())
{
    if (tempFile) {
        tempFile->flush();
        tempFile->setAutoRemove(false);
        tempFile = nullptr;
    }
}

NonExclusiveTemporaryFileOwner::~NonExclusiveTemporaryFileOwner()
{
    if (!filename.isEmpty())
        QFile::remove(filename);
}

int getNewFileNumber()
{
    static int count = 0;
    count++;
    return count;
}
