#include "camara.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Camara w;
    w.show();
    
    return a.exec();
}
