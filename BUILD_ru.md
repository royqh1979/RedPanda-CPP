# Основые замечания по вопросу разработки

Prerequisites:

- Qt 6.8+ или 5.15.
  - Building with Qt 5.15 is possible, but the `update_translations` target is missing.
- C++ development environment that support CMake and/or xmake. Рекомендуемые:
  - Visual Studio Code -- лучшая производительность, better AI integration.
  - Qt Creator -- встроенный дизайнер пользовательского интерфейса (UI designer), интеграция отладчика с Qt.
    - `lupdate`: Add an external tool in “Edit > Preferences > Environment > External Tools” – Executable: `cmake`; Arguments: `--build . --target update_translations`; Working directory: Choose global variable `ActiveProject:BuildConfig:Path`.

# Windows


| Библиотека+Инструмент \ Цель | x86 | x64 | ARM64 |
| ---------------------------- | --- | --- | ----- |
| [Windows NT 5.x](https://github.com/redpanda-cpp/qtbase-xp) + [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) | ✔️ | ✔️ | ❌ |

<!--
| MSYS2 + GNU-based MinGW | ❌ | ✔️ | ❌ |
| MSYS2 + LLVM-based MinGW | ❌ | ✔️ | ✔️ |

Смотри также [другие инструкции сборки для Windows](./docs/detailed-build-win-ru.md).

## MSYS2 Библиотека Qt с набором инструментов MinGW (Рекомендуется)

Red Panda C++ должна работать с любым 64-битным набором инструментов MinGW от MSYS2, включая GCC и Clangs в средах на основе GNU (MINGW64 и UCRT64), и Clangs в средах на основе LLVM (CLANG64 и CLANGARM64; см. также [документацию MSYS2] (https://www.msys2.org/docs/environments/)), в то время как следующие наборы инструментов часто тестируются:
- MINGW64 GCC,
- UCRT64 GCC (рекомендуется для x64)
- CLANGARM64 Clang (единственный и рекомендуемый набор инструментов для ARM64).

Подготовительный этап:

0. Требуется Windows 10 x64 или более поздний, или Windows 11 ARM64.
1. Установить MSYS2.
2. В выбранном окружении, установить набор инструментов, Библиотеку Qt 5 и требуемые приложения:
   ```bash
   pacman -S \
     $MINGW_PACKAGE_PREFIX-{cc,make,qt5-static,7zip,cmake} \
     mingw-w64-i686-nsis \
     git curl
   ```

Для сборки в запущенном окружении MSYS2 выполните:
```bash
./packages/msys/build-mingw.sh
```
для сборки программы установи Red Panda C++ и портируемого пакета с помощью набора инструментов MinGW GCC или без компилятора; 

или выполните:
```bash
./packages/msys/build-llvm.sh
```
для сборки программы установки Red Panda C++ с набором инструментов LLVM MinGW.

Основные аргументы коммандной строки:
- `-h`, `--help`: показать справочную информацию.
- `-c`, `--clean`: очистить каталог сборки.
- `-nd`, `--no-deps`: не проверять зависимости.
- `-t <dir>`, `--target-dir <dir>`: установить целевой каталог для пакетов. По умолчанию: `dist/`.

Дополнительные аргументя для `build-mingw.sh`:
- `--mingw32`: и `assets/mingw32.7z` для сборки пакета.
- `--mingw64`: и `assets/mingw64.7z` для сборки пакета.
- `--mingw`: псевдоним для  `--mingw64` (x64-приложение).
- `--gcc-linux-x86-64`: добавить `assets/gcc-linux-x86-64.7z` и `assets/alpine-minirootfs-x86_64.tar` в пакет.
- `--gcc-linux-aarch64`: добавить `assets/gcc-linux-aarch64.7z` и `assets/alpine-minirootfs-aarch64.tar` в пакет.
- `--ucrt`: include UCRT installer (VC_redist) in the package.
-->

## Windows NT 5.x с библиотекой Qt с набором инструментов MinGW Lite

Скрипты `build-xp.sh` подобны `build-mingw.sh`, но набор инструментов обеспечивается библиотекой Qt.

Подготовительные действия для сборки в естественной среде:

0. Требуется Windows 10 x64 или более поздний.
1. Установить MSYS2.

Для сборки в естественной среде в запущенном окружении MSYS2 выполните:
```bash
./packages/mingw/build-xp.sh -p 32-msvcrt
```

Эти скрипты принимают такие же аргументы, как `build-mingw.sh`, дополнительно к этому:
- `-p|--profile <profile>`: (ТРЕБУЕТСЯ) профиль MinGW Lite с библиотекой Qt. Доступные профили `64-ucrt`, `64-msvcrt`, `32-ucrt`, `32-msvcrt`.

# Linux

Смотри также [другие инструкции сборки для настольных систем freedesktop.org-conforming (XDG)](./docs/detailed-build-xdg-ru.md).

## Alpine Linux, Arch Linux, Debian и их производные, Fedora, openSUSE

1. Установите окружение сборки (документация для [Alpine](https://wiki.alpinelinux.org/wiki/Abuild_and_Helpers), [Arch](https://wiki.archlinux.org/title/Makepkg), [Debians](https://wiki.debian.org/BuildingTutorial), [RPM](https://rpm-packaging-guide.github.io/#prerequisites)).
   - Для Debians:
     ```sh
     sudo apt install --no-install-recommends build-essential debhelper devscripts equivs
     ```
2. Запустите скрипт сборки:
   - Alpine Linux: `./packages/alpine/buildapk.sh`
   - Arch Linux: `./packages/archlinux/buildpkg.sh`
   - Debians: `./packages/debian/builddeb.sh`
   - Fedora: `./packages/fedora/buildrpm.sh`
   - openSUSE: `./packages/opensuse/buildrpm.sh`
3. Установите пакет:
   - Alpine Linux: `~/packages/unsupported/$(uname -m)/redpanda-cpp-git-*.apk`
   - Arch Linux: `/tmp/redpanda-cpp-git/redpanda-cpp-git-*.pkg.tar.zst`
   - Debians: `/tmp/redpanda-cpp_*.deb`
   - Fedora, openSUSE: `~/rpmbuild/RPMS/$(uname -m)/redpanda-cpp-git-*.rpm`
4. Запустите Red Panda C++:
   ```bash
   RedPandaIDE
   ```

Обратите внимание, что некоторые из этих сценариев проверяют ветку HEAD рекозитория, поэтому любые изменения должны быть зафиксированы (commit) перед созданием.

Альтернативно, можно собрать в контейнере (предпочтительно без корня Podman; Docker может нарушить права доступа к файлу):


```sh
podman run --rm -v $PWD:/mnt -w /mnt <image> ./packages/<distro>/01-in-docker.sh

# Arch Linux for example
podman run --rm -v $PWD:/mnt -w /mnt docker.io/archlinux:latest ./packages/archlinux/01-in-docker.sh
```

Пакет будет помещен в `dist/`.

<!--
## Статическая сборка двоичных файлов для Ubuntu 20.04 x86_64 (NOI Linux 2.0)

Пакет `redpanda-cpp-bin` подобен “AppImage repack”. Двоичный файл фактически собран в контейнере. Таким образом, хост сборки - это не обязательно Ubuntu 20.04; должен работать любой дистрибутив Linux с Podman и dpkg.

1. Установите Podman и dpkg, если компьютер сборки не Debian или его производные:
   ```sh
   sudo apt install podman
   ```
   ВНИМАНИЕ: НЕ УСТАНАВЛИВАЙТЕ пакеты с dpkg на не-Debian-системах, или Ваша система будет уничтожена.
2. Запустите скрипт сборки:
   ```sh
   ./packages/debian-static/builddeb.sh
   ```

Пакет будет помещён в `dist/`.

## Linux AppImage

```bash
podman run --rm -v $PWD:/mnt -w /mnt ghcr.io/redpanda-cpp/appimage-builder-x86_64:20241204.0 ./packages/appimage/01-in-docker.sh
```

Dockerfiles доступны в [redpanda-cpp/appimage-builder](https://github.com/redpanda-cpp/appimage-builder). Доступные архитектуры: `x86_64`, `aarch64`, `riscv64`, `loong64`, `i686`.
-->

<!--
# macOS

## Qt.io библиотека Qt

Подготовительные действия:

0. Недавно выпущенная macOS, удовлятворяющая требованиям [Qt 5](https://doc.qt.io/qt-5/macos.html) или [Qt 6](https://doc.qt.io/qt-6/macos.html).
1. Установите инструменты командной строки для Xcode:
   ```zsh
   xcode-select --install
   ```
2. Установите Qt с помощью онлайн-установщика из [Qt.io](https://www.qt.io/download-qt-installer-oss).
   - Выберите библиотеку (в группе _Qt_, подгруппа _Qt 5.15.2_ или _Qt 6.8.0_, проверьте, что для _macOS_).

Для сборки, запустите один из скриптов:

```zsh
./packages/macos/build.sh -a x86_64 --qt-version 5.15.2
./packages/macos/build.sh -a x86_64 --qt-version 6.8.0
./packages/macos/build.sh -a arm64 --qt-version 6.8.0
./packages/macos/build.sh -a universal --qt-version 6.8.0
```
-->
