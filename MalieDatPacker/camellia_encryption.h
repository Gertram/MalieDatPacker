#pragma once

#include <map>
#include <string>
#include <array>

#include "IEncryption.h"


typedef std::array<uint32_t, 52> CamelliaKey;

class CamelliaEncryption :public IEncryption {
public:
	CamelliaEncryption(const CamelliaKey& key):m_key(key) {

	}
	void encrypt(unsigned char* data, size_t data_length, size_t offset) const override;
	void decrypt(unsigned char* data, size_t data_length, size_t offset) const override;
private:
	const CamelliaKey m_key;
};