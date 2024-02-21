/* by XY0797 2024.2.21 */

#include "filenamedelegate.h"
#include <QLineEdit>
#include <QFileInfo>

// Custom edit box control. This is necessary because the default behavior of a QLineEdit when it gains focus is to select all its text, and if a selection is set before gaining focus, it will be overridden by the select-all action upon focusing.
class MyLineEdit : public QLineEdit
{
public:
    explicit MyLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

    // Add a custom method to set the selection on focus gain.
    void setFocusSelectState(int index, int length)
    {
        m_focusSelectionStart = index;
        m_focusSelectionLength = length;
    }

protected:
    int m_focusSelectionStart = -1;
    int m_focusSelectionLength = -1;

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

// Below follows the implementation of the delegate class.

FileNameDelegate::FileNameDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

// Use our custom component when creating the editor.
QWidget *FileNameDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    MyLineEdit *editor = new MyLineEdit(parent);
    return editor;
}

// Set the content, and if the item is a file, set the selection on focus gain to exclude the file extension.
void FileNameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    MyLineEdit *lineEdit = (MyLineEdit *)editor;
    QString fileName = index.data().toString();
    lineEdit->setText(fileName);
    // Determine whether the currently edited item is a directory or a file; if it's a directory, there's no need to set a selection.
    Qt::ItemFlags flags = index.flags();
    if (flags & Qt::ItemNeverHasChildren) {
        lineEdit->setFocusSelectState(0, findDotPosition(fileName));
    }
}

// Return the edited data back to the QTreeView.
void FileNameDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    MyLineEdit *lineEdit = (MyLineEdit *)editor;
    model->setData(index, lineEdit->text());
}

// Recalculate the rectangle's left side and width to avoid overlapping with the icon display.
QRect adjustForIcon(const QRect &originalRect)
{
    // Adjust the position to accommodate the icon display.
    int theiconWidth = 20;
    return QRect(originalRect.x() + theiconWidth, originalRect.y(), originalRect.width() - theiconWidth, originalRect.height());
}

// Override the method for updating the editor's position.
void FileNameDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    MyLineEdit *lineEdit = (MyLineEdit *)editor;
    lineEdit->setGeometry(adjustForIcon(option.rect));
}
