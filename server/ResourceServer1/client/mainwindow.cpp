#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QUuid>
#include "taskmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    // 连接TaskManager的信号
    connect(TaskManager::instance(), &TaskManager::progressUpdated,
            this, &MainWindow::updateProgress);
    connect(TaskManager::instance(), &TaskManager::taskCompleted,
            this, [this](const QString& fileId) {
                if (progressBars.contains(fileId)) {
                    QListWidgetItem* item = progressBars[fileId].first;
                    delete item;
                    progressBars.remove(fileId);
                }
            });
    connect(TaskManager::instance(), &TaskManager::taskError,
            this, [this](const QString& fileId, const QString& errorMessage) {
                QMessageBox::critical(this, "错误", errorMessage);
                if (progressBars.contains(fileId)) {
                    QListWidgetItem* item = progressBars[fileId].first;
                    delete item;
                    progressBars.remove(fileId);
                }
            });
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 文件列表
    fileListWidget = new QListWidget(this);
    mainLayout->addWidget(fileListWidget);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    uploadButton = new QPushButton("上传文件", this);
    downloadButton = new QPushButton("下载文件", this);
    
    buttonLayout->addWidget(uploadButton);
    buttonLayout->addWidget(downloadButton);
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::onUploadButtonClicked);
    connect(downloadButton, &QPushButton::clicked, this, &MainWindow::onDownloadButtonClicked);

    setWindowTitle("文件传输客户端");
    resize(600, 400);
}

void MainWindow::onUploadButtonClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "选择要上传的文件",
        QString(),
        "所有文件 (*.*)"
    );

    if (!files.isEmpty()) {
        for (const QString& file : files) {
            QString currentDateTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
            QString fileId = currentDateTime + "_" + QFileInfo(file).fileName(); // 日期 + 文件名
            createProgressBar(fileId, QFileInfo(file).fileName());
            TaskManager::instance()->addUploadTask(file, fileId);
        }
    }
}

void MainWindow::onDownloadButtonClicked()
{
    QString saveDir = QFileDialog::getExistingDirectory(
        this,
        "选择保存目录",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!saveDir.isEmpty()) {
        QStringList fileIds = QFileDialog::getOpenFileNames(
            this,
            "选择要下载的文件",
            QString(),
            "所有文件 (*.*)"
        );

        for (const QString& fileId : fileIds) {
            QString savePath = saveDir + "/" + QFileInfo(fileId).fileName();
            createProgressBar(fileId, QFileInfo(fileId).fileName());
            TaskManager::instance()->addDownloadTask(fileId, savePath);
        }
    }
}

void MainWindow::createProgressBar(const QString& fileId, const QString& fileName)
{
    QWidget* widget = new QWidget(fileListWidget);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    QLabel* nameLabel = new QLabel(fileName, widget);
    QProgressBar* progressBar = new QProgressBar(widget);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    
    layout->addWidget(nameLabel);
    layout->addWidget(progressBar);
    
    QListWidgetItem* item = new QListWidgetItem(fileListWidget);
    item->setSizeHint(widget->sizeHint());
    fileListWidget->addItem(item);
    fileListWidget->setItemWidget(item, widget);
    
    progressBars[fileId] = qMakePair(item, progressBar);
}

void MainWindow::updateProgress(const QString& fileId, int progress)
{
    if (progressBars.contains(fileId)) {
        progressBars[fileId].second->setValue(progress);
    }
}
