#include <stdint.h>
#include <stdio.h>

#define NUM_TESTS 4

int main()
{
    uint32_t counter;
    double test_cases_a[NUM_TESTS] = {2.5e-6, 4.231e-11, 6.77e3, 8.98e12};
    double test_cases_b[NUM_TESTS] = {1.0e-6, 4.0e-11, 6.0e3, 8.0e12};
    double test_cases_c[NUM_TESTS] = {-1.0, 2.0, 7.3312e1, 0.0};
    double add_result[NUM_TESTS]   = {3.50000000000000037101398342454E-6, 8.231e-11, 12770.0000, 1.698e+13};
    double sub_result[NUM_TESTS]   = {1.5000000000000002e-6, 2.3100000000000030e-12, 770.000000, 9.8e+11};
    double mul_result[NUM_TESTS]   = {2.5000000000000003e-12, 1.6924000000000000e-21, 4.062e+7, 7.184000e+25};
    double div_result[NUM_TESTS]   = {2.5000000000000004, 1.05775, 1.1283333333333334, 1.12250000};
    double fma_result[NUM_TESTS]   = {-1.5000000000000002e-6, 1.2462e-10, 5.0232224e+5, 8.0e12};

    register double a asm("fs2");
    register double b asm("fs3");
    register double c asm("fs4");

    register double x asm("fs0");
    register double y asm("fs1");

    register uint64_t i asm("s2") = 99;
    register uint64_t j asm("s3") = 400;

    __asm__ __volatile__ ("ADD_TEST_:");
    for( counter = 0; counter < NUM_TESTS; ++counter ) {
        a = test_cases_a[counter];
        b = test_cases_b[counter];
        y = add_result[counter];

        __asm__ __volatile__ ("fadd.d  %0, %1, %2;" : "=f"(x) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(i) : "f"(x) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(j) : "f"(y) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e -- %lx %lx\n", a, b, x, i, j);
    }

    __asm__ __volatile__ ("SUB_TEST_:");
    for( counter = 0; counter < NUM_TESTS; ++counter ) {
        a = test_cases_a[counter];
        b = test_cases_b[counter];
        y = sub_result[counter];
        __asm__ __volatile__ ("fsub.d %0, %1, %2;" : "=f"(x) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(i) : "f"(x) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(j) : "f"(y) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e -- %lx %lx\n", a, b, x, i, j);
    }

    __asm__ __volatile__ ("MUL_TEST_:");
    for( counter = 0; counter < NUM_TESTS; ++counter ) {
        a = test_cases_a[counter];
        b = test_cases_b[counter];
        y = mul_result[counter];
        __asm__ __volatile__ ("fmul.d  %0, %1, %2;" : "=f"(x) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(i) : "f"(x) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(j) : "f"(y) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e -- %lx %lx\n", a, b, x, i, j);
    }

    __asm__ __volatile__ ("DIV_TEST_:");
    for( counter = 0; counter < NUM_TESTS; ++counter ) {
        a = test_cases_a[counter];
        b = test_cases_b[counter];
        y = div_result[counter];
        __asm__ __volatile__ ("fdiv.d  %0, %1, %2;" : "=f"(x) : "f"(a), "f"(b) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(i) : "f"(x) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(j) : "f"(y) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e -- %lx %lx\n", a, b, x, i, j);
    }

    __asm__ __volatile__ ("FMA_TEST_:");
    for( counter = 0; counter < NUM_TESTS; ++counter ) {
        a = test_cases_a[counter];
        b = test_cases_b[counter];
        c = test_cases_c[counter];
        y = fma_result[counter];
        __asm__ __volatile__ ("fmadd.d  %0, %1, %2, %3;" : "=f"(x) : "f"(a), "f"(c), "f"(b) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(i) : "f"(x) );
        __asm__ __volatile__ ("fmv.x.d %0, %1;" : "=r"(j) : "f"(y) );
        __asm__ __volatile__ ("bne     %0, %1, FAIL_;" : : "r"(i), "r"(j) );

        printf("Done %e %e %e %e -- %lx %lx\n", a, c, b, x, i, j);
    }

    __asm__ __volatile__ ("beq zero, zero, FINIS_;");

    __asm__ __volatile__ ("FAIL_:");
    printf("FAIL %e %e %e %e -- %lx %lx\n", a, b, c, x, i, j);

    __asm__ __volatile__ ("FINIS_:");

}
