#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "IEncryption.h"
#include "encryption.h"

typedef void (*WorkerFunc)(const IEncryption* encryption,uint8_t *data,uint32_t data_length, uint32_t offset);

static void encrypt_worker(const IEncryption* encryption, uint8_t* data, uint32_t data_length, uint32_t offset) {
	encryption->encrypt(data, data_length, offset);
}
static void decrypt_worker(const IEncryption* encryption, uint8_t* data, uint32_t data_length, uint32_t offset) {
	encryption->decrypt(data, data_length, offset);
}

static void file_worker(const IEncryption *encryption, const WorkerFunc worker, const wchar_t* src, const wchar_t* dst, int offset, const int length) {
	FILE* input;
	if (_wfopen_s(&input, src, L"rb") != 0) {
		std::wcout << L"File open error: " << src << std::endl;
		return;
	}

	FILE* output;
	if (_wfopen_s(&output, dst, L"wb") != 0) {
		std::wcout << L"File create error: " << dst << std::endl;
		return;
	}

	fseek(input, offset, SEEK_SET);

	uint8_t buffer[0x10000];
	size_t need_to_read;
	size_t count;
	const auto end_offset = offset + length;
	while (!feof(input) && offset < end_offset) {
		need_to_read = sizeof(buffer);
		if (offset + sizeof(buffer) > end_offset) {
			need_to_read = end_offset - offset;
		}
		count = fread(buffer, sizeof(uint8_t), need_to_read, input);
		if (count == 0) {
			break;
		}
		if (count != need_to_read) {
			count = count;
		}
		worker(encryption, buffer, count, offset);
		fwrite(buffer, sizeof(uint8_t), count, output);
		offset += count;
	}

	fclose(input);
	fclose(output);
}

static void file_multithread(const IEncryption* encryption, WorkerFunc worker, const std::wstring& src, const std::wstring& dst, int thread_count) {
	FILE* input;
	if (_wfopen_s(&input, src.c_str(), L"rb") != 0) {
		std::wcout << L"File open error: " << src << std::endl;
		return;
	}

	fseek(input, 0, SEEK_END);

	const auto file_size = ftell(input);

	const auto block_count = file_size / BlockLen;

	fclose(input);

	if (file_size % BlockLen != 0) {
		printf("file size must be a multiple of 16");
		return;
	}

	if (thread_count == -1) {
		thread_count = std::thread::hardware_concurrency();
	}

	std::vector<std::wstring> thread_dsts;

	std::vector<std::thread*> threads;

	const auto block_length_for_thread = block_count / thread_count;

	auto block_index = 0;

	for (int i = 0; i < thread_count; i++) {
		thread_dsts.push_back(dst + std::to_wstring(i));
		const auto offset = block_index * BlockLen;
		auto length = block_length_for_thread;
		if (block_index + block_length_for_thread > block_count) {
			length = block_count - block_index;
		}
		block_index += length;
		length *= BlockLen;
		std::wcout << L"Thread " << i << L" offset " << offset << L" length " << length << std::endl;

		const auto thread = new std::thread(file_worker, encryption, worker, src.c_str(), thread_dsts[i].c_str(), offset, length);
		threads.push_back(thread);
	}

	for (const auto thread : threads) {
		thread->join();
	}

	FILE* output;
	if (_wfopen_s(&output, dst.c_str(), L"wb") != 0) {
		std::wcout << L"File create error: " << dst << std::endl;
		return;
	}
	uint8_t buffer[0x10000];
	size_t count;
	size_t offset = 0;

	for (const auto thread_dst : thread_dsts) {
		FILE* input;
		if (_wfopen_s(&input, thread_dst.c_str(), L"rb") != 0) {
			std::wcout << L"File open error: " << src << std::endl;
			return;
		}
		while (!feof(input)) {
			count = fread(buffer, sizeof(uint8_t), sizeof(buffer), input);
			if (count == 0) {
				break;
			}
			if (count != sizeof(buffer)) {
				count = count;
			}
			fwrite(buffer, sizeof(uint8_t), count, output);
			offset += count;
		}
		fclose(input);
		_wremove(thread_dst.c_str());
	}
	fclose(output);
}

static void file_singlethread(const IEncryption* encryption, WorkerFunc worker, const std::wstring& src, const std::wstring& dst)
{
	FILE* input;
	if (_wfopen_s(&input, src.c_str(), L"rb") != 0) {
		std::wcout << L"File open error: " << src << std::endl;
		return;
	}

	FILE* output;
	if (_wfopen_s(&output, dst.c_str(), L"wb") != 0) {
		std::wcout << L"File create error: " << dst << std::endl;
		return;
	}

	uint8_t buffer[0x10000];
	size_t need_to_read;
	size_t count;
	size_t offset = 0;
	while (!feof(input)) {
		count = fread(buffer, sizeof(uint8_t), sizeof(buffer), input);
		if (count == 0) {
			break;
		}
		
		worker(encryption,buffer, count, offset);
		fwrite(buffer, sizeof(uint8_t), count, output);
		offset += count;
	}

	fclose(input);
	fclose(output);
}

void encrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count)
{
	file_multithread(encryption, encrypt_worker, src, dst, thread_count);
}

void encrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst)
{
	file_singlethread(encryption, encrypt_worker, src, dst);
}

void decrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count)
{
	file_multithread(encryption, decrypt_worker, src, dst, thread_count);
}

void decrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst)
{
	file_singlethread(encryption, decrypt_worker, src, dst);
}

void encrypt_file(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, const int thread_count)
{
	std::wcout << "Encryption start..." << std::endl;
	if (thread_count == 1) {
		encrypt_file_singlethread(encryption, src, dst);
	}
	else {
		encrypt_file_multithread(encryption, src, dst, thread_count);
	}
	std::wcout << "Encryption end." << std::endl;
}

void decrypt_file(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, const int thread_count)
{
	std::wcout << "Decryption start..." << std::endl;
	if (thread_count == 1) {
		decrypt_file_singlethread(encryption, src, dst);
	}
	else {
		decrypt_file_multithread(encryption, src, dst, thread_count);
	}
	std::wcout << "Decryption end." << std::endl;
}
