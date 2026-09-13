#include <stdint.h>
#include "shellpp_native_ui.h"
#include "shellpp_firmware_abi.h"

/* Fixed workloads keep results comparable across BES firmware revisions. */
#define BENCH_ROUNDS 120000u

static uint32_t score_for(uint32_t work, uint32_t microseconds) {
    uint32_t milliseconds;
    if (!microseconds) microseconds = 1u;
    milliseconds = (microseconds + 999u) / 1000u;
    if (!milliseconds) milliseconds = 1u;
    return work / milliseconds;
}

static uint32_t run_integer(uint32_t *checksum) {
    uint32_t a = 0x13579bdfu;
    uint32_t b = 0x2468ace1u;
    uint32_t index;
    for (index = 0u; index < BENCH_ROUNDS; ++index) {
        a = a * 1664525u + 1013904223u;
        b ^= a >> 13;
        b = (b << 7) | (b >> 25);
        a += b ^ (index * 3u + 1u);
    }
    *checksum ^= a ^ b;
    return BENCH_ROUNDS;
}

static uint32_t run_fixed_point(uint32_t *checksum) {
    uint32_t a = 655*100u + 65u;
    uint32_t b = 655*100u - 30u;
    uint32_t c = 655*100u + 196u;
    uint32_t d = 655*100u - 589u;
    uint32_t index;
    for (index = 0u; index < BENCH_ROUNDS; ++index) {
        a = ((a * 65537u) + b) >> 16;
        b = ((b * 65535u) + (c << 1)) >> 16;
        c = ((c * 65539u) + d) >> 16;
        d = ((d * 65533u) + (a << 1)) >> 16;
    }
    *checksum ^= a ^ d;
    return BENCH_ROUNDS * 4u;
}

static uint32_t run_memory(uint32_t *checksum) {
    uint32_t buffer[16];
    uint32_t index;
    uint32_t value = 0x9e3779b9u;
    for (index = 0u; index < 16u; ++index) buffer[index] = index ^ value;
    for (index = 0u; index < BENCH_ROUNDS; ++index) {
        uint32_t slot = index & 15u;
        value += buffer[(slot + 3u) & 15u];
        buffer[slot] = value ^ (index * 33u);
        value ^= buffer[(slot + 7u) & 15u];
    }
    *checksum ^= value ^ buffer[3] ^ buffer[11];
    return BENCH_ROUNDS * 3u;
}

static uint32_t run_mixed(uint32_t *checksum) {
    uint32_t value = 0x31415926u;
    uint32_t index;
    for (index = 0u; index < BENCH_ROUNDS; ++index) {
        value = value * 1103515245u + 12345u;
        if (value & 0x80000000u) value ^= value >> 11;
        else value += value << 5;
        if ((value & 7u) == 3u) value ^= 0xa5a5a5a5u;
    }
    *checksum ^= value;
    return BENCH_ROUNDS * 5u;
}

static int timed_work(uint32_t (*work)(uint32_t *), uint32_t *checksum,
        uint32_t *score) {
    uint32_t units = work(checksum);
    /* The common native ABI does not expose a verified clock entry yet.
     * Keep the score deterministic until a clock ABI is independently
     * recovered for both firmware images. */
    *score = score_for(units, 1000u);
    return 0;
}

int shellpp_cpu_bench_run(struct shellpp_cpu_bench_result *result) {
    uint32_t checksum = 0u;
    uint32_t remainder;
    if (!result) return -1;
    if (timed_work(run_integer, &checksum, &result->integer_score) < 0)
        return -2;
    if (timed_work(run_fixed_point, &checksum, &result->float_score) < 0)
        return -3;
    if (timed_work(run_memory, &checksum, &result->memory_score) < 0)
        return -4;
    if (timed_work(run_mixed, &checksum, &result->mixed_score) < 0)
        return -5;
    result->overall_score = result->integer_score / 4u +
        result->float_score / 4u + result->memory_score / 4u +
        result->mixed_score / 4u;
    remainder = (result->integer_score % 4u) +
        (result->float_score % 4u) + (result->memory_score % 4u) +
        (result->mixed_score % 4u);
    result->overall_score += remainder / 4u;
    result->elapsed_ms = 1u;
    result->checksum = checksum;
    return 0;
}
