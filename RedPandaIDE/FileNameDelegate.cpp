/* by XY0797 2024.2.21 */

#include "FileNameDelegate.h"
#include <QLineEdit>
#include <QFileInfo>

// 自定义编辑框控件。之所以要这样子，是因为QLineEdit获取焦点后的默认行为是全选，如果在获取焦点前设置选区，会被获取焦点后的全选给覆盖
// Custom edit box control. This is necessary because the default behavior of a QLineEdit when it gains focus is to select all its text, and if a selection is set before gaining focus, it will be overridden by the select-all action upon focusing.
class MyLineEdit : public QLineEdit
{
public:
    explicit MyLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

    // 添加自定义方法设置焦点获取时的选区
    // Add a custom method to set the selection on focus gain.
    void setFocusSelectState(int index, int length)
    {
        m_focusSelectionStart = index;
        m_focusSelectionLength = length;
    }

protected:
    int m_focusSelectionStart = -1;
    int m_focusSelectionLength = -1;

    // 覆写焦点获取事件，在执行默认操作前重置选区
    // Override the focus-in event, resetting the selection before executing the default operation.
    void focusInEvent(QFocusEvent *event) override
    {
        if (m_focusSelectionStart != -1 && m_focusSelectionLength > 0)
        {
            deselect();
            setSelection(m_focusSelectionStart, m_focusSelectionLength);
        }
        QLineEdit::focusInEvent(event);
    }
};

// 返回'.'在字符串里最后一次出现的位置，如果找不到就返回字符串的长度
// Return the last occurrence index of '.' in the string; if not found, return the length of the string.
int findDotPosition(const QString &fileName)
{
    int dotPosition = fileName.lastIndexOf('.');
    if (dotPosition != -1)
    {
        return dotPosition;
    }
    else
    {
        return fileName.length();
    }
}

// 下面是委托类的实现
// Below follows the implementation of the delegate class.

FileNameDelegate::FileNameDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

// 创建编辑器时使用我们的自定义组件
// Use our custom component when creating the editor.
QWidget *FileNameDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    MyLineEdit *editor = new MyLineEdit(parent);
    return editor;
}

// 设置内容，如果是文件就设置获取焦点后的选区为非后缀内容
// Set the content, and if the item is a file, set the selection on focus gain to exclude the file extension.
void FileNameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    MyLineEdit *lineEdit = (MyLineEdit *)editor;
    QString fileName = index.data().toString();
    lineEdit->setText(fileName);
    // 判断目前待编辑的是目录还是文件，如果是目录就没必要设置选区
    // Determine whether the currently edited item is a directory or a file; if it's a directory, there's no need to set a selection.
    Qt::ItemFlags flags = index.flags();
    if (flags & Qt::ItemNeverHasChildren) {
        lineEdit->setFocusSelectState(0, findDotPosition(fileName));
    }
}

// 把编辑后的数据返回到QTreeView
// Return the edited data back to the QTreeView.
void FileNameDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    MyLineEdit *lineEdit = (MyLineEdit *)editor;
    model->setData(index, lineEdit->text());
}

// 重新计算矩形的左边和宽度，避开图标显示
// Recalculate the rectangle's left side and width to avoid overlapping with the icon display.
QRect adjustForIcon(const QRect &originalRect)
{
    // 避让图标显示
    // Adjust the position to accommodate the icon display.
    int theiconWidth = 20;
    return QRect(originalRect.x() + theiconWidth, originalRect.y(), originalRect.width() - theiconWidth, originalRect.height());
}

// 重写位置更新方法
// Override the method for updating the editor's position.
void FileNameDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    MyLineEdit *lineEdit = (MyLineEdit *)editor;
    lineEdit->setGeometry(adjustForIcon(option.rect));
}
