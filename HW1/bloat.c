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

uint32_t pbf16_encode(bf16_t a, bf16_t b) { return ((uint32_t)a << 16) | b; }

void pack_fp32_matrix_to_packed_bf16_row_major(float matrix[N][N],
                                               uint32_t packed_matrix[N][n]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < n; j++) {
            bf16_t bf16_a = fp32_to_bf16(matrix[i][j << 1]);
            bf16_t bf16_b = fp32_to_bf16(matrix[i][j << 1 + 1]);
            packed_matrix[i][j] = pbf16_encode(bf16_a, bf16_b);
        }
    }
}

void pack_fp32_matrix_to_packed_bf16_column_major(
    float matrix[N][N], uint32_t packed_matrix[n][N]) {
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < n; i++) {
            bf16_t bf16_a = fp32_to_bf16(matrix[i << 1][j]);
            bf16_t bf16_b = fp32_to_bf16(matrix[i << 1 | 1][j]);
            packed_matrix[i][j] = pbf16_encode(bf16_a, bf16_b);
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
