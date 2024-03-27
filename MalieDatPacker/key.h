#pragma once

#include "CamelliaConfigItem.h"
#include <vector>

CamelliaConfigItem* read_key(const std::wstring &path);

bool read_config(const std::wstring &dirpath, std::map<std::wstring, CamelliaConfigItem*> &config);