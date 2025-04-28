#ifndef ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MCP9808_H_
#define ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MCP9808_H_

#include <zephyr/drivers/sensor.h>

// MCP9808 registers
#define MCP9808_REG_CONFIG     0x01
#define MCP9808_REG_TEMP_AMB   0x05
#define MCP9808_REG_RESOLUTION 0x08

// Ambient temperature register information
#define MCP9808_TEMP_SCALE_CEL 16
#define MCP9808_TEMP_SIGN_BIT  BIT(12)
#define MCP9808_TEMP_ABS_MASK  0x0FFF

// Sensor data
struct mcp9808_data {
	uint16_t reg_val;
};

// Configuration data
struct mcp9808_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MCP9808_H_ */
