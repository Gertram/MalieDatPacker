#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "utils.h"

#include "Packer.h"

size_t Packer::fillingAlign(size_t current_size) const {
	std::size_t remain = current_size - (current_size / m_align) * m_align;
	if (remain == 0)
		return current_size;
	const auto need = m_align - remain;
	return current_size + need;
}

void Packer::fillingAlign(FILE* output, bool hard) const {
	const auto current_size = ftell(output);
	std::size_t remain = current_size - (current_size / m_align) * m_align;
	if (remain == 0)
		return;
	const auto need = m_align - remain;
	if (!hard) {
		fseek(output, static_cast<int>(m_align - remain), SEEK_CUR);
		return;
	}
	//For end of file
	const size_t buffer_length = 0x1000;
	uint8_t temp[buffer_length];
	memset(temp, 0, buffer_length);
	for (size_t i = 0; i < need; i += buffer_length) {
		if (i + buffer_length > need) {
			fwrite(temp, sizeof(uint8_t),need-i , output);
		}
		else {
			fwrite(temp, sizeof(uint8_t), buffer_length, output);
		}
	}
}

void Packer::traverse(const std::wstring& path, Index* index) {
	// Файл
	if (std::filesystem::is_regular_file(path)) {
		const auto seq = static_cast<uint32_t>(fileSection.size());

		File* file = new File();
		std::ifstream fileOld(path, std::ios::binary);
		if (!fileOld.is_open()) {
			throw std::exception("File open error");
		}
		fileOld.seekg(0, std::ios::end);
		file->seq = seq;
		file->filesize = static_cast<uint32_t>(fileOld.tellg());
		file->path = path;
		fileOld.close();
		index->flag = 0x1;
		index->seq = seq;
		index->count = file->filesize;
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
		index->flag = 0x0;
		index->seq = 0x0;
		index->count = 0x0;
		std::wcout << L"\033[31mFolder is empty:\033[0m " << path << std::endl;
		return;
	}

	//Take a seat first
	std::vector<Index*> indexList;
	for (const auto& name : children) {
		Index* childIndex = new Index();
		childIndex->str = name;
		childIndex->indexSeq = static_cast<uint32_t>(indexSection.size());

		indexSection.push_back(childIndex);
		indexList.push_back(childIndex);
	}

	// Установка текущего индекса
	index->flag = 0x0;
	index->seq = indexList[0]->indexSeq;
	index->count = static_cast<uint32_t>(indexList.size());

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

void Packer::indexing(const std::wstring &dirpath) {
	// Индексация
	std::cout << "Indexing..." << std::endl;
	Index* root = new Index();
	root->str = L"";
	root->indexSeq = 0;
	indexSection.push_back(root);
	traverse(dirpath, root);
}

void Packer::write_header(FILE* output) const {
	std::cout << "Write header... " << std::hex << ftell(output) << std::dec << std::endl;

	fwrite(Signature, sizeof(uint8_t), sizeof(Signature), output);
	const auto indexSectionSize = indexSection.size();
	fwrite(&indexSectionSize, sizeof(uint32_t), 1, output);
	const auto offsetSectionSize = fileSection.size();
	fwrite(&offsetSectionSize, sizeof(uint32_t), 1, output);
	fseek(output, 4, SEEK_CUR);
	std::cout << "Write indexes... " << std::hex << ftell(output) << std::dec << std::endl;

	for (const auto& index : indexSection) {
		char data[index_name_length];
		memset(data, 0, index_name_length);
		ToShiftJis(index->str, data, index_name_length);
		fwrite(data, sizeof(char), index_name_length, output);
		fwrite(&index->flag, sizeof(uint16_t), 1, output);
		fwrite(&index->seq, sizeof(uint32_t), 1, output);
		fwrite(&index->count, sizeof(uint32_t), 1, output);
	}
}

size_t Packer::write_header(uint8_t* data,size_t offset) const {
	std::cout << "Write header... " << std::hex << offset << std::dec << std::endl;

	memcpy(data + offset, Signature, sizeof(Signature));
	offset += sizeof(Signature);
	const auto indexSectionSize = indexSection.size();
	*reinterpret_cast<uint32_t*>(data + offset) = indexSectionSize;
	offset += sizeof(uint32_t);
	const auto offsetSectionSize = fileSection.size();
	*reinterpret_cast<uint32_t*>(data + offset) = offsetSectionSize;
	offset += sizeof(uint32_t);
	*reinterpret_cast<uint32_t*>(data + offset) = 0;
	offset += sizeof(uint32_t);
	std::cout << "Write indexes... " << std::hex << offset << std::dec << std::endl;

	for (const auto& index : indexSection) {
		memset(data+offset, 0, index_name_length);
		ToShiftJis(index->str, reinterpret_cast<char*>(data) + offset, index_name_length);
		offset += index_name_length;
		*reinterpret_cast<uint16_t*>(data + offset) = index->flag;
		offset += sizeof(uint16_t);
		*reinterpret_cast<uint32_t*>(data + offset) = index->seq;
		offset += sizeof(uint32_t);
		*reinterpret_cast<uint32_t*>(data + offset) = index->count;
		offset += sizeof(uint32_t);
	}
	return offset;
}

void Packer::write_offsets(FILE* output) const {

	auto addr = ftell(output);
	std::cout << "Write offsets... " << std::hex << addr << std::dec << std::endl;
	for (auto& file : fileSection) {
		auto offset = file->offset;
		offset /= 0x400;
		fwrite(&offset, sizeof(uint32_t), 1, output);
	}

	fillingAlign(output);
}

size_t Packer::write_offsets(uint8_t *data,size_t offset) const {
	std::cout << "Write offsets... " << std::hex << offset << std::dec << std::endl;
	for (auto& file : fileSection) {
		auto temp_offset = file->offset;
		temp_offset /= 0x400;
		*reinterpret_cast<uint32_t*>(data + offset) = temp_offset;
		offset += sizeof(uint32_t);
	}

	const auto remain = fillingAlign(offset) - offset;

	memset(data + offset, 0, remain);

	offset += remain;

	return offset;
}

void Packer::calc_positions(size_t offset) const {

	std::cout << "Calc offsets... " << std::hex << offset << std::dec << std::endl;

	offset += fileSection.size() * sizeof(uint32_t);

	offset = fillingAlign(offset);

	//file
	const size_t filesStart = offset;
	for (auto& file : fileSection) {
		file->offset = static_cast<uint32_t>(offset - filesStart);
		offset += file->filesize;
		offset = fillingAlign(offset);
	}
}

void Packer::write_files(FILE* output) const {
	size_t offset = ftell(output);
	std::cout << "Write files..." << std::hex << offset << std::dec << std::endl;
	size_t index = 0;

	const size_t buffer_length = 0x100000;
	std::shared_ptr<uint8_t> buffer(new uint8_t[buffer_length]);
	for (auto& file : fileSection) {
		FILE* input;
		const auto result = _wfopen_s(&input, file->path.c_str(), L"rb");
		if (result != 0 || input == nullptr) {
			throw std::exception("File open error");
		}
		size_t count;
		while (!feof(input)) {
			count = fread(buffer.get(), sizeof(uint8_t), buffer_length, input);
			fwrite(buffer.get(), sizeof(uint8_t), count, output);
		}

		fclose(input);

		index++;
		fillingAlign(output, index == fileSection.size());
		std::cout << "\rProcessing " << index << " of " << fileSection.size();
#ifdef DEBUG

		offset = ftell(output);

#endif // DEBUG
	}
	std::cout << std::endl;
}

size_t Packer::write_files(uint8_t *data,size_t offset) const {
	std::cout << "Write files..." << std::hex << offset << std::dec << std::endl;
	size_t index = 0;

	const size_t buffer_length = 0x100000;
	std::shared_ptr<uint8_t> buffer(new uint8_t[buffer_length]);
	for (auto& file : fileSection) {
		FILE* input;
		const auto result = _wfopen_s(&input, file->path.c_str(), L"rb");
		if (result != 0 || input == nullptr) {
			throw std::exception("File open error");
		}
		size_t count;
		while (!feof(input)) {
			count = fread(buffer.get(), sizeof(uint8_t), buffer_length, input);
			memcpy(data + offset, buffer.get(), count);
			offset += count;
		}

		fclose(input);

		index++;
		const auto remain = fillingAlign(offset) - offset;

		memset(data + offset, 0, remain);

		offset += remain;
		std::cout << "\rProcessing " << index << " of " << fileSection.size();
	}
	std::cout << std::endl;
	return offset;
}

void Packer::generate_uncrypted(const std::wstring &dirpath,const std::wstring& filepath) {
	indexSection.clear();
	fileSection.clear();
	indexing(dirpath);

	FILE* output;
	if (_wfopen_s(&output, filepath.c_str(), L"wb") != 0 || output == nullptr) {
		std::wcout << L"File create error: " << filepath;
		return;
	}

	write_header(output);

	calc_positions(ftell(output));

	write_offsets(output);

	write_files(output);

	fflush(output);

	fclose(output);

	for (const auto index : indexSection) {
		delete index;
	}
	for (const auto file : fileSection) {
		delete file;
	}
	indexSection.clear();
	fileSection.clear();
}

size_t Packer::calc_filesize() const
{
	const auto header_size = 0x10;
	size_t filesize = header_size;
	filesize += (sizeof(uint16_t)+sizeof(uint32_t)*2+index_name_length) * indexSection.size();
	filesize = fillingAlign(filesize);
	filesize += sizeof(uint32_t) * fileSection.size();
	filesize = fillingAlign(filesize);
	for (const auto& file : fileSection) {
		filesize += file->filesize;
		filesize = fillingAlign(filesize);
	}
	return filesize;
}

bool Packer::generate_uncrypted_in_memory(const std::wstring& dirpath, uint8_t** data, size_t& filesize)
{
	indexSection.clear();
	fileSection.clear();
	indexing(dirpath);

	filesize = calc_filesize();

	*data = new uint8_t[filesize];

	size_t offset = 0;
	
	offset = write_header(*data, offset);

	calc_positions(offset);

	offset = write_offsets(*data,offset);

	write_files(*data,offset);

	for (const auto index : indexSection) {
		delete index;
	}
	for (const auto file : fileSection) {
		delete file;
	}
	indexSection.clear();
	fileSection.clear();
	return true;
}
