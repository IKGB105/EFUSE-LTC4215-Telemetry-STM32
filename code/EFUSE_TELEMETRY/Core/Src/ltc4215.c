#include "ltc4215.h"

uint32_t ltc4215_sense_reg_to_uv(uint8_t raw)
{
    return (uint32_t)raw * LTC4215_SENSE_LSB_UV;
}

uint32_t ltc4215_sense_reg_to_ma(uint8_t raw)
{
    /* I = V / R. V in uV, R in mOhm -> I in mA: (uV / mOhm) = mA directly,
     * since 1 uV / 1 mOhm = 1 uA / 1 Ohm = ... keep it explicit instead of
     * relying on that unit coincidence. */
    uint32_t v_uv = ltc4215_sense_reg_to_uv(raw);
    /* mA = (uV / mOhm_as_ohm_times_1000) ... do it in nV to stay in integer
     * math without losing the low bits: I[mA] = V[uV] / R[mOhm]. */
    return v_uv / LTC4215_RSENSE_MOHM;
}

uint32_t ltc4215_source_reg_to_mv(uint8_t raw)
{
    uint32_t v_uv = (uint32_t)raw * LTC4215_SOURCE_LSB_UV;
    return v_uv / 1000U;
}

uint32_t ltc4215_adin_reg_to_mv(uint8_t raw)
{
    uint32_t v_uv = (uint32_t)raw * LTC4215_ADIN_LSB_UV;
    return v_uv / 1000U;
}

#ifdef STM32F411xE

HAL_StatusTypeDef ltc4215_read_register(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *out)
{
    return HAL_I2C_Mem_Read(hi2c, (uint16_t)(LTC4215_I2C_ADDR_7BIT << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, out, 1, HAL_MAX_DELAY);
}

HAL_StatusTypeDef ltc4215_write_register(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(hi2c, (uint16_t)(LTC4215_I2C_ADDR_7BIT << 1), reg,
                              I2C_MEMADD_SIZE_8BIT, &value, 1, HAL_MAX_DELAY);
}

HAL_StatusTypeDef ltc4215_read_status(I2C_HandleTypeDef *hi2c, LTC4215_Status *status)
{
    uint8_t raw;
    HAL_StatusTypeDef st = ltc4215_read_register(hi2c, LTC4215_REG_STATUS, &raw);
    if (st != HAL_OK) {
        return st;
    }

    status->fet_on          = (raw & LTC4215_STATUS_FET_ON) != 0;
    status->gpio_input_high = (raw & LTC4215_STATUS_GPIO_INPUT) != 0;
    status->fet_short       = (raw & LTC4215_STATUS_FET_SHORT) != 0;
    status->en_pin_low      = (raw & LTC4215_STATUS_EN_LOW) != 0;
    status->power_bad       = (raw & LTC4215_STATUS_POWER_BAD) != 0;
    status->overcurrent     = (raw & LTC4215_STATUS_OVERCURRENT) != 0;
    status->undervoltage    = (raw & LTC4215_STATUS_UNDERVOLTAGE) != 0;
    status->overvoltage     = (raw & LTC4215_STATUS_OVERVOLTAGE) != 0;
    return HAL_OK;
}

HAL_StatusTypeDef ltc4215_read_fault(I2C_HandleTypeDef *hi2c, LTC4215_Fault *fault)
{
    uint8_t raw;
    HAL_StatusTypeDef st = ltc4215_read_register(hi2c, LTC4215_REG_FAULT, &raw);
    if (st != HAL_OK) {
        return st;
    }

    fault->fet_short_occurred     = (raw & LTC4215_FAULT_FET_SHORT) != 0;
    fault->en_changed             = (raw & LTC4215_FAULT_EN_CHANGED) != 0;
    fault->power_bad_occurred     = (raw & LTC4215_FAULT_POWER_BAD) != 0;
    fault->overcurrent_occurred   = (raw & LTC4215_FAULT_OVERCURRENT) != 0;
    fault->undervoltage_occurred  = (raw & LTC4215_FAULT_UNDERVOLTAGE) != 0;
    fault->overvoltage_occurred   = (raw & LTC4215_FAULT_OVERVOLTAGE) != 0;
    return HAL_OK;
}

HAL_StatusTypeDef ltc4215_clear_faults(I2C_HandleTypeDef *hi2c)
{
    /* FAULT register is R/W; writing 0 clears the latched occurrence bits
     * (datasheet Table 5 -- bits D7:6 reserved, write back as 0 too). */
    return ltc4215_write_register(hi2c, LTC4215_REG_FAULT, 0x00U);
}

HAL_StatusTypeDef ltc4215_read_telemetry(I2C_HandleTypeDef *hi2c, LTC4215_Telemetry *telemetry)
{
    uint8_t raw_sense, raw_source, raw_adin;
    HAL_StatusTypeDef st;

    st = ltc4215_read_register(hi2c, LTC4215_REG_SENSE, &raw_sense);
    if (st != HAL_OK) {
        return st;
    }
    st = ltc4215_read_register(hi2c, LTC4215_REG_SOURCE, &raw_source);
    if (st != HAL_OK) {
        return st;
    }
    st = ltc4215_read_register(hi2c, LTC4215_REG_ADIN, &raw_adin);
    if (st != HAL_OK) {
        return st;
    }

    telemetry->current_ma = ltc4215_sense_reg_to_ma(raw_sense);
    telemetry->source_mv  = ltc4215_source_reg_to_mv(raw_source);
    telemetry->adin_mv    = ltc4215_adin_reg_to_mv(raw_adin);
    return HAL_OK;
}

#endif /* STM32F411xE */
