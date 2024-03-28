#pragma once

#include "CamelliaConfigItem.h"
#include <vector>


typedef std::map<std::wstring, CamelliaConfigItem> CamelliaConfig;

bool read_key(const std::wstring &path, CamelliaConfigItem &configItem);

bool read_config(const std::wstring &dirpath, CamelliaConfig &config);