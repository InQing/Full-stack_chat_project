#include "chunkuploadtask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QDebug>
#include <QThread>

ChunkUploadTask::ChunkUploadTask(const QString &fileId,
                                 const QString &uploadId,
                                 const QByteArray &chunkData,
                                 int chunkNumber,
                                 int totalChunks,
                                 QObject *parent)
    : QObject(parent), m_fileId(fileId), m_uploadId(uploadId), m_chunkData(chunkData),
      m_chunkNumber(chunkNumber), m_totalChunks(totalChunks),
      m_httpClient(nullptr), m_eventLoop(nullptr)
{
    setAutoDelete(true);
}

ChunkUploadTask::~ChunkUploadTask()
{
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

void ChunkUploadTask::run()
{
    qDebug() << "\n=== 开始上传分片 ==="
             << "\n文件ID:" << m_fileId
             << "\nUploadID:" << m_uploadId
             << "\n分片编号:" << m_chunkNumber
             << "\n总分片数:" << m_totalChunks
             << "\n分片大小:" << m_chunkData.size() << "字节"
             << "\n线程ID:" << QThread::currentThreadId();

    // 创建事件循环
    m_eventLoop = new QEventLoop();

    // 创建HttpClient并移动到当前线程
    m_httpClient = new HttpClient();
    m_httpClient->moveToThread(QThread::currentThread());

    // 连接信号
    connect(m_httpClient, &HttpClient::requestFinished, this, &ChunkUploadTask::handleUploadFinished);
    connect(m_httpClient, &HttpClient::requestError, this, &ChunkUploadTask::handleUploadError);
    connect(m_httpClient, &HttpClient::uploadProgress, this, &ChunkUploadTask::handleUploadProgress);

    // 计算MD5
    QString md5 = calculateMD5(m_chunkData);

    // 准备JSON数据
    QJsonObject json;
    json["file_id"] = m_fileId;
    json["upload_id"] = m_uploadId;
    json["chunk"] = QString::fromLatin1(m_chunkData.toBase64());
    json["chunk_number"] = m_chunkNumber;
    json["total_chunks"] = m_totalChunks;
    json["md5"] = md5;

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    // 准备请求头
    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Upload-ID"] = m_uploadId;

    // 发送请求
    m_httpClient->sendPostRequest("http://localhost:8080/upload/chunk", postData, headers);

    // 等待上传完成
    m_eventLoop->exec();
}

void ChunkUploadTask::handleUploadFinished(const QByteArray &response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject responseObj = doc.object();

    if (responseObj["status"].toString() != "success")
    {
        qDebug() << "分片" << m_chunkNumber << "上传失败:" << responseObj["message"].toString();
        emit error(m_chunkNumber, responseObj["message"].toString());
    }
    else
    {
        qDebug() << "分片" << m_chunkNumber << "上传完成";
        emit completed(m_chunkNumber);
    }

    m_eventLoop->quit();
    emit finished();
}

void ChunkUploadTask::handleUploadError(const QString &error)
{
    qDebug() << "分片" << m_chunkNumber << "上传错误:" << error;
    emit this->error(m_chunkNumber, error);
    m_eventLoop->quit();
    emit finished();
}

void ChunkUploadTask::handleUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    emit progressUpdated(m_chunkNumber, bytesSent, bytesTotal);
}

QByteArray ChunkUploadTask::calculateMD5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}
