#pragma once


const int BlockLen = 0x10;

struct IEncryption {
public:
	virtual void encrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const = 0;
	virtual void decrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const = 0;
};