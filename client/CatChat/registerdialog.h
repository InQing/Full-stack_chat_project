#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
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
    void on_pushButton_clicked();
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

private:
    Ui::RegisterDialog *ui;

    void showState(QString str, bool flag);
    // 初始化_handlers
    void initHttpHandlers();

    // 根据RedId，判断消息类型，调用相应的回调（注册/发送验证码)
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
};

#endif // REGISTERDIALOG_H
