#ifndef CHUNKUPLOADTASK_H
#define CHUNKUPLOADTASK_H

#include <QObject>
#include <QRunnable>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>

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
    void run() override;

signals:
    void completed(int chunkNumber);
    void error(int chunkNumber, const QString &errorMessage);
    void progressUpdated(int chunkNumber, qint64 bytesSent, qint64 bytesTotal);
    void finished();

private:
    QString m_fileId;
    QString m_uploadId;
    QByteArray m_chunkData;
    int m_chunkNumber;
    int m_totalChunks;
    QByteArray calculateMD5(const QByteArray &data);
};

#endif // CHUNKUPLOADTASK_H
