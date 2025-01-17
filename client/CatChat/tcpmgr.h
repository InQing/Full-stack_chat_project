#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTcpSocket>
#include <QJsonArray>
#include <functional>
#include "Singleton.h"
#include "global.h"
#include "userdata.h"

class TcpMgr : public QObject, public Singleton<TcpMgr>,
               public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    friend class Singleton<TcpMgr>;
    TcpMgr();
private:
    void initHandlers();
    void handleMsg(ReqId id, int len, QByteArray data);

    QTcpSocket socket_;
    QString host_;
    uint16_t port_;
    QByteArray buffer_;
    bool is_recv_pending_; // 指示是否有尚未接收完的数据
    quint16 message_id_; // 发送数据的模块ID
    quint16 message_len_;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> handlers_;

public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QByteArray data);
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_swich_chatdlg();
    void sig_load_apply_list(QJsonArray json_array);
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
};

#endif // TCPMGR_H
