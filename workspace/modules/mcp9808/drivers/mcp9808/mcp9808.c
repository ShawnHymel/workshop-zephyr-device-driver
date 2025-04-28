// Ties to the 'compatible = "microchip,mcp9808"' node in the Devicetree
#define DT_DRV_COMPAT microchip_mcp9808

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "mcp9808.h"

// Enable logging at a given level
LOG_MODULE_REGISTER(MCP9808, CONFIG_SENSOR_LOG_LEVEL);

//------------------------------------------------------------------------------
// Forward declarations

static int mcp9808_reg_read(const struct device *dev,
							uint8_t reg,
							uint16_t *val);
static int mcp9808_reg_write_8bit(const struct device *dev,
								  uint8_t reg,
								  uint8_t val);
static int mcp9808_init(const struct device *dev);
static int mcp9808_sample_fetch(const struct device *dev,
								enum sensor_channel chan);
static int mcp9808_channel_get(const struct device *dev,
							   enum sensor_channel chan,
							   struct sensor_value *val);

//------------------------------------------------------------------------------
// Private functions

// Read from a register (at address reg) on the device
static int mcp9808_reg_read(const struct device *dev, 
							uint8_t reg, 
							uint16_t *val)
{
	const struct mcp9808_config *cfg = dev->config;

	// Write the register address first then read from the I2C bus
	int ret = i2c_write_read_dt(&cfg->i2c, &reg, sizeof(reg), val, sizeof(*val));
	if (ret == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return ret;
}

// Write to a register (at address reg) on the device
static int mcp9808_reg_write_8bit(const struct device *dev, 
								  uint8_t reg, 
								  uint8_t val)
{
	const struct mcp9808_config *cfg = dev->config;

	// Construct 2-bute message (address, value)
	uint8_t buf[2] = {
		reg,
		val,
	};

	// Perform write operation
	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

// Initialize the MCP9808 (performed by kernel at boot)
static int mcp9808_init(const struct device *dev)
{
	const struct mcp9808_config *cfg = dev->config;
	int ret = 0;

	// Print to console
	LOG_DBG("Initializing");

	// Check the bus is ready and there is a software handle to the device
	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	// Set temperature resolution (make sure we can write to the device)
	ret = mcp9808_reg_write_8bit(dev, MCP9808_REG_RESOLUTION, cfg->resolution);
	LOG_DBG("Setting resolution to index %d", cfg->resolution);
	if (ret) {
		LOG_ERR("Could not set the resolution of mcp9808 module");
		return ret;
	}

	return ret;
}

//------------------------------------------------------------------------------
// Public functions (API)

// Read temperature value from the device and store it in the device data struct
// Call this before calling mcp9808_channel_get()
static int mcp9808_sample_fetch(const struct device *dev, 
								enum sensor_channel chan)
{
	struct mcp9808_data *data = dev->data;

	// Check if the channel is supported
	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}

	// Perform the I2C read, store the data in the device data struct
	return mcp9808_reg_read(dev, MCP9808_REG_TEMP_AMB, &data->reg_val);
}

// Get the temperature value stored in the device data struct
// Make sure to call mcp9808_sample_fetch() to update the device data
static int mcp9808_channel_get(const struct device *dev, 
							   enum sensor_channel chan,
			    			   struct sensor_value *val)
{
	const struct mcp9808_data *data = dev->data;

	// Convert the 12-bit two's complement to a signed integer value
	int temp = data->reg_val & MCP9808_TEMP_ABS_MASK;
	if (data->reg_val & MCP9808_TEMP_SIGN_BIT) {
		temp = -(1U + (temp ^ MCP9808_TEMP_ABS_MASK));
	}

	// Check if the channel is supported
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}

	// Store the value as integer (val1) and millionths (val2)
	val->val1 = temp / MCP9808_TEMP_SCALE_CEL;
	temp -= val->val1 * MCP9808_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / MCP9808_TEMP_SCALE_CEL;

	return 0;
}

//------------------------------------------------------------------------------
// Devicetree handling - This is the magic that connects this driver source code
// 	to the Devicetree so that you can use it in your application!

// Define the public API functions for the driver
static const struct sensor_driver_api mcp9808_api_funcs = {
	.sample_fetch = mcp9808_sample_fetch,
	.channel_get = mcp9808_channel_get,
};

// Expansion macro to define the driver instances
// If inst is set to "42" by the Devicetree compiler, this macro creates code
// with the unique id of "42" for the structs, e.g. mcp9808_data_42.
#define MCP9808_DEFINE(inst)                                        		   \
																			   \
	/* Create an instance of the data struct */								   \
	static struct mcp9808_data mcp9808_data_##inst;                 		   \
                                                                    		   \
	/* Create an instance of the config struct and populate with DT values */  \
	static const struct mcp9808_config mcp9808_config_##inst = {			   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                          		   \
		.resolution = DT_INST_PROP(inst, resolution),               		   \
	};        																   \
                                                                    		   \
	/* Create a "device" instance from a Devicetree node identifier and */	   \
	/* registers the init function to run during boot. */					   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, 										   \
								 mcp9808_init, 								   \
								 NULL, 										   \
								 &mcp9808_data_##inst,						   \
				     			 &mcp9808_config_##inst, 					   \
								 POST_KERNEL,                       		   \
				     			 CONFIG_SENSOR_INIT_PRIORITY, 				   \
								 &mcp9808_api_funcs);						   \

// The Devicetree build process calls this to create an instance of structs for 
// each device (MCP9808) defined in the Devicetree Source (DTS)
DT_INST_FOREACH_STATUS_OKAY(MCP9808_DEFINE)
