#pragma once


#include "encoder_camellia.h"

#include <string>

void encrypt_file_multithread(const CamelliaKey& key, const std::wstring& src, const std::wstring& dst, int thread_count);

void encrypt_file_singlethread(const CamelliaKey& key, const std::wstring& src, const std::wstring& dst);

void encrypt_file(const CamelliaKey& key, const std::wstring& src, const std::wstring& dst, const int thread_count);