/* by XY0797 2024.2.21 */

#include "filenameeditdelegate.h"
#include <QLineEdit>
#include <qapplication.h>

// Custom edit box control. This is necessary because the default behavior of a QLineEdit when it gains focus is to select all its text, and if a selection is set before gaining focus, it will be overridden by the select-all action upon focusing.
class FilenameLineEdit : public QLineEdit
{
public:
    explicit FilenameLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

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

FilenameEditDelegate::FilenameEditDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

// Use our custom component when creating the editor.
QWidget *FilenameEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    FilenameLineEdit *editor = new FilenameLineEdit(parent);
    return editor;
}

// Set the content, and if the item is a file, set the selection on focus gain to exclude the file extension.
void FilenameEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    FilenameLineEdit *lineEdit = (FilenameLineEdit *)editor;
    if (!lineEdit) { return; }

    QString fileName = index.data().toString();
    lineEdit->setText(fileName);
    // Determine whether the currently edited item is a directory or a file; if it's a directory, there's no need to set a selection.
    Qt::ItemFlags flags = index.flags();
    if (flags & Qt::ItemNeverHasChildren) {
        lineEdit->setFocusSelectState(0, findDotPosition(fileName));
    }
}

// Return the edited data back to the QTreeView.
void FilenameEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    FilenameLineEdit *lineEdit = (FilenameLineEdit *)editor;
    if (!lineEdit) { return; }

    model->setData(index, lineEdit->text());
}

// Override the method for updating the editor's position.
void FilenameEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    FilenameLineEdit *lineEdit = (FilenameLineEdit *)editor;
    if (!lineEdit) { return; }

    const QWidget *widget = option.widget;
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.showDecorationSelected = true;

    QStyle *style = widget ? widget->style() : QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);

    lineEdit->setGeometry(geom);
}
