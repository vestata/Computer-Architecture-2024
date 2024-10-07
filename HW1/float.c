#include <stdio.h>

#include <stdint.h>
uint64_t mask_lowest_zero(uint64_t x)
{
    uint64_t mask = x;
    mask &= (mask << 1) | 0x1;
    mask &= (mask << 2) | 0x3;
    mask &= (mask << 4) | 0xF;
    mask &= (mask << 8) | 0xFF;
    mask &= (mask << 16) | 0xFFFF;
    mask &= (mask << 32) | 0xFFFFFFFF;
    return mask;
}
int64_t inc(int64_t x)
{
    if (~x == 0)
        return 0;
    /* TODO: Carry flag */
    int64_t mask = mask_lowest_zero(x);
    int64_t z1 = mask ^ ((mask << 1) | 1);
    return (x & ~mask) | z1;
}
static inline int64_t getbit(int64_t value, int n)
{
    return (value >> n) & 1;
}
/* int32 multiply */
int64_t imul32(int32_t a, int32_t b)
{
    int64_t r = 0, a64 = (int64_t) a, b64 = (int64_t) b;
    for (int i = 0; i < 32; i++) {
        if (getbit(b64, i))
            r += a64 << i;
    }
    return r;
}
/* float32 multiply */
float fmul32(float a, float b)
{
    /* TODO: Special values like NaN and INF */
    int32_t ia = *(int32_t *) &a, ib = *(int32_t *) &b;

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
    if(ea == 0xFF && ma != 0x800000) {
        int32_t f_nan = 0x7FF80001;
        return *(float *) &f_nan;
    }
    // handle B NAN
    if(eb == 0xFF && mb != 0x800000) {
        int32_t f_nan = 0x7FF80001;
        return *(float *) &f_nan;
    }
    
    if(ea == 0xFF && ma == 0x800000) {
        if(eb == 0) {
            int32_t f_nan = 0x7F800001;
            return *(float *) &f_nan;
        }
        else {
            int32_t f_inf = 0x7F800000 | (sa ^ sb) << 31;
            return *(float *) &f_inf;
        }
    }
    if(eb == 0xFF && mb == 0x800000) {
        if(ea == 0) {
            int32_t f_nan = 0x7F800001;
            return *(float *) &f_nan;
        }
        else {
            int32_t f_inf = 0x7F800000 | (sa ^ sb) << 31;
            return *(float *) &f_inf;
        }
    }
    // handle zero return 
    if(ea == 0 || eb == 0) {
        int32_t f_zero = 0 | (sa ^ sb) << 31;
        return *(float *) &f_zero;
    }

    /* 'r' = result */
    int64_t mrtmp = imul32(ma, mb) >> 23;
    int mshift = getbit(mrtmp, 24);

    int64_t mr = mrtmp >> mshift;
    int32_t ertmp = ea + eb - 127;
    int32_t er = mshift ? inc(ertmp) : ertmp;
    /* TODO: Overflow ^ */
    int sr = sa ^ sb;
    int32_t r = (sr << 31) | ((er & 0xFF) << 23) | (mr & 0x7FFFFF);
    return *(float *) &r;
}


void print_float(float f) {
    union {
        float f;
        uint32_t i;
    } u;
    u.f = f;
    
    // 以 16 進位輸出內部表示
    printf("float (in hex): 0x%x\n", u.i);
}


int main(){
    float a = 0x7FA00000;
    float b = 0.5;
    printf("std:%f\n", a * b);
    print_float(fmul32(a, b));
    // printf("my:%f\n", print_float(fmul32(a, b)));

    return 0;
}