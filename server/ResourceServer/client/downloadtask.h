#ifndef DOWNLOADTASK_H
#define DOWNLOADTASK_H

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
    explicit DownloadTask(const QString& fileId, const QString& savePath, QObject *parent = nullptr);
    void run() override;

signals:
    void progressUpdated(const QString& fileId, int progress);
    void completed(const QString& fileId);
    void error(const QString& fileId, const QString& errorMessage);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QString m_fileId;
    QString m_savePath;
    QFile m_file;
    QNetworkAccessManager* m_networkManager;
    QEventLoop m_eventLoop;
};

#endif // DOWNLOADTASK_H
