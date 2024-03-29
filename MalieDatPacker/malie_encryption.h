#pragma once

#include <array>

#include "IEncryption.h"

typedef std::array<uint32_t,4> MalieKey;

class MalieEncryption :public IEncryption {
public:
	MalieEncryption(const MalieKey& key) :m_key(key) {

	}
	void encrypt(unsigned char* data, size_t data_length, size_t offset) const override;
	void decrypt(unsigned char* data, size_t data_length, size_t offset) const override;
private:
	const MalieKey m_key;
};