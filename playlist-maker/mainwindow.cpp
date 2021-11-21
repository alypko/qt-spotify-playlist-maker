#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clientid.h"

#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QDesktopServices>
#include <QtNetworkAuth>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isGranted(false)
{
    ui->setupUi(this);

    auto replyHandler = new QOAuthHttpServerReplyHandler(1234,this);
    spotify.setReplyHandler(replyHandler);
    spotify.setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    spotify.setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));
    spotify.setClientIdentifier(clientId);
    spotify.setClientIdentifierSharedKey(clientSecret);
    spotify.setScope("user-read-private user-top-read playlist-read-private playlist-modify-public playlist-modify-private");
    //oauth2.setScope("identity read");

    connect(&spotify, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
             &QDesktopServices::openUrl);

    connect(&spotify, &QOAuth2AuthorizationCodeFlow::statusChanged,
            this, &MainWindow::authStatusChanged);

    connect(&spotify, &QOAuth2AuthorizationCodeFlow::granted,
            this, &MainWindow::granted);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::granted ()
{
    QString token = spotify.token();
    ui->teOutput->appendPlainText("Token: " + token);

    ui->actionGet_Playlists->setEnabled(true);
    ui->actionGet_User_Information->setEnabled(true);
    isGranted = true;
}

void MainWindow::authStatusChanged(QAbstractOAuth::Status status)
{
    QString s;
    if (status == QAbstractOAuth::Status::Granted)
        s = "granted";

    if (status == QAbstractOAuth::Status::TemporaryCredentialsReceived) {
        s = "temp credentials";
    }

    ui->teOutput->appendPlainText("Auth Status changed: " + s +  "\n");
}

void MainWindow::on_actionGrant_triggered()
{
    spotify.grant();
}

void MainWindow::on_actionGet_User_Information_triggered()
{
    ui->teOutput->appendPlainText("Loading User Informations");

    QUrl u ("https://api.spotify.com/v1/me");

    auto reply = spotify.get(u);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(reply->errorString());
            return;
        }
        const auto data = reply->readAll();
        ui->teOutput->appendPlainText(data);

        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        userName = root.value("id").toString();

        ui->teOutput->appendPlainText("Username: " + userName);

        reply->deleteLater();
    });

}

void MainWindow::on_actionGet_Playlists_triggered()
{
    if (userName.length() == 0) return;

    ui->teOutput->appendPlainText("Loading Playlists ...");

    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists");

    auto reply = spotify.get(u);

    connect (reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(reply->errorString());
            return;
        }

        const auto data = reply->readAll();
        ui->teOutput->appendPlainText(data);

        reply->deleteLater();
    });

}

void MainWindow::on_actionCreate_Playlist_triggered()
{
    if (userName.length() == 0) return;

    ui->teOutput->appendPlainText("Creating Playlist....");

    QJsonObject obj;
    obj["name"] = "Your Coolest Playlist";
    obj["public"] = false;
    obj["description"] = "LOOOOOL";
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();

    //ui->teOutput->appendPlainText(jObj);

    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists");

    auto post = spotify.post(u,data);

    connect(post, &QNetworkReply::finished, [=](){
        if (post->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(post->errorString());
            return;
        }
        const auto data = post->readAll();
        ui->teOutput->appendPlainText(data);

        post->deleteLater();
    });

}
//QJsonValue::toVariant()
//void MainWindow::setParams()
//{
//    QJsonObject jObj;
//    jObj.insert("name", QJsonValue::fromVariant("My New Playlist"));
//    jObj.insert("public", QJsonValue::fromVariant(false));
//    jObj.insert("collaborative", QJsonValue::fromVariant(false));
//    jObj.insert("description", QJsonValue::fromVariant("My first Playlist"));


//}

