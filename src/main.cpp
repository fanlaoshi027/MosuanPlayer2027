#include "MainWindow.h"

#include <QApplication>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MosuanPlayer"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("Mosuan"));

    MainWindow window;
    window.show();
    return app.exec();
}
