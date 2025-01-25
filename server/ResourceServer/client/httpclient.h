#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);
    ~HttpClient();

    // 发送POST请求
    void sendPostRequest(const QString &url, const QByteArray &data, const QMap<QString, QString> &headers = QMap<QString, QString>());

signals:
    // 请求完成信号
    void requestFinished(const QByteArray &response);
    // 发生错误信号
    void requestError(const QString &error);
    // 上传进度信号
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private slots:
    void handleReply();
    void handleError(QNetworkReply::NetworkError error);
    void handleUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    QNetworkAccessManager *manager;
    QNetworkReply *currentReply;
};

#endif // HTTPCLIENT_H