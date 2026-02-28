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
