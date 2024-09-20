#include "httpmgr.h"

HttpMgr::~HttpMgr()
{

}

HttpMgr::HttpMgr() {
    //连接http请求和完成信号
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

void HttpMgr::postHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    // 发送的数据，转换成字节流
    QByteArray data = QJsonDocument(json).toJson();
    // 通过url构造请求
    QNetworkRequest request(url);
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    // 获取自己的智能指针，构造伪闭包并增加智能指针的引用计数
    // 目的：防止HttpMgr提前析构，导致下面操作奔溃
    auto _self = shared_from_this();
    // 发送请求
    QNetworkReply *reply = _manager.post(request, data);
    // 设置信号与槽等待发送完成并接收到回复
    connect(reply, &QNetworkReply::finished, [reply, _self, req_id, mod](){
        // 处理错误情况
        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();
            // HTTP请求完成，发送信号通知
            emit _self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        // 无错误，读取回复
        QString res = reply->readAll();
        emit _self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS,mod);
        reply->deleteLater();
        return;
    });

}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    // 发送信号通知指定模块http响应结束
    if(mod == Modules::MOD_REGISTER){
        emit sig_reg_mod_finish(id, res, err);
    }
}
