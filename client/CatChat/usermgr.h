#ifndef USERMGR_H
#define USERMGR_H
#include <QObject>
#include <memory>
#include <singleton.h>

class UserMgr:public QObject,public Singleton<UserMgr>,
                public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT
public:
    friend class Singleton<UserMgr>;
    ~ UserMgr();
    void SetName(QString name);
    void SetUid(int uid);
    void SetToken(QString token);
    QString GetName(){return name_;}
    QString GetToken(){return token_;}
    int GetUid(){return uid_;}

private:
    UserMgr();
    QString name_;
    QString token_;
    int uid_;
};

#endif // USERMGR_H
