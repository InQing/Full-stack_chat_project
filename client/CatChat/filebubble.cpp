#include "FileBubble.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <QDebug>
#include <QFileDialog>
#include "filemanager.h"
#include "usermgr.h"

FileBubble::FileBubble(ChatRole role, const QString &fileName, const QString &fileSize, const QString fileId, QWidget *parent)
    : BubbleFrame(role, parent), m_role(role), m_fileId(fileId)
{
    initUI(fileName, fileSize);
    initStyleSheet();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void FileBubble::initUI(const QString &fileName, const QString &fileSize)
{
    // 创建内容容器
    m_pContentWidget = new QWidget(this);
    m_pContentWidget->setObjectName("fileContent");

    // 创建图标
    m_pIconLabel = new QLabel();
    QPixmap icon(":/res/file_icon.png");
    if (icon.isNull())
    {
        qDebug() << "Icon not found!";
    }
    m_pIconLabel->setPixmap(icon.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_pIconLabel->setFixedSize(42, 42);
    m_pIconLabel->setObjectName("fileIcon");

    // 创建文件名和大小标签
    m_pFileNameLabel = new QLabel(fileName);
    m_pFileNameLabel->setObjectName("fileName");
    m_pFileSizeLabel = new QLabel(fileSize);
    m_pFileSizeLabel->setObjectName("fileSize");

    // 创建文本布局
    QVBoxLayout *pVLayout = new QVBoxLayout();
    pVLayout->addWidget(m_pFileNameLabel);
    pVLayout->addWidget(m_pFileSizeLabel);
    pVLayout->setSpacing(4);
    pVLayout->setContentsMargins(10, 5, 10, 5);

    // 创建主布局
    QHBoxLayout *pHLayout = new QHBoxLayout(m_pContentWidget);
    pHLayout->addLayout(pVLayout, 1);                     // 添加拉伸因子1，允许文本区域拉伸
    pHLayout->addWidget(m_pIconLabel, 0, Qt::AlignRight); // 设置图标右对齐，不拉伸
    pHLayout->setSpacing(8);
    pHLayout->setContentsMargins(0, 5, 10, 5); // 添加右边距10像素

    // 如果是接收方，添加下载图标
    if (m_role == ChatRole::Other)
    {
        m_pDownloadLabel = new ClickedLabel(m_pContentWidget);
        QPixmap downloadIcon(":/res/download.png");
        if (!downloadIcon.isNull())
        {
            m_pDownloadLabel->setPixmap(downloadIcon.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_pDownloadLabel->setFixedSize(24, 24);
            m_pDownloadLabel->setObjectName("downloadIcon");
            m_pDownloadLabel->setCursor(Qt::PointingHandCursor);

            // 连接点击信号
            connect(m_pDownloadLabel, &ClickedLabel::clicked, this, &FileBubble::onDownloadClicked);

            // 将下载图标添加到布局中，放在文件图标后面
            pHLayout->addWidget(m_pDownloadLabel, 0, Qt::AlignRight | Qt::AlignBottom);
        }
        else
        {
            qDebug() << "Download icon not found!";
        }
    }
    else
    {
        m_pDownloadLabel = nullptr;
    }

    // 使用BubbleFrame的setWidget方法设置内容
    setWidget(m_pContentWidget);
}

void FileBubble::initStyleSheet()
{
    // 设置整体样式
    QString styleSheet = R"(
        #fileName {
            font-family: "Microsoft YaHei";
            font-size: 16px;
            color: #333333;
            font-weight: bold;
        }

        #fileSize {
            font-family: "Microsoft YaHei";
            font-size: 13px;
            color: #999999;
        }

        #fileIcon {
            padding: 2px;
        }

        #downloadIcon {
            margin: 2px;
        }
    )";

    this->setStyleSheet(styleSheet);

    // 设置内容控件的尺寸策略
    m_pContentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    // 设置文件名标签自动换行
    m_pFileNameLabel->setWordWrap(true);
    m_pFileNameLabel->setMaximumWidth(200);
}

void FileBubble::onDownloadClicked()
{
    // 获取保存路径
    QString savePath = QFileDialog::getSaveFileName(
        this,
        "选择保存位置",
        m_pFileNameLabel->text(), // 默认使用文件原名
        "所有文件 (*.*)");

    if (!savePath.isEmpty())
    {
        // 添加下载任务
        FileManager::GetInstance()->addDownloadTask(
            m_fileId,
            savePath,
            UserMgr::GetInstance()->GetToken(),
            QString::number(UserMgr::GetInstance()->GetUid()));
    }
}
