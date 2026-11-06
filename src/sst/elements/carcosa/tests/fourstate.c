/*
 * fourstate: 4-kernel demo for FourStateAgent; lowlink traffic for state-gate tests.
 */
#include "hyades.h"
#include <unistd.h>

/* Working-set size: span multiple 64B lines; keep small for Vanadis sim time. */
#define BUF_LEN 64

/* Per-kernel working sets; volatile so loads/stores hit cache/lowlink. */
static volatile unsigned char  K0_buf[BUF_LEN];        /* K0: byte reads */
static volatile unsigned int   K1_tab[BUF_LEN];        /* K1: strided reads */
static volatile unsigned int   K2_src[BUF_LEN];        /* K2: read side */
static volatile unsigned int   K2_dst[BUF_LEN];        /* K2: write side */
static volatile unsigned int   K3_sink[BUF_LEN];       /* K3: writes */

/* Call counter once per 0..3 iteration; K2/K3 vary writes for distinct checksums. */
static unsigned int k_iter = 0;

static const char *role_tag = "r?";

/* Minimal "unsigned -> 8 hex chars + newline" emitter. Avoids printf (and
 * its libc-static-init cost) and any dynamic allocation. */
static void write_hex8(unsigned int v) {
    char buf[9];
    for (int i = 7; i >= 0; i--) {
        unsigned int nib = v & 0xF;
        buf[i] = (nib < 10) ? (char)('0' + nib) : (char)('a' + nib - 10);
        v >>= 4;
    }
    buf[8] = '\n';
    write(1, buf, 9);
}

/* Emit "<tag3><role_tag> v=<hex>\n" — e.g. "K2 r0 v=deadbeef\n". */
static void emit(const char *tag3, unsigned int v) {
    write(1, tag3,     3);
    write(1, role_tag, 2);
    write(1, " v=",    3);
    write_hex8(v);
}

/* K0 — sequential byte reads; dense contiguous 8-bit loads. */
static void kernel0(void) {
    unsigned int sum = 0;
    for (int i = 0; i < BUF_LEN; i++) sum += K0_buf[i];
    emit("K0 ", sum);
}

/* K1 — strided word reads (every 4th); one word per line => cold-line fills. */
static void kernel1(void) {
    unsigned int xorv = 0;
    for (int i = 0; i < BUF_LEN; i += 4) xorv ^= K1_tab[i];
    emit("K1 ", xorv);
}

/* K2 — RMW: load/combine/store. Checksum depends on every store completing. */
static void kernel2(void) {
    for (int i = 0; i < BUF_LEN; i++) {
        K2_dst[i] = K2_src[i] + k_iter + (unsigned int)i;
    }
    emit("K2 ", K2_dst[0] ^ K2_dst[BUF_LEN - 1]);
}

/* K3 — sequential word writes then read-back checksum. */
static void kernel3(void) {
    unsigned int pat = k_iter * 0x9E3779B1u;   /* fractional golden ratio */
    for (int i = 0; i < BUF_LEN; i++) {
        K3_sink[i] = pat + (unsigned int)i;
    }
    unsigned int sum = 0;
    for (int i = 0; i < BUF_LEN; i++) sum += K3_sink[i];
    emit("K3 ", sum);
    k_iter++;
}

/* Initialize the read-side working sets with non-zero, address-dependent
 * values. Done once at startup so subsequent K0/K1/K2 reads see stable
 * deterministic data (absent fault injection). */
static void init_data(void) {
    for (int i = 0; i < BUF_LEN; i++) {
        K0_buf[i]  = (unsigned char)(i * 3u + 7u);
        K1_tab[i]  = (unsigned int)i * 0x01010101u + 0x13579BDFu;
        K2_src[i]  = (unsigned int)i * 0xDEADBEEFu;
        K2_dst[i]  = 0;
        K3_sink[i] = 0;
    }
}

int main(int argc, char *argv[]) {
    int role = hyades_role_from_argv(argc, argv);
    role_tag = (role == 0) ? "r0" : "r1";

    init_data();

    hyades_handler_t jump_table[4];
    jump_table[0] = kernel0;
    jump_table[1] = kernel1;
    jump_table[2] = kernel2;
    jump_table[3] = kernel3;

    hyades_run(jump_table, 4);
    return 0;
}
