#include "MysqlDAO.h"
#include "ConfigMgr.h"
#include "Logger.h"

MysqlDAO::MysqlDAO()
{
	auto& cfg = ConfigMgr::GetInstance();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MysqlPool(host + ":" + port, user, pwd, schema, std::thread::hardware_concurrency()));
}

MysqlDAO::~MysqlDAO()
{
    pool_->Close();
}

int MysqlDAO::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = pool_->GetConnection();
    try {
        if (con == nullptr) {
            pool_->ReturnConnection(std::move(con));
            return -1;
        }
        // 准备调用存储过程
        std::unique_ptr<sql::PreparedStatement> stmt(con->con_->prepareStatement("CALL reg_user(?,?,?,@result)"));
        // 设置输入参数
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        // 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

          // 执行存储过程
        stmt->execute();
        // 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
       // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
        std::unique_ptr<sql::Statement> stmtResult(con->con_->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next()) {
            int result = res->getInt("result");
            LOGI("MysqlDAO: Register user succeed! UID = %d", result);
            pool_->ReturnConnection(std::move(con));
            return result;
        }
        pool_->ReturnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException& e) {
        pool_->ReturnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool MysqlDAO::CheckEmail(const std::string& name, const std::string& email) {
    auto con = pool_->GetConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        // 准备查询语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT email FROM user WHERE name = ?"));

        // 绑定参数
        pstmt->setString(1, name);

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // 遍历结果集
        while (res->next()) {
            LOGI("MysqlDAO: Check Email: %s", res->getString("email"));
            if (email != res->getString("email")) {
                pool_->ReturnConnection(std::move(con));
                return false;
            }
            pool_->ReturnConnection(std::move(con));
            return true;
        }
    }
    catch (sql::SQLException& e) {
        pool_->ReturnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::UpdatePwd(const std::string& name, const std::string& new_pwd) {
    auto con = pool_->GetConnection();
    try {
        if (con == nullptr) {
            return false;
        }

        // 准备查询语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

        // 绑定参数
        pstmt->setString(2, name);
        pstmt->setString(1, new_pwd);

        // 执行更新
        int updateCount = pstmt->executeUpdate();

        pool_->ReturnConnection(std::move(con));
        return true;
    }
    catch (sql::SQLException& e) {
        pool_->ReturnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });

    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT * FROM user WHERE email = ?"));
        pstmt->setString(1, email);

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        // 遍历结果集
        while (res->next()) {
            origin_pwd = res->getString("pwd");
            // 输出查询到的密码
            LOGI("MysqlDAO: Password: %s", origin_pwd);
            break;
        }

        if (pwd != origin_pwd) {
            return false;
        }
        userInfo.name = res->getString("name");
        userInfo.email = email;
        userInfo.uid = res->getInt("uid");
        userInfo.pwd = origin_pwd;
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDAO::GetUserInfo(int uid)
{
    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });

    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        // 遍历结果集
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd = res->getString("pwd");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->desc = res->getString("desc");
            user_ptr->nick = res->getString("nick");
            user_ptr->icon = res->getString("icon");
            user_ptr->sex = res->getInt("sex");
            user_ptr->uid = uid;
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

bool MysqlDAO::AddFriendApply(const int& from, const int& to) {
    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
            "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid "));
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        //执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            return false;
        }

        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MysqlDAO::AuthFriendApply(const int& from, const int& to) {
    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });

    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("UPDATE friend_apply SET status = 1 "
            "WHERE from_uid = ? AND to_uid = ?"));
        // 反过来的申请时from，验证时to
        pstmt->setInt(1, to); // from id
        pstmt->setInt(2, from);
        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            return false;
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MysqlDAO::AddFriend(const int& from, const int& to, std::string back_name) {
    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });

    try {

        //开始事务
        con->con_->setAutoCommit(false);

        // 准备第一个SQL语句, 插入认证方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
            "VALUES (?, ?, ?) "
        ));
        // 反过来的申请时from，验证时to
        pstmt->setInt(1, from); // from id
        pstmt->setInt(2, to);
        pstmt->setString(3, back_name);
        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        // 如果插入失败，则回滚事务
        if (rowAffected < 0) {
            con->con_->rollback();
            return false;
        }

        //准备第二个SQL语句，插入申请方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt2(con->con_->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
            "VALUES (?, ?, ?) "
        ));
        //反过来的申请时from，验证时to
        pstmt2->setInt(1, to); // from id
        pstmt2->setInt(2, from);
        pstmt2->setString(3, "");
        // 执行更新
        int rowAffected2 = pstmt2->executeUpdate();
        // 如果插入失败，则回滚事务
        if (rowAffected2 < 0) {
            con->con_->rollback();
            return false;
        }

        // 提交事务
        con->con_->commit();
        LOGI("MysqlDAO: addfriend insert friends success");

        return true;
    }
    catch (sql::SQLException& e) {
        // 如果发生错误，回滚事务
        if (con) {
            con->con_->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MysqlDAO::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });


    try {
        // 准备SQL语句, 根据起始id和限制条数返回列表
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("select apply.from_uid, apply.status, user.name, "
            "user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
            "and apply.id > ? order by apply.id ASC LIMIT ? "));

        pstmt->setInt(1, touid); // 将uid替换为你要查询的uid
        pstmt->setInt(2, begin); // 起始id
        pstmt->setInt(3, limit); //偏移量
        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
            auto name = res->getString("name");
            auto uid = res->getInt("from_uid");
            auto status = res->getInt("status");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            applyList.push_back(apply_ptr);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info_list) {

    auto con = pool_->GetConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->ReturnConnection(std::move(con));
        });

    try {
        // 准备SQL语句, 根据起始id和限制条数返回列表
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("select * from friend where self_id = ? "));

        pstmt->setInt(1, self_id); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
            auto friend_id = res->getInt("friend_id");
            auto back = res->getString("back");
            //再一次查询friend_id对应的信息
            auto user_info = GetUserInfo(friend_id);
            if (user_info == nullptr) {
                continue;
            }

            user_info->back = user_info->name;
            user_info_list.push_back(user_info);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}