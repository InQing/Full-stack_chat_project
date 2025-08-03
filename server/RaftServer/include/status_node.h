#ifndef STATUS_NODE_H
#define STATUS_NODE_H

#include <braft/raft.h>
#include <braft/storage.h>
#include <braft/util.h>
#include <brpc/server.h>
#include <butil/status.h>
#include "status_server.h"

namespace status_server {

// Raft状态机实现
class StatusNode : public braft::StateMachine {
public:
    StatusNode(StatusServer* status_server);
    ~StatusNode() = default;

    // 初始化节点
    bool init(const std::string& group_id, const std::string& conf);

    // 关闭节点
    void shutdown();

    // 处理来自客户端的写请求（心跳信息）
    void handle_client_write_request(const std::string& server_type,
                                   const std::string& server_ip,
                                   int server_port,
                                   const ServerHealth& health,
                                   braft::Closure* done);

    // 实现StateMachine接口
    void on_apply(braft::Iterator& iter) override;
    void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;
    int on_snapshot_load(braft::SnapshotReader* reader) override;
    void on_leader_start(int64_t term) override;
    void on_leader_stop(const butil::Status& status) override;
    void on_error(const ::braft::Error& e) override;
    void on_configuration_committed(const ::braft::Configuration& conf) override;
    void on_stop_following(const ::braft::LeaderChangeContext& ctx) override;
    void on_start_following(const ::braft::LeaderChangeContext& ctx) override;

private:
    StatusServer* _status_server;
    scoped_refptr<braft::NodeImpl> _node;
    butil::EndPoint _addr;  // 服务器地址
    std::string _group_id;  // Raft组ID
};

} // namespace status_server

#endif // STATUS_NODE_H