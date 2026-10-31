/*
 * LTC4215 I2C/SMBus register driver.
 *
 * Register map and LSB/full-scale values taken directly from the LTC4215/LTC4215-2
 * datasheet (Rev E), Tables 2-8. Board uses the datasheet's own worked address
 * example (ADR2/ADR0 = INTVCC, ADR1 = floating -> address 19, 0x13) and a 5mOhm
 * sense resistor (R5).
 *
 * Not tested on hardware -- this board (EFUSE_TELEMETRY_STM32) is simulation and
 * PCB layout only, never built. Verified by (a) compiling against the STM32F411
 * HAL and (b) ltc4215_regmap_test.c, which checks the pure conversion functions
 * below against synthetic register bytes derived from the datasheet's own LSB
 * values -- not by any claim of real I2C traffic.
 */
#ifndef LTC4215_H
#define LTC4215_H

#include <stdint.h>
#include <stdbool.h>

#ifdef STM32F411xE
#include "stm32f4xx_hal.h"
#endif

/* 7-bit I2C address set by ADR2=INTVCC, ADR1=floating, ADR0=INTVCC (datasheet
 * Figure 5 worked example, binary 1010011 = decimal 19). HAL_I2C calls need the
 * 8-bit form, i.e. (LTC4215_I2C_ADDR_7BIT << 1). */
#define LTC4215_I2C_ADDR_7BIT   0x13U

/* Register addresses (datasheet Tables 2-8). */
#define LTC4215_REG_CONTROL     0x00U  /* A, R/W */
#define LTC4215_REG_ALERT       0x01U  /* B, R/W */
#define LTC4215_REG_STATUS      0x02U  /* C, read-only */
#define LTC4215_REG_FAULT       0x03U  /* D, R/W (write 0 to clear) */
#define LTC4215_REG_SENSE       0x04U  /* E, R/W */
#define LTC4215_REG_SOURCE      0x05U  /* F, R/W */
#define LTC4215_REG_ADIN        0x06U  /* G, R/W -- tied to GND on this board */

/* STATUS Register C (Table 4) bit masks. */
#define LTC4215_STATUS_FET_ON          (1U << 7)
#define LTC4215_STATUS_GPIO_INPUT      (1U << 6)
#define LTC4215_STATUS_FET_SHORT       (1U << 5)
#define LTC4215_STATUS_EN_LOW          (1U << 4)
#define LTC4215_STATUS_POWER_BAD       (1U << 3)
#define LTC4215_STATUS_OVERCURRENT     (1U << 2)
#define LTC4215_STATUS_UNDERVOLTAGE    (1U << 1)
#define LTC4215_STATUS_OVERVOLTAGE     (1U << 0)

/* FAULT Register D (Table 5) bit masks -- latched occurrences, write 0 to clear. */
#define LTC4215_FAULT_FET_SHORT        (1U << 5)
#define LTC4215_FAULT_EN_CHANGED       (1U << 4)
#define LTC4215_FAULT_POWER_BAD        (1U << 3)
#define LTC4215_FAULT_OVERCURRENT      (1U << 2)
#define LTC4215_FAULT_UNDERVOLTAGE     (1U << 1)
#define LTC4215_FAULT_OVERVOLTAGE      (1U << 0)

/* LSB / full-scale constants, in microvolts, straight from the datasheet so the
 * conversion math is traceable to a source rather than a magic number. */
#define LTC4215_SENSE_LSB_UV     151U     /* Table 6: 151uV/LSB, 38.45mV FS */
#define LTC4215_SOURCE_LSB_UV    60500U   /* Table 7: 60.5mV/LSB, 15.44V FS */
#define LTC4215_ADIN_LSB_UV      4820U    /* Table 8: 4.82mV/LSB, 1.23V FS */

/* R5, the sense resistor actually populated on this board (Power block, eFuse
 * schematic) -- needed to turn the SENSE register into a real output current. */
#define LTC4215_RSENSE_MOHM      5U

/* ---- Pure conversion functions -------------------------------------------
 * No I2C, no HAL -- take a raw register byte, return a physical value. Kept
 * separate from the HAL calls specifically so they can be unit tested on the
 * host (see ltc4215_regmap_test.c) without any STM32 hardware or mocking. */

/* Raw SENSE register byte -> millivolts across R5. */
uint32_t ltc4215_sense_reg_to_uv(uint8_t raw);

/* Raw SENSE register byte -> output current in milliamps, via R5 = 5mOhm. */
uint32_t ltc4215_sense_reg_to_ma(uint8_t raw);

/* Raw SOURCE register byte -> output (SOURCE pin) voltage in millivolts. */
uint32_t ltc4215_source_reg_to_mv(uint8_t raw);

/* Raw ADIN register byte -> ADIN pin voltage in millivolts. Always reads ~0 on
 * this board since ADIN is tied to GND (not used) -- kept for completeness. */
uint32_t ltc4215_adin_reg_to_mv(uint8_t raw);

/* ---- HAL-based register I/O ------------------------------------------- */
#ifdef STM32F411xE

typedef struct {
    bool fet_on;
    bool gpio_input_high;
    bool fet_short;
    bool en_pin_low;
    bool power_bad;
    bool overcurrent;
    bool undervoltage;
    bool overvoltage;
} LTC4215_Status;

typedef struct {
    bool fet_short_occurred;
    bool en_changed;
    bool power_bad_occurred;
    bool overcurrent_occurred;
    bool undervoltage_occurred;
    bool overvoltage_occurred;
} LTC4215_Fault;

typedef struct {
    uint32_t source_mv;   /* output/SOURCE voltage, mV */
    uint32_t current_ma;  /* output current through R5, mA */
    uint32_t adin_mv;     /* ADIN pin voltage, mV (expect ~0, tied to GND) */
} LTC4215_Telemetry;

HAL_StatusTypeDef ltc4215_read_register(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *out);
HAL_StatusTypeDef ltc4215_write_register(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value);

HAL_StatusTypeDef ltc4215_read_status(I2C_HandleTypeDef *hi2c, LTC4215_Status *status);
HAL_StatusTypeDef ltc4215_read_fault(I2C_HandleTypeDef *hi2c, LTC4215_Fault *fault);
HAL_StatusTypeDef ltc4215_clear_faults(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ltc4215_read_telemetry(I2C_HandleTypeDef *hi2c, LTC4215_Telemetry *telemetry);

#endif /* STM32F411xE */

#endif /* LTC4215_H */
