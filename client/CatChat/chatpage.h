#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include "userdata.h"
#include "ChatItemBase.h"
#include <QMap>

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendChatMsg(std::shared_ptr<ChatData> msg);
protected:
    void paintEvent(QPaintEvent *event);
private:
    void clearItems();
    void CreateAndAppendChatItem(ChatRole role, QString name, QString icon, std::shared_ptr<ChatData> msg);
    Ui::ChatPage *ui;
    std::shared_ptr<UserInfo> _user_info;
    QMap<QString, QWidget*>  _bubble_map;
signals:
    void sig_append_send_chat_msg(std::shared_ptr<ChatData> msg);
private slots:
    void on_send_btn_clicked();
    void on_receive_btn_clicked();
    void on_file_lb_clicked();

};

#endif // CHATPAGE_H
