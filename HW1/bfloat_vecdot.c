#include <stdint.h>
#include <stdio.h>

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

void f32tobf16vec(float *A, int n, uint32_t *result) {
    int packed_size = n >> 1;
    for (int i = 0; i < packed_size; i++) {
        bf16_t bf16_a = fp32_to_bf16(A[i << 1]);
        bf16_t bf16_b = fp32_to_bf16(A[i << 1 + 1]);
        result[i] = bf16_encode(bf16_a, bf16_b);
    }
}

uint32_t imul16(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    for (int i = 0; i < 8; i++)
        if ((b >> i) & 1) r += a << i;
    r &= 0xFFFF;
    b >>= 16;
    a &= 0xFFFF0000;
    for (int i = 0; i < 8; i++)
        if ((b >> i) & 1) r += a << i;
    return r;
}

// uint32_t bf16_mul(uint32_t a, uint32_t b)
// {
//     uint32_t sr = (a ^ b) & 0x80008000;

//     uint32_t ma = (a & 0x007F007F) | 0x00800080;
//     uint32_t mb = (b & 0x007F007F) | 0x00800080;

//     uint32_t mr = (imul16(ma, mb) >> 7) & 0x007F007F;
//     uint32_t msh = (mr >> 8) & 1;
//     mr >>= msh;

//     uint32_t ea = (a >> 7) & 0x00FF00FF;
//     uint32_t eb = (b >> 7) & 0x00FF00FF;
//     uint32_t er = ea + eb - 0x007F007F; // 127 = 0b1111111 = 0x7F
//     er = msh ? imul16(er) : er;

//     // check overflow, under flow
//     uint32_t er_upper = (er >> 16) & 0xFF;
//     uint32_t mr_upper = (mr >> 16) & 0x7F;
//     if (er_upper >= 0xFF) {
//         er_upper = 0xFF;
//         mr_upper = 0;
//     } else if (er_upper <= 0) {
//         er_upper = 0;
//         mr_upper = 0;
//     }

//     uint32_t er_lower = er & 0xFF;
//     uint32_t mr_lower = mr & 0x7F;
//     if (er_lower >= 0xFF) {
//         er_lower = 0xFF;
//         mr_lower = 0;
//     } else if (er_lower <= 0) {
//         er_lower = 0;
//         mr_lower = 0;
//     }

//     uint32_t er_combined = (er_upper << 16) | er_lower;
//     uint32_t mr_combined = (mr_upper << 16) | mr_lower;

//     uint32_t r = sr | (er_combined << 7) | mr_combined;
//     return r;
// }

uint32_t mask_lowest_zero(uint32_t x) {
    uint32_t mask = x;
    mask &= (mask << 1) | 0x1;
    mask &= (mask << 2) | 0x3;
    mask &= (mask << 4) | 0xF;
    mask &= (mask << 8) | 0xFF;
    mask &= (mask << 16) | 0xFFFF;
    return mask;
}

int64_t inc(int64_t x) {
    if (~x == 0) return 0;
    /* TODO: Carry flag */
    int64_t mask = mask_lowest_zero(x);
    int64_t z1 = mask ^ ((mask << 1) | 1);
    return (x & ~mask) | z1;
}

uint32_t pbf16_mul(uint32_t a, uint32_t b) {
    uint32_t sr = (a ^ b) & 0x80008000;

    uint32_t ma = (a & 0x007F007F) | 0x00800080;
    uint32_t mb = (b & 0x007F007F) | 0x00800080;

    uint32_t mr = (imul16(ma, mb) >> 7) & 0x007F007F;
    uint32_t msh = (mr >> 8) & 1;
    mr >>= msh;

    uint32_t ea = (a >> 7) & 0x00FF00FF;
    uint32_t eb = (b >> 7) & 0x00FF00FF;
    uint32_t er = ea + eb - 0x007F007F;  // 127 = 0b1111111 = 0x7F
    er = msh ? inc(er) : er;

    uint32_t r = sr | ((er & 0x00FF00FF) << 7) | (mr & 0x007F007F);
    return r;
}

// BF16 加法函数
uint32_t bf16_add(uint32_t a, uint32_t b) {
    // 提取高 16 位和低 16 位的 BF16 数字
    bf16_t a_high = (bf16_t)(a >> 16);
    bf16_t a_low = (bf16_t)(a & 0xFFFF);
    bf16_t b_high = (bf16_t)(b >> 16);
    bf16_t b_low = (bf16_t)(b & 0xFFFF);

    // 分别对高位和低位进行 BF16 加法
    bf16_t result_high = bf16_add_single(a_high, b_high);
    bf16_t result_low = bf16_add_single(a_low, b_low);

    // 将结果打包回 uint32_t
    uint32_t result = ((uint32_t)result_high << 16) | result_low;
    return result;
}

// 单个 BF16 数字的加法
bf16_t bf16_add_single(bf16_t a, bf16_t b) {
    // 提取符号位、指数位和尾数位
    uint16_t sa = a >> 15;
    uint16_t sb = b >> 15;
    uint16_t ea = (a >> 7) & 0xFF;
    uint16_t eb = (b >> 7) & 0xFF;
    uint16_t ma = a & 0x7F;
    uint16_t mb = b & 0x7F;

    // 处理特殊情况（NaN，Infinity，Zero）
    if (ea == 0xFF) {
        if (ma != 0) return a;  // a 为 NaN
        if (eb == 0xFF) {
            if (mb != 0) return b;  // b 为 NaN
            if (sa == sb)
                return a;  // Inf + Inf，符号相同
            else {
                // Inf + (-Inf) = NaN
                return 0x7FC0;
            }
        }
        return a;  // a 为 Infinity
    }
    if (eb == 0xFF) {
        if (mb != 0) return b;  // b 为 NaN
        return b;               // b 为 Infinity
    }

    // 处理零
    if (ea == 0 && ma == 0) return b;
    if (eb == 0 && mb == 0) return a;

    // 添加隐含的最高位 1（BF16 没有隐含位，因此需要在尾数中添加）
    if (ea != 0)
        ma |= 0x80;
    else
        ea = 1;  // 次正规数

    if (eb != 0)
        mb |= 0x80;
    else
        eb = 1;  // 次正规数

    // 对齐指数
    int16_t exp_diff = ea - eb;
    if (exp_diff > 0) {
        // 移位 mb，使指数对齐
        mb >>= exp_diff;
        eb += exp_diff;
    } else if (exp_diff < 0) {
        // 移位 ma，使指数对齐
        ma >>= -exp_diff;
        ea -= exp_diff;
    }

    // 执行加法或减法
    uint16_t mr;
    uint16_t er = ea;
    uint16_t sr;

    if (sa == sb) {
        // 符号相同，执行加法
        mr = ma + mb;
        sr = sa;
        // 检查是否有进位，需要规范化
        if (mr & 0x100) {
            mr >>= 1;
            er = inc(er);
        }
    } else {
        // 符号不同，执行减法
        if (ma > mb) {
            mr = ma - mb;
            sr = sa;
        } else if (ma < mb) {
            mr = mb - ma;
            sr = sb;
        } else {
            // 结果为零
            return 0;
        }

        // 规范化结果
        while ((mr & 0x80) == 0 && er > 0) {
            mr <<= 1;
            er -= 1;
        }
    }

    // 处理指数上溢和下溢
    if (er >= 0xFF) {
        // 上溢，返回 Infinity
        return (sr << 15) | (0xFF << 7);
    } else if (er <= 0) {
        // 下溢，返回零
        return (sr << 15);
    }

    // 移除隐含的最高位 1
    mr &= 0x7F;

    // 组装结果
    bf16_t result = (sr << 15) | (er << 7) | mr;
    return result;
}

void print_fp32_vector_hex(float *A, int n) {
    union {
        float f;
        uint32_t u32;
    } converter;

    printf("FP32 Vector in Hex:\n");
    for (int i = 0; i < n; i++) {
        converter.f = A[i];
        printf("0x%08X\n", converter.u32);
    }
    printf("\n");
}

// int main() {
//     // float A[] = {1.2345f, -2.3456f, 3.4567f, -4.5678f, 5.6789f,
//     -6.7890f, 7.8901f, -8.9012f};
//     // int n = sizeof(A) / sizeof(A[0]);

//     // uint32_t packed_result[n / 2];

//     // f32tobf16vecdot(A, n, packed_result);

//     // print_fp32_vector_hex(A, n);
//     // printf("Packed BF16 Result:\n");
//     // for (int i = 0; i < n / 2; i++) {
//     //     printf("0x%08X\n", packed_result[i]);
//     // }
//     // float a_fp32[] = {1.5f, -2.5f, 1.5f, -2.5f};
//     // float b_fp32[] = {3.0f, 2.5f, 1.5f, -2.5f};
//     // uint32_t bf_a[];
//     // uint32_t bf_b[];

//     // f32tobf16vec(a_fp32, 4, bf16_a);
//     // f32tobf16vec(b_fp32, 4, bf16_b);

//     // uint32_t result = bf16_add(bf16_a, bf16_b);

//     return 0;
// }
int main() {
    // 定义两个 BF16 向量，每个包含两个元素
    float a_fp32[] = {1.5f, -2.5f};
    float b_fp32[] = {3.0f, 2.5f};

    // 转换为 BF16
    bf16_t a_bf16_high = fp32_to_bf16(a_fp32[0]);
    bf16_t a_bf16_low = fp32_to_bf16(a_fp32[1]);
    bf16_t b_bf16_high = fp32_to_bf16(b_fp32[0]);
    bf16_t b_bf16_low = fp32_to_bf16(b_fp32[1]);

    // 打包为 uint32_t
    uint32_t a_packed = bf16_encode(a_bf16_high, a_bf16_low);
    uint32_t b_packed = bf16_encode(b_bf16_high, b_bf16_low);

    // 进行 BF16 加法
    uint32_t result_packed = bf16_add(a_packed, b_packed);

    // 解包结果
    bf16_t result_high, result_low;
    bf16_decode(result_packed, &result_high, &result_low);

    // 转换回 FP32
    float result_fp32_high = bf16_to_fp32(result_high);
    float result_fp32_low = bf16_to_fp32(result_low);

    // 打印结果
    printf("BF16 Addition Results:\n");
    printf("High: %f + %f = %f\n", a_fp32[0], b_fp32[0], result_fp32_high);
    printf("Low : %f + %f = %f\n", a_fp32[1], b_fp32[1], result_fp32_low);

    // 验证结果
    float expected_high = a_fp32[0] + b_fp32[0];
    float expected_low = a_fp32[1] + b_fp32[1];
    printf("Expected Results:\n");
    printf("High: %f\n", expected_high);
    printf("Low : %f\n", expected_low);

    return 0;
}