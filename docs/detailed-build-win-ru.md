# Дополнительные инструкции по сборке для Windows

| Библиотека + Инструменты \ Цель | x86 | x64 | ARM64 |
| ---------------------------- | --- | --- | ----- |
| MSYS2 + GNU-based MinGW | ❌ | ✔️ | ❌ |
| MSYS2 + LLVM-based MinGW | ❌ | ✔️ | ✔️ |
| [Windows XP](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW UCRT](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |
| Qt.io + MinGW | ✔️ | ✔️ | ❌ |
| Qt.io + MSVC | ✔️ | ✔️ | ❌ |
| vcpkg + MSVC | ✔️ | ✔️ | ❌ |

Переменные qmake:
- `PREFIX`: куда `$MAKE install` устанавливает файлы.
- `WINDOWS_PREFER_OPENCONSOLE=ON` (этап сборки): предпочесть UTF-8-совместимый `OpenConsole.exe`.
  - `OpenConsole.exe` является частью Windows Terminal. Поддержка ввода в UTF-8 была добавлена в версии 1.18.
  - `OpenConsole.exe` требует ConPTY, который появился в  Windows 10 1809.

Замечания для Windows на ARM:
- Red Panda C++ может быть собран для ARM64 ABI только на Windows 11 ARM64.
  - Запуск на Windows 10 ARM64 больше не поддерживается. Установщики предполагают, что эмуляция x64 всегда доступна. (Родной пакет “Red Panda C++ с инструментарием LLVM MinGW” может работать.)
  - ARM64EC (“совместимый с эмуляцией”) хост не поддерживается, т.е. Red Panda C++ не можер быть собрана с инструментарием ARM64EC.
  - Цель ARM64EC (теоретический) поддерживается, т.е. Red Panda C++ будет создавать двоичные файлы для ARM64EC, если инструментарий сборки поддерживал ARM64EC.
- В связи с [ARM32 deprecation in Windows 11 Insider Preview Build 25905](https://blogs.windows.com/windows-insider/2023/07/12/announcing-windows-11-insider-preview-build-25905/), поддержка ARM32 никогда не будет добавлена.

## Сборка вручную в MSYS2

Подготовка:

0. Windows 8.1 x64 или более поздний, или Windows 11 ARM64.
1. Установить MSYS2.
2. В выбранном окружении установить инструментарий и библиотеку Qt 5:
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{toolchain,qt5-static} \
     git
   ```

Сборка:

1. В выбранном окружении, установить необходимые переменные:
   ```bash
   SRC_DIR="/c/src/redpanda-src" # Например: “C:\src\redpanda-src”
   BUILD_DIR="/c/src/redpanda-build" # Например: “C:\src\redpanda-build”
   INSTALL_DIR="/c/src/redpanda-pkg" # Например: “C:\src\redpanda-pkg”
   ```
2. Перейти в каталог сборки:
   ```bash
   rm -rf "$BUILD_DIR" # при необходимости для очистки каталога сборки
   mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
   ```
3. Настройка, сборка и установка:
   ```bash
   $MSYSTEM_PREFIX/qt5-static/bin/qmake PREFIX="$INSTALL_DIR" "$SRC_DIR/Red_Panda_CPP.pro"
   mingw32-make -j$(nproc)
   mingw32-make install
   ```

## Библиотека Qt Library из Qt.io с инструментарием MinGW или MSVC

Подготовка:

0. Windows 10 x64 или более поздний. ARM64 не поддерживается.
   - Для инструментария MSVC, Windows должна использовать Unicode UTF-8 для поддержки языков всего мира.
1. Установить Qt с помощью установки по сети из [Qt.io](https://www.qt.io/download-qt-installer-oss).
   - Выбрать библиотеку (в группе _Qt_, подгруппе _Qt 5.15.2_, выберите одну из _MinGW 8.1.0 32-bit_, _MinGW 8.1.0 64-bit_, _MSVC 2019 32-bit_ или _MSVC 2019 64-bit_).
   - Для инструментария MinGW, выбрать инструментарий (в группе _Qt_, подгруппе _Developer and Designer Tools_, выберите _MinGW 8.1.0 32-bit_ иди _MinGW 8.1.0 64-bit_, соответствующий библиотеке).
   - По желанию выбрать Qt Creator (в группе _Qt_, подгруппе _Developer and Designer Tools_; рекомендуется для инструментария MSVC для поддержки параллельной сборки).
2. Для инструментария MSVC установить Visual Studio 2019 или более поздний, или Visual Studio Build Tools 2019 или более поздний, с поддержкой _Desktop Development with C++_.
   - На панели _Installation Details_, в разделе _Desktop Development with C++_, выберите один инструмент из _MSVC x86/x64 build tools_ и один из _Windows SDK_.

Сборка:

1. Запустить Qt environment из меню Пуск.
2. В окружении Qt environment установить необходимые переменные:
   ```bat
   rem Без кавычек, даже если путь содержит пробелы!!!
   set SRC_DIR=C:\src\redpanda-src
   set BUILD_DIR=C:\src\redpanda-build
   set INSTALL_DIR=C:\src\redpanda-pkg
   rem Для инструментария MSVC
   set VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
   rem Для инструментария MSVC; или x86
   set VC_ARCH=amd64
   rem Для инструментария MSVC; не устанавливать переменную, если Qt Creator не установлен
   set QT_CREATOR_DIR=C:\Qt\Tools\QtCreator
   ```
3. Перейдите в каталог сборки:
   ```bat
   rem При необходимости для очистки каталога сборки
   rmdir /s /q "%BUILD_DIR%"
   mkdir "%BUILD_DIR%" && cd /d "%BUILD_DIR%"
   ```
4. Настройка, сборка и установка. Для инструментария MinGW:
   ```bat
   qmake PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"
   mingw32-make -j%NUMBER_OF_PROCESSORS%
   mingw32-make install
   windeployqt "%INSTALL_DIR%\RedPandaIDE.exe"
   ```
   Для инструментария  MSVC:
   ```bat
   call "%VS_INSTALL_PATH%\Common7\Tools\VsDevCmd.bat" -arch=%VC_ARCH%
   qmake PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"

   set JOM=%QT_CREATOR_DIR%\bin\jom\jom.exe
   if "%QT_CREATOR_DIR%" neq "" (
      "%JOM%" -j%NUMBER_OF_PROCESSORS%
      "%JOM%" install
   ) else (
      nmake
      nmake install
   )
   windeployqt "%INSTALL_DIR%\RedPandaIDE.exe"
   ```

## Продвинутый вариант: Статическая библиотека vcpkg Qt с инструментарием MSVC

Подготовка:

0. Windows 10 x64 или более поздний. ARM64 не поддерживается.
   - Для инструментария MSVC, Windows должна использовать Unicode UTF-8 для поддержки языков всего мира.
1. Установить Visual Studio 2017 или более поздний, или Visual Studio Build Tools 2017 или более поздние, с поддержкой _Desktop Development with C++_.
   - На панели _Installation Details_, в разделе _Desktop Development with C++_, выберите один инструмент из _MSVC x86/x64 build tools_ и один из _Windows SDK_.
2. Установить [standalone vcpkg](https://vcpkg.io/en/getting-started).
3. Установить Qt с vcpkg.
   ```ps1
   $TARGET = "x64-windows-static" # Или "x86-windows-static"
   vcpkg install qt5-base:$TARGET qt5-svg:$TARGET qt5-tools:$TARGET qt5-translations:$TARGET
   ```

Для сборки с помощью VS 2019 или более поздним под PowerShell (Core) или Windows PowerShell:

1. Установить требуемые переменные:
   ```ps1
   $SRC_DIR = "C:\src\redpanda-src"
   $BUILD_DIR = "C:\src\redpanda-build"
   $INSTALL_DIR = "C:\src\redpanda-pkg"
   $VCPKG_ROOT = "C:\src\vcpkg"
   $VCPKG_TARGET = "x64-windows-static" # or "x86-windows-static"
   $VS_INSTALL_PATH = "C:\Program Files\Microsoft Visual Studio\2022\Community"
   $VC_ARCH = "amd64" # Или "x86"
   $JOM = "$VCPKG_ROOT\downloads\tools\jom\jom-1.1.3\jom.exe" # check the version
   ```
2. Перейдите в каталог сборки:
   ```ps1
   Remove-Item -Recurse -Force "$BUILD_DIR" # При необходимости для очистки каталога сборки
   (New-Item -ItemType Directory -Force "$BUILD_DIR") -and (Set-Location "$BUILD_DIR")
   ```
3. Настройка, сборка и установка:
   ```ps1
   Import-Module "$VS_INSTALL_PATH\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
   Enter-VsDevShell -VsInstallPath "$VS_INSTALL_PATH" -SkipAutomaticLocation -DevCmdArguments "-arch=$VC_ARCH"
   & "$VCPKG_ROOT\installed\$VCPKG_TARGET\tools\qt5\bin\qmake.exe" PREFIX="$INSTALL_DIR" "$SRC_DIR\Red_Panda_CPP.pro"
   & "$JOM" "-j${Env:NUMBER_OF_PROCESSORS}"
   & "$JOM" install
   ```

Для сборки с VS 2017 или более поздним из коммандной строки:

1. Запустить предпочитаемое окружение VC из меню Пуск.
2. Установить необходимые переменные:
   ```bat
   rem Без кавычек, даже если путь содержит пробелы!!!
   set SRC_DIR=C:\src\redpanda-src
   set BUILD_DIR=C:\src\redpanda-build
   set INSTALL_DIR=C:\src\redpanda-pkg
   set VCPKG_ROOT=C:\src\vcpkg
   rem Или x86-windows-static
   set VCPKG_TARGET=x64-windows-static
   rem Проверка версии
   set JOM=%VCPKG_ROOT%\downloads\tools\jom\jom-1.1.3\jom.exe
   ```
3. Перейдите в каталог сборки:
   ```bat
   rem При необходимости для очистки каталога сборки
   rmdir /s /q "%BUILD_DIR%"
   mkdir "%BUILD_DIR%" && cd /d "%BUILD_DIR%"
   ```
4. Настройка, сборка и установка:
   ```bat
   "%VCPKG_ROOT%\installed\%VCPKG_TARGET%\tools\qt5\bin\qmake.exe" PREFIX="%INSTALL_DIR%" "%SRC_DIR%\Red_Panda_CPP.pro"
   "%JOM%" -j%NUMBER_OF_PROCESSORS%
   "%JOM%" install
   ```
