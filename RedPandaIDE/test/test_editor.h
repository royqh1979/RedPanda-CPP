#ifndef TEST_EDITOR_H
#define TEST_EDITOR_H
#include <QObject>
#include <memory>

class Editor;
class SettingsPersistor;
class DirSettings;
class EditorSettings;
class ColorManager;

class TestEditor : public QObject
{
    Q_OBJECT
public:
	TestEditor(QObject *parent=nullptr);
private slots:
    void test_complete_double_quote1();
protected:
    std::shared_ptr<Editor> mEditor;
    std::shared_ptr<SettingsPersistor> mSettingsPersistor;
    std::shared_ptr<DirSettings> mDirSettings;
    std::shared_ptr<EditorSettings> mEditorSettings;
    std::shared_ptr<ColorManager> mColorManager;

};

#endif
