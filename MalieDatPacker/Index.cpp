#include "Index.h"
#include "utils.h"

Index::Index(const std::wstring& str, const std::uint32_t& indexSeq) :m_str(str) {
	m_indexSeq = indexSeq;
}

void Index::set(uint16_t flag, uint32_t seq, uint32_t count) {
	m_flag = flag;
	m_seq = seq;
	m_count = count;
}

void Index::write(FILE* output) const {
	char data[0x16];
	memset(data, 0, 0x16);
	ToShiftJis(m_str, data, sizeof(data));
	fwrite(data, sizeof(char), sizeof(data), output);
	fwrite(&m_flag, sizeof(uint16_t), 1, output);
	fwrite(&m_seq, sizeof(uint32_t), 1, output);
	fwrite(&m_count, sizeof(uint32_t), 1, output);
}

uint32_t Index::getIndexSeq() const {
	return m_indexSeq;
}
