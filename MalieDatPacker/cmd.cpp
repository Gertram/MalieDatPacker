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
#include "camellia_encryption.h"
#include "malie_encryption.h"

int parseWorkMode(const std::vector<std::string>& arguments, WorkMode &workMode)
{
	const auto modePos = findPosition<std::string>(arguments, "-mode");
	if (modePos == -1) {
		workMode = WorkMode::Pack;
		return 0;
	}
	if (modePos + 1 == arguments.size()) {
		std::wcout << "Parse mode error. Used pack" << std::endl;
		return -1;
	}
	const auto offsetStr = arguments[modePos + 1];
	if (offsetStr == "encrypt") {
		workMode = WorkMode::Encryption;
		return 1;
	}
	if (offsetStr == "decrypt") {
		workMode = WorkMode::Decryption;
		return 1;
	}
	if (offsetStr == "pack") {
		workMode = WorkMode::Pack;
		return 1;
	}
	std::wcout << "Undefined workmode error" << std::endl;
	return -2;
}

int parseCheckOffset(const std::vector<std::string>& arguments,size_t &checkOffset) {
	const auto offsetPos = findPosition<std::string>(arguments, "-offset");
	if (offsetPos == -1) {
		checkOffset = DefaultCheckOffset;
		return 0;
	}
	if (offsetPos + 1 == arguments.size()) {
		std::wcout << "Parse offset error" << std::endl;
		return -1;
	}
	const auto &offsetStr = arguments[offsetPos + 1];
	std::from_chars(offsetStr.data(), offsetStr.data() + offsetStr.size(), checkOffset, 16);
	return 1;
}

int parseEncryption(const std::vector<std::string>& arguments, EncryptionType& encoder) {
	const auto encoder_pos = findPosition<std::string>(arguments, "-encrpytion");
	if (encoder_pos == -1) {
		encoder = EncryptionType::Camellia128;
		return 0;
	}
	if (encoder_pos + 1 == arguments.size()) {
		std::wcout << L"Encryption expected but end of line detected" << std::endl;
		return -1;
	}
	const auto encoderStr = arguments[encoder_pos + 1];
	if (encoderStr == "camellia" || encoderStr == "Camellia") {
		encoder= EncryptionType::Camellia128;
		return 1;
	}
	if (encoderStr == "malie" || encoderStr == "Malie") {
		encoder = EncryptionType::Malie;
		return 1;
	}

	std::cout << "Unknown encryption: " << encoderStr << std::endl;

	return -2;
}

const IEncryption* parseEncryption(const GameConfig& configItem) {
	if (configItem.encryption == EncryptionType::Camellia128) {
		CamelliaKey key;
		for (size_t i = 0; i < key.size(); i++) {
			key[i] = configItem.key[i];
		}
		return new CamelliaEncryption(key);
	}
	if (configItem.encryption == EncryptionType::Malie) {
		MalieKey key;
		for (size_t i = 0; i < key.size(); i++) {
			key[i] = configItem.key[i];
		}
		return new MalieEncryption(key);
	}

	return nullptr;
}

int initConfigByExpectHeader(Config config,GameConfig &configItem, const std::vector<uint8_t>& expect_header, int checkOffset) {

	if (expect_header.size() == 0) {
		return -1;
	}

	printf("Try to find expect...\n");

	uint8_t bs[16];

	for (const auto &item : config)
	{
		const auto encryption = parseEncryption(configItem);

		encryption->encrypt(bs, sizeof(bs), checkOffset);

		delete encryption;

		if (memcmp(bs, expect_header.data(), expect_header.size()) == 0) {
			std::wcout << L"Find expect config : " << item.first << std::endl;
			configItem = item.second;
			return 1;
		}
	}
	printf("Cannot found expect ExpectHeader.\n");
	return -2;
}

int initConfigByExpectHeader(Config config, GameConfig& configItem, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-expect");

	if (expectPos == -1) {
		return 0;
	}

	if (expectPos + 1 == arguments.size()) {
		std::wcout << "Parse expected header error" << std::endl;
		return -1;
	}

	const auto& expectStr = arguments[expectPos + 1];

	std::vector<uint8_t> expect_header;
	try {

		const auto strs = splitString(expectStr, " ");
		if (strs.size() == 0) {
			std::wcout << "Expected header was empty" << std::endl;
			return -2;
		}

		for (const auto& hexStr : strs) {
			int value;
			std::from_chars(hexStr.data(), hexStr.data() + hexStr.size(), value, 16);
			expect_header.push_back(value);
		}
	}
	catch (std::exception& exp) {
		std::wcout << "Parse expected header error" << std::endl;
		return -2;
	}

	size_t checkOffset;
	
	if (parseCheckOffset(arguments, checkOffset) < 0) {
		return -2;
	}

	return initConfigByExpectHeader(config,configItem, expect_header, checkOffset);
}

int initConfigByDatHeader(Config config, GameConfig& configItem, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-dat");

	if (expectPos == -1) {
		return 0;
	}

	if (expectPos + 1 == arguments.size()) {
		std::wcout << "Parse dat error" << std::endl;
		return -1;
	}

	const auto& datpath = arguments[expectPos + 1];

	size_t checkOffset;

	if (parseCheckOffset(arguments, checkOffset) < 0) {
		return -2;
	}

	FILE* file;
	const auto result = fopen_s(&file, datpath.c_str(), "rb");
	if (result != 0 || file == nullptr) {
		std::wcout << "Dat file not found" << std::endl;
		return -3;
	}

	uint8_t buffer[8];

	fseek(file, checkOffset, SEEK_SET);

	const auto count = fread(buffer, sizeof(uint8_t), sizeof(buffer), file);

	std::vector<uint8_t> expect_header;
	for (auto i = 0; i < count; i++) {
		expect_header.push_back(buffer[i]);
	}

	fclose(file);

	return initConfigByExpectHeader(config,configItem, expect_header, checkOffset);
}

int initConfigByGame(Config config, GameConfig& configItem, const std::vector<std::string>& arguments)
{
	const auto gamePos = findPosition<std::string>(arguments, "-game");

	if (gamePos == -1) {
		return 0;
	}

	if (gamePos + 1 == arguments.size()) {
		std::wcout << "Parse game error" << std::endl;
		return -1;
	}

	const auto& gameStr = arguments[gamePos + 1];

	std::wstring game = std::filesystem::path(gameStr).filename().wstring();

	std::vector<const GameConfig*> results;

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

	try {

		size_t number;

		std::wcin >> number;

		if (number < 1 || number > results.size()) {
			std::wcout << L"Incorrect input" << std::endl;
			return -2;
		}

		configItem = *results[number - 1];
	}
	catch (std::exception& exp) {
		std::wcout << L"Incorrect input" << std::endl;
		return -3;
	}
	return 1;
}


int initConfigByInternalKeyFileName(Config config, GameConfig& configItem, const std::vector<std::string>& arguments) {
	const auto keyPos = findPosition<std::string>(arguments, "-internal_key");

	if (keyPos == -1) {
		return 0;
	}

	if (keyPos + 1 == arguments.size()) {
		std::wcout << "Parse key error" << std::endl;
		return -1;
	}

	const auto& keyStr = arguments[keyPos + 1];

	std::wstring filename = std::filesystem::path(keyStr).filename().wstring();

	for (const auto& pair : config) {
		if (pair.first.find(filename) != std::wstring::npos) {
			configItem = pair.second;
			return 1;
		}
	}

	return -2;
}

int initConfigByKeyFileName(Config config, GameConfig& configItem, const std::vector<std::string>& arguments) {
	const auto keyPos = findPosition<std::string>(arguments, "-key");

	if (keyPos == -1) {
		return 0;
	}

	if (keyPos + 1 == arguments.size()) {
		std::wcout << L"Parse key error" << std::endl;
		return -1;
	}

	const auto& keyStr = arguments[keyPos + 1];

	std::wifstream input(keyStr);

	if (!input.is_open()) {
		std::cout << "File \"" << keyStr << "\" was not opened" << std::endl;
		return -2;
	}

	wchar_t magic[magic_length];

	input.read(magic, magic_length);

	if (wmemcmp(magic, UTF8_MAGIC, magic_length) != 0) {
		input.seekg(0);
	}
	input.imbue(std::locale(".UTF-8"));

	int line_number = 0;
	std::wstring line;
	auto &key = configItem.key;
	while (!input.eof()) {
		std::getline(input, line);
		try {
			const auto k = std::stoul(line);
			key.push_back(k);
		}
		catch (std::invalid_argument) {
			std::wcout << L"Invalid number in line " << line_number << std::endl;
			return false;
		}
		catch (std::out_of_range) {
			std::wcout << L"So big number in line " << line_number << std::endl;
			return false;
		}
		line_number++;
	}

	if (parseEncryption(arguments, configItem.encryption) < 0) {
		std::wcout << L"Key option need encryption" << std::endl;
		return -3;
	}


	if (parseAlign(arguments, configItem.align) < 0) {
		std::wcout << L"Key option need align" << std::endl;
		return -4;
	}

	return 1;
}

int initConfigByExternalKeyFileName(Config config, GameConfig& configItem, const std::vector<std::string>& arguments) {
	const auto expectPos = findPosition<std::string>(arguments, "-external_key");

	if (expectPos == -1) {
		return 0;
	}

	if (expectPos + 1 == arguments.size()) {
		std::wcout << "Parse external_key error" << std::endl;
		return -1;
	}

	const auto& keyStr = arguments[expectPos + 1];

	return read_key(std::filesystem::path(keyStr),configItem);
}

int parseConfig(const std::vector<std::string>& arguments,const Config &config, GameConfig& configItem) {

	int result;

	if ((result = initConfigByDatHeader(config, configItem, arguments)) != 0) {
		return result;
	}
	if ((result = initConfigByExpectHeader(config, configItem, arguments)) != 0) {
		return result;
	}

	if ((result = initConfigByKeyFileName(config, configItem, arguments)) != 0) {
		return result;
	}

	if ((result = initConfigByInternalKeyFileName(config, configItem, arguments)) !=0 ) {
		return result;
	}

	if ((result = initConfigByGame(config, configItem, arguments)) != 0){
		return result;
	}
	return -10;
}

int parseConfig(const std::vector<std::string>& arguments, ConfigItem &configItem) {
	Config config;

	if (!read_config(L"keys", config)) {
		return -1;
	}
	GameConfig gameConfig;
	if(!parseConfig(arguments, config,gameConfig)){
		return -2;
	}

	configItem.games = gameConfig.games;
	configItem.align = gameConfig.align;
	
	configItem.encryption = parseEncryption(gameConfig);

	uint8_t buffer[16];
	memset(buffer, 0, sizeof(buffer));
	configItem.encryption->encrypt(buffer, sizeof(buffer), 0x10);

	return 1;
}

int parseUseEncrypt(const std::vector<std::string>& arguments,bool &use_encrypt)
{
	const auto encryptPos = findPosition<std::string>(arguments, "-encrypt");
	if (encryptPos == -1) {
		use_encrypt = false;
		return 0;
	}
	if (encryptPos + 1 == arguments.size()) {
		std::wcout << L"Parse encrypt error" << std::endl;
		return -1;
	}
	const auto encryptStr = arguments[encryptPos + 1];
	if (encryptStr == "1") {
		use_encrypt = true;
		return 1;
	}
	if (encryptStr == "0") {
		use_encrypt = false;
		return 1;
	}
	std::wcout << L"Parse encrypt error" << std::endl;
	return -2;
}

int parseNoTemp(const std::vector<std::string>& arguments,bool &notemp)
{
	const auto encryptPos = findPosition<std::string>(arguments, "-notemp");
	if (encryptPos == -1) {
		notemp = false;
		return 0;
	}
	if (encryptPos + 1 == arguments.size()) {
		std::wcout << L"Parse notemp error" << std::endl;
		return -1;
	}
	const auto encryptStr = arguments[encryptPos + 1];
	if (encryptStr == "1") {
		notemp = true;
		return 1;
	}
	if (encryptStr == "0") {
		notemp = false;
		return 1;
	}
	std::wcout << L"Parse notemp error" << std::endl;
	return -2;
}

int parseInput(const std::vector<std::string>& arguments, std::wstring& input)
{
	const auto inputPos = findPosition<std::string>(arguments, "-i");
	if (inputPos == -1) {
		std::wcout << L"input not found" << std::endl;
		return 0;
	}
	if (inputPos + 1 == arguments.size()) {
		std::wcout << L"Parse input error" << std::endl;
		return -1;
	}
	const auto& inputStr = arguments[inputPos + 1];
	input = std::filesystem::path(inputStr);
	return 1;
}

int parseOutput(const std::vector<std::string>& arguments, std::wstring& output)
{
	const auto outputPos = findPosition<std::string>(arguments, "-o");
	if (outputPos == -1) {
		std::wcout << L"output not found" << std::endl;
		return 0;
	}
	if (outputPos + 1 == arguments.size()) {
		std::wcout << L"Parse output error" << std::endl;
		return -1;
	}
	const auto& outputStr = arguments[outputPos + 1];
	output = std::filesystem::path(outputStr);
	return 1;
}

int parseAlign(const std::vector<std::string>& arguments, uint32_t& align)
{
	const auto alignPos = findPosition<std::string>(arguments, "-align");
	if (alignPos == -1) {
		return 0;
	}
	if (alignPos + 1 == arguments.size()) {
		std::wcout << "Parse align error" << std::endl;
		return -1;
	}
	const auto& outputStr = arguments[alignPos + 1];
	try {
		align = std::stoi(outputStr);

		return 1;
	}
	catch (std::exception& exp) {
		std::wcout << "Parse align error" << std::endl;
		return -2;
	}
}

int parseThread(const std::vector<std::string>& arguments, uint32_t &thread_count)
{
	const auto threadPos = findPosition<std::string>(arguments, "-thread");
	if (threadPos == -1) {
		thread_count = 1;
		return 0;
	}
	if (threadPos + 1 == arguments.size()) {
		std::wcout << "Parse thread error" << std::endl;
		return -1;
	}
	const auto& threadStr = arguments[threadPos + 1];
	try {
		thread_count = std::stoi(threadStr);
		return 1;
	}
	catch (std::exception& exp) {
		std::wcout << "Parse thread error" << std::endl;
		return -2;
	}
}
