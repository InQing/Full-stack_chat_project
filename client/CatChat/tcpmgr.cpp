#include <QJsonObject>
#include <QJsonDocument>
#include "tcpmgr.h"
#include "usermgr.h"

TcpMgr::TcpMgr() : host_(""), port_(0), is_recv_pending_(false), message_id_(0), message_len_(0)
{
    connect(&socket_, &QTcpSocket::connected, [&]()
            {
        qDebug() << "Connected to server!";
            // 连接建立后发送消息
        emit sig_con_success(true); });

    connect(&socket_, &QTcpSocket::readyRead, [&]()
            {
        // 当有数据可读时，读取所有数据并追加到缓冲区
        buffer_.append(socket_.readAll());

        QDataStream stream(&buffer_, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_0);

        forever {
            // 没有未接受完的数据，则此次接受的数据从头部开始
            if(!is_recv_pending_){
                // 解析头部，检查缓冲区中的数据是否足够解析出一个消息头（消息ID + 消息长度)
                if(buffer_.size() < static_cast<int>(sizeof(quint16) * 2)){
                    return; // 数据不够，等待更多数据
                }

                // 预读取消息ID和消息长度，但不从缓冲区中移除
                stream >> message_id_ >> message_len_;
                // 移除id与len
                buffer_ = buffer_.mid(sizeof(quint16) * 2);

                qDebug() << "Message ID:" << message_id_ << ", Length:" << message_len_;
            }

            // 接受消息体
            if(buffer_.size() < message_len_){
                is_recv_pending_ = true;
                return; // 继续等待数据
            }

            is_recv_pending_ = false;
            // 读取消息体
            QByteArray messageBody = buffer_.mid(0, message_len_);
            qDebug() << "receive body msg is " << messageBody ;

            buffer_ = buffer_.mid(message_len_);
            handleMsg(ReqId(message_id_),message_len_, messageBody);
        } });

    // 处理错误
    connect(&socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError)
            {
          Q_UNUSED(socketError)
          qDebug() << "Error:" << socket_.errorString(); });
    // 处理连接断开
    connect(&socket_, &QTcpSocket::disconnected, [&]()
            { qDebug() << "Disconnected from server"; });
    // 连接发送信号用来发送数据
    connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
    // 注册消息
    initHandlers();
}

void TcpMgr::initHandlers()
{
    handlers_.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug()<< "handle ID_CHAT_LOGIN_RSP" ;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if(jsonDoc.isNull()){
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err ;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err);
            return;
        }

        // 登录成功
        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto token = jsonObj["token"].toString();

        // 加载用户信息
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex);
        UserMgr::GetInstance()->SetUserInfo(user_info);

        // 加载好友申请列表与好友列表
        UserMgr::GetInstance()->SetToken(token);
        if(jsonObj.contains("apply_list")){
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }
        if (jsonObj.contains("friend_list")) {
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }

        qDebug() << "登录成功";
        qDebug() << "uid: " << uid;
        qDebug() << "Name: " << name;
        qDebug() << "Token: " << token;

        emit sig_swich_chatdlg(); });

    handlers_.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_SEARCH_USER_RSP";
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;

            emit sig_user_search(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Login Failed, err is " << err;
            emit sig_user_search(nullptr);
            return;
        }
        auto search_info =  std::make_shared<SearchInfo>(jsonObj["uid"].toInt(), jsonObj["name"].toString(),
                                                        jsonObj["nick"].toString(), jsonObj["desc"].toString(),
                                                        jsonObj["sex"].toInt(), jsonObj["icon"].toString());

        emit sig_user_search(search_info); });

    handlers_.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_ADD_FRIEND_RSP ";
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Add Friend Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Add Friend Failed, err is " << err;
            return;
        }

        qDebug() << "Add Friend Success " ; });

    handlers_.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_NOTIFY_ADD_FRIEND_REQ ";
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;

            emit sig_user_search(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Login Failed, err is " << err;
            emit sig_user_search(nullptr);
            return;
        }

        int from_uid = jsonObj["applyuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString desc = jsonObj["desc"].toString();
        QString icon = jsonObj["icon"].toString();
        QString nick = jsonObj["nick"].toString();
        int sex = jsonObj["sex"].toInt();

        auto apply_info = std::make_shared<AddFriendApply>(
            from_uid, name, desc,
            icon, nick, sex);

        emit sig_friend_apply(apply_info); });

    handlers_.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_NOTIFY_AUTH_FRIEND_REQ ";
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString nick = jsonObj["nick"].toString();
        QString icon = jsonObj["icon"].toString();
        int sex = jsonObj["sex"].toInt();

        auto auth_info = std::make_shared<AuthInfo>(from_uid,name,
                                                    nick, icon, sex);

        emit sig_add_auth_friend(auth_info); });

    handlers_.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_AUTH_FRIEND_RSP ";
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
        emit sig_auth_rsp(rsp);

        qDebug() << "Auth Friend Success " ; });

    handlers_.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data)
                     {
                         Q_UNUSED(len);
                         qDebug() << "handle ID_TEXT_CHAT_MSG_RSP ";
                         // 将QByteArray转换为QJsonDocument
                         QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

                         // 检查转换是否成功
                         if (jsonDoc.isNull())
                         {
                             qDebug() << "Failed to create QJsonDocument.";
                             return;
                         }

                         QJsonObject jsonObj = jsonDoc.object();

                         if (!jsonObj.contains("error"))
                         {
                             int err = ErrorCodes::ERR_JSON;
                             qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
                             return;
                         }

                         int err = jsonObj["error"].toInt();
                         if (err != ErrorCodes::SUCCESS)
                         {
                             qDebug() << "Chat Msg Rsp Failed, err is " << err;
                             return;
                         }

                         qDebug() << "Receive Text Chat Rsp Success ";
                         // ui设置红点 todo...
                     });

    handlers_.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_NOTIFY_TEXT_CHAT_MSG_REQ ";
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Notify Success " ;
        // 构造文本类型消息
        auto chat_msg = std::make_shared<ChatMsg>(jsonObj["fromuid"].toInt(),
                                                     jsonObj["touid"].toInt());
        for(auto msg_data : jsonObj["text_array"].toArray()){
            auto msg_obj = msg_data.toObject();
            auto content = msg_obj["content"].toString();
            auto msgid = msg_obj["msgid"].toString();
            auto msg_ptr = std::make_shared<TextChatData>(msgid, content, jsonObj["fromuid"].toInt(), jsonObj["touid"].toInt());    
            chat_msg->AppendChatMsgs(msg_ptr);
        }
        emit sig_chat_msg(chat_msg); });

    handlers_.insert(ID_FILE_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_FILE_CHAT_MSG_RSP ";
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull())
        {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error"))
        {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS)
        {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }

        qDebug() << "Receive File Chat Rsp Success ";
        // ui设置红点 todo...
        });

    handlers_.insert(ID_NOTIFY_FILE_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data)
                     {
        Q_UNUSED(len);
        qDebug() << "handle ID_NOTIFY_FILE_CHAT_MSG_REQ ";
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive File Chat Notify Success " ;
        // 构造文件类型消息
        auto chat_msg = std::make_shared<ChatMsg>(jsonObj["fromuid"].toInt(),
                                                 jsonObj["touid"].toInt());
        for(auto msg_data : jsonObj["file_array"].toArray()){
            auto msg_obj = msg_data.toObject();
            auto file_name = msg_obj["file_name"].toString();
            auto file_id = msg_obj["file_id"].toString();
            auto file_size = msg_obj["file_size"].toString();
            auto msg_ptr = std::make_shared<FileChatData>(jsonObj["fromuid"].toInt(),
                                                     jsonObj["touid"].toInt(), file_name, file_id, file_size);
            chat_msg->AppendChatMsgs(msg_ptr);
        }

        emit sig_chat_msg(chat_msg); });
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = handlers_.find(id);
    if (find_iter == handlers_.end())
    {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "receive tcp connect signal";
    // 尝试连接到服务器
    qDebug() << "Connecting to server...";
    host_ = si.Host;
    port_ = static_cast<uint16_t>(si.Port.toUInt());
    socket_.connectToHost(host_, port_);
}

// 因为客户端发送数据可能在任何线程，为了保证线程安全
// 故用信号与槽的机制来保证发送包的顺序（信号与槽会自动根据触发顺序按队列发送）
void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;

    // 计算长度（使用网络字节序转换）
    quint16 len = static_cast<quint16>(dataBytes.size());

    // 创建一个QByteArray用于存储要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);

    // 写入ID和长度
    out << id << len;

    // 添加字符串数据
    block.append(dataBytes);

    // 发送数据
    socket_.write(block);
    qDebug() << "tcp mgr send byte data is " << block;
}
