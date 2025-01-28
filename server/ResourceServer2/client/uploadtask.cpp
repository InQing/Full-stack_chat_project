#include "uploadtask.h"
#include "chunkuploadtask.h"
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QThreadPool>

UploadTask::UploadTask(const QString &filePath, const QString &fileId,
                       const QString &token, const QString &uid,
                       QObject *parent)
    : QObject(parent), m_filePath(filePath), m_fileId(fileId), m_token(token), m_uid(uid),
      m_file(filePath), m_httpClient(nullptr), m_eventLoop(nullptr)
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
    if (m_eventLoop)
    {
        m_eventLoop->quit();
        delete m_eventLoop;
    }
    if (m_httpClient)
    {
        m_httpClient->deleteLater();
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
    // 创建事件循环和HttpClient
    m_eventLoop = new QEventLoop();
    m_httpClient = new HttpClient();
    m_httpClient->moveToThread(QThread::currentThread());

    // 连接信号
    connect(m_httpClient, &HttpClient::requestFinished, this, &UploadTask::onInitUploadFinished);
    connect(m_httpClient, &HttpClient::requestError, this, &UploadTask::onInitUploadError);

    // 准备初始化请求数据
    QJsonObject json;
    json["file_id"] = m_fileId;
    json["filename"] = QFileInfo(m_filePath).fileName();
    json["uid"] = m_uid;

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    // 准备请求头
    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = m_token;

    // 发送请求
    m_httpClient->sendPostRequest("http://localhost:8080/upload/init", postData, headers);

    // 等待响应
    m_eventLoop->exec();

    return !m_uploadId.isEmpty();
}

void UploadTask::onInitUploadFinished(const QByteArray &response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject responseObj = doc.object();

    if (responseObj["code"].toInt() != 200)
    {
        emit error(m_fileId, "初始化上传失败: " + responseObj["message"].toString());
        m_eventLoop->quit();
        return;
    }

    // 保存uploadId
    m_uploadId = responseObj["data"].toObject()["upload_id"].toString();
    m_eventLoop->quit();
}

void UploadTask::onInitUploadError(const QString &error)
{
    emit this->error(m_fileId, "初始化上传失败: " + error);
    m_eventLoop->quit();
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
