#include <QtGui/QApplication>

int main(int argc, char* argv[])
{
#ifdef WIN32
    return 1;
#else
    QApplication app(argc, argv);
    return app.exec();
#endif
}
