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
private:
	const uint32_t m_align;
	void fillingAlign(FILE* output, bool hard = false) const;

	void traverse(const std::wstring& path, Index* index);

	void indexing(const std::wstring& dirpath);

	void write_header(FILE* output) const;

	void write_offsets(FILE* output) const;

	void calc_positions(FILE* output) const;

	void write_files(FILE* output) const;

	std::vector<Index*> indexSection;
	std::vector<File*> fileSection;
};

