# More Build Instructions for freedesktop.org-conforming (XDG) Desktop Systems

## Traditional Unix Way (`./configure`–`make`–`make install`)

- Install recent version of GCC (≥ 7) or Clang (≥ 6) that supports C++17.
- Install Qt 5.15 or 6.7+ Base, SVG and Tools modules, including both libraries and development files.
- Optionally install fcitx5-qt for building with static Qt library.
- Install astyle for code formatting in Red Panda C++.

### qmake-based Build Steps

1. Configure:
   ```bash
   qmake PREFIX=/usr/local /path/to/src/Red_Panda_CPP.pro
   ```
2. Build:
   ```bash
   make -j$(nproc)
   ```
3. Install:
   ```bash
   sudo make install
   ```

qmake variables:
- `PREFIX`: default to `/usr/local`. It should be set to `/usr` when packaging.
- `LIBEXECDIR`: directory for auxiliary executables, default to `$PREFIX/libexec`. Arch Linux uses `/usr/lib`.
- `LINUX_STATIC_IME_PLUGIN=ON` (make phase): link to static ime plugin. Recommended for building with static version of Qt; **DO NOT** set for dynamic version of Qt.

### xmake-based Build Steps

1. Configure:
   ```bash
   xmake f -p linux -a x86_64 -m release --qt=/usr --prefix=/usr/local
   ```
2. Build:
   ```bash
   xmake
   ```
3. Install:
   ```bash
   sudo xmake install --root -o /  # `-o ...` imitates `DESTDIR=...` in `make install`
   ```

Hint: `xmake f --help` for more options.

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

## Debian Packages for Multiple Architectures and Versions

These packages can be built in containers. Both Linux host and Windows host are supported.

```bash
podman run --rm -v $PWD:/mnt -w /mnt --platform linux/amd64 docker.io/debian:12 ./packages/debian/01-in-docker.sh
```

Platform can be `linux/amd64`, `linux/386`, `linux/arm64/v8`, `linux/arm/v7`, `linux/riscv64`, etc.

Image can be `docker.io/debian:12`, `docker.io/debian:11`, `docker.io/ubuntu:24.04`, `docker.io/ubuntu:23.10`, `docker.io/ubuntu:22.04`, etc.

Optional environment variables:
- `-e MIRROR=mirrors.kernel.org`: mirror for APT.
- `-e JOBS=4`: number of parallel jobs for make.

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
