#pragma once

#include <string>

class File {
public:
	File(const std::wstring& path, uint32_t seq);
	void write(FILE* output) const;
	uint32_t getSeq() const;
	uint32_t getFileSize() const;
	uint32_t getOffset() const;
	void setOffset(uint32_t offset);
private:
	uint32_t m_offset;
	uint32_t m_seq;
	uint32_t m_fileSize;
	std::wstring m_path;
};
