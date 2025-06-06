# Дополнительные инструкции сборки для freedesktop.org-conforming (XDG) Desktop Systems

## Традиционный путь Unix (`./configure`–`make`–`make install`)

- Установить последнюю версию GCC или Clang, поддерживающую C++17.
- Установить Qt 6.8+ Base, SVG и Tools modules, включающие все библиотеки и файлы для разработки.
- По желанию устанвоить fcitx5-qt для статической сборки с Библиотекой Qt.
- Установить astyle для форматирования кода в Red Panda C++.

### Шаги сборки с CMake

1. Настройка:
   ```bash
   cmake -S /path/to/src -B /path/to/build \
     -G "Unix Makefiles" \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/usr/local
   ```
2. Сборка:
   ```bash
   make -j$(nproc)
   ```
3. Установка:
   ```bash
   sudo make install
   ```

Переменные CMake:
- `CMAKE_INSTALL_PREFIX`: куда `$MAKE install` будет устанавливать файлы.
  - На саму среду Red Panda C++ `CMAKE_INSTALL_PREFIX` не влияет, поскольку она использует относительные пути.
  - На файл `.desktop` влияет `CMAKE_INSTALL_PREFIX`.
- `LIBEXECDIR`: каталог для вспомогательных исполнимых файлов, УКАЗЫВАЕТСЯ ОТНОСИТЕЛЬНО `CMAKE_INSTALL_PREFIX`.
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

<!--
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
-->

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
