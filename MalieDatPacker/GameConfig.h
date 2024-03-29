#pragma once

#include <string>
#include <vector>
#include <array>
#include <string>

enum EncryptionType {
	Malie,
	Camellia128
};

struct GameConfig {
	EncryptionType encryption;
	std::vector<uint32_t> key;
	uint32_t align = 0;
	std::vector<std::wstring> games;
};
