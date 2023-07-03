#include <QtPlugin>

// explicitily reference the symbols for the plugins
// so the linker will pull them in during static linking

Q_IMPORT_PLUGIN(QComposePlatformInputContextPlugin)
Q_IMPORT_PLUGIN(QFcitx5PlatformInputContextPlugin)
Q_IMPORT_PLUGIN(QIbusPlatformInputContextPlugin)
