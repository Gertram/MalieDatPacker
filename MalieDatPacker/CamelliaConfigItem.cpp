#include "CamelliaConfigItem.h"


CamelliaConfigItem::CamelliaConfigItem(uint32_t align, const CamelliaKey& key, const std::vector<std::wstring>& games)
	: key(key), align(align), games(games)
{
}
const CamelliaKey& CamelliaConfigItem::getKey() const
{
    return key;
}

const uint32_t CamelliaConfigItem::getAlign() const
{
    return align;
}

const std::vector<std::wstring>& CamelliaConfigItem::getGames() const
{
    return games;
}
