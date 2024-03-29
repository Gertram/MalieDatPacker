#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <numeric>
#include <memory>

#include "IEncryption.h"
#include "encryption.h"

typedef void (*WorkerFunc)(const IEncryption* encryption, uint8_t* data, size_t data_length, size_t offset);

static void encrypt_worker(const IEncryption* encryption, uint8_t* data, size_t data_length, size_t offset) {
	encryption->encrypt(data, data_length, offset);
}
static void decrypt_worker(const IEncryption* encryption, uint8_t* data, size_t data_length, size_t offset) {
	encryption->decrypt(data, data_length, offset);
}

static void file_worker(const IEncryption* encryption, size_t* done, const WorkerFunc worker, const wchar_t* src, const wchar_t* dst, size_t offset, size_t length) {
	FILE* input;
	if (_wfopen_s(&input, src, L"rb") != 0 || input == nullptr) {
		std::wcout << L"File open error: " << src << std::endl;
		return;
	}

	FILE* output;
	if (_wfopen_s(&output, dst, L"wb") != 0 || output == nullptr) {
		std::wcout << L"File create error: " << dst << std::endl;
		return;
	}

	fseek(input, static_cast<long>(offset), SEEK_SET);
	const auto start_offset = offset;
	const size_t buffer_length = 0x100000;
	std::shared_ptr<uint8_t> buffer(new uint8_t[buffer_length]);
	size_t need_to_read;
	size_t count;
	const auto end_offset = offset + length;
	while (!feof(input) && offset < end_offset) {
		need_to_read = buffer_length;
		if (offset + buffer_length > end_offset) {
			need_to_read = end_offset - offset;
		}
		count = fread(buffer.get(), sizeof(uint8_t), need_to_read, input);
		if (count == 0) {
			break;
		}
		if (count != need_to_read) {
			count = count;
		}
		worker(encryption, buffer.get(), count, offset);
		fwrite(buffer.get(), sizeof(uint8_t), count, output);
		offset += count;
		*done = offset - start_offset;
	}

	fclose(input);
	fclose(output);
}

static void memory_worker(const IEncryption* encryption, size_t* done, const WorkerFunc worker, uint8_t* data, size_t offset, size_t length) {
	const auto start_offset = offset;
	size_t count;
	const auto end_offset = offset + length;
	size_t buffer_length = 0x100000;
	while (offset < end_offset) {
		count = buffer_length;
		if (offset + buffer_length > end_offset) {
			count = end_offset - offset;
		}
		worker(encryption, data + offset, count, offset);
		offset += count;
		*done = offset - start_offset;
	}
}

void printStatus(const size_t* const done, const size_t all, const size_t length, const bool& isContinue) {
	try {
		size_t result = 0;
		while (result != 100 && isContinue) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			const size_t done_sum = std::accumulate(done, done + length, 0);
			result = (done_sum * 100) / all;
			std::wcout << L"\rProcessing " << result << "%";
		}
	}
	catch (std::exception& ex) {
		std::wcout << "PrintThread: " << ex.what();
	}
}

static void file_multithread(const IEncryption* encryption, WorkerFunc worker, const std::wstring& src, const std::wstring& dst, int thread_count) {
	FILE* input;
	if (_wfopen_s(&input, src.c_str(), L"rb") != 0 || input == nullptr) {
		std::wcout << L"File open error: " << src << std::endl;
		return;
	}

	fseek(input, 0, SEEK_END);

	const size_t filesize = static_cast<size_t>(ftell(input));

	const size_t blockCount = filesize / BlockLen;

	fclose(input);

	if (filesize % BlockLen != 0) {
		printf("file size must be a multiple of 16");
		return;
	}

	if (thread_count == -1) {
		thread_count = std::thread::hardware_concurrency();
	}

	std::vector<std::wstring> threadDestinations;

	std::vector<std::thread*> threads;

	std::shared_ptr<size_t> done(new size_t[thread_count]);

	const auto blockLengthForThread = blockCount
		/ thread_count;

	size_t blockIndex = 0;

	for (size_t i = 0; i < thread_count; i++) {
		threadDestinations.push_back(dst + std::to_wstring(i));
		const auto offset = blockIndex * BlockLen;
		auto length = blockLengthForThread;
		if (blockIndex + blockLengthForThread > blockCount) {
			length = blockCount - blockIndex;
		}
		blockIndex += length;
		length *= BlockLen;
		std::wcout << L"Thread " << i << std::hex << L" offset " << offset << L" length " << length << std::dec << std::endl;
		done.get()[i] = 0;

		const auto thread = new std::thread(file_worker, encryption, &done.get()[i], worker, src.c_str(), threadDestinations[i].c_str(), offset, length);
		threads.push_back(thread);
	}

	bool isContinue = true;
	std::thread statusThread1(printStatus, done.get(), filesize, thread_count, std::ref(isContinue));

	for (const auto thread : threads) {
		thread->join();
	}

	isContinue = false;

	statusThread1.join();

	std::cout << std::endl;

	std::cout << "Collecting the results into one file" << std::endl;

	FILE* output;
	if (_wfopen_s(&output, dst.c_str(), L"wb") != 0 || output == nullptr) {
		std::wcout << L"File create error: " << dst << std::endl;
		return;
	}
	const size_t bufferLength = 0x100000;
	std::shared_ptr<uint8_t> buffer(new uint8_t[bufferLength]);
	size_t count;
	size_t offset = 0;

	isContinue = true;

	std::thread statusThread2(printStatus, &offset, filesize, 1, std::ref(isContinue));

	for (const auto& threadDestination : threadDestinations) {
		FILE* input;
		if (_wfopen_s(&input, threadDestination.c_str(), L"rb") != 0 || input == nullptr) {
			std::wcout << L"File open error: " << src << std::endl;
			return;
		}
		while (!feof(input)) {
			count = fread(buffer.get(), sizeof(uint8_t), bufferLength, input);
			if (count == 0) {
				break;
			}
			if (count != bufferLength) {
				count = count;
			}
			fwrite(buffer.get(), sizeof(uint8_t), count, output);
			offset += count;
		}
		fclose(input);
		_wremove(threadDestination.c_str());
	}
	isContinue = false;

	statusThread2.join();
	std::cout << std::endl;

	fclose(output);
}

static void memory_multithread(const IEncryption* encryption, WorkerFunc worker, uint8_t* data, const size_t filesize, int thread_count) {

	const size_t blockCount = filesize / BlockLen;

	if (filesize % BlockLen != 0) {
		printf("file size must be a multiple of 16");
		return;
	}

	if (thread_count == -1) {
		thread_count = std::thread::hardware_concurrency();
	}

	std::vector<std::thread*> threads;

	std::shared_ptr<size_t> done(new size_t[thread_count]);

	const auto blockLengthForThread = blockCount / thread_count;

	size_t blockIndex = 0;

	for (size_t i = 0; i < thread_count; i++) {
		const auto offset = blockIndex * BlockLen;
		auto length = blockLengthForThread;
		if (blockIndex + blockLengthForThread > blockCount) {
			length = blockCount - blockIndex;
		}
		blockIndex += length;
		length *= BlockLen;
		std::wcout << L"Thread " << i << std::hex << L" offset " << offset << L" length " << length << std::dec << std::endl;
		done.get()[i] = 0;

		const auto thread = new std::thread(memory_worker, encryption, &done.get()[i], worker, data, offset, length);
		threads.push_back(thread);
	}

	bool isContinue = true;
	std::thread statusThread1(printStatus, done.get(), filesize, thread_count, std::ref(isContinue));

	for (const auto thread : threads) {
		thread->join();
	}

	isContinue = false;

	statusThread1.join();

	std::cout << std::endl;
}

static void file_singlethread(const IEncryption* encryption, WorkerFunc worker, const std::wstring& src, const std::wstring& dst)
{
	FILE* input;
	if (_wfopen_s(&input, src.c_str(), L"rb") != 0 || input == nullptr) {
		std::wcout << L"File open error: " << src << std::endl;
		return;
	}

	FILE* output;
	if (_wfopen_s(&output, dst.c_str(), L"wb") != 0 || output == nullptr) {
		std::wcout << L"File create error: " << dst << std::endl;
		return;
	}

	const size_t bufferLength = 0x100000;
	std::shared_ptr<uint8_t> buffer(new uint8_t[bufferLength]);
	size_t count;
	size_t offset = 0;
	fseek(input, 0, SEEK_END);
	size_t filesize = ftell(input);
	fseek(input, 0, SEEK_SET);

	bool isContinue = true;
	std::thread statusThread(printStatus, &offset, filesize, 1, std::ref(isContinue));

	while (!feof(input)) {
		count = fread(buffer.get(), sizeof(uint8_t), bufferLength, input);
		if (count == 0) {
			break;
		}

		worker(encryption, buffer.get(), count, offset);
		fwrite(buffer.get(), sizeof(uint8_t), count, output);
		offset += count;
	}

	isContinue = false;
	statusThread.join();

	std::wcout << std::endl;

	fclose(input);
	fclose(output);
}

static void memory_singlethread(const IEncryption* encryption, WorkerFunc worker, uint8_t* data, const size_t filesize)
{
	size_t offset = 0;

	bool isContinue = true;
	std::thread statusThread(printStatus, &offset, filesize, 1, std::ref(isContinue));

	size_t section_length = 0x100000;
	size_t count;

	for (size_t i = 0; i < filesize; i += section_length) {

		if (i + section_length > filesize) {
			count = filesize - i;
		}
		else {
			count = section_length;
		}
		worker(encryption, data + i, count, offset);
		offset += count;
	}

	isContinue = false;
	statusThread.join();

	std::wcout << std::endl;
}

void encrypt_file_multithread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst, int thread_count)
{
	file_multithread(encryption, encrypt_worker, src, dst, thread_count);
}

void encrypt_file_singlethread(const IEncryption* encryption, const std::wstring& src, const std::wstring& dst)
{
	file_singlethread(encryption, encrypt_worker, src, dst);
}

void encrypt_memory_multithread(const IEncryption* encryption, uint8_t* data, const size_t filesize, int thread_count)
{
	memory_multithread(encryption, encrypt_worker, data, filesize, thread_count);
}

void encrypt_memory_singlethread(const IEncryption* encryption, uint8_t* data, const size_t filesize)
{
	memory_singlethread(encryption, encrypt_worker, data, filesize);
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

void encrypt_memory(const IEncryption* encryption, uint8_t* data, const size_t filesize, const int thread_count)
{
	std::wcout << "Encryption start..." << std::endl;
	if (thread_count == 1) {
		encrypt_memory_singlethread(encryption, data, filesize);
	}
	else {
		encrypt_memory_multithread(encryption, data, filesize, thread_count);
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
