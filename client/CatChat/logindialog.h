#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void switchRegister();
    void switchReset();
    void sig_connect_tcp(ServerInfo si);
private slots:
    void on_login_btn_clicked();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_tcp_con_finish(bool is_success);
    void slot_login_failed(int err);

private:
    Ui::LoginDialog *ui;

    // 初始化登录界面头像
    void InitLoginPicture();
    // 错误检测
    void showTip(QString str,bool b_ok);
    bool checkUserValid();
    bool checkPwdValid();
    void AddTipErr(TipErr te,QString tips);
    void DelTipErr(TipErr te);

    // 初始化收到HTTP回包的回调逻辑
    void initHttpHandlers();
    // 点击登录后置按钮无效/有效，防止重复登录
    bool enableBtn(bool enabled);

    QMap<TipErr, QString> _tip_errs;

    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    int _uid;
    QString _token;
};

#endif // LOGINDIALOG_H
