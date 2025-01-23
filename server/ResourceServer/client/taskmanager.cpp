#include "taskmanager.h"
#include "uploadtask.h"
#include "downloadtask.h"
#include <QThread>

TaskManager* TaskManager::m_instance = nullptr;
QMutex TaskManager::m_mutex;

TaskManager* TaskManager::instance()
{
    if (m_instance == nullptr) {
        QMutexLocker locker(&m_mutex);
        if (m_instance == nullptr) {
            m_instance = new TaskManager();
        }
    }
    return m_instance;
}

TaskManager::TaskManager(QObject *parent) : QObject(parent)
{
    // 设置线程池最大线程数为CPU核心数的2倍
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount() * 2);
}

void TaskManager::setMaxThreadCount(int count)
{
    m_threadPool.setMaxThreadCount(count);
}

void TaskManager::startTask(QRunnable* task)
{
    m_threadPool.start(task);
}

void TaskManager::addUploadTask(const QString& filePath, const QString& fileId)
{
    UploadTask* task = new UploadTask(filePath, fileId);
    
    // 连接信号
    connect(task, &UploadTask::progressUpdated,
            this, &TaskManager::progressUpdated,
            Qt::QueuedConnection);
    connect(task, &UploadTask::completed,
            this, &TaskManager::taskCompleted,
            Qt::QueuedConnection);
    connect(task, &UploadTask::error,
            this, &TaskManager::taskError,
            Qt::QueuedConnection);

    // 将任务提交到线程池
    m_threadPool.start(task);
}

void TaskManager::addDownloadTask(const QString& fileId, const QString& savePath)
{
    DownloadTask* task = new DownloadTask(fileId, savePath);
    
    // 连接信号
    connect(task, &DownloadTask::progressUpdated,
            this, &TaskManager::progressUpdated,
            Qt::QueuedConnection);
    connect(task, &DownloadTask::completed,
            this, &TaskManager::taskCompleted,
            Qt::QueuedConnection);
    connect(task, &DownloadTask::error,
            this, &TaskManager::taskError,
            Qt::QueuedConnection);

    // 将任务提交到线程池
    m_threadPool.start(task);
}
