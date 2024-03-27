#include <iostream>
#include <filesystem>
#include <fstream>
#include <exception>
#include <charconv>
#include <map>

#include "cmd.h"
#include "utils.h"
#include "key.h"
#include "encryption.h"

WorkMode parseWorkMode(const std::vector<std::string>& arguments)
{
	const auto modePos = findPosition<std::string>(arguments, "-mode");
	if (modePos == -1) {
		return WorkMode::Pack;
	}
	if (modePos + 1 == arguments.size()) {
		return WorkMode::Pack;
	}
	const auto offsetStr = arguments[modePos + 1];
	if (offsetStr == "encrypt") {
		return WorkMode::Encryption;
	}
	return WorkMode::Pack;
}

int parseCheckOffset(const std::vector<std::string>& arguments) {
	const auto offsetPos = findPosition<std::string>(arguments, "-offset");
	if (offsetPos == -1) {
		return DefaultCheckOffset;
	}
	if (offsetPos + 1 == arguments.size()) {
		return DefaultCheckOffset;
	}
	const auto offsetStr = arguments[offsetPos + 1];
	int offset;
	std::from_chars(offsetStr.data(), offsetStr.data() + offsetStr.size(), offset, 16);
	return offset;
}

bool parseEncryption(const std::vector<std::string>& arguments, EncryptionType& encoder) {
	const auto encoder_pos = findPosition<std::string>(arguments, "-encrpytion");
	if (encoder_pos == -1) {
		encoder = EncryptionType::Camellia128;
		return true;
	}
	if (encoder_pos + 1 == arguments.size()) {
		std::wcout << L"Encryption expected but end of line detected" << std::endl;
		return false;
	}
	const auto encoderStr = arguments[encoder_pos + 1];
	if (encoderStr == "Camellia" || encoderStr == "Camellia128") {
		return EncryptionType::Camellia128;
	}

	std::cout << "Unknown encryption: " << encoderStr << std::endl;

	return false;
}

const CamelliaConfigItem* initConfigByExpectHeader(CamelliaConfig config, const std::vector<uint8_t>& expect_header, int checkOffset) {

	if (expect_header.size() == 0) {
		return nullptr;
	}


	printf("Try to find expect...\n");

	uint8_t bs[16];

	for (const auto configItem : config)
	{
		memset(bs, 0, sizeof(bs));

		encrypt(configItem.second->getKey(), bs, sizeof(bs), checkOffset, false);

		if (memcmp(bs, expect_header.data(), expect_header.size()) == 0) {
			std::wcout << L"Find expect config : " << configItem.first << std::endl;
			return configItem.second;
		}
	}
	printf("Cannot found expect ExpectHeader.\n");
	return nullptr;
}

const CamelliaConfigItem* initConfigByExpectHeader(CamelliaConfig config, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-expect");

	if (expectPos == -1) {
		return nullptr;
	}

	if (expectPos + 1 == arguments.size()) {
		return nullptr;
	}

	const auto& expectStr = arguments[expectPos + 1];

	const auto strs = splitString(expectStr, " ");

	std::vector<uint8_t> expect_header;
	for (const auto& hexStr : strs) {
		int value;
		std::from_chars(hexStr.data(), hexStr.data() + hexStr.size(), value, 16);
		expect_header.push_back(value);
	}


	const auto checkOffset = parseCheckOffset(arguments);

	return initConfigByExpectHeader(config, expect_header, checkOffset);
}


const CamelliaConfigItem* initConfigByDatHeader(CamelliaConfig config, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-dat");

	if (expectPos == -1) {
		return nullptr;
	}

	if (expectPos + 1 == arguments.size()) {
		return nullptr;
	}

	const auto& datpath = arguments[expectPos + 1];

	FILE* file;
	const auto result = fopen_s(&file, datpath.c_str(), "rb");
	if (result != 0 || file == nullptr) {
		return nullptr;
	}

	uint8_t buffer[8];


	const auto checkOffset = parseCheckOffset(arguments);

	fseek(file, checkOffset, SEEK_SET);

	const auto count = fread(buffer, sizeof(uint8_t), sizeof(buffer), file);

	std::vector<uint8_t> expect_header;
	for (auto i = 0; i < count; i++) {
		expect_header.push_back(buffer[i]);
	}

	fclose(file);

	return initConfigByExpectHeader(config, expect_header, checkOffset);
}

const CamelliaConfigItem* initConfigByGame(CamelliaConfig config, const std::vector<std::string>& arguments)
{
	const auto gamePos = findPosition<std::string>(arguments, "-game");

	if (gamePos == -1) {
		return nullptr;
	}

	if (gamePos + 1 == arguments.size()) {
		return nullptr;
	}

	const auto& gameStr = arguments[gamePos + 1];

	std::wstring game = std::filesystem::path(gameStr).filename().wstring();

	std::vector<CamelliaConfigItem*> results;

	std::wcout << L"Search results: " << std::endl;

	size_t index = 1;

	for (const auto& pair : config) {
		for (const auto& name : pair.second->getGames()) {
			if (name.find(game) != std::wstring::npos) {
				std::wcout << index << L"." << name << std::endl;
				results.push_back(pair.second);
				index++;
			}
		}
	}

	std::wcout << L"Enter game number: ";

	size_t number;

	std::wcin >> number;

	if (number < 1 || number > results.size()) {
		std::wcout << L"Incorrect input" << std::endl;
		return nullptr;
	}

	return results[number-1];
}


const CamelliaConfigItem* initConfigByInternalKeyFileName(CamelliaConfig config, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-key");

	if (expectPos == -1) {
		return nullptr;
	}

	if (expectPos + 1 == arguments.size()) {
		return nullptr;
	}

	const auto& keyStr = arguments[expectPos + 1];

	std::wstring filename = std::filesystem::path(keyStr).filename().wstring();

	for (const auto& pair : config) {
		if (pair.first.find(filename) != std::wstring::npos) {
			return pair.second;
		}
	}

	return nullptr;
}

const CamelliaConfigItem* initConfigByExternalKeyFileName(CamelliaConfig config, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-external_key");

	if (expectPos == -1) {
		return nullptr;
	}

	if (expectPos + 1 == arguments.size()) {
		return nullptr;
	}

	const auto& keyStr = arguments[expectPos + 1];

	return read_key(std::filesystem::path(keyStr));
}

const CamelliaConfigItem* initConfig(const std::vector<std::string>& arguments) {

	EncryptionType encoder;

	if (!parseEncryption(arguments, encoder)) {
		return nullptr;
	}

	CamelliaConfig config;

	if (!read_config(L"keys", config)) {
		return nullptr;
	}

	auto configItem = initConfigByDatHeader(config, arguments);

	if (configItem != nullptr) {
		return configItem;
	}
	configItem = initConfigByExpectHeader(config, arguments);

	if (configItem != nullptr) {
		return configItem;
	}
	configItem = initConfigByExternalKeyFileName(config, arguments);

	if (configItem != nullptr) {
		return configItem;
	}
	configItem = initConfigByInternalKeyFileName(config, arguments);

	if (configItem != nullptr) {
		return configItem;
	}
	return nullptr;
}

bool parseUseEncrypt(const std::vector<std::string>& arguments)
{
	const auto encryptPos = findPosition<std::string>(arguments, "-encrypt");
	if (encryptPos == -1) {
		return true;
	}
	if (encryptPos + 1 == arguments.size()) {
		return true;
	}
	const auto encryptStr = arguments[encryptPos + 1];
	if (encryptStr == "0") {
		return false;
	}
	return true;
}

bool parseInput(const std::vector<std::string>& arguments, std::wstring& input)
{
	const auto inputPos = findPosition<std::string>(arguments, "-i");
	if (inputPos == -1) {
		return false;
	}
	if (inputPos + 1 == arguments.size()) {
		return false;
	}
	const auto& inputStr = arguments[inputPos + 1];
	input = std::filesystem::path(inputStr);
	return true;
}

bool parseOutput(const std::vector<std::string>& arguments, std::wstring& output)
{
	const auto outputPos = findPosition<std::string>(arguments, "-o");
	if (outputPos == -1) {
		return false;
	}
	if (outputPos + 1 == arguments.size()) {
		return false;
	}
	const auto& outputStr = arguments[outputPos + 1];
	output = std::filesystem::path(outputStr);
	return true;
}

bool parseAlign(const std::vector<std::string>& arguments, uint32_t& align)
{
	const auto alignPos = findPosition<std::string>(arguments, "-align");
	if (alignPos == -1) {
		return false;
	}
	if (alignPos + 1 == arguments.size()) {
		return false;
	}
	const auto& outputStr = arguments[alignPos + 1];
	align = std::stoi(outputStr);
	return true;
}

int parseThread(const std::vector<std::string>& arguments)
{
	const auto threadPos = findPosition<std::string>(arguments, "-thread");
	if (threadPos == -1) {
		return -1;
	}
	if (threadPos + 1 == arguments.size()) {
		return -1;
	}
	const auto& threadStr = arguments[threadPos + 1];
	return std::stoi(threadStr);
}
