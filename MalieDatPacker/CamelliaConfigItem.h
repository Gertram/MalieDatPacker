#pragma once

#include <string>
#include <vector>
#include <array>
#include <string>

typedef std::array<uint32_t, 52> CamelliaKey;

struct CamelliaConfigItem {
	CamelliaKey key;
	uint32_t align;
	std::vector<std::wstring> games;
};
