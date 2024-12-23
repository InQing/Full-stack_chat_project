#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTcpSocket>
#include "Singleton.h"
#include "global.h"

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
    void slot_send_data(ReqId reqId, QString data);
signals:
    void sig_con_success(bool is_success);
    void sig_send_data(ReqId reqId, QString data);
    void sig_login_failed(int err);
    void sig_swich_chatdlg();
};

#endif // TCPMGR_H
