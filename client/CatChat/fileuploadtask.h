#ifndef FILEUPLOADTASK_H
#define FILEUPLOADTASK_H

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
#include "httpclient.h"
#include "global.h"

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
    void sig_upload_init_finished();
    void sig_upload_init_error(ErrorCodes code);
    void sig_upload_chunk_finished(int chunkNumber);
    void sig_upload_chunk_error(int chunkNumber,ErrorCodes code);
    void sig_upload_finished();

private slots:
    void on_chunk_completed(int chunkNumber);
    void on_chunk_error(int chunkNumber);
    void on_upload_init_finished(const QString &response);
    void on_upload_init_error(const QString &error);

private:
    bool InitializeUpload();
    void StartChunkUploads();
    void CheckCompletion();

    QString filePath_;
    QString fileId_;
    QString token_;
    QString uid_;
    QString uploadId_;
    QFile file_;
    int totalChunks_;
    QSet<int> completedChunks_;
    QMutex mutex_;
    QMap<int, qint64> chunkProgress_;
    const int CHUNK_SIZE = 2 * 1024 * 1024; // 2MB

    HttpClient *httpClient_;
    QEventLoop *eventLoop_;
};

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
    void sig_completed(int chunkNumber);
    void sig_error(int chunkNumber);

private slots:
    void on_upload_chunk_finished(const QString &response);
    void on_upload_chunk_error(const QString &error);

private:
    QString fileId_;
    QString uploadId_;
    QByteArray chunkData_;
    int chunkNumber_;
    int totalChunks_;
    HttpClient *httpClient_;
    QEventLoop *eventLoop_;
    QByteArray CalculateMD5(const QByteArray &data);
};


#endif // FILEUPLOADTASK_H
