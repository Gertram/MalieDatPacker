#pragma once

#include "GameConfig.h"
#include "IEncryption.h"


const int DefaultCheckOffset = 0x10;

struct ConfigItem {
	std::vector<std::wstring> games;
	uint32_t align;
	const IEncryption *encryption;
};

enum WorkMode {
	Encryption,
	Decryption,
	Pack,
	Unpack
};

WorkMode parseWorkMode(const std::vector<std::string>& arguments);

int parseCheckOffset(const std::vector<std::string>& arguments);

bool parseEncryption(const std::vector<std::string>& arguments, EncryptionType& encoder);

bool parseConfig(const std::vector <std::string>& arguments,ConfigItem &configItem);

bool parseUseEncrypt(const std::vector<std::string>& arguments);

bool parseInput(const std::vector<std::string>& arguments, std::wstring& input);

bool parseOutput(const std::vector<std::string>& arguments, std::wstring& output);

bool parseAlign(const std::vector<std::string>& arguments, uint32_t& align);

int parseThread(const std::vector<std::string>& arguments);
