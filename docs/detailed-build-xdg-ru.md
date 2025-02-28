# Дополнительные инструкции сборки для freedesktop.org-conforming (XDG) Desktop Systems

## Традиционный путь Unix (`./configure`–`make`–`make install`)

- Установить последнюю версию GCC (≥ 7) или Clang (≥ 6), поддерживающую C++17.
- Установить Qt 5.15 или 6.8+ Base, SVG и Tools modules, включающие все библиотеки и файлы для разработки.
- По желанию устанвоить fcitx5-qt для статической сборки с Библиотекой Qt.
- Установить astyle для форматирования кода в Red Panda C++.

### Шаги сборки с qmake

1. Настройка:
   ```bash
   qmake PREFIX=/usr/local /path/to/src/Red_Panda_CPP.pro
   ```
2. Сборка:
   ```bash
   make -j$(nproc)
   ```
3. Установка:
   ```bash
   sudo make install
   ```

Переменные qmake:
- `PREFIX`: куда `$MAKE install` будет устанавливать файлы.
  - На саму среду Red Panda C++ `PREFIX` не влияет, поскольку она использует относительные пути.
  - На файл `.desktop` влияет `PREFIX`.
- `LIBEXECDIR`: каталог для вспомогательных исполнимых файлов, УКАЗЫВАЕТСЯ ОТНОСИТЕЛЬНО `PREFIX`.
  - Arch Linux использует `lib`.

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

### Инструкции сборки для Debian/Ubuntu (просто скопируй и вставь)

```bash
# подготовка
apt install gcc g++ make git gdb gdbserver astyle qterminal # устанвоить интрументы, необходимые для сборки и времени выполнения
apt install qtbase5-dev qttools5-dev-tools libqt5svg5-dev   # установить заголовочные файлы и библиотеки для сборки
git clone https://github.com/royqh1979/RedPanda-CPP.git     # скачать исходный код

# сборка
mkdir -p RedPanda-CPP/build && cd RedPanda-CPP/build        # создать каталог сборки
qmake ../Red_Panda_CPP.pro                                  # настройка
make -j$(nproc)                                             # сборка
sudo make install                                           # установка

# запуск
RedPandaIDE
```

## Пакеты Debian для различных архитектур и версий

These packages can be built in containers. Both Linux host and Windows host are supported.
Эти пакеты могут быть собраны в контейнерах. Поддерживаются как системы с Linux, так и с Windows.

```bash
podman run --rm -v $PWD:/mnt -w /mnt --platform linux/amd64 docker.io/debian:12 ./packages/debian/01-in-docker.sh
```

Платформа может быть `linux/amd64`, `linux/386`, `linux/arm64/v8`, `linux/arm/v7`, `linux/riscv64`, и т.д.

Образ может быть `docker.io/debian:12`, `docker.io/debian:11`, `docker.io/ubuntu:24.04`, `docker.io/ubuntu:23.10`, `docker.io/ubuntu:22.04`, и т.д.

Дополнительные переменные окружения:
- `-e MIRROR=mirrors.kernel.org`: зеркало для APT.
- `-e JOBS=4`: число параллельных процессов для make.

## Эмуляция родной сборки для прочих архитектур

Можно собрать Red Panda C++ для других архитектур, используя родные инструменты сборки для целевых архитектур с эмуляцией пользовательского пространства QEMU.

Примечание: Всегда запускайте эмулируемую родную сборку **в контейнерах или jails**. Смешивание архитектур может привести к сбою в работе вашей системы.

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
