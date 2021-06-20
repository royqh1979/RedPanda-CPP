#include "qpatchedcombobox.h"

QPatchedComboBox::QPatchedComboBox(QWidget *parent):
    QComboBox(parent)
{
    setView(new QPatchedComboBoxListView(this));
}
