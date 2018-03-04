#include "converter.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Converter w;
    w.show();

    return a.exec();
}
