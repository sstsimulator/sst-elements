#include <stdint.h>
#include <stdio.h>

#define NUM_TESTS 4

int main()
{
    uint32_t x;
    float test_cases_a[NUM_TESTS] = {2.5e-6, 4.231e-11, 6.77e3, 8.98e12};
    float test_cases_b[NUM_TESTS] = {1.0e-6, 4.0e-11, 6.0e3, 8.0e12};
    float add_result[NUM_TESTS]   = {3.49999982063e-06, 8.23099990943e-11, 12770.0000, 1.69800001126e+13};
    float sub_result[NUM_TESTS]   = {1.49999993937e-06, 2.30999941397e-12, 770.000000, 9.79999653888e+11};
    float mul_result[NUM_TESTS]   = {2.49999999001e-12, 1.69239992125e-21, 40620000.0, 7.18399962448e+25};
    float div_result[NUM_TESTS]   = {2.5, 1.05774998665, 1.12833333015, 1.12249994278};

    register float a asm("fs2") = 2.5e-6;
    register float b asm("fs3") = 1.0e-6;
    register float c asm("ft0");
    register float d asm("ft1");

    register uint64_t i asm("a2") = 99;
    register uint64_t j asm("a3") = 400;

    __asm__ __volatile__ ("ADD_TEST_:");
    for( x = 0; x < NUM_TESTS; ++x ) {
        a = test_cases_a[x];
        b = test_cases_b[x];
        d = add_result[x];
        __asm__ __volatile__ ("fadd.s  %0, %1, %2;" : "=f"(c) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(i) : "f"(c) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(j) : "f"(d) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e %lx %lx\n", a, b, c, i, j);
    }

    __asm__ __volatile__ ("SUB_TEST_:");
    for( x = 0; x < NUM_TESTS; ++x ) {
        a = test_cases_a[x];
        b = test_cases_b[x];
        d = sub_result[x];
        __asm__ __volatile__ ("fsub.s %0, %1, %2;" : "=f"(c) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(i) : "f"(c) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(j) : "f"(d) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e %lx %lx\n", a, b, c, i, j);
    }

    __asm__ __volatile__ ("MUL_TEST_:");
    for( x = 0; x < NUM_TESTS; ++x ) {
        a = test_cases_a[x];
        b = test_cases_b[x];
        d = mul_result[x];
        __asm__ __volatile__ ("fmul.s  %0, %1, %2;" : "=f"(c) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(i) : "f"(c) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(j) : "f"(d) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e %lx %lx\n", a, b, c, i, j);
    }

    __asm__ __volatile__ ("DIV_TEST_:");
    for( x = 0; x < NUM_TESTS; ++x ) {
        a = test_cases_a[x];
        b = test_cases_b[x];
        d = div_result[x];
        __asm__ __volatile__ ("fdiv.s  %0, %1, %2;" : "=f"(c) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(i) : "f"(c) );
        __asm__ __volatile__ ("fmv.x.s %0, %1;" : "=r"(j) : "f"(d) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e %lx %lx\n", a, b, c, i, j);
    }

    __asm__ __volatile__ ("beq zero, zero, FINIS_;");

    __asm__ __volatile__ ("FAIL_:");
    printf("FAIL %e %e %e %lu %lu\n", a, b, c, i, j);

    __asm__ __volatile__ ("FINIS_:");

}
