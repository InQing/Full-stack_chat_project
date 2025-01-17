#pragma once
#include <string>
#include <memory>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/prepared_statement.h>
#include "const.h"
#include "logger.h"

class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :con_(con), last_oper_time_(lasttime) {}
	std::unique_ptr<sql::Connection> con_;
	int64_t last_oper_time_;
};

class MysqlPool
{
public:
	MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int pool_size) :
		url_(url), user_(user), pass_(pass), schema_(schema), pool_size_(pool_size), is_stop_(false) {
		try {
			for (std::size_t i = 0; i < pool_size_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* con = driver->connect(url_, user_, pass_);
				con->setSchema(schema_);

				auto current_time = std::chrono::system_clock::now().time_since_epoch();
				long long time_stamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
				pool_.push(std::make_unique<SqlConnection>(con, time_stamp));
			}

			// 启动检测线程，定时发送心跳，防止mysql连接断开
			check_thread_ = std::thread([this]() {
				while (!is_stop_) {
					CheckConnection();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
				});
			// 设置线程分离
			check_thread_.detach();
		}
		catch (sql::SQLException& exp) {
			LOGE("MysqlPool: mysql pool init failed");
		}
	}

	std::unique_ptr<SqlConnection> GetConnection()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this]() {
			return is_stop_ || !pool_.empty();
			});
		if (is_stop_) {
			return nullptr;
		}
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void ReturnConnection(std::unique_ptr<SqlConnection> con)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (is_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one();
	}

	void CheckConnection()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		int pool_size = pool_.size();
		// 获取当前时间戳
		auto current_time = std::chrono::system_clock::now().time_since_epoch();
		// 将时间戳转换成秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
		for (std::size_t i = 0; i < pool_size; ++i) {
			auto con = std::move(pool_.front());
			pool_.pop();
			// Defer操作，每个分支结束后，都会自动执行pool_.push(std::move(con))操作
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
				});
			if (timestamp - con->last_oper_time_ < 60) {
				continue;
			}

			try {
				std::unique_ptr<sql::Statement> stmt(con->con_->createStatement());
				stmt->executeQuery("SELECT 1");
				con->last_oper_time_ = timestamp;
			}
			catch (sql::SQLException& exp) {
				// 重新创建新连接并替换旧连接
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
				auto* new_con = driver->connect(url_, user_, pass_);
				con->con_.reset(new_con);
				con->last_oper_time_ = timestamp;
			}

		}
	}

	void Close()
	{
		is_stop_ = true;
		cond_.notify_all();
	}

	~MysqlPool()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}
private:
	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int pool_size_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> is_stop_;
	std::thread check_thread_;
};



class MysqlDAO
{
public:
	MysqlDAO();
	~MysqlDAO();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& new_pwd);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::shared_ptr<UserInfo> GetUser(int uid);
private:
	std::unique_ptr<MysqlPool> pool_;
};

