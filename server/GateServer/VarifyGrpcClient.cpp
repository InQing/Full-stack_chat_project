#include "VarifyGrpcClient.h"
#include "ConfigMgr.h"
#include "const.h"
#include "Logger.h"

GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
	ClientContext context;
	GetVarifyReq request;
	GetVarifyRsp response;
	request.set_email(email);

	LOGI("VarifyGrpcClient: Send verification service!");

	auto stub = con_pool_->GetConnection();
	Status status = stub->GetVarifyCode(&context, request, &response);

	if (status.ok()) {
		LOGI("VarifyGrpcClient: verificy succeed!");
		con_pool_->ReturnConnection(std::move(stub));
		return response;
	}
	else {
		LOGW("VarifyGrpcClient: verificy failed!");
		con_pool_->ReturnConnection(std::move(stub));
		response.set_error(ErrorCodes::ERR_RPC);
		return response;
	}
}

VarifyGrpcClient::VarifyGrpcClient() {
	auto& gCfgMgr = ConfigMgr::GetInstance();
	std::string host = gCfgMgr["VarifyServer"]["Host"];
	std::string port = gCfgMgr["VarifyServer"]["Port"];
	con_pool_.reset(new VarifyConPool(std::thread::hardware_concurrency(), host, port));
	LOGI("VarifyGrpcClient: Grpc stubs created£¬ VerifyServer host is %s, port is %s", host.c_str(), port.c_str());
}
