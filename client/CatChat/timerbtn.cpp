#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),counter_(10)
{
    timer_ = new QTimer(this);

    connect(timer_, &QTimer::timeout, [this](){
        counter_--;
        if(counter_ <= 0){
            timer_->stop();
            counter_ = 10;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(counter_));
    });
}

TimerBtn::~TimerBtn()
{
    timer_->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // 在这里处理鼠标左键释放事件
        this->setEnabled(false);
        this->setText(QString::number(counter_));
        timer_->start(1000); // 每一秒触发一次
        emit clicked();
    }
    // 调用基类的mouseReleaseEvent以确保正常的事件处理（如点击效果）
    QPushButton::mouseReleaseEvent(e);
}
