#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QMap>
#include <QString>
#include "Singleton.h"

class QRunnable;

class FileManager : public QObject, public Singleton<FileManager>
{
    Q_OBJECT
    friend class Singleton<FileManager>;
public:
    void addUploadTask(const QString& filePath, const QString& fileId);
    // void addDownloadTask(const QString& fileId, const QString& savePath);
    void startTask(QRunnable* task);
    void setMaxThreadCount(int count);
    QString formatFileSize(qint64 bytes);

signals:
    void progressUpdated(const QString& fileId, int progress);
    void taskCompleted(const QString& fileId);
    void taskError(const QString& fileId, const QString& errorMessage);

private:
    FileManager();
    static QMutex m_mutex;
    QThreadPool thread_pool_;
};

#endif // FILEMANAGER_H
