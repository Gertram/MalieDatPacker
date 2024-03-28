#pragma once


#include "IEncryption.h"

#include <string>

void encrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count);

void encrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst);

void encrypt_file(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, const int thread_count);

void decrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count);

void decrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst);

void decrypt_file(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, const int thread_count);