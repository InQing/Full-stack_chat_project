#include "uploadtask.h"
#include "chunkuploadtask.h"
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

UploadTask::UploadTask(const QString &filePath, const QString &fileId,
                       const QString &token, const QString &uid,
                       QObject *parent)
    : QObject(parent), m_filePath(filePath), m_fileId(fileId), m_token(token), m_uid(uid), m_file(filePath)
{
    setAutoDelete(false);

    if (!m_file.open(QIODevice::ReadOnly))
    {
        emit error(m_fileId, "无法打开文件");
        return;
    }

    qint64 fileSize = m_file.size();
    m_totalChunks = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
}

UploadTask::~UploadTask()
{
    if (m_file.isOpen())
    {
        m_file.close();
    }
}

void UploadTask::run()
{
    if (!initializeUpload())
    {
        return;
    }
    startChunkUploads();
}

bool UploadTask::initializeUpload()
{
    QNetworkAccessManager networkManager;
    QEventLoop eventLoop;

    // 准备初始化请求数据
    QJsonObject json;
    json["file_id"] = m_fileId;
    json["filename"] = QFileInfo(m_filePath).fileName();
    json["uid"] = m_uid;

    QJsonDocument doc(json);

    // 发送初始化请求
    QUrl url("http://localhost:8080/upload/init");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", m_token.toUtf8());

    QNetworkReply *reply = networkManager.post(request, doc.toJson());

    // 等待响应
    connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit error(m_fileId, "初始化上传失败: " + reply->errorString());
        reply->deleteLater();
        return false;
    }

    // 解析响应
    QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
    QJsonObject responseObj = response.object();

    if (responseObj["code"].toInt() != 200)
    {
        emit error(m_fileId, "初始化上传失败: " + responseObj["message"].toString());
        reply->deleteLater();
        return false;
    }

    // 保存uploadId
    m_uploadId = responseObj["data"].toObject()["upload_id"].toString();

    reply->deleteLater();
    return true;
}

void UploadTask::startChunkUploads()
{
    // 读取所有分片并创建上传任务
    for (int i = 0; i < m_totalChunks; ++i)
    {
        m_file.seek(i * CHUNK_SIZE);
        QByteArray chunkData = m_file.read(CHUNK_SIZE);

        // 创建ChunkUploadTask，分片编号从1开始
        ChunkUploadTask *chunkTask = new ChunkUploadTask(
            m_fileId, m_uploadId, chunkData, i + 1, m_totalChunks);

        // 连接信号
        connect(chunkTask, &ChunkUploadTask::completed,
                this, &UploadTask::onChunkCompleted,
                Qt::QueuedConnection);
        connect(chunkTask, &ChunkUploadTask::error,
                this, &UploadTask::onChunkError,
                Qt::QueuedConnection);
        connect(chunkTask, &ChunkUploadTask::progressUpdated,
                this, &UploadTask::onChunkProgress,
                Qt::QueuedConnection);

        // 提交任务到线程池
        QThreadPool::globalInstance()->start(chunkTask);
    }
}

void UploadTask::onChunkCompleted(int chunkNumber)
{
    QMutexLocker locker(&m_mutex);
    m_completedChunks.insert(chunkNumber);
    checkCompletion();
}

void UploadTask::onChunkError(int chunkNumber, const QString &errorMessage)
{
    emit error(m_fileId, QString("分片 %1 上传失败: %2").arg(chunkNumber).arg(errorMessage));
}

void UploadTask::onChunkProgress(int chunkNumber, qint64 bytesSent, qint64 bytesTotal)
{
    QMutexLocker locker(&m_mutex);

    // 使用已完成的分片数量来计算进度
    int totalProgress = static_cast<int>((m_completedChunks.size() * 100.0) / m_totalChunks);

    // 当前分片的进度贡献
    if (bytesTotal > 0)
    {
        double chunkProgress = (bytesSent * 1.0) / bytesTotal;
        double chunkContribution = (chunkProgress * 100.0) / m_totalChunks;
        totalProgress += static_cast<int>(chunkContribution);
    }

    // 确保进度不超过100%
    qDebug() << "===上传进度" << totalProgress << "===";
    totalProgress = qMin(totalProgress, 100);
    emit progressUpdated(m_fileId, totalProgress);
}

void UploadTask::checkCompletion()
{
    if (m_completedChunks.size() == m_totalChunks)
    {
        emit completed(m_fileId);
    }
}
