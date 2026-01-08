# Дополнительные инструкции сборки для freedesktop.org-conforming (XDG) Desktop Systems

## Традиционный путь Unix (`./configure`–`make`–`make install`)

- Установить последнюю версию GCC или Clang, поддерживающую C++17.
- Установить Qt 6.8+ Base, SVG и Tools modules, включающие все библиотеки и файлы для разработки.
- По желанию устанвоить fcitx5-qt для статической сборки с Библиотекой Qt.
- Установить astyle для форматирования кода в Red Panda C++.

### Шаги сборки с CMake

1. Настройка:
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
   ```
2. Сборка:
   ```bash
   cmake --build build -- --parallel
   ```
3. Установка:
   ```bash
   sudo cmake --install build --strip
   ```

Переменные CMake:
- `CMAKE_INSTALL_PREFIX`: куда `$MAKE install` будет устанавливать файлы.
  - На саму среду Red Panda C++ `CMAKE_INSTALL_PREFIX` не влияет, поскольку она использует относительные пути.
  - На файл `.desktop` влияет `CMAKE_INSTALL_PREFIX`.
- `LIBEXECDIR`: каталог для вспомогательных исполнимых файлов, УКАЗЫВАЕТСЯ ОТНОСИТЕЛЬНО `CMAKE_INSTALL_PREFIX`.
  - Arch Linux использует `lib`.
- `OVERRIDE_MALLOC`: link specific memory allocation library. e.g. `-DOVERRIDE_MALLOC=mimalloc`.

### Шаги сборки с xmake

1. Настройка:
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr
   ```
2. Сборка:
   ```bash
   xmake
   ```
3. Установка:
   ```bash
   sudo xmake install --root -o /usr/local
   ```

Примечание: для информации о других параметрах выполните `xmake f --help`.

## Прочих архитектур

There are 2 ways to build Red Panda C++ for foreign architectures:
- Cross build: using cross toolchain.
  - As fast as native build;
  - Building cross Qt is not so easy;
  - QEMU user space emulation is still required if you want to run test cases.
- Emulated native build: используя родные инструменты сборки для целевых архитектур с эмуляцией пользовательского пространства QEMU.
  - As easy as native build;
  - Very slow (~10x build time).

### Cross Build

Follow [CMake’s general cross compiling instructions](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html). Also set `CMAKE_CROSSCOMPILING_EMULATOR` if you want to run test cases.

The [AppImage build environment](https://github.com/redpanda-cpp/appimage-builder) is an example for bootstrapping musl-based, static cross toolchain and Qt.

### Эмуляция родной сборки

Примечание: Всегда запускайте эмулируемую родную сборку **в chroot’ed environment, контейнерах или jails**. Смешивание архитектур может привести к сбою в работе вашей системы.

Для машин с Linux или BSD установите статически связанный эмулятор пользовательского пространства QEMU (имя пакета, скорее всего, "qemu-user-static") и убедитесь, что включена поддержка binfmt.


Для Windows-систем, в Docker и Podman должна быть включена эмуляция пользовательского пространства QEMU. В противном случае:
* Для Docker:
  ```ps1
  docker run --rm --privileged multiarch/qemu-user-static:register
  ```
* Для Podman, чья виртуальная машина основана на Fedora WSL, просто включите поддержку binfmt:
  ```ps1
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-aarch64-static.conf /proc/sys/fs/binfmt_misc/register
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-riscv64-static.conf /proc/sys/fs/binfmt_misc/register
  ```
