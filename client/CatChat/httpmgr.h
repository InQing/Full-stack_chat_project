#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "global.h"
#include "Singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>

class HttpMgr:public QObject, public Singleton<HttpMgr>,
                public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:
    ~HttpMgr();
private:
    friend class Singleton<HttpMgr>;
    HttpMgr();

    QNetworkAccessManager _manager;

    // HTTP POST请求
    void postHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);
private slots:
    // HTTP响应结束的槽函数，根据mod将信号发送到相应模块
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
signals:
    // HTTP响应结束，发送信号
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    // 发送信号到注册模块
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
