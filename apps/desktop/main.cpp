#include "main_window.hpp"

#include <QApplication>
#include <QString>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName("Proxima");
    application.setApplicationDisplayName("Proxima");
    application.setApplicationVersion(QString::fromUtf8(PROXIMA_VERSION));
    application.setStyle(QStyleFactory::create("Fusion"));

    proxima::desktop::MainWindow window;
    window.show();
    return application.exec();
}
