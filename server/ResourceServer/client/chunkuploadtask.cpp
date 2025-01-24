#include "chunkuploadtask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QDebug>

ChunkUploadTask::ChunkUploadTask(const QString &fileId,
                                 const QString &uploadId,
                                 const QByteArray &chunkData,
                                 int chunkNumber,
                                 int totalChunks,
                                 QObject *parent)
    : QObject(parent), m_fileId(fileId), m_uploadId(uploadId), m_chunkData(chunkData), m_chunkNumber(chunkNumber), m_totalChunks(totalChunks)
{
    setAutoDelete(true);
}

void ChunkUploadTask::run()
{
    qDebug() << "\n=== 开始上传分片 ==="
             << "\n文件ID:" << m_fileId
             << "\nUploadID:" << m_uploadId
             << "\n分片编号:" << m_chunkNumber
             << "\n总分片数:" << m_totalChunks
             << "\n分片大小:" << m_chunkData.size() << "字节";

    QNetworkAccessManager networkManager;
    QEventLoop eventLoop;

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

    // 发送请求
    QUrl url("http://localhost:8080/upload/chunk");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Upload-ID", m_uploadId.toUtf8());

    QNetworkReply *reply = networkManager.post(request, postData);

    // 连接完成信号
    connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "分片" << m_chunkNumber << "上传错误:" << reply->errorString();
        emit error(m_chunkNumber, reply->errorString());
        reply->deleteLater();
        return;
    }

    // 解析响应
    QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
    QJsonObject responseObj = response.object();

    if (responseObj["status"].toString() != "success")
    {
        qDebug() << "分片" << m_chunkNumber << "上传失败:" << responseObj["message"].toString();
        emit error(m_chunkNumber, responseObj["message"].toString());
    }
    else
    {
        qDebug() << "分片" << m_chunkNumber << "上传完成";
        // 计算上传进度
        int progress = static_cast<int>((m_chunkNumber * 100.0) / m_totalChunks);
        emit progressUpdated(m_chunkNumber, progress, 100);
        emit completed(m_chunkNumber);
    }

    reply->deleteLater();
}

QByteArray ChunkUploadTask::calculateMD5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}
