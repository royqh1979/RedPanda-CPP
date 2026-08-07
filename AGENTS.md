# QImageViewer — Agent Guide

This file is a concise, accurate reference for AI coding agents working on the QImageViewer project. It is derived from the actual source tree, build files, and README.

---

## 1. Project Overview

QImageViewer is a lightweight desktop image viewer written in C++14 with the Qt Widgets framework. It targets Linux, Windows, and macOS, supports Qt 5 and Qt 6, and is licensed under the GNU General Public License v3.0 (or later).

Main capabilities:

- Browse a directory as a thumbnail list.
- View single images with zoom, pan, rotate, and flip.
- Fit modes: none, width, height, page.
- Play multi-frame images (GIF, TIFF, etc.) via `QImageReader`.
- Full-screen mode and slideshow with configurable delay.
- Drag-and-drop opening.
- Print and print preview.
- Copy image to clipboard / copy file path.
- Delete current file.
- Display EXIF metadata via TinyEXIF.
- Simplified Chinese UI translation (`QImageViewer_zh_CN.ts`).

Repository root: `/home/roy/sources/QImageViewer` (use this as the working directory).

---

## 2. Technology Stack

| Layer | Technology |
|---|---|
| Language | C++14 |
| Build system | CMake 3.5+ |
| UI framework | Qt 5 or Qt 6 (`Widgets`, `Svg`, `PrintSupport`, `LinguistTools`) |
| Image I/O | `QImageReader` / `QImageWriter` (plus optional Qt image format plugins) |
| EXIF parsing | TinyEXIF (vendored in `libs/tinyexif/`) |
| XML parsing | TinyXML2 (vendored in `libs/tinyxml2/`) |
| Resources | Qt `.qrc` (`icons.qrc`) |
| Internationalization | Qt `.ts` / `.qm` translation files |
| Configuration | `QSettings` backed by `config.ini` in the platform app-data location |

The vendored libraries are compiled directly into the executable; there is no external package manager.

---

## 3. Project Layout

```text
QImageViewer/
├── CMakeLists.txt              # Top-level CMake project / build entry
├── main.cpp                    # Application entry point
├── mainwindow.{h,cpp,ui}       # Main window, menus, docks, status bar
├── imagewidget.{h,cpp}         # Image rendering, zoom, rotate, flip, animation
├── dirmodel.{h,cpp}            # Directory/thumbnail list model + ThumbnailLoader thread
├── thumbnailview.{h,cpp}       # ThumbnailDelegate for the directory list
├── imagemetainfomodel.{h,cpp}  # EXIF metadata model
├── settings.{h,cpp}            # Persistent settings (singleton via pSettings)
├── aboutdialog.{h,cpp,ui}      # About dialog
├── settingsdialog/             # Settings dialog and pages
│   ├── settingsdialog.{h,cpp,ui}
│   ├── settingswidget.{h,cpp}         # Base class for settings pages
│   ├── appearancesettingswidget.{h,cpp,ui}
│   └── viewsettingswidget.{h,cpp,ui}
├── widgets/
│   └── resizeawarelistview.{h,cpp}    # QListView that emits resized() + enterPressed()
├── images/                     # SVG toolbar icons
├── icons.qrc                   # Resource manifest for icons
├── libs/                       # Vendored third-party sources
│   ├── tinyexif/
│   ├── tinyxml2/
│   └── qtimageformats/webp.diff
├── platforms/linux/            # Linux desktop entry (.desktop file)
├── test/                       # Sample images for manual testing
│   ├── 1.webp
│   └── Multi_page24bpp.tif
├── doc/
│   └── Exif2-2.pdf
└── build/                      # Pre-existing out-of-source build directories
    ├── unknown-Debug/
    └── unknown-Release/
```

---

## 4. Build Instructions

### 4.1 Prerequisites

- CMake >= 3.5
- Qt 5 or Qt 6 development packages:
  - `Qt6::Widgets` / `Qt5::Widgets`
  - `Qt6::Svg` / `Qt5::Svg`
  - `Qt6::PrintSupport` / `Qt5::PrintSupport`
  - `Qt6::LinguistTools` / `Qt5::LinguistTools`
- A C++14-capable compiler (GCC, Clang, MSVC)

### 4.2 Configure and build (out-of-source)

```bash
cd /home/roy/sources/QImageViewer
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

On Windows with MSVC you may use `-DCMAKE_BUILD_TYPE=Release` or multi-config generators as usual.

### 4.3 Run

```bash
./QImageViewer                          # open with no image
./QImageViewer /path/to/image.png       # open a specific image
```

### 4.4 Translation update target (Qt 5 only)

For Qt 5 builds CMake creates an `update_translations` custom target that runs `lupdate` over the source files:

```bash
cmake --build . --target update_translations
```

For Qt 6 the project uses `qt_create_translation` and translation resources are generated automatically.

### 4.5 Build artifacts

The executable produced is named `QImageViewer` (or `QImageViewer.exe` on Windows). There is no install target defined in `CMakeLists.txt`; deployment is currently manual.

---

## 5. Runtime Architecture

### 5.1 Startup flow (`main.cpp`)

1. Create `QApplication`.
2. Load the best matching Qt translation from `:/i18n/`.
3. Locate the platform app-data directory and create/open `config.ini`.
4. Construct a `Settings` instance and expose the global pointer `pSettings`.
5. Create and show `MainWindow`.
6. If a path was passed on the command line, open it.
7. Run the event loop; save settings on exit.

### 5.2 Key object relationships

- `MainWindow` owns:
  - `ImageWidget` — the central image viewport.
  - `DirModel` — model backing the directory/thumbnail dock.
  - `ThumbnailDelegate` — renders thumbnails in the directory view.
  - `ImageMetaInfoModel` — model for the EXIF/metadata dock.
  - `QTimer` — slideshow timer.
- `DirModel` spawns `ThumbnailLoader` (a `QThread` subclass) to generate thumbnails in the background.
- `ImageWidget` uses `QImageReader` for decoding and frame iteration.
- `Settings` wraps `QSettings` and is reachable globally via `extern Settings* pSettings`.

### 5.3 Typical data flow

```text
User opens a directory/image
        │
        ▼
MainWindow::open(path)
        │
        ├──► DirModel::open(path) ──► ThumbnailLoader (background)
        │
        └──► ImageWidget::setImage(currentFilePath)
                    │
                    ▼
            QImageReader loads/decodes
                    │
                    ▼
            ImageWidget renders / scales / rotates
                    │
                    ▼
            ImageMetaInfoModel reads EXIF via TinyEXIF
```

---

## 6. Code Style and Conventions

### 6.1 Naming

- Classes use `PascalCase` (`MainWindow`, `ImageWidget`, `Settings`).
- Member variables are prefixed with `m` + `PascalCase` (`mImageWidget`, `mCurrentFileIdx`).
- Private slots and helper functions use `camelCase` (`on_actionOpen_triggered`, `updateStatusBar`).
- Files match their primary class name (`mainwindow.cpp`, `imagewidget.h`).

### 6.2 Includes and headers

- Use Qt includes with the module prefix: `<QApplication>`, `<QImageReader>`, etc.
- Third-party vendored headers are included via the `libs/` path (`libs/tinyxml2/tinyxml2.h`, `libs/tinyexif/TinyEXIF.h`).
- The root source directory is a private include path.

### 6.3 Qt patterns

- `Q_OBJECT` is declared in every QObject-derived class header.
- UI files are handled by CMake `AUTOUIC` / `AUTOMOC` / `AUTORCC`.
- Signals/slots are used for cross-component communication.
- `Q_PROPERTY` is used where QML-style binding is useful (e.g. `ImageWidget::swapLeftRightWhenTurnPage`).

### 6.4 C++ standard

- The project is fixed to C++14 (`CMAKE_CXX_STANDARD 14`).
- Modern Qt helpers such as `std::unique_ptr` and `std::shared_ptr` are used where appropriate.

---

## 7. Testing Strategy

- **There is no automated test framework in this project.** `CMakeLists.txt` does not define any tests, and running `ctest` reports *No tests were found*.
- `test/` contains only sample images (`1.webp`, `Multi_page24bpp.tif`) for manual verification.
- When making changes, build the project and exercise the affected feature manually with these sample images.

If you add automated tests, place them in a dedicated `tests/` directory and register them with `enable_testing()` / `add_test()` in `CMakeLists.txt`.

---

## 8. Configuration and State

Settings are stored in `config.ini` under the platform-specific `AppDataLocation`:

- Linux: `~/.local/share/QImageViewer/QImageViewer/config.ini`
- Windows: `%APPDATA%/QImageViewer/QImageViewer/config.ini`
- macOS: `~/Library/Application Support/QImageViewer/QImageViewer/config.ini`

Groups defined in `settings.h`:

- `Dirs` — application directory and executable path.
- `UI` — window geometry, contents panel width, font name/size, settings dialog geometry.
- `VIEW` — fit mode and slideshow delay.

Use the `Settings` API (`pSettings->ui()`, `pSettings->view()`, etc.) rather than reading/writing `QSettings` directly.

---

## 9. Security and Safety Considerations

- File paths from command-line arguments and drag-and-drop are passed to `QImageReader` and `QFile`/`QFileInfo`. Validate any new path handling you add.
- The delete action (`on_actionDelete_triggered`) removes the current file from disk. Keep the confirmation dialog behavior intact.
- The project links Qt PrintSupport and uses native print dialogs; be cautious with untrusted printer configurations if you extend printing code.
- Third-party libraries are vendored as source; update them deliberately and review their licenses (TinyEXIF and TinyXML2 are BSD-style).

---

## 10. Common Tasks for Agents

| Task | Where to look / what to change |
|---|---|
| Add a new menu action | `mainwindow.ui`, `mainwindow.h`, `mainwindow.cpp` |
| Add a new image transform | `imagewidget.h` / `imagewidget.cpp` |
| Add a new settings page | Inherit from `SettingsWidget` in `settingsdialog/` and register it in `MainWindow::on_actionOption_triggered` |
| Add a new persistent setting | Add a group/property in `settings.h` / `settings.cpp` |
| Add or update a translation | Edit `QImageViewer_zh_CN.ts`; for Qt 5 run the `update_translations` target |
| Add a new toolbar icon | Place SVG in `images/`, register in `icons.qrc`, reference from `mainwindow.ui` |
| Add a new image format plugin | See `libs/qtimageformats/webp.diff` for the existing WebP patch example |

---

## 11. Notes and Caveats

- The repository already contains pre-built `build/unknown-Debug/` and `build/unknown-Release/` directories. `.gitignore` ignores `*build-*` and `CMakeLists.txt.user*` but **does not ignore `build/`**, so new files under `build/` may appear as untracked in Git unless you add the directory to `.gitignore`.
- `CMakeLists.txt` has a deprecation warning with CMake >= 3.10 because it declares `cmake_minimum_required(VERSION 3.5)`. This does not break the build but may be worth updating if you touch the build system.
- The macOS bundle identifier is currently hard-coded as `my.example.com` in `CMakeLists.txt`.
- There is no `make install` or packaging logic; deployment is manual.
