#include <stdint.h>
#include <stdio.h>

int my_clz64(uint64_t x) {
    if (x == 0) return 64;
    int n = 0;
    if (x <= 0x00000000FFFFFFFF) {
        n += 32;
        x <<= 32;
    }
    if (x <= 0x0000FFFFFFFFFFFF) {
        n += 16;
        x <<= 16;
    }
    if (x <= 0x00FFFFFFFFFFFFFF) {
        n += 8;
        x <<= 8;
    }
    if (x <= 0x0FFFFFFFFFFFFFFF) {
        n += 4;
        x <<= 4;
    }
    if (x <= 0x3FFFFFFFFFFFFFFF) {
        n += 2;
        x <<= 2;
    }
    if (x <= 0x7FFFFFFFFFFFFFFF) {
        n += 1;
        x <<= 1;
    }
    return n;
}

int ilog2(long long n) { return 63 - my_clz64(n); }

void printll(long long num) {
    printf("low: %d\nhigh: %d\n", (uint32_t)((num << 32) >> 32),
           (uint32_t)(num >> 32));
}

char kthCharacter(long long k, int* operations, int operationsSize) {
    int mutations = 0;
    for (int op = ilog2(k); op >= 0; op--)
        if (k > 1LL << op) {
            k -= 1LL << op;
            mutations += operations[op];
        }
    return 'a' + mutations % 26;
}

void run_test_case(long long k, int* operations, int operationsSize,
                   char expected_result) {
    char result = kthCharacter(k, operations, operationsSize);
    printf("kthCharacter(%lld) = %c, Expected = %c --> %s\n", k, result,
           expected_result, result == expected_result ? "PASS" : "FAIL");
}

int main() {
    // Test case 1
    int op1[] = {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0,
                 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1};
    long long k1 = 0x2D3E80C15;
    int n1 = 35;
    char expected1 = 'i';

    // Test case 2
    int op2[] = {0, 1, 0, 1};
    long long k2 = 10;
    int n2 = 4;
    char expected2 = 'b';

    // Test case 3
    int op3[] = {1, 1, 1, 0, 1, 0, 1, 0, 1, 0};
    long long k3 = 25;
    int n3 = 10;
    char expected3 = 'b';

    // Run test cases
    run_test_case(k1, op1, n1, expected1);
    run_test_case(k2, op2, n2, expected2);
    run_test_case(k3, op3, n3, expected3);
    return 0;
}