#ifndef TEST_EDITOR_BASE_H
#define TEST_EDITOR_BASE_H
#include <QObject>
#include <memory>

class Editor;
class SettingsPersistor;
class DirSettings;
class EditorSettings;
class ColorManager;

class TestEditorBase: public QObject
{
    Q_OBJECT
public:
    TestEditorBase(QObject *parent=nullptr);
protected:
    void init_editor();

protected:
    std::shared_ptr<Editor> mEditor;
    std::shared_ptr<SettingsPersistor> mSettingsPersistor;
    std::shared_ptr<DirSettings> mDirSettings;
    std::shared_ptr<EditorSettings> mEditorSettings;
    std::shared_ptr<ColorManager> mColorManager;

};

#endif
