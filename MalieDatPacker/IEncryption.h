#pragma once


const size_t BlockLen = 0x10;

struct IEncryption {
public:
	virtual void encrypt(unsigned char* data, size_t data_length, size_t offset) const = 0;
	virtual void decrypt(unsigned char* data, size_t data_length, size_t offset) const = 0;
};