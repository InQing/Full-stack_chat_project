#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "httpmgr.h"

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);

    // 修改密码框样式
    ui->password_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);

    // 设置state_lable为正常登录状态
    ui->state_lable->setProperty("state", "normal");
    repolish(ui->state_lable);

    // 连接信号与槽，HTTP响应结束
    connect(HttpMgr::getInstance().get(), &HttpMgr::sig_reg_mod_finish, this, &RegisterDialog::slot_reg_mod_finish);

    initHttpHandlers();
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

// 检查邮箱是否匹配格式
void RegisterDialog::on_pushButton_clicked()
{
    auto email = ui->email_edit->text();
    // 邮箱格式的正则表达式
    QRegularExpression regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    bool match = regex.match(email).hasMatch();
    if (match){
        // 发送http请求获取验证码
    }
    else{
        // 提示邮箱不正确
        showState("邮箱格式不正确", false);
    }


}

// HTTP响应结束，注册界面作出反应
void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showState("网络请求错误", false);
        return;
    }

    // 将res转化为json格式
    // 解析 JSON 字符串,res需先转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());

    // json解析错误
    if(jsonDoc.isNull()){
        showState("json解析错误", false);
        return;
    }
    if(!jsonDoc.isObject()){
        showState("json解析错误", false);
        return;
    }

    // json解析成功
    QJsonObject jsonObj = jsonDoc.object();

    // 根据ReqId，调用相应的回调函数
    _handlers[id](jsonObj);

    return;

}

// 修改state_lable显示的状态
void RegisterDialog::showState(QString str, bool flag)
{
    if(flag){
        ui->state_lable->setProperty("state", "normal");
    }
    else{
        ui->state_lable->setProperty("state", "error");
    }

    ui->state_lable->setText(str);
    repolish(ui->state_lable);
}

void RegisterDialog::initHttpHandlers()
{
    // 注册获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showState("参数错误", false);
            return;
        }

        auto email = jsonObj["email"].toString();
        showState("验证码已发送到邮箱，注意查收", true);
        qDebug() << "email is" << email;
    });
}

