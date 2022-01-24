#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

#include <QOAuth2AuthorizationCodeFlow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    /* Events */
    void on_actionGrant_triggered();
    void on_actionGet_Playlists_triggered();
    void on_listPlaylist_itemDoubleClicked(QListWidgetItem *item);
    void on_addButton_clicked();
    void on_createButton_clicked();

    /* Actions */
    void granted();
    void authStatusChanged (QAbstractOAuth::Status status);
    void Get_User_Information();
    void Get_Playlists();
    void Get_Recs();
    void Put_Recs(QString recSongsUri);








    void on_showButton_clicked();

private:
    Ui::MainWindow *ui;
    QOAuth2AuthorizationCodeFlow spotify;
    bool isGranted;
    QString userName;
    QString playlistId;
};

#endif // MAINWINDOW_H
