#pragma once

#include "GameConfig.h"
#include "IEncryption.h"


const int DefaultCheckOffset = 0x10;

struct ConfigItem {
	std::vector<std::wstring> games;
	uint32_t align = 0;
	const IEncryption *encryption = nullptr;
};

enum WorkMode {
	Encryption,
	Decryption,
	Pack,
	Unpack
};

int parseWorkMode(const std::vector<std::string>& arguments, WorkMode &workMode);

int parseCheckOffset(const std::vector<std::string>& arguments, uint32_t checkOffset);

int parseEncryption(const std::vector<std::string>& arguments, EncryptionType& encoder);

int parseConfig(const std::vector <std::string>& arguments,ConfigItem &configItem);

int parseUseEncrypt(const std::vector<std::string>& arguments,bool &use_encrypt);

int parseNoTemp(const std::vector<std::string>& arguments,bool &notemp);

int parseInput(const std::vector<std::string>& arguments, std::wstring& input);

int parseOutput(const std::vector<std::string>& arguments, std::wstring& output);

int parseAlign(const std::vector<std::string>& arguments, uint32_t& align);

int parseThread(const std::vector<std::string>& arguments, uint32_t &thread_count);
