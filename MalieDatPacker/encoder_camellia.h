#pragma once

#include <map>
#include <string>
#include <array>

const int BlockLen = 0x10;

struct CamelliaConfigItem {
	const std::array<unsigned int,52> key;
	const int align;

	CamelliaConfigItem(int align,const std::array<unsigned int, 52>& key)
		: key(key), align(align)
	{
	}
};

const std::map<std::wstring, CamelliaConfigItem> CamelliaConfig = {
	{
		L"Omerta ~Chinmoku no Okite~",
		{
			4096,
			{
			  1316495790,
			  2819474132,
			  2248757531,
			  4047237730,
			  783096101,
			  581648179,
			  565089812,
			  3048937126,
			  4180748092,
			  282547206,
			  3781837572,
			  2827876509,
			  2337478486,
			  2358415701,
			  2677641431,
			  1258969821,
			  762234281,
			  2343257673,
			  1219153868,
			  3362497925,
			  706969127,
			  2118928847,
			  70636801,
			  3092943041,
			  562189382,
			  4233034904,
			  2737087573,
			  1743152181,
			  3362497925,
			  762234281,
			  2343257673,
			  1219153868,
			  3781837572,
			  2827876509,
			  4180748092,
			  282547206,
			  2819474132,
			  2248757531,
			  4047237730,
			  1316495790,
			  581648179,
			  565089812,
			  3048937126,
			  783096101,
			  1130188827,
			  2242448402,
			  2721571447,
			  3838090480,
			  843728214,
			  2120631133,
			  740911990,
			  759979354
			}
		}
	}
};

void encrypt_all(const std::array<unsigned int, 52> &key, unsigned char* data,const int size, int offset, bool printed = true);