#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPair>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onUploadButtonClicked();
    void onDownloadButtonClicked();
    void updateProgress(const QString& fileId, int progress);

private:
    void setupUI();
    void createProgressBar(const QString& fileId, const QString& fileName);

    QListWidget* fileListWidget;
    QPushButton* uploadButton;
    QPushButton* downloadButton;
    QMap<QString, QPair<QListWidgetItem*, QProgressBar*>> progressBars;
};

#endif // MAINWINDOW_H
