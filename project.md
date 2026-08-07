# QImageViewer 项目结构分析

## 1. 项目概述

`QImageViewer` 是一个基于 Qt 的轻量级图片查看器，采用 C++14 编写，使用 CMake 构建。项目定位简洁，支持常见的图片浏览、缩放、旋转、幻灯片、EXIF 信息查看等功能。

## 2. 技术栈

| 层级 | 技术 |
|---|---|
| 框架 | Qt 5 / Qt 6（Widgets、Svg、PrintSupport） |
| 构建工具 | CMake 3.5+ |
| 语言标准 | C++14 |
| 第三方库 | TinyEXIF（EXIF 解析）、TinyXML2（XML 解析） |
| 资源 | Qt `.qrc` 资源文件、SVG 图标 |
| 国际化 | Qt 翻译机制（`QImageViewer_zh_CN.ts`） |

## 3. 目录结构

```
QImageViewer/
├── CMakeLists.txt          # 构建入口
├── main.cpp                # 程序入口
├── mainwindow.* / .ui      # 主窗口
├── imagewidget.*           # 图片显示/渲染控件
├── dirmodel.*              # 目录/缩略图数据模型
├── thumbnailview.*         # 缩略图绘制委托
├── imagemetainfomodel.*    # 图片 EXIF 元信息模型
├── settings.*              # 配置持久化
├── aboutdialog.*           # 关于对话框
├── settingsdialog/         # 设置对话框模块
│   ├── settingsdialog.*    # 设置对话框主窗口
│   ├── settingswidget.*    # 设置页面基类
│   ├── appearancesettingswidget.* # 外观设置页
│   └── viewsettingswidget.*       # 视图设置页
├── widgets/                # 通用自定义控件
│   └── resizeawarelistview.*      # 可感知尺寸变化的列表视图
├── images/                 # SVG 图标资源
├── icons.qrc               # 图标资源文件
├── libs/                   # 内嵌第三方库
│   ├── tinyexif/
│   ├── tinyxml2/
│   └── qtimageformats/
├── platforms/linux/        # Linux 桌面入口文件
├── test/                   # 测试图片
├── doc/                    # 文档（Exif2-2.pdf）
└── build/                  # 已生成的构建目录
```

## 4. 核心组件分析

### 4.1 主程序入口 `main.cpp`

- 创建 `QApplication`
- 加载系统区域对应的翻译文件
- 在 `AppDataLocation` 下创建/加载 `config.ini`
- 暴露全局 `pSettings` 指针
- 实例化 `MainWindow`，支持命令行传入图片路径

### 4.2 主窗口 `MainWindow`

- 继承 `QMainWindow`，UI 由 `mainwindow.ui` 定义
- 包含两个 Dock 区域：
  - 目录/缩略图面板（`dockDir`）
  - 图片元信息面板（`dockMetaInfo`）
- 核心职责：
  - 目录切换、文件切换
  - 缩放状态栏显示
  - 全屏、幻灯片、打印、复制、删除等操作
  - 拖拽打开图片
- 通过信号槽连接 `DirModel`、`ImageWidget`、`ImageMetaInfoModel`

### 4.3 图片显示控件 `ImageWidget`

- 继承 `QAbstractScrollArea`
- 负责单张图片的加载、缩放、旋转、翻转、动画帧播放
- 支持多种自适应模式：`None / Width / Height / Page`
- 通过 `QImageReader` 支持多帧图片（GIF/TIFF）
- 支持鼠标拖拽滚动、滚轮缩放、双击全屏
- 提供 `requestPrevImage` / `requestNextImage` 信号，实现翻页联动

### 4.4 目录数据模型 `DirModel`

- 继承 `QAbstractListModel`
- 管理当前目录下的文件/图片列表
- 维护当前选中文件索引 `mCurrentFileIdx`
- 使用 `ThumbnailLoader`（`QThread` 子类）在后台加载缩略图
- 提供 `mimeTypes` / `mimeData` / `flags`，支持拖拽

### 4.5 缩略图委托 `ThumbnailDelegate`

- 继承 `QStyledItemDelegate`
- 自定义绘制缩略图项
- 可根据 `thumbnailSize` 调整显示大小

### 4.6 元信息模型 `ImageMetaInfoModel`

- 继承 `QStandardItemModel`
- 读取当前图片的 EXIF 信息并在表格/树中展示

### 4.7 配置系统 `Settings`

- 封装 `QSettings`
- 分三组持久化：
  - `Dirs`：应用目录、可执行文件路径
  - `UI`：窗口尺寸、字体、内容面板状态
  - `View`：适配模式、幻灯片延迟
- 提供类型安全的读写辅助函数
- 全局 `pSettings` 单例指针

### 4.8 设置对话框 `settingsdialog/`

- `SettingsDialog`：左侧导航列表 + 右侧设置页容器
- `SettingsWidget`：所有设置页的抽象基类
- `AppearanceSettingsWidget`：外观/字体设置
- `ViewSettingsWidget`：视图/幻灯片设置
- 支持 `settingsChanged` 状态、Apply/OK/Cancel 流程

### 4.9 自定义控件 `widgets/`

- `ResizeawareListView`：在尺寸变化时通过定时器触发 `resized()` 信号，并响应 `Enter` 键

## 5. 关键数据流

```
用户打开目录/图片
    │
    ▼
MainWindow::open() ──► DirModel::open(path)
    │                       │
    │                       ▼
    │               ThumbnailLoader 后台加载缩略图
    │                       │
    ▼                       ▼
ImageWidget::setImage() ◄── 当前文件变更
    │
    ▼
QImageReader 加载图片 ──► ImageWidget 渲染/缩放/旋转
    │
    ▼
ImageMetaInfoModel 读取 EXIF 信息
```

## 6. 主要功能

- 目录树/缩略图浏览
- 图片缩放、自适应宽度/高度/页面
- 旋转、水平/垂直翻转
- 动画图片播放控制（暂停、停止、下一帧、上一帧）
- 全屏模式
- 幻灯片放映
- 打印/打印预览
- 复制图片/复制文件路径
- 删除当前文件
- EXIF 元信息查看
- 国际化（中文翻译）
- 拖拽打开

## 7. 构建系统要点

- CMake 自动处理 UIC/MOC/RCC
- 兼容 Qt 5 和 Qt 6
- Qt 6 使用 `qt_create_translation`
- Qt 5 使用 `qt5_add_translation` 并自动生成 `translations.qrc`
- Windows 下附加图标资源 `imageviewer.ico`
- 链接库：`Qt::Core`、`Qt::Gui`、`Qt::Widgets`、`Qt::PrintSupport`、`Qt::Svg`

## 8. 观察与备注

- 项目结构清晰，按功能模块化分层，耦合度较低。
- 缩略图加载使用独立线程模型，避免 UI 阻塞。
- 设置对话框采用插件化页面设计，易于扩展新设置页。
- 代码注释较少，主要依赖命名自解释。
- `build/` 目录已存在生成的二进制文件，`.gitignore` 未忽略 `build/`，Git 状态显示其为未跟踪目录。
- 当前没有单元测试框架，仅提供 `test/` 下的样例图片用于手动测试。
- 第三方库以内嵌源码方式引入，不依赖外部包管理器。
