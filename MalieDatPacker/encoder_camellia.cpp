#include "camellia_tables.h"

#include "encoder_camellia.h"


const int BitLen = 32;
const uint32_t ValueMask = (1u << BitLen) - 1;

// Функция для поворота битов вправо
uint32_t rotateRight(uint32_t value, int shift) {
    return (value >> shift) | (value << (BitLen - shift)) & ValueMask;
}

// Функция для поворота битов влево
uint32_t rotateLeft(uint32_t value, int shift) {
    return (value << shift) | (value >> (BitLen - shift)) & ValueMask;
}

uint32_t mutate_value(uint32_t value) {
    return (rotateLeft(value, 8) & 0x00FF00FF) | (rotateRight(value, 8) & 0xFF00FF00);
}

void mutate_block(uint32_t *block){
    block[0] = mutate_value(block[0]);
    block[1] = mutate_value(block[1]);
    block[2] = mutate_value(block[2]);
    block[3] = mutate_value(block[3]);
}

void encrypt_block(const CamelliaKey& keytable, unsigned char* block, int offset) {
    
    const int total_rounds = 3;

    uint32_t *dst_block = reinterpret_cast<uint32_t*>(block);

    mutate_block(dst_block);

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

    mutate_block(dst_block);

    uint32_t roll_bits = ((offset >> 4) & 0x0F) + 16;
    dst_block[0] = rotateRight(dst_block[0], roll_bits);
    dst_block[1] = rotateLeft(dst_block[1], roll_bits);
    dst_block[2] = rotateRight(dst_block[2], roll_bits);
    dst_block[3] = rotateLeft(dst_block[3], roll_bits);

}

void encrypt(const CamelliaKey& key, unsigned char* data, const int size, int offset, bool printed)
{
    int numOfBlocks = size / BlockLen;
    for (int line = 0; line < numOfBlocks; line++) {
        const auto start = data+line * BlockLen;

        encrypt_block(key,start, offset);

        offset += BlockLen;
    }

}
