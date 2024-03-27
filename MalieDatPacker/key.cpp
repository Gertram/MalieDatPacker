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

const wchar_t UTF8_MAGIC[] = { 0xEF,0xBB,0xBF };

const uint8_t magic_length = sizeof(UTF8_MAGIC) / sizeof(wchar_t);

enum ReadState {
	None,
	Games,
	Align,
	Key
};

CamelliaConfigItem* read_key(const std::wstring& path)
{
	std::wifstream input(path);

	if (!input.is_open()) {
		std::wcout << L"File \"" << path << L"\" was not opened" << std::endl;
		return nullptr;
	}

	wchar_t magic[magic_length];

	input.read(magic, magic_length);


	if (wmemcmp(magic, UTF8_MAGIC, magic_length) != 0) {
		input.seekg(0);
	}
	input.imbue(std::locale(".UTF-8"));
	std::vector<std::wstring> games;
	uint32_t align = -1;
	CamelliaKey key;
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
		if (line == L"KEY") {
			state = ReadState::Key;
			continue;
		}
		if (state == ReadState::Key) {
			try {
				key[key_index] = std::stoul(line);
				key_index++;
				continue;
			}
			catch (std::invalid_argument) {
				std::wcout << L"Invalid string for KEY in line " << line_number << L" in key \"" << path << "\"" << std::endl;
				return nullptr;
			}
			catch (std::out_of_range) {
				std::wcout << L"So big string for KEY in line " << line_number << L" in key \"" << path << "\"" << std::endl;
				return nullptr;
			}
		}
		if (state == ReadState::Games) {
			games.push_back(line);
			continue;
		}
		if (state == ReadState::Align) {
			try {
				align = std::stoul(line);
				continue;
			}
			catch (std::invalid_argument) {
				std::wcout << L"Invalid string for ALIGN in line " << line_number << L" in key \"" << path << L"\"" << std::endl;
				return nullptr;
			}
			catch (std::out_of_range) {
				std::wcout << L"So big string for ALIGN in line " << line_number << L" in key \"" << path << L"\"" << std::endl;
				return nullptr;
			}
		}
		std::wcout << L"Unresolved line \"" << line << L"\" in key \"" << path << L"\"" << std::endl;
	}

	input.close();

	if (align == -1) {
		std::wcout << L"ALIGN NOT FOUND in key \"" << path << L"\"" << std::endl;
		return nullptr;
	}

	if (key_index != 52) {
		std::wcout << L"Invalid size for KEY(" << key_index << L") expected 52 in key \"" << path << L"\"" << std::endl;
		return nullptr;
	}

	return new CamelliaConfigItem(align, key, games);
}

bool read_config(const std::wstring& dirpath, std::map<std::wstring, CamelliaConfigItem*>& config)
{
	std::filesystem::path dir(dirpath);
	if (!std::filesystem::is_directory(dir)) {
		std::wcout << dirpath << L" is not directory" << std::endl;
		return false;
	}

	for (const auto& entry : std::filesystem::directory_iterator(dir)) {

		const auto configItem = read_key(entry.path());
		if (configItem != nullptr) {
			config[entry.path().filename()] = configItem;
		}
	}
	return true;
}
