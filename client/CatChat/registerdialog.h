#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QTimer>
#include "global.h"

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    void on_get_varify_btn_clicked();
    void on_confirm_btn_clicked();
    void on_return_btn_clicked();
    void on_cancel_btn_clicked();

    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

private:
    Ui::RegisterDialog *ui;

    void showState(QString str, bool flag);
    // 初始化_handlers
    void initHttpHandlers();
    // 添加输入框错误
    void AddTipErr(TipErr te, QString tips);
    // 删除输入框错误
    void DelTipErr(TipErr te);
    // 检测输入框错误
    bool checkUserValid();
    bool checkPassValid();
    bool checkEmailValid();
    bool checkVarifyValid();
    bool checkConfirmValid();

    // 根据RedId，判断消息类型，调用相应的回调（注册/发送验证码)
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    // 切换到登录跳转页面
    void ChangeTipPage();

    // 缓存各个输入框输入完成后提示的错误，如果该输入框错误清除后就显示剩余的错误，每次只显示一条
    QMap<TipErr, QString> tip_errs_;

    QTimer* countdown_timer_;
    int countdown_;

signals:
    void sigSwitchLogin();
};

#endif // REGISTERDIALOG_H
