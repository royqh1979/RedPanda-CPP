#include "dialog.h"

#include <QApplication>
#include <stdio.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (argc>1) {
        Dialog w;
        if (w.showPrompt(argv[1])==QDialog::Accepted) {
            char* input = w.getInput().toLocal8Bit().data();
            printf("%s",input);
        }
    }
    return 0;
}
