#include "MainWindow.h"

#include <QApplication>
#include <QTextCodec>

int main(int argc, char* argv[])
{
    std::srand(std::time(0));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif

    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
