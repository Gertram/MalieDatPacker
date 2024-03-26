#include <iostream>
#include <filesystem>
#include <thread>
#include <Windows.h>
#include <fstream>
#include <exception>

#include "encoder_camellia.h"

std::filesystem::path defaultPath = "";
std::filesystem::path dirPath = "";
std::vector<std::filesystem::path> filenameList;

const unsigned char expect_header[] = { 0xBF,0x1C, 0x20,0x6c,0x77,0xf2,0xf4,0x6c };

const int CheckOffset = 0x10;

const unsigned char CheckPlain[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, };

const CamelliaConfigItem* config;

const unsigned char Signature[4] = { 'L','I','B','P' };

const std::wstring PackName = L"new2.dat";

void ToShiftJis(const std::wstring& str, char* dst, size_t dst_size) {
	int nJisLen = ::WideCharToMultiByte(932, 0, str.c_str(), str.size(), NULL, 0, NULL, NULL);
	if (nJisLen > dst_size) {
		throw std::exception("nJisLen > dst_size");
	}
	::WideCharToMultiByte(932, 0, str.c_str(), str.size(), dst, nJisLen, NULL, NULL);
}

class Index;
class Offset;
class File;

std::vector<Index*> indexSection;
//std::vector<Offset> offsetSection;
std::vector<File*> fileSection;

class Index {
public:
	Index(const std::wstring& str, const std::uint32_t& indexSeq) :m_str(str) {
		m_indexSeq = indexSeq;
	}

	void set(uint16_t flag, uint32_t seq, uint32_t count) {
		m_flag = flag;
		m_seq = seq;
		m_count = count;
	}
	void write(FILE* output) const {
		char data[0x16];
		memset(data, 0, 0x16);
		ToShiftJis(m_str, data, sizeof(data));
		fwrite(data, sizeof(char), sizeof(data), output);
		fwrite(&m_flag, sizeof(uint16_t), 1, output);
		fwrite(&m_seq, sizeof(uint32_t), 1, output);
		fwrite(&m_count, sizeof(uint32_t), 1, output);
	}
	inline uint32_t getIndexSeq() const {
		return m_indexSeq;
	}
private:
	uint16_t m_flag = 0;
	uint32_t m_seq = 0;
	uint32_t m_count = 0;
	std::uint32_t m_indexSeq;
	const std::wstring m_str;
};

//class Offset {
//public:
//	void set(uint32_t fileAddr, uint8_t* data) {
//		uint32_t i = fileAddr / 0x400; // Fixed
//		*reinterpret_cast<uint32_t*>(data + m_addr) = i;
//	}
//	void setAddr(uint32_t addr) {
//		m_addr = addr;
//	}
//	void write(FILE* output) const {
//		fwrite(data, sizeof(uint8_t), sizeof(data), output);
//	}
//private:
//	uint8_t data[4] = { 0,0,0,0 };
//	uint32_t m_addr = 0; // Address in the packet
//};

class File {
public:
	File(const std::wstring& path, uint32_t seq) :m_seq(seq), m_path(path) {
		std::ifstream fileOld(path, std::ios::binary);
		if (!fileOld.is_open()) {
			throw std::exception("File open error");
		}
		fileOld.seekg(0, std::ios::end);
		m_fileSize = fileOld.tellg();
		fileOld.close();
	}
	void write(FILE* output) {
		uint8_t buffer[0x10000];
		FILE* file;
		const auto result = _wfopen_s(&file, m_path.c_str(), L"rb");
		if (result != 0 || file == nullptr) {
			throw std::exception("File open error");
		}
		size_t count;
		while (!feof(file)) {
			count = fread(buffer, sizeof(uint8_t), sizeof(buffer), file);
			fwrite(buffer, sizeof(uint8_t), count, output);
		}

		fclose(file);
	}
	inline uint32_t getSeq() const {
		return m_seq;
	}
	inline uint32_t getFileSize() const {
		return m_fileSize;
	}
	uint32_t getOffset() const {
		return m_offset;
	}
	void setOffset(uint32_t offset) {
		m_offset = offset;
	}
private:
	uint32_t m_offset;
	uint32_t m_seq;
	uint32_t m_fileSize;
	std::wstring m_path;
};

void encrypt(unsigned char* data, const int size, int offset, bool printed) {
	encrypt_all(config->key, data, size, offset, printed);
}

bool initConfig() {
	if (expect_header == nullptr) {
		return false;
	}

	printf("Try to find expect...\n");

	unsigned char bs[16];

	for (const auto& pair : CamelliaConfig)
	{
		config = &pair.second;

		memset(bs, 0, sizeof(bs));

		encrypt(bs, sizeof(bs), CheckOffset, false);

		if (memcmp(bs, expect_header, sizeof(expect_header)) == 0) {
			std::wcout << L"Find expect config : " << pair.first << std::endl;
			return true;
		}
	}
	printf("Cannot find expect ExpectHeader.\n");
	return false;
}


void fillingAlign(FILE* output) {
	std::size_t size = config->align;
	const auto current_size = ftell(output);
	std::size_t remain = current_size - (current_size / size) * size;
	if (remain == 0)
		return;

	// Заполнение
	fseek(output, (size - remain), SEEK_CUR);
}


static void traverse(const std::wstring& path, Index* index) {
	// Файл
	if (std::filesystem::is_regular_file(path)) {
		const auto seq = fileSection.size();
		File* file = new File(path, seq);
		index->set(0x1, seq, file->getFileSize());
		fileSection.push_back(file);
		return;
	}

	// Получение файлов и подпапок в директории
	std::vector<std::wstring> children;
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		children.push_back(entry.path().filename().wstring());
	}
	std::sort(children.begin(), children.end());

	if (children.empty()) {
		index->set(0, 0, 0);
		std::wcout << L"\033[31mFolder is empty:\033[0m " << path << std::endl;
		return;
	}

	//Take a seat first
	std::vector<Index*> indexList;
	for (const auto& name : children) {
		Index* childIndex = new Index(name, indexSection.size());
		indexSection.push_back(childIndex);
		indexList.push_back(childIndex);
	}

	// Установка текущего индекса
	index->set(0, indexList[0]->getIndexSeq(), indexList.size());

	// Построение индекса
	for (std::size_t i = 0; i < children.size(); ++i) {
		const auto& name = children[i];
		const auto& pathChild = path / std::filesystem::path(name);
		if (std::filesystem::is_regular_file(pathChild)) { // Файл
			traverse(pathChild, indexList[i]);
		}
	}
	for (std::size_t i = 0; i < children.size(); ++i) {
		const auto& name = children[i];
		const auto& pathChild = path / std::filesystem::path(name);
		if (std::filesystem::is_directory(pathChild)) { // Папка
			traverse(pathChild, indexList[i]);
		}
	}
}

void indexing() {
	// Индексация
	std::cout << "Indexing..." << std::endl;
	Index* root = new Index(L"", 0);
	indexSection.push_back(root);
	traverse(dirPath, root);
}

std::wstring generate_output_filename() {
	auto output_path = std::filesystem::path(dirPath).parent_path();
	if (!std::filesystem::exists(output_path)) {
		std::filesystem::create_directories(output_path);
	}
	return output_path / PackName;
}

void write_header(FILE* output) {

	fwrite(Signature, sizeof(uint8_t), sizeof(Signature), output);
	const auto indexSectionSize = indexSection.size();
	fwrite(&indexSectionSize, sizeof(uint32_t), 1, output);
	const auto offsetSectionSize = fileSection.size();
	fwrite(&offsetSectionSize, sizeof(uint32_t), 1, output);
	fseek(output, 4, SEEK_CUR);

	for (const auto& index : indexSection) {
		index->write(output);
	}
}

void write_offsets(FILE* output) {

	auto addr = ftell(output);
	for (auto& file : fileSection) {
		auto offset = file->getOffset();
		offset /= 0x400;
		fwrite(&offset, sizeof(uint32_t), 1, output);
		addr = ftell(output);
	}

	fillingAlign(output);
}

void calc_positions(FILE* output) {
	const auto offsets_pos = ftell(output);

	fseek(output, fileSection.size() * sizeof(uint32_t), SEEK_CUR);

	fillingAlign(output);

	//file
	std::size_t fileAddr = ftell(output);
	std::size_t fileStart = fileAddr;
	for (auto& file : fileSection) {
		file->setOffset(fileAddr - fileStart);
		fseek(output, file->getFileSize(), SEEK_CUR);
		fillingAlign(output);
		fileAddr = ftell(output);
	}

	fseek(output, offsets_pos, SEEK_SET);
}

void write_files(FILE* output) {

	for (auto& file : fileSection) {
		file->write(output);
		fillingAlign(output);
	}
}

void generate_uncrypted(const std::wstring& filepath) {
	FILE* output;
	if (_wfopen_s(&output, filepath.c_str(), L"wb") != 0 || output == nullptr) {
		std::wcout << "File create error: " << filepath;
		return;
	}

	write_header(output);

	calc_positions(output);

	write_offsets(output);

	write_files(output);

	fclose(output);

	for (const auto index : indexSection) {
		delete index;
	}
	for (const auto file : fileSection) {
		delete file;
	}
}

void encrypt_file_worker(const unsigned int *t_key,const wchar_t *src, const wchar_t *dst,int offset,const int length) {
	std::array<uint32_t, 52> key;
	for (int i = 0; i < key.size(); i++) {
		key[i] = t_key[i];
	}
	FILE* input;
	if (_wfopen_s(&input, src, L"rb") != 0) {
		std::wcout << "File open error: " << src << std::endl;
		return;
	}

	FILE* output;
	if (_wfopen_s(&output, dst, L"wb") != 0) {
		std::wcout << "File create error: " << dst << std::endl;
		return;
	}

	fseek(input, offset,SEEK_SET);

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
		encrypt_all(key, buffer, count, offset, false);
		fwrite(buffer, sizeof(uint8_t), count, output);
		offset += count;
	}

	fclose(input);
	fclose(output);
}

void encrypt_file(const std::array<uint32_t, 52>& key, const std::wstring& src, const std::wstring& dst) {
	std::cout << "Encrypting... Each line's bytes: 0x" << std::hex << BlockLen << std::endl;
	FILE* input;
	if (_wfopen_s(&input, src.c_str(), L"rb") != 0) {
		std::wcout << "File open error: " << src << std::endl;
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

	const auto thread_count = std::thread::hardware_concurrency();
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
		std::cout << "Thread " << i << " offset " << offset << " length " << length << std::endl;
		
		const auto thread = new std::thread(encrypt_file_worker,key.data(), src.c_str(), thread_dsts[i].c_str(), offset, length);
		threads.push_back(thread);
	}

	for (const auto thread : threads) {
		thread->join();
	}

	FILE* output;
	if (_wfopen_s(&output, dst.c_str(), L"wb") != 0) {
		std::wcout << "File create error: " << dst << std::endl;
		return;
	}
	uint8_t buffer[0x10000];
	size_t count;
	size_t offset = 0;

	for (const auto thread_dst : thread_dsts) {
		FILE* input;
		if (_wfopen_s(&input, thread_dst.c_str(), L"rb") != 0) {
			std::wcout << "File open error: " << src << std::endl;
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

	std::cout << "Encrypted." << std::endl;
}

void pack(const std::filesystem::path &filepath,bool encrypt = true) {
	indexSection.clear();
	fileSection.clear();
	indexing();

	generate_uncrypted(L"temp.dat");

	//const auto filepath = generate_output_filename();

	if (!encrypt) {
		std::filesystem::rename(L"temp.dat", filepath);
		return;
	}

	encrypt_file(config->key, L"temp.dat", filepath);

	std::filesystem::remove(L"temp.dat");
}

std::vector<std::filesystem::path> listFiles(const std::filesystem::path& startPath) {
	std::vector<std::filesystem::path> fileList;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(startPath)) {
		if (!entry.is_directory()) {

			std::filesystem::path relativePath = std::filesystem::relative(entry.path(), startPath);
			fileList.push_back(relativePath.generic_string());
		}
	}
	return fileList;
}

void cut_file(const wchar_t* filename, const wchar_t* output_filename, size_t file_size) {
	FILE* input;
	auto result = _wfopen_s(&input, filename, L"rb");
	if (result != 0) {
		throw std::exception("file open error");
	}
	FILE* output;
	result = _wfopen_s(&output, output_filename, L"wb");
	if (result != 0) {
		throw std::exception("file create error");
	}
	char buffer[4096];
	size_t offset = 0;
	size_t count;
	while (!feof(input) && offset <= file_size) {
		count = fread(buffer, sizeof(char), sizeof(buffer), input);
		fwrite(buffer, sizeof(char), count, output);
		offset += sizeof(buffer);
	}

	fclose(input);
	fclose(output);
}

int encode(int argc,char* argv[]) {
	auto start = clock();
	/*argc = 3;
	argv = new char* [3];
	argv[1] = (char*)"D:\\Games\\danzai\\data\\new2.dat";
	argv[2] = (char*)"D:\\Games\\danzai\\data\\game.dat";*/

	if (argc < 3) {
		printf("you need to put 2 file\n");
		return -2;
	}

	std::ifstream keyfile;

	keyfile.open("key.txt");

	if (!keyfile.good()) {
		printf("key.txt was not opened\n");
		return -1;
	}

	std::array<uint32_t, 52> key;
	unsigned char UTFMAGIC[] = { 0xEF,0xBB,0xBF };
	char magic[sizeof(UTFMAGIC)];
	keyfile.read(magic, sizeof(UTFMAGIC));
	if (memcmp(magic, UTFMAGIC, sizeof(UTFMAGIC)) != 0) {
		keyfile.seekg(0);
	}

	std::string line;
	printf("Key\n");
	for (int i = 0; i < 52; i++) {
		getline(keyfile, line);
		key[i] = std::stoul(line);
		std::cout << key[i] << std::endl;
	}

	keyfile.close();


	std::filesystem::path path(argv[1]);

	const auto src = path.wstring();

	//const auto extension = path.extension();

	//const auto dst = path.replace_filename(path.filename().replace_extension("").wstring() + L"_encrypted").replace_extension(extension).wstring();

	const auto dst = std::filesystem::path(argv[2]).wstring();

	encrypt_file(key, src, dst);

	auto end = clock();

	std::cout << "Time: " << float(end - start) / CLOCKS_PER_SEC << std::endl;

	getchar();

	return 0;
}

int main(int argc, char* argv[]) {
	const auto start = clock();

	if (argc < 3) {
		printf("Must be 2 argument\n");
		return -1;
	}

	std::filesystem::path path = argv[1];
	std::filesystem::path path2 = argv[2];

	if (std::filesystem::is_directory(path)) {

		if (!initConfig()) {
			return -1;
		}
		dirPath = path;
		const auto files = listFiles(dirPath);
		filenameList.insert(filenameList.end(), files.begin(), files.end());
		pack(path2,true);
	}

	const auto end = clock();

	std::cout << "Time: " << float(end - start) / CLOCKS_PER_SEC << std::endl;

	getchar();

	return 0;
}