#include <math.h>
#include <stdint.h>
#include <stdio.h>

typedef uint16_t bf16_t;

// 将 FP32 转换为 BF16 的函数
static inline bf16_t fp32_to_bf16(float s) {
    union {
        float f;
        uint32_t u32;
    } u = {.f = s};

    uint32_t sign = u.u32 & 0x80000000;
    uint32_t exponent = u.u32 & 0x7F800000;
    uint32_t mantissa = u.u32 & 0x007FFFFF;

    // 处理 NaN 和无穷大
    if (exponent == 0x7F800000) {
        if (mantissa != 0) {
            // NaN
            return (bf16_t)((sign >> 16) | 0x7FC0);
        } else {
            // Infinity
            return (bf16_t)((sign >> 16) | 0x7F80);
        }
    }

    // 正常数值，舍入到最近的偶数
    uint32_t rounding_bias = 0x00007FFF + ((u.u32 >> 16) & 1);
    u.u32 += rounding_bias;
    bf16_t bf16 = (uint16_t)(u.u32 >> 16);

    return bf16;
}

// 将两个 BF16 值打包为一个 32 位整数
uint32_t pbf16_encode(bf16_t a, bf16_t b) { return ((uint32_t)a << 16) | b; }

// 将 4x4 的 FP32 矩阵 A 打包为 4x2 的 BF16 矩阵（行优先）
void pack_fp32_matrix_to_packed_bf16_row_major(float matrix[4][4],
                                               uint32_t packed_matrix[4][2]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            bf16_t bf16_a = fp32_to_bf16(matrix[i][j * 2]);
            bf16_t bf16_b = fp32_to_bf16(matrix[i][j * 2 + 1]);
            packed_matrix[i][j] = pbf16_encode(bf16_a, bf16_b);
        }
    }
}

// 将 4x4 的 FP32 矩阵 B 打包为 2x4 的 BF16 矩阵（列优先）
void pack_fp32_matrix_to_packed_bf16_column_major(
    float matrix[4][4], uint32_t packed_matrix[2][4]) {
    for (int j = 0; j < 4; j++) {  // 对于每一列
        // 打包列中的前两个元素
        bf16_t bf16_a = fp32_to_bf16(matrix[0][j]);
        bf16_t bf16_b = fp32_to_bf16(matrix[1][j]);
        packed_matrix[0][j] = pbf16_encode(bf16_a, bf16_b);

        // 打包列中的后两个元素
        bf16_t bf16_c = fp32_to_bf16(matrix[2][j]);
        bf16_t bf16_d = fp32_to_bf16(matrix[3][j]);
        packed_matrix[1][j] = pbf16_encode(bf16_c, bf16_d);
    }
}

// 打印打包的 BF16 矩阵
void print_packed_bf16_matrix_row_major(uint32_t packed_matrix[4][2]) {
    printf("Packed BF16 Matrix (Row-Major):\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            printf("0x%08X ", packed_matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_packed_bf16_matrix_column_major(uint32_t packed_matrix[2][4]) {
    printf("Packed BF16 Matrix (Column-Major):\n");
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            printf("0x%08X ", packed_matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_fp32_matrix(float matrix[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main() {
    float A[4][4] = {{1.2345f, -2.3456f, 3.4567f, -4.5678f},
                     {5.6789f, -6.7890f, 7.8901f, -8.9012f},
                     {9.0123f, -0.1234f, 1.2345f, -2.3456f},
                     {3.4567f, -4.5678f, 5.6789f, -6.7890f}};

    float B[4][4] = {{7.8901f, -8.9012f, 9.0123f, -0.1234f},
                     {1.2345f, -2.3456f, 3.4567f, -4.5678f},
                     {5.6789f, -6.7890f, 7.8901f, -8.9012f},
                     {9.0123f, -0.1234f, 1.2345f, -2.3456f}};

    uint32_t packed_A[4][2];  // 4x2
    uint32_t packed_B[2][4];  // 2x4

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
