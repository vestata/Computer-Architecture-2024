#include <math.h>
#include <stdint.h>
#include <stdio.h>
#define N 4
#define n N >> 1

typedef uint16_t bf16_t;

static inline bf16_t fp32_to_bf16(float s) {
    union {
        float f;
        uint32_t u32;
    } u = {.f = s};

    uint32_t sign = u.u32 & 0x80000000;
    uint32_t exponent = u.u32 & 0x7F800000;
    uint32_t mantissa = u.u32 & 0x007FFFFF;

    if (exponent == 0x7F800000) {
        if (mantissa != 0) {
            // NaN
            return (bf16_t)((sign >> 16) | 0x7FC0);
        } else {
            // Infinity
            return (bf16_t)((sign >> 16) | 0x7F80);
        }
    }

    uint32_t rounding_bias = 0x00007FFF + ((u.u32 >> 16) & 1);
    u.u32 += rounding_bias;
    bf16_t bf16 = (uint16_t)(u.u32 >> 16);

    return bf16;
}

uint32_t bf16_encode(bf16_t a, bf16_t b) { return ((uint32_t)a << 16) | b; }

void pack_fp32_matrix_to_packed_bf16_row_major(float matrix[N][N],
                                               uint32_t packed_matrix[N][n]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < n; j++) {
            bf16_t bf16_a = fp32_to_bf16(matrix[i][j << 1]);
            bf16_t bf16_b = fp32_to_bf16(matrix[i][j << 1 + 1]);
            packed_matrix[i][j] = bf16_encode(bf16_a, bf16_b);
        }
    }
}

void pack_fp32_matrix_to_packed_bf16_column_major(
    float matrix[N][N], uint32_t packed_matrix[n][N]) {
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < n; i++) {
            bf16_t bf16_a = fp32_to_bf16(matrix[i << 1][j]);
            bf16_t bf16_b = fp32_to_bf16(matrix[i << 1 | 1][j]);
            packed_matrix[i][j] = bf16_encode(bf16_a, bf16_b);
        }
    }
}

void print_packed_bf16_matrix_row_major(uint32_t packed_matrix[N][n]) {
    printf("Packed BF16 Matrix (Row-Major):\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < n; j++) {
            printf("0x%08X ", packed_matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_packed_bf16_matrix_column_major(uint32_t packed_matrix[2][4]) {
    printf("Packed BF16 Matrix (Column-Major):\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < N; j++) {
            printf("0x%08X ", packed_matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}


uint32_t mask_lowest_zero(uint32_t x)
{
    uint32_t mask = x;
    mask &= (mask << 1) | 0x1;
    mask &= (mask << 2) | 0x3;
    mask &= (mask << 4) | 0xF;
    mask &= (mask << 8) | 0xFF;
    mask &= (mask << 16) | 0xFFFF;
    return mask;
}

uint32_t inc(uint32_t x)
{
    if (~x == 0)
        return 0;
    /* TODO: Carry flag */
    uint32_t mask = mask_lowest_zero(x);
    uint32_t z1 = mask ^ ((mask << 1) | 1);
    return (x & ~mask) | z1;
}


uint32_t imul16(uint32_t a, uint32_t b)
{
    uint32_t r = 0;
    for (int i = 0; i < 8; i++)
        if ((b >> i) & 1)
            r += a << i;
    r &= 0xFFFF;
    b >>= 16;
    a &= 0xFFFF0000;
    for (int i = 0; i < 8; i++)
        if ((b >> i) & 1)
            r += a << i;
    return r;
}


bf16_t bf16_mul(bf16_t a, bf16_t b)
{
    uint32_t sr = (a ^ b) & 0x80008000;

    uint32_t ma = (a & 0x007F007F) | 0x00800080;
    uint32_t mb = (b & 0x007F007F) | 0x00800080;

    uint32_t mr = (imul16(ma, mb) >> 7) & 0x007F007F;
    uint32_t msh = (mr >> 8) & 1;
    mr >>= msh;

    uint32_t ea = (a >> 7) & 0x00FF00FF;
    uint32_t eb = (b >> 7) & 0x00FF00FF;
    uint32_t er = ea + eb - 0x007F007F; // 127 = 0b1111111 = 0x7F
    er = msh ? inc(er) : er;

    bf16_t r = sr | ((er & 0x00FF00FF) << 7) | (mr & 0x007F007F);
    return r;
}


void print_fp32_matrix(float matrix[4][4]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main() {
    float A[N][N] = {{1.2345f, -2.3456f, 3.4567f, -4.5678f},
                     {5.6789f, -6.7890f, 7.8901f, -8.9012f},
                     {9.0123f, -0.1234f, 1.2345f, -2.3456f},
                     {3.4567f, -4.5678f, 5.6789f, -6.7890f}};

    float B[N][N] = {{7.8901f, -8.9012f, 9.0123f, -0.1234f},
                     {1.2345f, -2.3456f, 3.4567f, -4.5678f},
                     {5.6789f, -6.7890f, 7.8901f, -8.9012f},
                     {9.0123f, -0.1234f, 1.2345f, -2.3456f}};

    uint32_t packed_A[N][n];  // 4x2
    uint32_t packed_B[n][N];  // 2x4

    pack_fp32_matrix_to_packed_bf16_row_major(A, packed_A);
    pack_fp32_matrix_to_packed_bf16_column_major(B, packed_B);

    printf("Matrix A:\n");
    print_fp32_matrix(A);
    printf("Packed Matrix A (Row-Major):\n");
    print_packed_bf16_matrix_row_major(packed_A);

    printf("Matrix B:\n");
    print_fp32_matrix(B);
    printf("Packed Matrix B (Column-Major):\n");
    print_packed_bf16_matrix_column_major(packed_B);

    return 0;
}
