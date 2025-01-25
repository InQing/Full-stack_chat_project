#ifndef CHUNKUPLOADTASK_H
#define CHUNKUPLOADTASK_H

#include <QObject>
#include <QRunnable>
#include <QByteArray>
#include <QEventLoop>>
#include "httpclient.h"

class ChunkUploadTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit ChunkUploadTask(const QString &fileId,
                             const QString &uploadId,
                             const QByteArray &chunkData,
                             int chunkNumber,
                             int totalChunks,
                             QObject *parent = nullptr);
    ~ChunkUploadTask();
    void run() override;

signals:
    void completed(int chunkNumber);
    void error(int chunkNumber, const QString &errorMessage);
    void progressUpdated(int chunkNumber, qint64 bytesSent, qint64 bytesTotal);
    void finished();

private slots:
    void handleUploadFinished(const QByteArray &response);
    void handleUploadError(const QString &error);
    void handleUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    QString m_fileId;
    QString m_uploadId;
    QByteArray m_chunkData;
    int m_chunkNumber;
    int m_totalChunks;
    HttpClient *m_httpClient;
    QEventLoop *m_eventLoop;
    QByteArray calculateMD5(const QByteArray &data);
};

#endif // CHUNKUPLOADTASK_H
