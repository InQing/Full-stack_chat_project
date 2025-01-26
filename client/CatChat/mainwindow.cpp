#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"
#include "usermgr.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 显示登录界面
    log_dlg_ = new LoginDialog(this);
    // 设置自定义 | 无边框属性，以让对话框嵌入mainwindow
    log_dlg_->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(log_dlg_);

    // 跳转注册页面
    connect(log_dlg_, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    // 跳转忘记密码页面
    connect(log_dlg_, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //跳转聊天界面
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);

    // 测试用，模拟数据
    // auto user_info = std::make_shared<UserInfo>(1, "black_cat", "猫", ":/res/cat_1.ico", 1);
    // UserMgr::GetInstance()->SetUserInfo(user_info);
    // emit TcpMgr::GetInstance()->sig_swich_chatdlg();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    reg_dlg_ = new RegisterDialog(this);
    reg_dlg_->hide();

    reg_dlg_->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    //连接注册界面返回登录信号
    connect(reg_dlg_, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(reg_dlg_);
    log_dlg_->hide();
    reg_dlg_->show();
}

//从注册界面返回登录界面
void MainWindow::SlotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    log_dlg_ = new LoginDialog(this);
    log_dlg_->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(log_dlg_);

    reg_dlg_->hide();
    log_dlg_->show();
    //连接登录界面注册信号
    connect(log_dlg_, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(log_dlg_, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    reset_dlg_ = new ResetDialog(this);
    reset_dlg_->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(reset_dlg_);

    log_dlg_->hide();
    reset_dlg_->show();
    // 连接返回登录界面
    connect(reset_dlg_, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

//从重置界面返回登录界面
void MainWindow::SlotSwitchLogin2()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    log_dlg_ = new LoginDialog(this);
    log_dlg_->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(log_dlg_);

    reset_dlg_->hide();
    log_dlg_->show();
    //连接登录界面忘记密码信号
    connect(log_dlg_, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(log_dlg_, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    chat_dlg_ = new ChatDialog();
    chat_dlg_->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(chat_dlg_);
    chat_dlg_->show();
    log_dlg_->hide();
    this->setMinimumSize(QSize(1050,900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}
