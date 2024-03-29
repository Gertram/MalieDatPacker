#pragma once


#include "IEncryption.h"

#include <string>

void encrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count);

void encrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst);

void encrypt_file(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, const int thread_count);

void encrypt_memory_multithread(const IEncryption* encryption, uint8_t* data, const size_t filesize, int thread_count);

void encrypt_memory_singlethread(const IEncryption* encryption, uint8_t* data, const size_t filesize);

void encrypt_memory(const IEncryption* encryption, uint8_t *data,const size_t filesize, const int thread_count);

void decrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count);

void decrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst);

void decrypt_file(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, const int thread_count);