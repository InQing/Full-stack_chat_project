#ifndef UPLOADTASK_H
#define UPLOADTASK_H

#include <QObject>
#include <QRunnable>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QEventLoop>
#include <QSet>
#include <QMutex>
#include <QMap>
#include "taskmanager.h"
#include "httpclient.h"

class UploadTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit UploadTask(const QString &filePath, const QString &fileId,
                        const QString &token, const QString &uid,
                        QObject *parent = nullptr);
    ~UploadTask();
    void run() override;

signals:
    void progressUpdated(const QString &fileId, int progress);
    void completed(const QString &fileId);
    void error(const QString &fileId, const QString &errorMessage);

private slots:
    void onChunkCompleted(int chunkNumber);
    void onChunkError(int chunkNumber, const QString &errorMessage);
    void onChunkProgress(int chunkNumber, qint64 bytesSent, qint64 bytesTotal);
    void onInitUploadFinished(const QByteArray &response);
    void onInitUploadError(const QString &error);

private:
    bool initializeUpload();
    void startChunkUploads();
    void checkCompletion();

    QString m_filePath;
    QString m_fileId;
    QString m_token;
    QString m_uid;
    QString m_uploadId;
    QFile m_file;
    int m_totalChunks;
    QSet<int> m_completedChunks;
    QMutex m_mutex;
    QMap<int, qint64> m_chunkProgress;
    const int CHUNK_SIZE = 1024 * 1024; // 1MB chunks

    HttpClient *m_httpClient;
    QEventLoop *m_eventLoop;
};

#endif // UPLOADTASK_H
