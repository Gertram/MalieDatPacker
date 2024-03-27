#pragma once

#include "CamelliaConfigItem.h"


const int DefaultCheckOffset = 0x10;


enum EncryptionType {
	Camellia128
};

enum WorkMode {
	Encryption,
	Pack
};

WorkMode parseWorkMode(const std::vector<std::string>& arguments);

int parseCheckOffset(const std::vector<std::string>& arguments);

bool parseEncryption(const std::vector<std::string>& arguments, EncryptionType& encoder);

typedef std::map<std::wstring, CamelliaConfigItem*> CamelliaConfig;

const CamelliaConfigItem* initConfigByExpectHeader(CamelliaConfig config, const std::vector<uint8_t>& expect_header, int checkOffset);

const CamelliaConfigItem* initConfigByExpectHeader(CamelliaConfig config, const std::vector<std::string>& arguments);

const CamelliaConfigItem* initConfigByDatHeader(CamelliaConfig config, const std::vector<std::string>& arguments);

const CamelliaConfigItem* initConfigByInternalKeyFileName(CamelliaConfig config, const std::vector<std::string>& arguments);

const CamelliaConfigItem* initConfigByExternalKeyFileName(CamelliaConfig config, const std::vector<std::string>& arguments);

const CamelliaConfigItem* initConfig(const std::vector <std::string>& arguments);

bool parseUseEncrypt(const std::vector<std::string>& arguments);

bool parseInput(const std::vector<std::string>& arguments, std::wstring& input);

bool parseOutput(const std::vector<std::string>& arguments, std::wstring& output);

bool parseAlign(const std::vector<std::string>& arguments, uint32_t& align);

int parseThread(const std::vector<std::string>& arguments);
