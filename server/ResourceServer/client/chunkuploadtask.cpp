#include "chunkuploadtask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QDebug>

ChunkUploadTask::ChunkUploadTask(const QString& fileId,
                               const QByteArray& chunkData,
                               int chunkNumber,
                               int totalChunks,
                               QObject* parent)
    : QObject(parent)
    , m_fileId(fileId)
    , m_chunkData(chunkData)
    , m_chunkNumber(chunkNumber)
    , m_totalChunks(totalChunks)
{
    setAutoDelete(true);
}

void ChunkUploadTask::run()
{
    qDebug() << "\n=== 开始上传分片 ===" 
             << "\n文件ID:" << m_fileId
             << "\n分片编号:" << m_chunkNumber
             << "\n总分片数:" << m_totalChunks
             << "\n分片大小:" << m_chunkData.size() << "字节";
    
    QNetworkAccessManager networkManager;
    
    // 先计算原始数据的MD5
    QString md5 = calculateMD5(m_chunkData);
    
    // 准备JSON数据
    QJsonObject json;
    json["file_id"] = m_fileId;
    json["chunk"] = QString::fromLatin1(m_chunkData.toBase64());
    json["chunk_number"] = m_chunkNumber;
    json["total_chunks"] = m_totalChunks;
    json["md5"] = md5;

    QJsonDocument doc(json);

    // 发送请求
    QUrl url("http://localhost:8080/upload");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "\n=== 发送请求 ===\n"
             << "URL:" << url.toString()
             << "\nContent-Type:" << request.header(QNetworkRequest::ContentTypeHeader).toString();

    QNetworkReply* reply = networkManager.post(request, doc.toJson());
    
    // 连接信号
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // 使用errorOccurred信号替代error信号
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(reply, &QNetworkReply::errorOccurred,
                [this, reply, &loop](QNetworkReply::NetworkError error) {
                    qDebug() << "\n=== 上传错误 ===\n"
                            << "分片编号:" << m_chunkNumber
                            << "\n错误类型:" << error
                            << "\n错误信息:" << reply->errorString();
                    
                    // 输出服务器响应
                    QByteArray response = reply->readAll();
                    qDebug() << "服务器响应:" << QString::fromUtf8(response);
                    
                    emit this->error(m_chunkNumber, reply->errorString());
                    reply->deleteLater();
                    loop.quit();
                });
    #else
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                [this, reply, &loop](QNetworkReply::NetworkError error) {
                    qDebug() << "\n=== 上传错误 ===\n"
                            << "分片编号:" << m_chunkNumber
                            << "\n错误类型:" << error
                            << "\n错误信息:" << reply->errorString();
                    
                    // 输出服务器响应
                    QByteArray response = reply->readAll();
                    qDebug() << "服务器响应:" << QString::fromUtf8(response);
                    
                    emit this->error(m_chunkNumber, reply->errorString());
                    reply->deleteLater();
                    loop.quit();
                });
    #endif

    connect(reply, &QNetworkReply::uploadProgress,
            [this](qint64 bytesSent, qint64 bytesTotal) {
                qDebug() << "分片" << m_chunkNumber << "上传进度:" 
                        << bytesSent << "/" << bytesTotal;
                emit progressUpdated(m_chunkNumber, bytesSent, bytesTotal);
            });

    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "\n=== 上传成功 ===\n"
                 << "分片编号:" << m_chunkNumber
                 << "\n服务器响应:" << QString::fromUtf8(response);
        emit completed(m_chunkNumber);
    }

    reply->deleteLater();
    emit finished();
}

QByteArray ChunkUploadTask::calculateMD5(const QByteArray& data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}
