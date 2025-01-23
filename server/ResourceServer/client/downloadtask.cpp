#include "downloadtask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QEventLoop>

DownloadTask::DownloadTask(const QString& fileId, const QString& savePath, QObject *parent)
    : QObject(parent)
    , m_fileId(fileId)
    , m_savePath(savePath)
    , m_file(savePath)
{
    // 设置自动删除，这样任务完成后会自动清理
    setAutoDelete(true);
    
    m_networkManager = new QNetworkAccessManager(this);
    
    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error(m_fileId, "无法创建文件");
        return;
    }
}

void DownloadTask::run()
{
    // 将NetworkAccessManager移动到当前线程
    m_networkManager->moveToThread(QThread::currentThread());

    QUrl url(QString("http://localhost:8080/download/%1").arg(m_fileId));
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::downloadProgress,
            this, &DownloadTask::onDownloadProgress);

    // 等待下载完成
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // 使用errorOccurred信号替代error信号
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(reply, &QNetworkReply::errorOccurred,
                [this, reply, &loop](QNetworkReply::NetworkError error) {
                    emit this->error(m_fileId, reply->errorString());
                    reply->deleteLater();
                    loop.quit();
                });
    #else
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                [this, reply, &loop](QNetworkReply::NetworkError error) {
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
        }
    });

    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        // 确保所有数据都写入文件
        QByteArray remainingData = reply->readAll();
        if (!remainingData.isEmpty()) {
            m_file.write(remainingData);
        }
        m_file.close();
        emit completed(m_fileId);
    }

    reply->deleteLater();
}

void DownloadTask::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) return;
    
    int progress = (bytesReceived * 100) / bytesTotal;
    emit progressUpdated(m_fileId, progress);
}
