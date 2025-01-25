#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>

class HttpClient : public QObject, public std::enable_shared_from_this<HttpClient>
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);
    ~HttpClient();

    // 发送POST请求
    void PostRequest(const QString &url, const QByteArray &data, const QMap<QString, QString> &headers = QMap<QString, QString>());

signals:
    // Http请求完成信号
    void sig_http_finish(const QString &response);
    void sig_http_error(const QString &error);

private slots:

private:
    QNetworkAccessManager *manager_;
};

#endif // HTTPCLIENT_H
