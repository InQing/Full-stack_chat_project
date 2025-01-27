#include "filedownloadtask.h"
#include "global.h"
#include <QJsonObject>
#include <QJsonDocument>

DownloadTask::DownloadTask(const QString &fileId, const QString &savePath, const QString &token, const QString &uid, QObject *parent)
    : QObject(parent), fileId_(fileId), savePath_(savePath), file_(savePath), token_(token), uid_(uid)
{
    setAutoDelete(true);

    if (!file_.open(QIODevice::WriteOnly))
    {
        qDebug() << "无法创建文件";
        // emit sig_download_error("无法创建文件");
        return;
    }
}

void DownloadTask::run()
{
    qDebug() << "\n=== 开始下载 ==="
             << "\n文件ID:" << fileId_
             << "\n保存路径:" << savePath_;

    // 在当前线程创建网络管理器
    QNetworkAccessManager networkManager;
    QEventLoop loop;

    // 构造URL和请求
    QUrl url(QString(resource_url_prefix + "/download/%1?uid=%2").arg(fileId_).arg(uid_));
    QNetworkRequest request(url);

    // 设置请求头
    request.setRawHeader("Authorization", token_.toUtf8());

    qDebug() << "下载URL:" << url.toString();

    // 发送GET请求
    QNetworkReply *reply = networkManager.get(request);

    // 错误处理
    connect(reply, &QNetworkReply::errorOccurred,
            [this, reply, &loop](QNetworkReply::NetworkError error)
            {
                qDebug() << "\n=== 下载错误 ===\n"
                         << "文件ID:" << fileId_
                         << "\n错误类型:" << error
                         << "\n错误信息:" << reply->errorString();

                file_.close();
                // emit sig_download_error(reply->errorString());
                reply->deleteLater();
                loop.quit();
            });

    // 处理接收到的数据流
    connect(reply, &QNetworkReply::readyRead, [this, reply]()
            {
                // 每次读取可用的数据
                QByteArray data = reply->readAll();
                if (!data.isEmpty()) {
                    // 写入文件
                    if (file_.write(data) == -1) {
                        qDebug() << "写入文件失败:" << file_.errorString();
                        reply->abort();
                        return;
                    }
                    // 立即刷新到磁盘
                    file_.flush();
                } });

    // 下载完成处理
    connect(reply, &QNetworkReply::finished, [this, reply, &loop]()
            {
                if (reply->error() == QNetworkReply::NoError) {
                    file_.close();
                    qDebug() << "\n=== 下载完成 ===\n"
                             << "文件ID:" << fileId_
                             << "\n保存路径:" << savePath_;
                    // emit sig_download_finished();
                }
                reply->deleteLater();
                loop.quit(); });

    loop.exec();
}
