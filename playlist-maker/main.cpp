#include "mainwindow.h"
#include <QApplication>

#include <QSslSocket>
#include <QDebug>

int main(int argc, char *argv[])
{

    qDebug()<<"Suppport"<<QSslSocket::supportsSsl();
    //  sprawdzenie czy sa biblioteki SSL
    if (!QSslSocket::supportsSsl()) {
        qWarning () << "No SSL Support";
        exit (1);
    }
    qDebug () << QSslSocket::sslLibraryVersionString();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
