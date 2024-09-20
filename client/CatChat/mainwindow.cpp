#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 显示登录界面
    log_dlg = new LoginDialog(this);
    // 设置自定义 | 无边框属性，以让对话框嵌入mainwindow
    log_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(log_dlg);

    // 点击注册按钮后显示注册界面
    connect(log_dlg, &LoginDialog::switchRegister, this, [=](){
        reg_dlg = new RegisterDialog(this);
        reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
        setCentralWidget(reg_dlg);
        log_dlg->hide();
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}
