#include <fstream>

#include "File.h"

File::File(const std::wstring& path, uint32_t seq) :m_seq(seq), m_path(path),m_offset(0) {
	std::ifstream fileOld(path, std::ios::binary);
	if (!fileOld.is_open()) {
		throw std::exception("File open error");
	}
	fileOld.seekg(0, std::ios::end);
	m_fileSize = fileOld.tellg();
	fileOld.close();
}

void File::write(FILE* output) const {
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

uint32_t File::getSeq() const {
	return m_seq;
}

uint32_t File::getFileSize() const {
	return m_fileSize;
}

uint32_t File::getOffset() const {
	return m_offset;
}

void File::setOffset(uint32_t offset) {
	m_offset = offset;
}