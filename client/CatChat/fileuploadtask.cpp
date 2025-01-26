#include "fileuploadtask.h"
#include "filemanager.h"
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QThreadPool>

UploadTask::UploadTask(const QString &filePath, const QString &fileId,
                       const QString &token, const QString &uid,
                       QObject *parent)
    : QObject(parent), filePath_(filePath), fileId_(fileId), token_(token), uid_(uid),
      file_(filePath), httpClient_(nullptr), eventLoop_(nullptr)
{
    setAutoDelete(true);

    qint64 fileSize = file_.size();
    totalChunks_ = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
}

UploadTask::~UploadTask()
{
    if (file_.isOpen())
    {
        file_.close();
    }
    if (eventLoop_)
    {
        eventLoop_->quit();
        delete eventLoop_;
    }
    if (httpClient_)
    {
        httpClient_->deleteLater();
    }
}

void UploadTask::run()
{
    if (!file_.open(QIODevice::ReadOnly))
    {
        qDebug() << "Failed to open file:" << filePath_;
        return;
    }

    if (!InitializeUpload())
    {
        file_.close();
        return;
    }
    StartChunkUploads();
}

bool UploadTask::InitializeUpload()
{
    // 创建事件循环和HttpClient
    eventLoop_ = new QEventLoop();
    httpClient_ = std::make_shared<HttpClient>();
    httpClient_->moveToThread(QThread::currentThread());

    // 连接信号
    connect(httpClient_.get(), &HttpClient::sig_http_finish, this, &UploadTask::on_upload_init_finished);
    connect(httpClient_.get(), &HttpClient::sig_http_error, this, &UploadTask::on_upload_init_error);

    // 准备初始化请求数据
    QJsonObject json;
    json["file_id"] = fileId_;
    json["filename"] = QFileInfo(filePath_).fileName();
    json["uid"] = uid_;

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    // 准备请求头
    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = token_;

    // 发送请求
    httpClient_->PostRequest(resource_url_prefix + "/upload/init", postData, headers);

    // 等待响应
    eventLoop_->exec();

    return !uploadId_.isEmpty();
}

void UploadTask::on_upload_init_finished(const QString &response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    QJsonObject responseObj = doc.object();

    if (responseObj["code"].toInt() != 200)
    {
        qDebug() << "初始化上传失败:" << responseObj["message"].toString();
        emit sig_upload_init_error(ErrorCodes::ERR_UPLOAD_INIT);
        eventLoop_->quit();
        return;
    }

    // 保存uploadId
    uploadId_ = responseObj["data"].toObject()["upload_id"].toString();
    eventLoop_->quit();
}

void UploadTask::on_upload_init_error(const QString &error)
{
    emit sig_upload_init_error(ErrorCodes::ERR_UPLOAD_INIT);
    eventLoop_->quit();
}

void UploadTask::StartChunkUploads()
{
    if (!file_.isOpen())
    {
        qDebug() << "File is not open:" << filePath_;
        return;
    }

    // 读取所有分片并创建上传任务
    for (int i = 0; i < totalChunks_; ++i)
    {
        if (!file_.seek(i * CHUNK_SIZE))
        {
            qDebug() << "Failed to seek file to position:" << i * CHUNK_SIZE;
            continue;
        }

        QByteArray chunkData = file_.read(CHUNK_SIZE);
        if (chunkData.isEmpty())
        {
            qDebug() << "Failed to read chunk" << i + 1;
            continue;
        }

        // 创建ChunkUploadTask，分片编号从1开始
        ChunkUploadTask *chunkTask = new ChunkUploadTask(
            fileId_, uploadId_, chunkData, i + 1, totalChunks_);

        // 连接信号
        connect(chunkTask, &ChunkUploadTask::sig_completed,
                this, &UploadTask::on_chunk_completed,
                Qt::QueuedConnection);
        connect(chunkTask, &ChunkUploadTask::sig_error,
                this, &UploadTask::on_chunk_error,
                Qt::QueuedConnection);

        // 提交任务到线程池
        FileManager::GetInstance()->startTask(chunkTask);
    }
}

void UploadTask::on_chunk_completed(int chunkNumber)
{
    QMutexLocker locker(&mutex_);
    completedChunks_.insert(chunkNumber);

    // 使用已完成的分片数量来计算进度
    int totalProgress = static_cast<int>((completedChunks_.size() * 100.0) / totalChunks_);

    // 确保进度不超过100%
    qDebug() << "===上传进度" << totalProgress << "===";
    totalProgress = qMin(totalProgress, 100);
    // 更新进度条 TODO
    // emit progressUpdated(fileId_, totalProgress);

    CheckCompletion();
}

void UploadTask::on_chunk_error(int chunkNumber)
{
    emit sig_upload_chunk_finished(ErrorCodes::ERR_UPLOAD_CHUNK);
}

void UploadTask::CheckCompletion()
{
    if (completedChunks_.size() == totalChunks_)
    {
        emit sig_upload_finished();
    }
}

ChunkUploadTask::ChunkUploadTask(const QString &fileId,
                                 const QString &uploadId,
                                 const QByteArray &chunkData,
                                 int chunkNumber,
                                 int totalChunks,
                                 QObject *parent)
    : QObject(parent), fileId_(fileId), uploadId_(uploadId), chunkData_(chunkData),
      chunkNumber_(chunkNumber), totalChunks_(totalChunks),
      httpClient_(nullptr), eventLoop_(nullptr)
{
    setAutoDelete(true);
}

ChunkUploadTask::~ChunkUploadTask()
{
    if (eventLoop_)
    {
        eventLoop_->quit();
        delete eventLoop_;
    }
    if (httpClient_)
    {
        httpClient_->deleteLater();
    }
}

void ChunkUploadTask::run()
{
    qDebug() << "\n=== 开始上传分片 ==="
             << "\n文件ID:" << fileId_
             << "\nUploadID:" << uploadId_
             << "\n分片编号:" << chunkNumber_
             << "\n总分片数:" << totalChunks_
             << "\n分片大小:" << chunkData_.size() << "字节"
             << "\n线程ID:" << QThread::currentThreadId();

    // 创建事件循环
    eventLoop_ = new QEventLoop();

    // 创建HttpClient并移动到当前线程
    httpClient_ = std::make_shared<HttpClient>();
    httpClient_->moveToThread(QThread::currentThread());

    // 连接信号
    connect(httpClient_.get(), &HttpClient::sig_http_finish, this, &ChunkUploadTask::on_upload_chunk_finished);
    connect(httpClient_.get(), &HttpClient::sig_http_error, this, &ChunkUploadTask::on_upload_chunk_error);

    // 计算MD5
    QString md5 = CalculateMD5(chunkData_);

    // 准备JSON数据
    QJsonObject json;
    json["file_id"] = fileId_;
    json["upload_id"] = uploadId_;
    json["chunk"] = QString::fromLatin1(chunkData_.toBase64());
    json["chunk_number"] = chunkNumber_;
    json["total_chunks"] = totalChunks_;
    json["md5"] = md5;

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    // 准备请求头
    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Upload-ID"] = uploadId_;

    // 发送请求
    httpClient_->PostRequest(resource_url_prefix + "/upload/chunk", postData, headers);

    // 等待上传完成
    eventLoop_->exec();
}

void ChunkUploadTask::on_upload_chunk_finished(const QString &response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    QJsonObject responseObj = doc.object();

    if (responseObj["status"].toString() != "success")
    {
        qDebug() << "分片" << chunkNumber_ << "上传失败:" << responseObj["message"].toString();
        emit sig_error(chunkNumber_);
    }
    else
    {
        qDebug() << "分片" << chunkNumber_ << "上传完成";
        emit sig_completed(chunkNumber_);
    }

    eventLoop_->quit();
}

void ChunkUploadTask::on_upload_chunk_error(const QString &error)
{
    qDebug() << "分片" << chunkNumber_ << "上传错误:" << error;
    emit sig_error(chunkNumber_);
    eventLoop_->quit();
}

QByteArray ChunkUploadTask::CalculateMD5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}
