#include "httpclient.h"
#include <QDebug>

HttpClient::HttpClient(QObject *parent) : QObject(parent)
{
    manager_ = new QNetworkAccessManager(this);
}

HttpClient::~HttpClient()
{
    
}

void HttpClient:: PostRequest(const QString &url, const QByteArray &data, const QMap<QString, QString> &headers)
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
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }

    auto _self = shared_from_this();
    // 发送请求
    QNetworkReply *reply = manager_->post(request, data);
    // 设置信号与槽等待发送完成并接收到回复
    connect(reply, &QNetworkReply::finished, [reply, _self](){
        // 处理错误情况
        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();
            emit _self->sig_http_error(reply->errorString());
            reply->deleteLater();
            return;
        }

        // 无错误，读取回复
        QString res = reply->readAll();
        emit _self->sig_http_finish(res);
        reply->deleteLater();
        return;
    });
}
