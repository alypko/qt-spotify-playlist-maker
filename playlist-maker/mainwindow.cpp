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

    connect(&spotify, &QOAuth2AuthorizationCodeFlow::granted,
            this, &MainWindow::Get_User_Information);
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
    ui->actionCreate_Playlist->setEnabled(true);
    ui->actionGrant->setEnabled(false);
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

void MainWindow::on_actionGet_Playlists_triggered()
{
    Get_Playlists();
}

void MainWindow::on_createButton_clicked()
{
    if (userName.length() == 0) return;

    ui->teOutput->appendPlainText("Creating Playlist....");

    QJsonObject obj;
    obj["name"] = ui->linePlaylistName->text();
    obj["public"] = false;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();


    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists");

    auto post = spotify.post(u,data);

    connect(post, &QNetworkReply::finished, [=](){
        if (post->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(post->errorString());
            return;
        }
        QByteArray data = post->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        playlistId = root.value("id").toString();

        Get_Recs();
        post->deleteLater();
    });

}

void MainWindow::Get_User_Information()
{

    QUrl u ("https://api.spotify.com/v1/me");

    auto reply = spotify.get(u);
    ui->teOutput->appendPlainText(reply->readAll());
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(reply->errorString());
            return;
        }
        ui->teOutput->appendPlainText("User Informations Loaded");
        QByteArray data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        userName = root.value("id").toString();
        ui->teOutput->appendPlainText(data);
        reply->deleteLater();
    });
}




void MainWindow::Get_Playlists()
{
    ui->listPlaylist->clear();

    if (userName.length() == 0) return;

    ui->teOutput->appendPlainText("Loading Playlists ...");

    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists?offset=0&limit=50");

    auto reply = spotify.get(u);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(reply->errorString());
            return;
        }

        const auto data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        const auto items = root.value("items").toArray();

        for(const auto &i: items)
        {
            auto *item = new QListWidgetItem(i.toObject().value("name").toString());
            QVariant v;
            v.setValue(i.toObject().value("id").toString());
            item->setData(Qt::UserRole, v);
            ui->listPlaylist->addItem(item);
        }
        reply->deleteLater();
    });
}

void MainWindow::Get_Recs()
{
    ui->teOutput->appendPlainText("get rec playlist id: "+playlistId);
    QString poolId;
    bool first = true;
    for(int i = 0; i < ui->listPool->count(); ++i)
    {
        QListWidgetItem* item = ui->listPool->item(i);
        QVariant v = item->data(Qt::UserRole);
        QString id = v.value<QString>();
        if(first)
            first = false;
        else
            poolId += ",";
        poolId += id;
    }

    QUrl u ("https://api.spotify.com/v1/recommendations?limit="+ui->spinLimit->text()+"&market=PL&seed_tracks="+poolId);
    auto reply = spotify.get(u);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(reply->errorString());
            return;
        }

        const auto data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        const auto tracks = root.value("tracks").toArray();

        QString recSongsUri;
        bool first = true;
        for(const auto &i: tracks)
        {
            if(first)
                first = false;
            else
                recSongsUri += ',';
            recSongsUri += i.toObject().value("uri").toString();
        }
        Put_Recs(recSongsUri);
        reply->deleteLater();
    });

}

void MainWindow::Put_Recs(QString recSongsUri)
{

    QUrl u ("https://api.spotify.com/v1/playlists/"+playlistId+"/tracks?uris="+recSongsUri);
    auto post = spotify.post(u);

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

void MainWindow::on_listPlaylist_itemDoubleClicked(QListWidgetItem *item)
{

    ui->listSong->clear();
    QVariant v = item->data(Qt::UserRole);
    QString id = v.value<QString>();

    QUrl u ("https://api.spotify.com/v1/playlists/"+id+"/tracks?fields=href%2Citems(track(artists(name)%2Cid%2Cname%2Chref))&limit=50");
    auto reply = spotify.get(u);


    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            ui->teOutput->appendPlainText(reply->errorString());
            return;
        }

        const auto data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        const auto items = root.value("items").toArray();

        for(const auto &i: items)
        {
            QString artistName;
            const auto track = i.toObject().value("track").toObject();
            const auto artists = track.value("artists").toArray();
            QString songName = track.value("name").toString();
            if(artists.size()>0)
            {
                bool first = true;
                for(const auto &artist: artists)
                {
                    if(first)
                        first = false;
                    else
                        artistName += ", ";
                    artistName += artist.toObject().value("name").toString();
                }
            }
            else
                artistName = artists[0].toObject().value("name").toString();

            QString showName = artistName + " - " + songName;
            auto *item = new QListWidgetItem(showName);
            QVariant v;
            v.setValue(track.value("id").toString());
            item->setData(Qt::UserRole, v);
            ui->listSong->addItem(item);
        }
        reply->deleteLater();
    });

}


void MainWindow::on_addButton_clicked()
{
    for(auto &item: ui->listSong->selectedItems())
    {
        QList<QListWidgetItem *> list = ui->listPool->findItems(item->text(), Qt::MatchExactly);
        for(const auto *i: list)
        {
            if(i->text() == item->text())
            {
                int row = ui->listPool->row(i);
                ui->listPool->takeItem(row);
            }
        }
        ui->listPool->addItem(item->clone());
    }
}





void MainWindow::on_showButton_clicked()
{
    ui->teOutput->appendPlainText(playlistId);
}

