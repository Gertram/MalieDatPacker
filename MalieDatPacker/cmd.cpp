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
		std::wcout << "Parse mode error. Used pack" << std::endl;
		return WorkMode::Pack;
	}
	const auto offsetStr = arguments[modePos + 1];
	if (offsetStr == "encrypt") {
		return WorkMode::Encryption;
	}
	if (offsetStr == "decrypt") {
		return WorkMode::Decryption;
	}
	if (offsetStr == "pack") {
		return WorkMode::Pack;
	}
	std::wcout << "Undefined mode error. Used pack" << std::endl;
	return WorkMode::Pack;
}

int parseCheckOffset(const std::vector<std::string>& arguments) {
	const auto offsetPos = findPosition<std::string>(arguments, "-offset");
	if (offsetPos == -1) {
		return DefaultCheckOffset;
	}
	if (offsetPos + 1 == arguments.size()) {
		std::wcout << "Parse offset error. Used default " << std::hex << DefaultCheckOffset << std::dec << std::endl;
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

bool initConfigByExpectHeader(CamelliaConfig config,CamelliaConfigItem &configItem, const std::vector<uint8_t>& expect_header, int checkOffset) {

	if (expect_header.size() == 0) {
		return false;
	}

	printf("Try to find expect...\n");

	uint8_t bs[16];

	for (const auto &item : config)
	{
		memset(bs, 0, sizeof(bs));

		CamelliaEncryption encryption(item.second.key);

		encryption.encrypt(bs, sizeof(bs), checkOffset);

		if (memcmp(bs, expect_header.data(), expect_header.size()) == 0) {
			std::wcout << L"Find expect config : " << item.first << std::endl;
			configItem = item.second;
			return true;
		}
	}
	printf("Cannot found expect ExpectHeader.\n");
	return false;
}

bool initConfigByExpectHeader(CamelliaConfig config, CamelliaConfigItem& configItem, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-expect");

	if (expectPos == -1) {
		return false;
	}

	if (expectPos + 1 == arguments.size()) {
		std::wcout << "Parse expected header error" << std::endl;
		return false;
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

	return initConfigByExpectHeader(config,configItem, expect_header, checkOffset);
}


bool initConfigByDatHeader(CamelliaConfig config, CamelliaConfigItem& configItem, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-dat");

	if (expectPos == -1) {
		return false;
	}

	if (expectPos + 1 == arguments.size()) {
		std::wcout << "Parse dat error" << std::endl;
		return false;
	}

	const auto& datpath = arguments[expectPos + 1];

	FILE* file;
	const auto result = fopen_s(&file, datpath.c_str(), "rb");
	if (result != 0 || file == nullptr) {
		return false;
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

	return initConfigByExpectHeader(config,configItem, expect_header, checkOffset);
}

bool initConfigByGame(CamelliaConfig config, CamelliaConfigItem& configItem, const std::vector<std::string>& arguments)
{
	const auto gamePos = findPosition<std::string>(arguments, "-game");

	if (gamePos == -1) {
		return false;
	}

	if (gamePos + 1 == arguments.size()) {
		std::wcout << "Parse game error" << std::endl;
		return false;
	}

	const auto& gameStr = arguments[gamePos + 1];

	std::wstring game = std::filesystem::path(gameStr).filename().wstring();

	std::vector<const CamelliaConfigItem*> results;

	std::wcout << L"Search results: " << std::endl;

	size_t index = 1;

	for (const auto& pair : config) {
		for (const auto& name : pair.second.games) {
			if (name.find(game) != std::wstring::npos) {
				std::wcout << index << L"." << name << std::endl;
				results.push_back(&pair.second);
				index++;
			}
		}
	}

	std::wcout << L"Enter game number: ";

	size_t number;

	std::wcin >> number;

	if (number < 1 || number > results.size()) {
		std::wcout << L"Incorrect input" << std::endl;
		return false;
	}

	configItem = *results[number - 1];
	return true;
}


bool initConfigByInternalKeyFileName(CamelliaConfig config, CamelliaConfigItem& configItem, const std::vector<std::string>& arguments) {
	const auto keyPos = findPosition<std::string>(arguments, "-key");

	if (keyPos == -1) {
		return false;
	}

	if (keyPos + 1 == arguments.size()) {
		std::wcout << "Parse key error" << std::endl;
		return false;
	}

	const auto& keyStr = arguments[keyPos + 1];

	std::wstring filename = std::filesystem::path(keyStr).filename().wstring();

	for (const auto& pair : config) {
		if (pair.first.find(filename) != std::wstring::npos) {
			configItem = pair.second;
			return true;
		}
	}

	return false;
}

bool initConfigByExternalKeyFileName(CamelliaConfig config, CamelliaConfigItem& configItem, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-external_key");

	if (expectPos == -1) {
		return false;
	}

	if (expectPos + 1 == arguments.size()) {
		std::wcout << "Parse external_key error" << std::endl;
		return false;
	}

	const auto& keyStr = arguments[expectPos + 1];

	return read_key(std::filesystem::path(keyStr),configItem);
}

bool parseCamelliaConfig(const std::vector<std::string>& arguments,const CamelliaConfig &config, CamelliaConfigItem& configItem) {

	if (initConfigByDatHeader(config, configItem, arguments)) {
		return true;
	}
	if (initConfigByExpectHeader(config, configItem, arguments)) {
		return true;
	}

	if (initConfigByExternalKeyFileName(config, configItem, arguments)) {
		return true;
	}
	if (initConfigByInternalKeyFileName(config, configItem, arguments)) {
		return true;
	}

	if (initConfigByGame(config, configItem, arguments)) {
		return true;
	}
	return false;
}
bool parseCamelliaConfig(const std::vector<std::string>& arguments, ConfigItem &configItem) {
	CamelliaConfig config;

	if (!read_config(L"keys", config)) {
		return false;
	}
	CamelliaConfigItem camelliaConfigItem;
	if(!parseCamelliaConfig(arguments, config,camelliaConfigItem)){
		return false;
	}

	configItem.games = std::move(camelliaConfigItem.games);
	configItem.align = camelliaConfigItem.align;
	configItem.encrption = new CamelliaEncryption(camelliaConfigItem.key);

	return true;
}
bool parseConfig(const std::vector<std::string>& arguments,ConfigItem &configItem) {

	EncryptionType encoder;

	if (!parseEncryption(arguments, encoder)) {
		return false;
	}

	if (encoder == EncryptionType::Camellia128) {
		return parseCamelliaConfig(arguments, configItem);
	}

	return false;
}

bool parseUseEncrypt(const std::vector<std::string>& arguments)
{
	const auto encryptPos = findPosition<std::string>(arguments, "-encrypt");
	if (encryptPos == -1) {
		return true;
	}
	if (encryptPos + 1 == arguments.size()) {
		std::wcout << "Parse encrypt error" << std::endl;
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
		std::wcout << L"input not found" << std::endl;
		return false;
	}
	if (inputPos + 1 == arguments.size()) {
		std::wcout << L"input not found" << std::endl;
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
		std::wcout << L"output not found" << std::endl;
		return false;
	}
	if (outputPos + 1 == arguments.size()) {
		std::wcout << L"output not found" << std::endl;
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
		std::wcout << "Parse align error" << std::endl;
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
		return 1;
	}
	if (threadPos + 1 == arguments.size()) {
		std::wcout << "Parse thread error. Used 1" << std::endl;
		return 1;
	}
	const auto& threadStr = arguments[threadPos + 1];
	return std::stoi(threadStr);
}
