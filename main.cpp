#include <QApplication>
#include "mainwindow.h"
#include "welcomewindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/images/icon.png"));

    a.setStyle("Fusion");
    WelcomeWindow welcome;
    if (welcome.exec() == QDialog::Accepted) {
        MainWindow w;
        w.show();
        return a.exec();
    }
    return 0;
}
