#ifndef FILEDOWNLOADTASK_H
#define FILEDOWNLOADTASK_H

#include <QObject>
#include <QRunnable>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

class DownloadTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit DownloadTask(const QString &fileId, const QString &savePath, const QString &token, const QString &uid, QObject *parent = nullptr);
    void run() override;

signals:
    void sig_download_finished();
    void sig_download_error(const QString &errorMessage);

private slots:

private:
    QString fileId_;
    QString savePath_;
    QString token_;
    QString uid_;
    QFile file_;
};

#endif // FILEDOWNLOADTASK_H
