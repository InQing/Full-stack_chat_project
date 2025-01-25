#include "filemanager.h"
#include "fileuploadtask.h"
// #include "downloadtask.h"
#include <QThread>

FileManager::FileManager()
{
    // 设置线程池最大线程数为CPU核心数
    thread_pool_.setMaxThreadCount(QThread::idealThreadCount());
}

void FileManager::setMaxThreadCount(int count)
{
    thread_pool_.setMaxThreadCount(count);
}

void FileManager::startTask(QRunnable* task)
{
    thread_pool_.start(task);
}

void FileManager::addUploadTask(const QString& filePath, const QString& fileId)
{
    UploadTask* task = new UploadTask(filePath, fileId, "5225f70b-1662-488f-a055-c11dbd856574", "3");

    // 连接信号
    // connect(task, &UploadTask::progressUpdated,
    //         this, &TaskManager::progressUpdated,
    //         Qt::QueuedConnection);
    // connect(task, &UploadTask::completed,
    //         this, &TaskManager::taskCompleted,
    //         Qt::QueuedConnection);
    // connect(task, &UploadTask::error,
    //         this, &TaskManager::taskError,
    //         Qt::QueuedConnection);

    // 将任务提交到线程池
    thread_pool_.start(task);
}

// void FileManager::addDownloadTask(const QString& fileId, const QString& savePath)
// {
//     DownloadTask* task = new DownloadTask(fileId, savePath);

//     // 连接信号
//     connect(task, &DownloadTask::progressUpdated,
//             this, &TaskManager::progressUpdated,
//             Qt::QueuedConnection);
//     connect(task, &DownloadTask::completed,
//             this, &TaskManager::taskCompleted,
//             Qt::QueuedConnection);
//     connect(task, &DownloadTask::error,
//             this, &TaskManager::taskError,
//             Qt::QueuedConnection);

//     // 将任务提交到线程池
//     thread_pool_.start(task);
// }

QString FileManager::formatFileSize(qint64 bytes)
{
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    else if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    else if (bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(bytes / 1024.0 / 1024.0, 0, 'f', 2);
    else
        return QString("%1 GB").arg(bytes / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2);
}
