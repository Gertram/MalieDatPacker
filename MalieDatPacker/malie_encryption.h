#pragma once

#include <array>

#include "IEncryption.h"

typedef std::array<uint32_t,4> MalieKey;

class MalieEncryption :public IEncryption {
public:
	MalieEncryption(const MalieKey& key) :m_key(key) {

	}
	void encrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const override;
	void decrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const override;
private:
	const MalieKey m_key;
};