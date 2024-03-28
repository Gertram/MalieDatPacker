#include "camellia_tables.h"

#include "camellia_encryption.h"


static const int BitLen = 32;
static const uint32_t ValueMask = (1u << BitLen) - 1;

static uint32_t rotateRight(uint32_t value, int shift) {
    return (value >> shift) | (value << (BitLen - shift)) & ValueMask;
}

static uint32_t rotateLeft(uint32_t value, int shift) {
    return (value << shift) | (value >> (BitLen - shift)) & ValueMask;
}

static uint32_t mutate_value(uint32_t value) {
    return (rotateLeft(value, 8) & 0x00FF00FF) | (rotateRight(value, 8) & 0xFF00FF00);
}

static void MutateBlock(uint32_t *block){
    block[0] = mutate_value(block[0]);
    block[1] = mutate_value(block[1]);
    block[2] = mutate_value(block[2]);
    block[3] = mutate_value(block[3]);
}

//https://github.com/satan53x/SExtractor/blob/main/tools/Malie/encoder_camellia.py
static void encrypt_block(const CamelliaKey& keytable, unsigned char* block, int offset) {
    
    const int total_rounds = 3;

    uint32_t *dst_block = reinterpret_cast<uint32_t*>(block);

    MutateBlock(dst_block);

    int k = 51;
    dst_block[0] ^= keytable[k - 3];
    dst_block[1] ^= keytable[k - 2];
    dst_block[2] ^= keytable[k - 1];
    dst_block[3] ^= keytable[k];

    k -= 4;

    for (uint32_t i = 0; i < total_rounds; ++i) {
        for (uint32_t j = 0; j < total_rounds; ++j) {
            uint32_t temp = keytable[k - 3] ^ dst_block[0];
            uint32_t U = (SBOX3_3033[(temp >> 8) & 0xFF] ^ SBOX4_4404[temp & 0xFF]
                ^ SBOX2_0222[(temp >> 16) & 0xFF] ^ SBOX1_1110[(temp >> 24)]);

            temp = keytable[k - 2] ^ dst_block[1];
            uint32_t D = (SBOX4_4404[(temp >> 8) & 0xFF] ^ SBOX1_1110[temp & 0xFF]
                ^ SBOX3_3033[(temp >> 16) & 0xFF] ^ SBOX2_0222[(temp >> 24)]);

            dst_block[2] ^= U ^ D;
            dst_block[3] ^= U ^ D ^ rotateRight(U, 8);

            temp = keytable[k - 1] ^ dst_block[2];
            U = (SBOX3_3033[(temp >> 8) & 0xFF] ^ SBOX4_4404[temp & 0xFF]
                ^ SBOX2_0222[(temp >> 16) & 0xFF] ^ SBOX1_1110[(temp >> 24)]);

            temp = keytable[k] ^ dst_block[3];
            D = (SBOX4_4404[(temp >> 8) & 0xFF] ^ SBOX1_1110[temp & 0xFF]
                ^ SBOX3_3033[(temp >> 16) & 0xFF] ^ SBOX2_0222[(temp >> 24)]);

            dst_block[0] ^= U ^ D;
            dst_block[1] ^= U ^ D ^ rotateRight(U, 8);

            k -= 4;
        }

        if (i < total_rounds - 1) {
            dst_block[1] ^= rotateLeft(dst_block[0] & keytable[k - 3], 1);
            dst_block[0] ^= (dst_block[1] | keytable[k - 2]);
            dst_block[2] ^= (dst_block[3] | keytable[k]);
            dst_block[3] ^= rotateLeft(dst_block[2] & keytable[k - 1], 1);
            k -= 4;
        }
    }

    std::swap(dst_block[0], dst_block[2]);
    std::swap(dst_block[1], dst_block[3]);

    dst_block[0] ^= keytable[k - 3];
    dst_block[1] ^= keytable[k - 2];
    dst_block[2] ^= keytable[k - 1];
    dst_block[3] ^= keytable[k];

    MutateBlock(dst_block);

    uint32_t roll_bits = ((offset >> 4) & 0x0F) + 16;
    dst_block[0] = rotateRight(dst_block[0], roll_bits);
    dst_block[1] = rotateLeft(dst_block[1], roll_bits);
    dst_block[2] = rotateRight(dst_block[2], roll_bits);
    dst_block[3] = rotateLeft(dst_block[3], roll_bits);

}

///https://github.com/Dir-A/MalieTools

static uint32_t MutateValue(uint32_t Value)
{
    return (_rotl(Value, 8) & 0x00FF00FF) | (_rotr(Value, 8) & 0xFF00FF00);
}

static void MutateBlock(uint32_t* pSrc, uint32_t* pMutated)
{
    pMutated[0] = MutateValue(pSrc[0]);
    pMutated[1] = MutateValue(pSrc[1]);
    pMutated[2] = MutateValue(pSrc[2]);
    pMutated[3] = MutateValue(pSrc[3]);
}

#define BYTE1(x) ((x      ) & 0xFF)
#define BYTE2(x) ((x >>  8) & 0xFF)
#define BYTE3(x) ((x >> 16) & 0xFF)
#define BYTE4(x) ((x >> 24) & 0xFF)

static void decrypt_block(const CamelliaKey &decrypt_key, unsigned char* pBuffer, size_t posOffset)
{
    uint32_t  dst_block[4] = { 0 };
    uint32_t* src_block = (uint32_t*)pBuffer;

    auto key = decrypt_key.data();
    const uint32_t  iterations = 3;

    uint32_t roll_bits = ((posOffset >> 4) & 0x0F) + 16;

    dst_block[0] = _rotl(src_block[0], roll_bits);
    dst_block[1] = _rotr(src_block[1], roll_bits);
    dst_block[2] = _rotl(src_block[2], roll_bits);
    dst_block[3] = _rotr(src_block[3], roll_bits);

    MutateBlock(dst_block, dst_block);

    dst_block[0] ^= key[0];
    dst_block[1] ^= key[1];
    dst_block[2] ^= key[2];
    dst_block[3] ^= key[3];

    key += 4;

    for (size_t i = 0; i < iterations; i++)
    {
        for (uint32_t j = 0; j < iterations; j++)
        {
            // Feistel rounds
            uint32_t temp = key[2] ^ dst_block[0];
            uint32_t scramble1 = SBOX4_4404[BYTE1(temp)] ^ SBOX3_3033[BYTE2(temp)] ^
                SBOX2_0222[BYTE3(temp)] ^ SBOX1_1110[BYTE4(temp)];

            temp = key[3] ^ dst_block[1];
            uint32_t scramble2 = SBOX1_1110[BYTE1(temp)] ^ SBOX4_4404[BYTE2(temp)] ^
                SBOX3_3033[BYTE3(temp)] ^ SBOX2_0222[BYTE4(temp)];

            dst_block[2] ^= scramble1 ^ scramble2;
            dst_block[3] ^= scramble1 ^ scramble2 ^ _rotr(scramble1, 8);

            temp = key[0] ^ dst_block[2];
            uint32_t scramble3 = SBOX4_4404[BYTE1(temp)] ^ SBOX3_3033[BYTE2(temp)] ^
                SBOX2_0222[BYTE3(temp)] ^ SBOX1_1110[BYTE4(temp)];

            temp = key[1] ^ dst_block[3];
            uint32_t scramble4 = SBOX1_1110[BYTE1(temp)] ^ SBOX4_4404[BYTE2(temp)] ^
                SBOX3_3033[BYTE3(temp)] ^ SBOX2_0222[BYTE4(temp)];

            dst_block[0] ^= scramble3 ^ scramble4;
            dst_block[1] ^= scramble3 ^ scramble4 ^ _rotr(scramble3, 8);

            key += 4;
        }

        if (i < iterations - 1) {
            dst_block[1] ^= _rotl(key[2] & dst_block[0], 1);
            dst_block[0] ^= dst_block[1] | key[3];
            dst_block[2] ^= dst_block[3] | key[1];
            dst_block[3] ^= _rotl(key[0] & dst_block[2], 1);

            key += 4;
        }
    }

    std::swap(dst_block[0], dst_block[2]);
    std::swap(dst_block[1], dst_block[3]);

    dst_block[0] ^= key[0];
    dst_block[1] ^= key[1];
    dst_block[2] ^= key[2];
    dst_block[3] ^= key[3];

    MutateBlock(dst_block, src_block);
}

void CamelliaEncryption::encrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const
{
    int numOfBlocks = data_length / BlockLen;
    for (int line = 0; line < numOfBlocks; line++) {
        const auto start = data + line * BlockLen;

        encrypt_block(m_key, start, offset);

        offset += BlockLen;
    }
}

void CamelliaEncryption::decrypt(unsigned char* data, unsigned int data_length, unsigned int offset) const
{
    int numOfBlocks = data_length / BlockLen;
    for (int line = 0; line < numOfBlocks; line++) {
        const auto start = data + line * BlockLen;

        decrypt_block(m_key, start, offset);

        offset += BlockLen;
    }
}
