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
#ifndef UTILS_FILE_H
#define UTILS_FILE_H
#include <QTemporaryFile>
#include <memory>
#include <QString>


enum class FileType{
    None,
    ATTASM, // deprecated: AT&T Style GNU assembler source file (.s)
    INTELASM, // deprecated: Intel Style GNU assembler source file (.s)
    LUA, // lua file (.lua)
    CSource, // c source file (.c)
    CppSource, // c++ source file (.cpp)
    CCppHeader, // c header (.h)
    PreprocessedSource, //(.p)
    GIMPLE, // gcc gimple file (.gimple)
    WindowsResourceSource, // resource source (.res)
    Project, //Red Panda C++ Project (.dev)
    Text, // text file
    FragmentShader,
    VerticeShader,
    ModuleDef, // Windows Module Definition
    MakeFile,
    Other, // any others
    NASM, // NASM Files
    GAS, //GAS Files
};

struct NonExclusiveTemporaryFileOwner {
    const QString filename;

    // take ownership
    explicit NonExclusiveTemporaryFileOwner(std::unique_ptr<QTemporaryFile> &tempFile);

    NonExclusiveTemporaryFileOwner(const NonExclusiveTemporaryFileOwner &) = delete;
    NonExclusiveTemporaryFileOwner(NonExclusiveTemporaryFileOwner &&) = delete;
    NonExclusiveTemporaryFileOwner& operator=(const NonExclusiveTemporaryFileOwner &) = delete;
    NonExclusiveTemporaryFileOwner& operator=(NonExclusiveTemporaryFileOwner &&) = delete;
    ~NonExclusiveTemporaryFileOwner();
};

using PNonExclusiveTemporaryFileOwner = std::unique_ptr<NonExclusiveTemporaryFileOwner>;

FileType getFileType(const QString& filename);

QString fileTypeToName(FileType fileType);

FileType nameToFileType(const QString& name);

constexpr bool isASMSourceFile(FileType fileType) { return fileType == FileType::NASM || fileType == FileType::GAS; }

constexpr bool isC_CPPSourceFile(FileType fileType) {
    return fileType == FileType::CSource || fileType == FileType::CppSource;
}

constexpr bool isC_CPPHeaderFile(FileType fileType) {
    return fileType == FileType::CCppHeader;
}

constexpr bool isC_CPP_ASMSourceFile(FileType fileType) {
    return isC_CPPSourceFile(fileType) || isASMSourceFile(fileType);
}


int getNewFileNumber();

#endif // UTILS_FILE_H
