#pragma once

#include <map>
#include <string>
#include <string>

#include "CamelliaConfigItem.h"

const int BlockLen = 0x10;

void encrypt(const CamelliaKey&key, unsigned char* data,const int size, int offset, bool printed = true);