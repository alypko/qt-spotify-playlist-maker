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

    ui->actionGet_Playlists->setEnabled(true);
    ui->actionGet_User_Information->setEnabled(true);
    ui->actionCreate_Playlist->setEnabled(true);
    ui->actionGrant->setEnabled(false);
    ui->createButton->setEnabled(false);
    ui->addButton->setEnabled(false);
    ui->deleteButton->setEnabled(false);
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

    QJsonObject obj;
    obj["name"] = ui->linePlaylistName->text();
    obj["public"] = false;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();


    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists");

    auto post = spotify.post(u,data);

    connect(post, &QNetworkReply::finished, [=](){
        if (post->error() != QNetworkReply::NoError) {
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
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }
        QByteArray data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        userName = root.value("id").toString();
        reply->deleteLater();
    });
}




void MainWindow::Get_Playlists()
{
    ui->listPlaylist->clear();

    if (userName.length() == 0) return;

    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists?offset=0&limit=50");

    auto reply = spotify.get(u);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            //ui->teOutput->appendPlainText(reply->errorString());
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
            return;
        }

        const auto data = reply->readAll();
        const auto document = QJsonDocument::fromJson(data);
        const auto root = document.object();
        const auto tracks = root.value("tracks").toArray();

        QJsonObject jsonObj;
        QJsonArray jsonArray;

        for(const auto &i: tracks)
        {
            jsonArray.append(i.toObject().value("uri").toString());
        }
        jsonObj["uris"] = jsonArray;
        QByteArray byteArray;
        byteArray = QJsonDocument(jsonObj).toJson();
        Put_Recs(byteArray);
        reply->deleteLater();
    });

}

void MainWindow::Put_Recs(QByteArray recs)
{

    QUrl u ("https://api.spotify.com/v1/playlists/"+playlistId+"/tracks");
    auto post = spotify.post(u,recs);

    connect(post, &QNetworkReply::finished, [=](){
        if (post->error() != QNetworkReply::NoError) {
            return;
        }
        const auto data = post->readAll();
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
    check_Limit();
    ui->addButton->setEnabled(true);
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
    check_Limit();
}

void MainWindow::on_deleteButton_clicked()
{
    qDeleteAll(ui->listPool->selectedItems());
    check_Limit();
}

void MainWindow::check_Limit()
{
    if(ui->listPool->count() == 0)
    {
        ui->createButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);

    }
    else
    {
        ui->createButton->setEnabled(true);
        ui->deleteButton->setEnabled(true);
    }
}
