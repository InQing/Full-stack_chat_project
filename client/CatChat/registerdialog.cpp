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
    ui->state_lable->setProperty("state","normal");
    repolish(ui->state_lable);

    // 输入框结束编辑后的错误提示，例如邮箱格式不匹配
    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });
    connect(ui->password_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });
    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });
    connect(ui->varify_edit, &QLineEdit::editingFinished, this, [this](){
        checkVarifyValid();
    });


    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                               "visible_hover","");
    ui->confirm_visible->SetState("unvisible","unvisible_hover","","visible",
                                  "visible_hover","");
    //连接点击事件
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->password_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->password_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });


    // 连接信号与槽，HTTP响应结束
    connect(HttpMgr::getInstance().get(), &HttpMgr::sig_reg_mod_finish, this, &RegisterDialog::slot_reg_mod_finish);
    initHttpHandlers();

    // 注册成功，页面跳转
    countdown_timer_ = new QTimer(this);
    connect(countdown_timer_, &QTimer::timeout, [this](){
        if(countdown_==0){
            countdown_timer_->stop();
            emit sigSwitchLogin();
            return;
        }
        countdown_--;
        auto str = QString("注册成功，%1 s后返回登录").arg(countdown_);
        ui->tip_lable->setText(str);
    });
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

// 检查邮箱是否匹配格式
void RegisterDialog::on_get_varify_btn_clicked()
{
    auto email = ui->email_edit->text();
    // 邮箱格式的正则表达式
    QRegularExpression regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    bool match = regex.match(email).hasMatch();
    if (match){
        // 发送http请求获取验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpMgr::getInstance()->postHttpReq(QUrl(gate_url_prefix + "/get_Varifycode"),
                                            json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::MOD_REGISTER);
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
    // 注册模块，获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error == ErrorCodes::ERR_VARIFY_REPEAT){
            showState("请勿重复发送，五分钟后再重试！", false);
            return;
        }
        if(error != ErrorCodes::SUCCESS){
            showState("未知错误", false);
            return;
        }

        auto email = jsonObj["email"].toString();
        showState("验证码已发送到邮箱，注意查收", true);
        qDebug() << "email is" << email;
    });

    // 注册模块，注册用户回包逻辑
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error == ErrorCodes::ERR_VARIFY_EXPIRED){
            showState(tr("验证码过期"),false);
            return;
        }
        if(error == ErrorCodes::ERR_VARIFY_CODE_ERR){
            showState(tr("验证码错误"),false);
            return;
        }
        if(error == ErrorCodes::ERR_USER_EXIT){
            showState(tr("用户名或邮箱已存在"),false);
            return;
        }
        if(error != ErrorCodes::SUCCESS){
            showState(tr("未知错误"),false);
            return;
        }
        auto uid = jsonObj["uid"].toString();
        showState(tr("用户注册成功"), true);
        qDebug()<< "uid is " << uid ;

        // 切换到登录跳转页面
        ChangeTipPage();
    });
}

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    tip_errs_[te] = tips;
    showState(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    tip_errs_.remove(te);
    if(tip_errs_.empty()){
        ui->state_lable->clear();
        return;
    }

    showState(tip_errs_.first(), false);
}


void RegisterDialog::on_confirm_btn_clicked()
{
    bool valid = checkUserValid();
    if(!valid){
        return;
    }

    valid = checkEmailValid();
    if(!valid){
        return;
    }

    valid = checkPassValid();
    if(!valid){
        return;
    }

    valid = checkConfirmValid();
    if(!valid){
        return;
    }

    valid = checkVarifyValid();
    if(!valid){
        return;
    }

    // 发送http请求注册用户
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->password_edit->text());
    json_obj["varifycode"] = ui->varify_edit->text();
    HttpMgr::getInstance()->postHttpReq(QUrl(gate_url_prefix + "/user_register"),
                                        json_obj, ReqId::ID_REG_USER, Modules::MOD_REGISTER);
}


bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}


bool RegisterDialog::checkPassValid()
{
    auto pass = ui->password_edit->text();

    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}



bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegisterDialog::checkVarifyValid()
{
    auto pass = ui->varify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->password_edit->text();
    auto confirm = ui->confirm_edit->text();

    if(confirm.length() < 6 || confirm.length() > 15 ){
        //提示长度不准确
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(confirm).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("不能包含非法字符"));
        return false;
    }

    DelTipErr(TipErr::TIP_CONFIRM_ERR);

    if(pass != confirm){
        //提示密码不匹配
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("确认密码和密码不匹配"));
        return false;
    }else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

void RegisterDialog::ChangeTipPage(){
    countdown_timer_->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    // 启动定时器，设置间隔为1000毫秒（1秒）
    countdown_timer_->start(1000);
}

void RegisterDialog::on_return_btn_clicked()
{
    countdown_timer_->stop();
    emit sigSwitchLogin();
}

void RegisterDialog::on_cancel_btn_clicked()
{
    countdown_timer_->stop();
    emit sigSwitchLogin();
}

