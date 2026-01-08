# More Build Instructions for freedesktop.org-conforming (XDG) Desktop Systems

## Traditional Unix Way (`./configure`–`make`–`make install`)

- Install recent version of GCC or Clang that supports C++17.
- Install Qt 6.8+ Base, SVG and Tools modules, including both libraries and development files.
- Optionally install fcitx5-qt for building with static Qt library.
- Install astyle for code formatting in Red Panda C++.

### CMake-based Build Steps

1. Configure:
   ```bash
   cmake -S . -B build \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
   ```
2. Build:
   ```bash
   cmake --build build -- --parallel
   ```
3. Install:
   ```bash
   sudo cmake --install build --strip
   ```

CMake options:
- `CMAKE_INSTALL_PREFIX`: where `$MAKE install` installs files to.
  - Red Panda C++ itself is not affected by `CMAKE_INSTALL_PREFIX`, because it internally uses relative path.
  - `.desktop` file is affected by `CMAKE_INSTALL_PREFIX`.
- `LIBEXECDIR`: directory for auxiliary executables, RELATIVE TO `CMAKE_INSTALL_PREFIX`.
  - Arch Linux uses `lib`.
- `OVERRIDE_MALLOC`: link specific memory allocation library. e.g. `-DOVERRIDE_MALLOC=mimalloc`.

### xmake-based Build Steps

1. Configure:
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr
   ```
2. Build:
   ```bash
   xmake
   ```
3. Install:
   ```bash
   sudo xmake install --root -o /usr/local
   ```

Hint: `xmake f --help` for more options.

## Foreign Architectures

There are 2 ways to build Red Panda C++ for foreign architectures:
- Cross build: using cross toolchain.
  - As fast as native build;
  - Building cross Qt is not so easy;
  - QEMU user space emulation is still required if you want to run test cases.
- Emulated native build: using targets’ native toolchains with QEMU user space emulation.
  - As easy as native build;
  - Very slow (~10x build time).

### Cross Build

Follow [CMake’s general cross compiling instructions](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html). Also set `CMAKE_CROSSCOMPILING_EMULATOR` if you want to run test cases.

The [AppImage build environment](https://github.com/redpanda-cpp/appimage-builder) is an example for bootstrapping musl-based, static cross toolchain and Qt.

### Emulated Native Build

There’s nothing special about emulated native build except for setting up QEMU user space emulation.

Note: Always run emulated native build **in chroot’ed environment, containers or jails**. Mixing architectures may kill your system.

For Linux or BSD host, install statically linked QEMU user space emulator (package name is likely `qemu-user-static`) and make sure that binfmt support is enabled.

For Windows host, Docker and Podman should have QEMU user space emulation enabled. If not,
* For Docker:
  ```ps1
  docker run --rm --privileged multiarch/qemu-user-static:register
  ```
* For Podman, whose virtual machine is based on Fedora WSL, simply enable binfmt support:
  ```ps1
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-aarch64-static.conf /proc/sys/fs/binfmt_misc/register
  wsl -d podman-machine-default sudo cp /usr/lib/binfmt.d/qemu-riscv64-static.conf /proc/sys/fs/binfmt_misc/register
  ```
