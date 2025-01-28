#include "httpclient.h"
#include <QDebug>

HttpClient::HttpClient(QObject *parent) : QObject(parent), currentReply(nullptr)
{
    // 在当前线程创建manager
    manager = new QNetworkAccessManager(this);
}

HttpClient::~HttpClient()
{
    if (currentReply)
    {
        currentReply->abort();
        currentReply->deleteLater();
    }
}

void HttpClient::sendPostRequest(const QString &url, const QByteArray &data, const QMap<QString, QString> &headers)
{
    // 创建请求
    QNetworkRequest request(url);

    // 设置请求头
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
    {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    // 如果没有设置Content-Type，默认设置
    if (!headers.contains("Content-Type"))
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    }

    // 发送请求
    currentReply = manager->post(request, data);

    // 连接信号
    connect(currentReply, &QNetworkReply::finished, this, &HttpClient::handleReply);
    connect(currentReply, &QNetworkReply::uploadProgress, this, &HttpClient::handleUploadProgress);
    connect(currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &HttpClient::handleError);
}

void HttpClient::handleReply()
{
    if (!currentReply)
        return;

    if (currentReply->error() == QNetworkReply::NoError)
    {
        QByteArray response = currentReply->readAll();
        emit requestFinished(response);
    }

    currentReply->deleteLater();
    currentReply = nullptr;
}

void HttpClient::handleError(QNetworkReply::NetworkError error)
{
    if (!currentReply)
        return;

    QString errorString = currentReply->errorString();
    qDebug() << "Network error:" << error << errorString;
    emit requestError(errorString);
}

void HttpClient::handleUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    emit uploadProgress(bytesSent, bytesTotal);
}