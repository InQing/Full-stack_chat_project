// downloadtask.cpp
#include "downloadtask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QEventLoop>
#include <QDebug>

DownloadTask::DownloadTask(const QString& fileId, const QString& savePath, QObject *parent)
    : QObject(parent)
    , m_fileId(fileId)
    , m_savePath(savePath)
    , m_file(savePath)
{
    setAutoDelete(true);
    
    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error(m_fileId, "无法创建文件");
        return;
    }
}

void DownloadTask::run()
{
    qDebug() << "\n=== 开始下载 ===" 
             << "\n文件ID:" << m_fileId
             << "\n保存路径:" << m_savePath;

    // 在当前线程创建网络管理器
    QNetworkAccessManager networkManager;
    QEventLoop loop;

    QUrl url(QString("http://localhost:8080/download/%1").arg("20250123172623_test.7z"));
    QNetworkRequest request(url);

    qDebug() << "下载URL:" << url.toString();

    QNetworkReply* reply = networkManager.get(request);
    
    connect(reply, &QNetworkReply::downloadProgress,
            this, &DownloadTask::onDownloadProgress);

    // 使用errorOccurred信号替代error信号
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(reply, &QNetworkReply::errorOccurred,
                [this, reply, &loop](QNetworkReply::NetworkError error) {
                    qDebug() << "\n=== 下载错误 ===\n"
                            << "文件ID:" << m_fileId
                            << "\n错误类型:" << error
                            << "\n错误信息:" << reply->errorString();
                    
                    emit this->error(m_fileId, reply->errorString());
                    reply->deleteLater();
                    loop.quit();
                });
    #else
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                [this, reply, &loop](QNetworkReply::NetworkError error) {
                    qDebug() << "\n=== 下载错误 ===\n"
                            << "文件ID:" << m_fileId
                            << "\n错误类型:" << error
                            << "\n错误信息:" << reply->errorString();
                    
                    emit this->error(m_fileId, reply->errorString());
                    reply->deleteLater();
                    loop.quit();
                });
    #endif

    // 处理接收到的数据
    connect(reply, &QNetworkReply::readyRead, [this, reply]() {
        QByteArray data = reply->readAll();
        if (!data.isEmpty()) {
            m_file.write(data);
            qDebug() << "写入数据:" << data.size() << "字节";
        }
    });

    connect(reply, &QNetworkReply::finished, [this, reply, &loop]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 确保所有数据都写入文件
            QByteArray remainingData = reply->readAll();
            if (!remainingData.isEmpty()) {
                m_file.write(remainingData);
                qDebug() << "写入剩余数据:" << remainingData.size() << "字节";
            }
            m_file.close();
            
            qDebug() << "\n=== 下载完成 ===\n"
                     << "文件ID:" << m_fileId
                     << "\n保存路径:" << m_savePath;
                     
            emit completed(m_fileId);
        }
        reply->deleteLater();
        loop.quit();
    });

    loop.exec();
}

void DownloadTask::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) return;
    
    int progress = (bytesReceived * 100) / bytesTotal;
    qDebug() << "下载进度:" << progress << "%"
             << "(" << bytesReceived << "/" << bytesTotal << "字节)";
             
    emit progressUpdated(m_fileId, progress);
}
