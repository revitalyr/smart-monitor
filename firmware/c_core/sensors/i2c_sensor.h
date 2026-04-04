/**
 * @file i2c_sensor.h
 * @brief I2C sensor interface for Smart Monitor system
 * 
 * This module provides I2C communication functionality for various sensors
 * including temperature, humidity, light, and motion sensors. Supports both
 * real hardware devices and mock implementations for testing.
 */

#ifndef I2C_SENSOR_H
#define I2C_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief I2C sensor device structure
 * 
 * Contains device state, file descriptor, and configuration
 * for I2C communication operations.
 */
typedef struct {
    bool initialized;        /**< Initialization status flag */
    int fd;                 /**< Device file descriptor */
    uint8_t device_addr;    /**< I2C device address */
    char* device_path;       /**< Path to I2C device file */
} i2c_sensor_t;

/**
 * @brief Sensor data structure
 * 
 * Contains all sensor readings including environmental data
 * and motion detection results with timestamp.
 */
typedef struct {
    float temperature;       /**< Temperature in Celsius */
    float humidity;          /**< Relative humidity percentage (0-100) */
    uint32_t light_level;   /**< Light level (0-65535) */
    bool motion_detected;    /**< Motion detection flag */
    uint64_t timestamp;     /**< Unix timestamp of reading */
} sensor_data_t;

/**
 * @brief Create I2C sensor instance
 * 
 * @param device_path Path to I2C device (e.g., "/dev/i2c-1")
 * @param device_addr I2C device address (0x08-0x77)
 * @return Pointer to i2c_sensor_t instance or NULL on failure
 */
i2c_sensor_t* i2c_create(const char* device_path, uint8_t device_addr);

/**
 * @brief Initialize I2C sensor communication
 * 
 * @param sensor Pointer to i2c_sensor_t instance
 * @return true on success, false on failure
 */
bool i2c_initialize(i2c_sensor_t* sensor);

/**
 * @brief Destroy I2C sensor and free resources
 * 
 * @param sensor Pointer to i2c_sensor_t instance
 */
void i2c_destroy(i2c_sensor_t* sensor);

/**
 * @brief Read from I2C register
 * 
 * @param sensor Pointer to i2c_sensor_t instance
 * @param reg Register address to read from
 * @param value Pointer to store read value
 * @return true on success, false on failure
 */
bool i2c_read_register(i2c_sensor_t* sensor, uint8_t reg, uint8_t* value);

/**
 * @brief Write to I2C register
 * 
 * @param sensor Pointer to i2c_sensor_t instance
 * @param reg Register address to write to
 * @param value Value to write
 * @return true on success, false on failure
 */
bool i2c_write_register(i2c_sensor_t* sensor, uint8_t reg, uint8_t value);

/**
 * @brief Read all sensor data
 * 
 * @param sensor Pointer to i2c_sensor_t instance
 * @return sensor_data_t structure with current readings
 */
sensor_data_t i2c_read_sensor_data(i2c_sensor_t* sensor);

/**
 * @brief Create mock I2C sensor for testing
 * 
 * @return Pointer to mock i2c_sensor_t instance
 */
i2c_sensor_t* i2c_create_mock(void);

/**
 * @brief Simulate environmental changes for testing
 * 
 * @param sensor Pointer to i2c_sensor_t instance
 */
void i2c_simulate_environment_changes(i2c_sensor_t* sensor);

#endif // I2C_SENSOR_H
