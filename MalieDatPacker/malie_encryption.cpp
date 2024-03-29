#include "malie_encryption.h"

#include <stdlib.h>
#include <iostream>
#include <stdint.h>

//https://github.com/Dir-A/MalieTools

inline uint32_t __ROR4__(uint32_t value, int count) { return _rotl(value, -count); }
inline uint32_t __ROL4__(uint32_t value, int count) { return _rotl(value, count); }

static uint8_t aStaticTable[32] =
{
	0xA4,0xA7,0xA6,0xA1,0xA0,0xA3,0xA2,0xAC,
	0xAF,0xAE,0xA9,0xA8,0xAB,0xAA,0xB4,0xB7,
	0xB6,0xB1,0xB0,0xB3,0xB2,0xBC,0xBF,0xBE,
	0xB9,0xB8,0xBB,0xBA,0xA1,0xA9,0xB1,0xB9,
};

static constexpr uint32_t BitLen = 32;
static constexpr uint32_t ValueMask = 0xFFFFFFFF;// (1 << BitLen) - 1;

static unsigned int rotateRight(unsigned int value, int shift) {
	value = (value >> shift) | (value << (BitLen - shift));
	return value & ValueMask;
}

static unsigned int rotateLeft(unsigned int value, int shift) {
	value = (value << shift) | (value >> (BitLen - shift));
	return value & ValueMask;
}

//https://github.com/satan53x/SExtractor/blob/main/tools/Malie/encoder_cfi.py
static void rotateEnc(uint32_t* data, const MalieKey& rotateKey, size_t offset) {
	uint32_t k = rotateRight(rotateKey[0], aStaticTable[offset & 0x1F] ^ 0xA5);

	data[0] = rotateLeft(data[0], aStaticTable[(offset + 12) & 0x1F] ^ 0xA5) ^ k;

	k = rotateLeft(rotateKey[1], aStaticTable[(offset + 3) & 0x1F] ^ 0xA5);

	data[1] = rotateRight(data[1], aStaticTable[(offset + 15) & 0x1F] ^ 0xA5) ^ k;

	k = rotateRight(rotateKey[2], aStaticTable[(offset + 6) & 0x1F] ^ 0xA5);

	data[2] = rotateLeft(data[2], aStaticTable[(offset - 14) & 0x1F] ^ 0xA5) ^ k;;

	k = rotateLeft(rotateKey[3], aStaticTable[(offset + 9) & 0x1F] ^ 0xA5);

	data[3] = rotateRight(data[3], aStaticTable[(offset - 11) & 0x1F] ^ 0xA5) ^ k;
}

static void encrypt_block(const MalieKey& keytable, uint8_t* block, size_t offset) {
	offset >>= 4;
	uint32_t *data32 = reinterpret_cast<uint32_t*>(block);
	rotateEnc(data32,keytable, offset);

	const uint8_t first = block[0];
	for (int i = 1; i < BlockLen; ++i) {
		block[i] ^= first;
	}
}

void MalieEncryption::encrypt(unsigned char* data, size_t data_length, size_t offset) const
{
	const size_t numOfBlocks = data_length / BlockLen;
	for (size_t line = 0; line < numOfBlocks; line++) {
		const auto start = data + line * BlockLen;

		encrypt_block(m_key, start, offset);

		offset += BlockLen;
	}
}

void MalieEncryption::decrypt(unsigned char* data, size_t data_length, size_t offset) const
{
	//Round 1
	const uint8_t szBlock = offset & 0xF;
	const uint8_t xorKey = data[szBlock];
	for (uint8_t iteBlock = 0; iteBlock < 0x10; iteBlock++)
	{
		uint8_t read = data[iteBlock];
		if (szBlock != iteBlock) {
			read ^= xorKey;
		}
		data[iteBlock] = read;
	}

	//Round 2
	uint32_t* const pDecBuffer = reinterpret_cast<uint32_t*>(data);
	const uint8_t shrOffset = static_cast<uint8_t>(offset >> 4);
	pDecBuffer[0] = __ROR4__
	(
		pDecBuffer[0] ^ __ROR4__(m_key[0], aStaticTable[(shrOffset + 3 * 0) & 0x1F] ^ 0xA5),
		aStaticTable[(shrOffset + 0x0C + 0x3 * 0x0) & 0x1F] ^ 0xA5
	);

	pDecBuffer[1] = __ROL4__
	(
		pDecBuffer[1] ^ __ROL4__(m_key[1], aStaticTable[(shrOffset + 3 * 1) & 0x1F] ^ 0xA5),
		aStaticTable[(shrOffset + 0x0C + 0x3 * 0x1) & 0x1F] ^ 0xA5
	);

	pDecBuffer[2] = __ROR4__
	(
		pDecBuffer[2] ^ __ROR4__(m_key[2], aStaticTable[(shrOffset + 3 * 2) & 0x1F] ^ 0xA5),
		aStaticTable[(shrOffset + 0x0C + 0xE0 + 0x3 * 0x2) & 0x1F] ^ 0xA5
	);

	pDecBuffer[3] = __ROL4__
	(
		pDecBuffer[3] ^ __ROL4__(m_key[3], aStaticTable[(shrOffset + 3 * 3) & 0x1F] ^ 0xA5),
		aStaticTable[(shrOffset + 0x0C + 0xE0 + 0x3 * 0x3) & 0x1F] ^ 0xA5
	);

	//return pDecBuffer[3];
}
