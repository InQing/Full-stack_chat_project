#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载qss样式
    QFile qss(":/style/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)){
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }

    // 解析config.ini中的配置
    // 获取网关服务器的url前缀

    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                                                   QDir::separator() + fileName);
    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/Host").toString();
    QString gate_port = settings.value("GateServer/Port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;
    qDebug() << "gate_url_prefix:" << gate_url_prefix;

    QString resource_host = settings.value("ResourceServer/Host").toString();
    QString resource_port = settings.value("ResourceServer/Port").toString();
    resource_url_prefix = "http://" + resource_host + ":" + resource_port;
    qDebug() << "resource_url_prefix:" << resource_url_prefix;

    MainWindow w;

    w.setWindowIcon(QIcon(":/res/cat_1.ico"));

    w.show();
    return a.exec();
}
