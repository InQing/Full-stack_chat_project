#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    // 设置login_pictrue
    QPixmap pixmap(":/pictrues/cat1.jpg");
    ui->login_pictrue->setPixmap(pixmap);
    ui->login_pictrue->setScaledContents(true);

    // 发送打开注册界面的信号
    connect(ui->register_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);

    // 修改密码框样式
    ui->password_edit->setEchoMode(QLineEdit::Password);

}

LoginDialog::~LoginDialog()
{
    delete ui;
}
