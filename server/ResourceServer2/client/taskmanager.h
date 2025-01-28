#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QMap>
#include "uploadtask.h"
#include "downloadtask.h"

class QRunnable;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    static TaskManager* instance();
    void addUploadTask(const QString& filePath, const QString& fileId);
    void addDownloadTask(const QString& fileId, const QString& savePath);
    void startTask(QRunnable* task);
    void setMaxThreadCount(int count);

signals:
    void progressUpdated(const QString& fileId, int progress);
    void taskCompleted(const QString& fileId);
    void taskError(const QString& fileId, const QString& errorMessage);

private:
    explicit TaskManager(QObject *parent = nullptr);
    static TaskManager* m_instance;
    static QMutex m_mutex;
    QThreadPool m_threadPool;
};

#endif // TASKMANAGER_H
