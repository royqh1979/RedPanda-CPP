#ifndef TEST_EDITOR_H
#define TEST_EDITOR_H
#include <QObject>

class Editor;
class TestEditor : public QObject
{
    Q_OBJECT
public:
	TestEditor(QObject *parent=nullptr);
private slots:
    void test_compare();
protected:
	Editor *mEditor;
};

#endif
