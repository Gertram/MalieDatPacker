#include <string>
#include <iostream>
#include <filesystem>

#include "Packer.h"

void Packer::fillingAlign(FILE* output, bool hard) const {
	const auto current_size = ftell(output);
	std::size_t remain = current_size - (current_size / m_align) * m_align;
	if (remain == 0)
		return;
	const auto need = m_align - remain;
	if (!hard) {
		fseek(output, (m_align - remain), SEEK_CUR);
		return;
	}
	//For end of file
	uint8_t temp[4096];
	memset(temp, 0, sizeof(temp));
	for (int i = 0; i < need; i += sizeof(temp)) {
		if (i + sizeof(temp) > need) {
			fwrite(temp, sizeof(uint8_t),need-i , output);
		}
		else {
			fwrite(temp, sizeof(uint8_t), sizeof(temp), output);
		}
	}
}

void Packer::traverse(const std::wstring& path, Index* index) {
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

void Packer::indexing(const std::wstring &dirpath) {
	// Индексация
	std::cout << "Indexing..." << std::endl;
	Index* root = new Index(L"", 0);
	indexSection.push_back(root);
	traverse(dirpath, root);
}

void Packer::write_header(FILE* output) const {

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

void Packer::write_offsets(FILE* output) const {

	auto addr = ftell(output);
	for (auto& file : fileSection) {
		auto offset = file->getOffset();
		offset /= 0x400;
		fwrite(&offset, sizeof(uint32_t), 1, output);
		addr = ftell(output);
	}

	fillingAlign(output);
}

void Packer::calc_positions(FILE* output) const {
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

void Packer::write_files(FILE* output) const {
	size_t index = 0;
	for (auto& file : fileSection) {
		file->write(output);
		index++;
		fillingAlign(output, index == fileSection.size());
	}
}

void Packer::generate_uncrypted(const std::wstring &dirpath,const std::wstring& filepath) {
	indexSection.clear();
	fileSection.clear();
	indexing(dirpath);

	FILE* output;
	if (_wfopen_s(&output, filepath.c_str(), L"wb") != 0 || output == nullptr) {
		std::wcout << "File create error: " << filepath;
		return;
	}

	write_header(output);

	calc_positions(output);

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