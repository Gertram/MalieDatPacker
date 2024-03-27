#pragma once

#include <string>

class Index{
public:
	Index(const std::wstring& str, const std::uint32_t& indexSeq);

	void set(uint16_t flag, uint32_t seq, uint32_t count);
	void write(FILE* output) const;
	uint32_t getIndexSeq() const;
private:
	uint16_t m_flag = 0;
	uint32_t m_seq = 0;
	uint32_t m_count = 0;
	std::uint32_t m_indexSeq;
	const std::wstring m_str;
};
