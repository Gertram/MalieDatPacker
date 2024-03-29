#pragma once

#include <cstdio>
#include "Index.h"
#include "File.h"

const unsigned char Signature[4] = { 'L','I','B','P' };

class Packer
{
public:
	Packer(const uint32_t align):m_align(align) {

	}

	void generate_uncrypted(const std::wstring& dirpath, const std::wstring& filepath);
	bool generate_uncrypted_in_memory(const std::wstring& dirpath, uint8_t **data, size_t &filesize);
private:
	const uint32_t m_align;
	void fillingAlign(FILE* output, bool hard = false) const;

	size_t fillingAlign(size_t current_size) const;

	void traverse(const std::wstring& path, Index* index);

	void indexing(const std::wstring& dirpath);

	void write_header(FILE* output) const;

	size_t write_header(uint8_t* data,size_t offset) const;

	void write_offsets(FILE* output) const;

	size_t write_offsets(uint8_t *data,size_t offset) const;

	void calc_positions(size_t offset) const;

	void write_files(FILE* output) const;

	size_t write_files(uint8_t* data, size_t offset) const;

	size_t calc_filesize() const;

	std::vector<Index*> indexSection;
	std::vector<File*> fileSection;
};

