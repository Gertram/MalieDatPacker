#pragma once

#include "GameConfig.h"
#include <vector>
#include <map>


const wchar_t UTF8_MAGIC[] = { 0xEF,0xBB,0xBF };

const uint8_t magic_length = sizeof(UTF8_MAGIC) / sizeof(wchar_t);

typedef std::map<std::wstring, GameConfig> Config;

bool read_key(const std::wstring &path, GameConfig &configItem);

bool read_config(const std::wstring &dirpath, Config &config);