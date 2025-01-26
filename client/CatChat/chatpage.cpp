#include <QStyleOption>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QFileDialog>
#include "chatpage.h"
#include "ui_chatpage.h"
#include "TextBubble.h"
#include "PictureBubble.h"
#include "usermgr.h"
#include "tcpmgr.h"
#include "clickedlabel.h"
#include "FileBubble.h"
#include "filemanager.h"

ChatPage::ChatPage(QWidget *parent) : QWidget(parent),
                                      ui(new Ui::ChatPage)
{
    ui->setupUi(this);
    // 设置按钮样式
    ui->receive_btn->SetState("normal", "hover", "press");
    ui->send_btn->SetState("normal", "hover", "press");

    // 设置图标样式
    ui->emo_lb->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui->file_lb->SetState("normal", "hover", "press", "normal", "hover", "press");

    // 连接点击文件图标信号
    // 不需要！由于槽函数的名称，qt会自动匹配，如果这里连接会导致槽函数触发两次
    // connect(ui->file_lb, &ClickedLabel::clicked, this, &ChatPage::on_file_lb_clicked);
}

ChatPage::~ChatPage()
{
    delete ui;
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    // 设置ui界面
    ui->title_lb->setText(_user_info->_name);
    ui->chat_data_list->removeAllItem();
    for (auto &msg : user_info->_chat_msgs)
    {
        AppendChatMsg(msg);
    }
}

void ChatPage::AppendChatMsg(std::shared_ptr<ChatData> msg)
{
    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    ChatRole role;
    QString name, icon;
    if (msg->_from_uid == self_info->_uid)
    {
        role = ChatRole::Self;
        name = self_info->_name;
        icon = self_info->_icon;
    }
    else
    {
        role = ChatRole::Other;
        auto friend_info = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        if (friend_info == nullptr)
        {
            return;
        }
        name = friend_info->_name;
        icon = friend_info->_icon;
    }

    CreateAndAppendChatItem(role, name, icon, msg);
}

void ChatPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::on_send_btn_clicked()
{
    if (_user_info == nullptr)
    {
        qDebug() << "friend_info is empty";
        return;
    }

    auto user_info = UserMgr::GetInstance()->GetUserInfo();
    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::Self;
    QString userName = user_info->_name;
    QString userIcon = user_info->_icon;

    const QVector<MsgInfo> &msgList = pTextEdit->getMsgList();
    QJsonObject textObj;
    QJsonArray textArray;
    int txt_size = 0;

    for (int i = 0; i < msgList.size(); ++i)
    {
        // 消息内容长度不合规就跳过
        if (msgList[i].content.length() > 1024)
        {
            continue;
        }

        QString type = msgList[i].msgFlag;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));
        QWidget *pBubble = nullptr;

        if (type == "text")
        {
            // 生成唯一id
            QUuid uuid = QUuid::createUuid();
            // 转为字符串
            QString uuidString = uuid.toString();

            pBubble = new TextBubble(role, msgList[i].content);
            if (txt_size + msgList[i].content.length() > 1024)
            {
                textObj["fromuid"] = user_info->_uid;
                textObj["touid"] = _user_info->_uid;
                textObj["text_array"] = textArray;
                QJsonDocument doc(textObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                // 发送并清空之前累计的文本列表
                txt_size = 0;
                textArray = QJsonArray();
                textObj = QJsonObject();
                // 发送tcp请求给chatserver
                emit TcpMgr::GetInstance() -> sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }

            // 将bubble和uid绑定，以后可以等网络返回消息后设置是否送达
            //_bubble_map[uuidString] = pBubble;
            txt_size += msgList[i].content.length();
            QJsonObject obj;
            QByteArray utf8Message = msgList[i].content.toUtf8();
            obj["content"] = QString::fromUtf8(utf8Message);
            obj["msgid"] = uuidString;
            textArray.append(obj);
            auto txt_msg = std::make_shared<TextChatData>(uuidString, obj["content"].toString(),
                                                          user_info->_uid, _user_info->_uid);
            emit sig_append_send_chat_msg(txt_msg);
        }
        else if (type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        }

        // 发送消息
        if (pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }

    qDebug() << "textArray is " << textArray;
    // 发送给服务器
    textObj["text_array"] = textArray;
    textObj["fromuid"] = user_info->_uid;
    textObj["touid"] = _user_info->_uid;
    QJsonDocument doc(textObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    // 发送并清空之前累计的文本列表
    txt_size = 0;
    textArray = QJsonArray();
    textObj = QJsonObject();
    // 发送tcp请求给chat server
    emit TcpMgr::GetInstance() -> sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
}

void ChatPage::on_receive_btn_clicked()
{
    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::Other;
    QString userName = _user_info->_name;
    QString userIcon = _user_info->_icon;

    const QVector<MsgInfo> &msgList = pTextEdit->getMsgList();
    for (int i = 0; i < msgList.size(); ++i)
    {
        QString type = msgList[i].msgFlag;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));
        QWidget *pBubble = nullptr;
        if (type == "text")
        {
            pBubble = new TextBubble(role, msgList[i].content);
        }
        else if (type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        }
        else if (type == "file")
        {
        }
        if (pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }
}

void ChatPage::on_file_lb_clicked()
{
    qDebug() << _user_info->_name;
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "选择要上传的文件",
        QString(),
        "所有文件 (*.*)");

    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    auto role = ChatRole::Self;

    if (!files.isEmpty())
    {
        auto file_array = QJsonArray();
        for (const QString &file : files)
        {
            QString currentDateTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
            QString file_size = FileManager::GetInstance()->formatFileSize(QFileInfo(file).size());
            QString file_name = QFileInfo(file).fileName();
            QString file_id = currentDateTime + "_" + file_name; // 日期 + 文件名

            // 创建文件气泡并发送到聊天界面
            ChatItemBase *pChatItem = new ChatItemBase(role);
            pChatItem->setUserName(self_info->_name);
            pChatItem->setUserIcon(QPixmap(self_info->_icon));
            QWidget *pBubble = new FileBubble(role, file_name, file_size);
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);

            // 添加文件上传任务
            FileManager::GetInstance()->addUploadTask(file, file_id, UserMgr::GetInstance()->GetToken(), QString::number(UserMgr::GetInstance()->GetUid()));
            qDebug() << "UploadFile start, file is : " << file
                     << "file_id is : " << file_id
                     << "token is : " << UserMgr::GetInstance()->GetToken()
                     << "uid is : " << UserMgr::GetInstance()->GetUid();
            // 测试用
            // FileManager::GetInstance()->addUploadTask(file, file_id, "5225f70b-1662-488f-a055-c11dbd856574", "3");
            // 构造文件消息

            QJsonObject obj;
            obj["file_name"] = file_name;
            obj["file_size"] = file_size;
            obj["file_id"] = file_id;
            file_array.append(obj);

            auto file_msg = std::make_shared<FileChatData>(self_info->_uid, _user_info->_uid, file_name, file_id, file_size);
            emit sig_append_send_chat_msg(file_msg);
        }

        // 发送文件消息到ChatServer
        QJsonObject file_obj;
        file_obj["fromuid"] = self_info->_uid;
        file_obj["touid"] = _user_info->_uid;
        file_obj["file_array"] = file_array;
        QJsonDocument doc(file_obj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_FILE_CHAT_MSG_REQ, jsonData);
    }
}

void ChatPage::clearItems()
{
    ui->chat_data_list->removeAllItem();
}

void ChatPage::CreateAndAppendChatItem(ChatRole role, QString name, QString icon, std::shared_ptr<ChatData> msg)
{
    ChatItemBase *pChatItem = new ChatItemBase(role);
    pChatItem->setUserName(name);
    pChatItem->setUserIcon(QPixmap(icon));
    QWidget *pBubble = nullptr;
    if (msg->_data_type == DataType::TEXT)
    {
        auto text_msg = std::dynamic_pointer_cast<TextChatData>(msg);
        pBubble = new TextBubble(role, text_msg->_msg_content);
    }
    else if (msg->_data_type == DataType::FILE)
    {
        auto file_msg = std::dynamic_pointer_cast<FileChatData>(msg);
        pBubble = new FileBubble(role, file_msg->_file_name, file_msg->_file_size);
    }
    pChatItem->setWidget(pBubble);
    ui->chat_data_list->appendChatItem(pChatItem);
}
