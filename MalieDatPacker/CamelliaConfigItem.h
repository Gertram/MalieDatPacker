#pragma once

#include <string>
#include <vector>
#include <array>
#include <string>

typedef std::array<uint32_t, 52> CamelliaKey;

class CamelliaConfigItem {
public:
	CamelliaConfigItem(uint32_t align, const CamelliaKey& key, const std::vector<std::wstring>& games);
	const CamelliaKey& getKey() const;
	const uint32_t getAlign() const;
	const std::vector<std::wstring>& getGames() const;
private:
	const CamelliaKey key;
	const uint32_t align;
	const std::vector<std::wstring> games;
};
