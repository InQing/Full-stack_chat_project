#pragma once

#include "const.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

struct SectionInfo {
	SectionInfo() {};
	~SectionInfo() {
		_section_datas.clear();
	}

	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}

	SectionInfo& operator=(const SectionInfo& src) {
		if (&src == this) {
			return *this;
		}
		this->_section_datas = src._section_datas;
		return *this;
	}

	std::string operator[](const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		return _section_datas[key];
	}

	// 存储配置文件的第二对key-val，key是port，val是8888
	std::unordered_map<std::string, std::string> _section_datas;
};

class ConfigMgr
{	
public:
	~ConfigMgr() {
		_config_map.clear();
	}

	static ConfigMgr& GetInstance() {
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}

	//ConfigMgr(const ConfigMgr &src) {
	//	_config_map = src._config_map;
	//}

	//ConfigMgr& operator=(const ConfigMgr& src) {
	//	if (this == &src) {
	//		return *this;
	//	}
	//	_config_map = src._config_map;
	//	return *this;
	//}

	ConfigMgr(const ConfigMgr&) = delete;
	ConfigMgr& operator=(const ConfigMgr&) = delete;

	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}

private:
	// 构造函数中读取配置
	ConfigMgr();

	// 存储配置文件的第一对key-val，key是[GateServer]等，val是SectionInfo
	std::unordered_map<std::string, SectionInfo> _config_map;
};

