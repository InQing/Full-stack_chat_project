#include "FileBubble.h"
#include <QHBoxLayout>
#include <QPixmap>
#include <QDebug>

FileBubble::FileBubble(ChatRole role, const QString &fileName, const QString &fileSize, QWidget *parent)
    : BubbleFrame(role, parent)
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
    )";

    this->setStyleSheet(styleSheet);

    // 设置内容控件的尺寸策略
    m_pContentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    // 设置文件名标签自动换行
    m_pFileNameLabel->setWordWrap(true);
    m_pFileNameLabel->setMaximumWidth(200);
}
