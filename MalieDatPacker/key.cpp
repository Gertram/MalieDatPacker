#include <string>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <locale>
#include <codecvt>
#include <map>

#include "key.h"

const wchar_t UTF16LE_MAGIC[] = { 0xFF,0xFE };

const wchar_t UTF16BE_MAGIC[] = { 0xFE,0xFF };


enum ReadState {
	None,
	Games,
	Align,
	Encryption,
	Key
};

bool read_key(const std::wstring& path, GameConfig &configItem)
{
	std::wifstream input(path);

	if (!input.is_open()) {
		std::wcout << L"File \"" << path << L"\" was not opened" << std::endl;
		return false;
	}

	wchar_t magic[magic_length];

	input.read(magic, magic_length);


	if (wmemcmp(magic, UTF8_MAGIC, magic_length) != 0) {
		input.seekg(0);
	}
	input.imbue(std::locale(".UTF-8"));

	auto &games = configItem.games;
	auto& align = configItem.align;
	auto& encryption = configItem.encryption;
	align = -1;
	auto &key = configItem.key;

	ReadState state = ReadState::None;
	int key_index = 0;
	std::wstring line;
	size_t line_number = 0;

	while (!input.eof()) {
		line_number++;
		std::getline(input, line);
		if (line == L"") {
			continue;
		}
		if (line == L"GAMES") {
			state = ReadState::Games;
			continue;
		}
		if (line == L"ALIGN") {
			state = ReadState::Align;
			continue;
		}
		if (line == L"ENCRYPTION") {
			state = ReadState::Encryption;
			continue;
		}
		if (line == L"KEY") {
			state = ReadState::Key;
			continue;
		}
		if (state == ReadState::Key) {
			try {
				key.push_back(std::stoul(line));
				key_index++;
				continue;
			}
			catch (std::invalid_argument) {
				std::wcout << L"Invalid number for KEY in line " << line_number << L" in key \"" << path << "\"" << std::endl;
				return false;
			}
			catch (std::out_of_range) {
				std::wcout << L"So big number for KEY in line " << line_number << L" in key \"" << path << "\"" << std::endl;
				return false;
			}
		}
		if (state == ReadState::Games) {
			games.push_back(line);
			continue;
		}
		if (state == ReadState::Encryption) {
			if (line == L"Malie") {
				encryption = EncryptionType::Malie;
			}
			else if (line == L"Camellia") {
				encryption = EncryptionType::Camellia128;
			}
			else {
				std::wcout << L"Undefined encryption in line " << line_number << L" in key \"" << path << "\"" << std::endl;
				return false;
			}
			continue;
		}
		if (state == ReadState::Align) {
			try {
				align = std::stoul(line);
				continue;
			}
			catch (std::invalid_argument) {
				std::wcout << L"Invalid number for ALIGN in line " << line_number << L" in key \"" << path << L"\"" << std::endl;
				return false;
			}
			catch (std::out_of_range) {
				std::wcout << L"So big number for ALIGN in line " << line_number << L" in key \"" << path << L"\"" << std::endl;
				return false;
			}
		}
		std::wcout << L"Unresolved line \"" << line << L"\" in key \"" << path << L"\"" << std::endl;
	}

	input.close();

	if (align == -1) {
		std::wcout << L"ALIGN NOT FOUND in key \"" << path << L"\"" << std::endl;
		return false;
	}

	return true;
}

bool read_config(const std::wstring& dirpath, Config & config)
{
	std::filesystem::path dir(dirpath);
	if (!std::filesystem::is_directory(dir)) {
		std::wcout << dirpath << L" is not directory" << std::endl;
		return false;
	}

	for (const auto& entry : std::filesystem::directory_iterator(dir)) {

		GameConfig configItem;
		if (read_key(entry.path(),configItem)) {
			
			config[entry.path().filename()] = configItem;
		}
	}
	return true;
}
