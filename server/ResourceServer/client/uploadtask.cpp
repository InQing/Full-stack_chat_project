#include "uploadtask.h"
#include "chunkuploadtask.h"
#include <QThread>
#include <QMutex>
#include <QMap>

UploadTask::UploadTask(const QString& filePath, const QString& fileId, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
    , m_fileId(fileId)
    , m_file(filePath)
{
    setAutoDelete(false);
    
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit error(m_fileId, "无法打开文件");
        return;
    }

    qint64 fileSize = m_file.size();
    m_totalChunks = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
}

UploadTask::~UploadTask()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

void UploadTask::run()
{
    startChunkUploads();
}

void UploadTask::startChunkUploads()
{
    // 读取所有分片并创建上传任务
    for (int i = 0; i < m_totalChunks; ++i) {
        m_file.seek(i * CHUNK_SIZE);
        QByteArray chunkData = m_file.read(CHUNK_SIZE);
        
        // 创建ChunkUploadTask，分片编号从1开始
        ChunkUploadTask* chunkTask = new ChunkUploadTask(m_fileId, chunkData, i + 1, m_totalChunks);
        
        // 连接信号，使用Qt::QueuedConnection确保跨线程信号安全
        connect(chunkTask, &ChunkUploadTask::completed,
                this, &UploadTask::onChunkCompleted,
                Qt::QueuedConnection);
        connect(chunkTask, &ChunkUploadTask::error,
                this, &UploadTask::onChunkError,
                Qt::QueuedConnection);
        connect(chunkTask, &ChunkUploadTask::progressUpdated,
                this, &UploadTask::onChunkProgress,
                Qt::QueuedConnection);
                
        // 连接finished信号来清理ChunkUploadTask
        connect(chunkTask, &ChunkUploadTask::finished,
                chunkTask, &ChunkUploadTask::deleteLater,
                Qt::QueuedConnection);

        // 提交到线程池
        TaskManager::instance()->startTask(chunkTask);
    }
}

void UploadTask::onChunkCompleted(int chunkNumber)
{
    QMutexLocker locker(&m_mutex);
    m_completedChunks.insert(chunkNumber);
    checkCompletion();
}

void UploadTask::onChunkError(int chunkNumber, const QString& errorMessage)
{
    emit error(m_fileId, QString("分片 %1 上传失败: %2").arg(chunkNumber).arg(errorMessage));
}

void UploadTask::onChunkProgress(int chunkNumber, qint64 bytesSent, qint64 bytesTotal)
{
    QMutexLocker locker(&m_mutex);
    m_chunkProgress[chunkNumber] = bytesSent;
    
    // 计算总进度
    qint64 totalSent = 0;
    for (qint64 sent : m_chunkProgress.values()) {
        totalSent += sent;
    }
    
    // 计算实际文件大小，避免进度超过100%
    qint64 actualFileSize = m_file.size();
    int totalProgress = static_cast<int>((totalSent * 100.0) / actualFileSize);
    
    // 确保进度不超过100%
    totalProgress = qMin(totalProgress, 100);
    emit progressUpdated(m_fileId, totalProgress);
}

void UploadTask::checkCompletion()
{
    if (m_completedChunks.size() == m_totalChunks) {
        emit completed(m_fileId);
    }
}