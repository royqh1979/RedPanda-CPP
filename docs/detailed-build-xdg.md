# More Build Instructions for freedesktop.org-conforming (XDG) Desktop Systems

## Traditional Unix Way (`./configure`–`make`–`make install`)

- Install recent version of GCC or Clang that supports C++17.
- Install Qt 6.8+ Base, SVG and Tools modules, including both libraries and development files.
- Optionally install fcitx5-qt for building with static Qt library.
- Install astyle for code formatting in Red Panda C++.

### CMake-based Build Steps

1. Configure:
   ```bash
   cmake -S /path/to/src -B /path/to/build \
     -G "Unix Makefiles" \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
   ```
2. Build:
   ```bash
   make -j$(nproc)
   ```
3. Install:
   ```bash
   sudo make install
   ```

CMake options:
- `CMAKE_INSTALL_PREFIX`: where `$MAKE install` installs files to.
  - Red Panda C++ itself is not affected by `CMAKE_INSTALL_PREFIX`, because it internally uses relative path.
  - `.desktop` file is affected by `CMAKE_INSTALL_PREFIX`.
- `LIBEXECDIR`: directory for auxiliary executables, RELATIVE TO `CMAKE_INSTALL_PREFIX`.
  - Arch Linux uses `lib`.

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

<!--
### Copy ’n’ Paste Instructions for Debian/Ubuntu

```bash
# prepare
apt install gcc g++ make git gdb gdbserver astyle qterminal # install build tools and runtime tools
apt install qtbase5-dev qttools5-dev-tools libqt5svg5-dev   # install development headers and libraries
git clone https://github.com/royqh1979/RedPanda-CPP.git     # fetch source code

# build
mkdir -p RedPanda-CPP/build && cd RedPanda-CPP/build        # create build directory
qmake ../Red_Panda_CPP.pro                                  # configure
make -j$(nproc)                                             # build
sudo make install                                           # install

# run
RedPandaIDE
```
-->

## Emulated Native Build for Foreign Architectures

It is possible to build Red Panda C++ for foreign architectures using targets’ native toolchains with QEMU user space emulation.

Note: Always run emulated native build **in containers or jails**. Mixing architectures may kill your system.

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
