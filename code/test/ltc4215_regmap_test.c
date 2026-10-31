/*
 * Host-side unit test for the LTC4215 register-to-physical-value conversion
 * functions (ltc4215_sense_reg_to_*, ltc4215_source_reg_to_mv,
 * ltc4215_adin_reg_to_mv).
 *
 * This is the "not tested on hardware" verification promised in the project
 * report and README: it feeds synthetic register bytes and checks the math
 * against the LSB/full-scale values given directly in the LTC4215 datasheet
 * (Rev E, Tables 6-8) -- it does not touch I2C, HAL, or any STM32 hardware,
 * and must never be presented as having run on the real board, which was
 * never built.
 *
 * Build and run on the host (normal gcc, NOT arm-none-eabi-gcc):
 *   gcc -I ../EFUSE_TELEMETRY/Core/Inc -o ltc4215_regmap_test \
 *       ltc4215_regmap_test.c ../EFUSE_TELEMETRY/Core/Src/ltc4215.c
 *   ./ltc4215_regmap_test
 */
#include <stdio.h>
#include <stdint.h>

#include "ltc4215.h"

static int g_failures = 0;

static void check_eq_u32(const char *what, uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("FAIL %-28s got=%u want=%u\n", what, got, want);
        g_failures++;
    } else {
        printf("ok   %-28s got=%u\n", what, got);
    }
}

int main(void)
{
    /* SENSE register (Table 6): 151uV/LSB, 38.45mV full scale.
     * raw=0 -> 0uV; raw=1 -> 151uV; raw=255 -> 255*151 = 38505uV (datasheet
     * rounds this as "38.45mV FS", close enough given LSB rounding -- the
     * driver's own constant (151uV/LSB) is the thing under test here, not
     * the datasheet's rounded FS label). */
    check_eq_u32("sense raw=0 -> uV",   ltc4215_sense_reg_to_uv(0),   0U);
    check_eq_u32("sense raw=1 -> uV",   ltc4215_sense_reg_to_uv(1),   151U);
    check_eq_u32("sense raw=255 -> uV", ltc4215_sense_reg_to_uv(255), 38505U);

    /* Current through R5 = 5mOhm: I[mA] = V[uV] / R[mOhm]. */
    check_eq_u32("current raw=0 -> mA",   ltc4215_sense_reg_to_ma(0),   0U);
    check_eq_u32("current raw=100 -> mA", ltc4215_sense_reg_to_ma(100), 3020U);
    check_eq_u32("current raw=255 -> mA", ltc4215_sense_reg_to_ma(255), 7701U);

    /* SOURCE register (Table 7): 60.5mV/LSB, 15.44V full scale.
     * raw=255 -> 255*60500uV = 15427500uV = 15427mV (datasheet's 15.44V FS
     * is the rounded label for the same LSB*255 computation). */
    check_eq_u32("source raw=0 -> mV",   ltc4215_source_reg_to_mv(0),   0U);
    check_eq_u32("source raw=1 -> mV",   ltc4215_source_reg_to_mv(1),   60U);
    check_eq_u32("source raw=255 -> mV", ltc4215_source_reg_to_mv(255), 15427U);

    /* ADIN register (Table 8): 4.82mV/LSB, 1.23V full scale. On this board
     * ADIN is tied to GND (not used), so raw should always read 0 in
     * practice -- these checks only verify the conversion math itself. */
    check_eq_u32("adin raw=0 -> mV",   ltc4215_adin_reg_to_mv(0),   0U);
    check_eq_u32("adin raw=255 -> mV", ltc4215_adin_reg_to_mv(255), 1229U);

    if (g_failures == 0) {
        printf("\nAll checks passed.\n");
        return 0;
    }
    printf("\n%d check(s) FAILED.\n", g_failures);
    return 1;
}
