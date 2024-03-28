#pragma once

#include <map>
#include <string>
#include <string>

#include "CamelliaConfigItem.h"
#include "IEncryption.h"

class CamelliaEncryption :public IEncryption {
public:
	CamelliaEncryption(const CamelliaKey& key):m_key(key) {

	}
	void encrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const override;
	void decrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const override;
private:
	const CamelliaKey m_key;
};