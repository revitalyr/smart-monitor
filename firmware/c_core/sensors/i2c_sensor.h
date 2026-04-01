#ifndef I2C_SENSOR_H
#define I2C_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool initialized;
    int fd;
    uint8_t device_addr;
    char* device_path;
} i2c_sensor_t;

typedef struct {
    float temperature;
    float humidity;
    uint32_t light_level;
    bool motion_detected;
    uint64_t timestamp;
} sensor_data_t;

// I2C sensor functions
i2c_sensor_t* i2c_create(const char* device_path, uint8_t device_addr);
bool i2c_initialize(i2c_sensor_t* sensor);
void i2c_destroy(i2c_sensor_t* sensor);
bool i2c_read_register(i2c_sensor_t* sensor, uint8_t reg, uint8_t* value);
bool i2c_write_register(i2c_sensor_t* sensor, uint8_t reg, uint8_t value);
sensor_data_t i2c_read_sensor_data(i2c_sensor_t* sensor);

// Mock sensor functions
i2c_sensor_t* i2c_create_mock(void);
void i2c_simulate_environment_changes(i2c_sensor_t* sensor);

#endif // I2C_SENSOR_H
