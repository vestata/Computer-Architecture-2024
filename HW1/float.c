#include <stdint.h>
#include <stdio.h>
#define N 8

uint64_t mask_lowest_zero(uint64_t x) {
    uint64_t mask = x;
    mask &= (mask << 1) | 0x1;
    mask &= (mask << 2) | 0x3;
    mask &= (mask << 4) | 0xF;
    mask &= (mask << 8) | 0xFF;
    mask &= (mask << 16) | 0xFFFF;
    mask &= (mask << 32) | 0xFFFFFFFF;
    return mask;
}
int64_t inc(int64_t x) {
    if (~x == 0) return 0;
    /* TODO: Carry flag */
    int64_t mask = mask_lowest_zero(x);
    int64_t z1 = mask ^ ((mask << 1) | 1);
    return (x & ~mask) | z1;
}
static inline int64_t getbit(int64_t value, int n) { return (value >> n) & 1; }

/* int32 multiply
 */
int64_t imul32(int32_t a, int32_t b) {
    int64_t r = 0, a64 = (int64_t)a, b64 = (int64_t)b;
    for (int i = 0; i < 32; i++) {
        if (getbit(b64, i)) r += a64 << i;
    }
    return r;
}
/* float32 multiply */
float fmul32(float a, float b) {
    /* TODO: Special values like NaN and INF */
    int32_t ia = *(int32_t *)&a, ib = *(int32_t *)&b;

    /* sign */
    int sa = ia >> 31;
    int sb = ib >> 31;

    /* mantissa */
    int32_t ma = (ia & 0x7FFFFF) | 0x800000;
    int32_t mb = (ib & 0x7FFFFF) | 0x800000;

    /* exponent */
    int32_t ea = ((ia >> 23) & 0xFF);
    int32_t eb = ((ib >> 23) & 0xFF);

    // handle A NAN
    if (ea == 0xFF && ma != 0x800000) {
        int32_t f_nan = 0x7FF80001;
        return *(float *)&f_nan;
    }
    // handle B NAN
    if (eb == 0xFF && mb != 0x800000) {
        int32_t f_nan = 0x7FF80001;
        return *(float *)&f_nan;
    }
    // Infinities
    if (ea == 0xFF && ma == 0x800000) {
        if (eb == 0) {
            int32_t f_nan = 0x7F800001;
            return *(float *)&f_nan;
        } else {
            int32_t f_inf = 0x7F800000 | (sa ^ sb) << 31;
            return *(float *)&f_inf;
        }
    }
    if (eb == 0xFF && mb == 0x800000) {
        if (ea == 0) {
            int32_t f_nan = 0x7F800001;
            return *(float *)&f_nan;
        } else {
            int32_t f_inf = 0x7F800000 | (sa ^ sb) << 31;
            return *(float *)&f_inf;
        }
    }
    // handle zero return
    if (ea == 0 || eb == 0) {
        int32_t f_zero = 0 | (sa ^ sb) << 31;
        return *(float *)&f_zero;
    }

    /* 'r' = result */
    int64_t mrtmp = imul32(ma, mb) >> 23;
    int mshift = getbit(mrtmp, 24);

    int64_t mr = mrtmp >> mshift;
    int32_t ertmp = ea + eb - 127;
    int32_t er = mshift ? inc(ertmp) : ertmp;
    int sr = sa ^ sb;
    /* TODO: Overflow ^ */
    if (er >= 255) {
        // Overflow, return infinity
        int32_t f_inf = (sr << 31) | 0x7F800000;
        return *(float *)&f_inf;
    } else if (er <= 0) {
        // Underflow, return zero
        int32_t f_zero = (sr << 31);
        return *(float *)&f_zero;
    }

    int32_t r = (sr << 31) | ((er & 0xFF) << 23) | (mr & 0x7FFFFF);
    return *(float *)&r;
}

/* float32 addition */
float fadd32(float a, float b) {
    /* Extract integer representations */
    int32_t ia = *(int32_t *)&a;
    int32_t ib = *(int32_t *)&b;

    /* Extract sign bits */
    int sa = ia >> 31;
    int sb = ib >> 31;

    /* Extract exponents */
    int32_t ea = (ia >> 23) & 0xFF;
    int32_t eb = (ib >> 23) & 0xFF;

    /* Extract mantissas */
    int32_t ma = ia & 0x7FFFFF;
    int32_t mb = ib & 0x7FFFFF;

    /* Handle special cases */
    // NaNs
    if (ea == 0xFF && ma != 0) return a;  // a is NaN
    if (eb == 0xFF && mb != 0) return b;  // b is NaN

    // Infinities
    if (ea == 0xFF) {
        if (eb == 0xFF) {
            // Inf + Inf
            if (sa == sb)
                return a;  // Same sign, return Inf
            else {
                // Inf + (-Inf) = NaN
                int32_t f_nan = 0x7FC00000;
                return *(float *)&f_nan;
            }
        } else
            return a;  // Inf + finite
    }
    if (eb == 0xFF) return b;  // finite + Inf

    // Zeros
    if (ea == 0 && ma == 0) return b;  // a is zero
    if (eb == 0 && mb == 0) return a;  // b is zero

    /* Normalize mantissas if exponents are not zero */
    if (ea != 0)
        ma |= 0x800000;
    else
        ea = 1;  // Denormalized number

    if (eb != 0)
        mb |= 0x800000;
    else
        eb = 1;  // Denormalized number

    /* Align exponents */
    int32_t exp_diff = ea - eb;
    if (exp_diff > 0) {
        // Shift mb to align exponents
        mb >>= exp_diff;
        eb += exp_diff;
    } else if (exp_diff < 0) {
        // Shift ma to align exponents
        ma >>= -exp_diff;
        ea -= exp_diff;
    }

    /* Perform addition or subtraction */
    int32_t mr;
    int32_t er = ea;
    int sr;

    if (sa == sb) {
        // Same sign, perform addition
        mr = ma + mb;
        sr = sa;
        // Check for carry and normalize
        if (mr & 0x1000000) {
            mr >>= 1;
            er += 1;
        }
    } else {
        // Different signs, perform subtraction
        if (ma > mb) {
            mr = ma - mb;
            sr = sa;
        } else if (ma < mb) {
            mr = mb - ma;
            sr = sb;
        } else {
            // Result is zero
            int32_t f_zero = 0 | (sa & sb) << 31;
            return *(float *)&f_zero;
        }

        // Normalize result
        while ((mr & 0x800000) == 0 && er > 0) {
            mr <<= 1;
            er -= 1;
        }
    }

    /* Handle exponent overflow/underflow */
    if (er >= 255) {
        // Overflow, return infinity
        int32_t f_inf = (sr << 31) | 0x7F800000;
        return *(float *)&f_inf;
    } else if (er <= 0) {
        // Underflow, return zero
        int32_t f_zero = (sr << 31);
        return *(float *)&f_zero;
    }

    /* Remove implicit leading 1 from mantissa */
    mr &= 0x7FFFFF;

    /* Assemble the result */
    int32_t r = (sr << 31) | (er << 23) | mr;
    return *(float *)&r;
}

void print_matrix(float matrix[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%8.4f ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_float(float f) {
    union {
        float f;
        uint32_t i;
    } u;
    u.f = f;

    // 以 16 進位輸出內部表示
    printf("float (in hex): 0x%08x\n", u.i);
}

void matrix_multiply_custom(float A[N][N], float B[N][N], float C[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                float prod = fmul32(A[i][k], B[k][j]);
                sum = fadd32(sum, prod);
            }
            C[i][j] = sum;
        }
    }
}

void matrix_multiply_standard(float A[N][N], float B[N][N], float C[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

int main() {
    float A[N][N];
    float B[N][N];
    float C[N][N];
    float C_std[N][N];

    // 初始化 A 和 B 矩陣為 3.14159
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = 3.14159f;
            B[i][j] = 3.14159f;
        }
    }

    matrix_multiply_custom(A, B, C);
    matrix_multiply_custom(A, B, C_std);

    printf("Resultant costum matrix C:\n");
    print_matrix(C);
    printf("Resultant std matrix C:\n");
    print_matrix(C_std);

    return 0;
}