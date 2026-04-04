#include "i2c_sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <math.h>

// Mock I2C implementation for demonstration
// In real deployment, this would use actual I2C headers

#define TEMP_REG 0x01
#define HUMIDITY_REG 0x02
#define LIGHT_REG 0x03
#define MOTION_REG 0x04

i2c_sensor_t* i2c_create(const char* device_path, uint8_t device_addr) {
    i2c_sensor_t* sensor = malloc(sizeof(i2c_sensor_t));
    if (!sensor) {
        return NULL;
    }
    
    memset(sensor, 0, sizeof(i2c_sensor_t));
    sensor->device_path = strdup(device_path ? device_path : "/dev/i2c-1");
    sensor->device_addr = device_addr;
    sensor->fd = -1;
    
    return sensor;
}

i2c_sensor_t* i2c_create_mock(void) {
    i2c_sensor_t* sensor = i2c_create("/mock/i2c", 0x48);
    if (sensor) {
        sensor->initialized = true;
    }
    return sensor;
}

bool i2c_initialize(i2c_sensor_t* sensor) {
    if (!sensor) {
        return false;
    }
    
    // Try to open real I2C device
    sensor->fd = open(sensor->device_path, O_RDWR);
    if (sensor->fd < 0) {
        printf("I2C device %s not available, using mock mode\n", sensor->device_path);
        sensor->initialized = true;
        return true;
    }
    
    // Set I2C slave address
    if (ioctl(sensor->fd, I2C_SLAVE, sensor->device_addr) < 0) {
        perror("Failed to set I2C slave address");
        close(sensor->fd);
        return false;
    }
    
    sensor->initialized = true;
    return true;
}

void i2c_destroy(i2c_sensor_t* sensor) {
    if (!sensor) {
        return;
    }
    
    if (sensor->fd >= 0) {
        close(sensor->fd);
    }
    
    if (sensor->device_path) {
        free(sensor->device_path);
    }
    
    free(sensor);
}

bool i2c_read_register(i2c_sensor_t* sensor, uint8_t reg, uint8_t* value) {
    if (!sensor || !sensor->initialized || !value) {
        return false;
    }
    
    if (sensor->fd < 0) {
        // Mock implementation
        switch (reg) {
            case TEMP_REG:
                *value = 20 + (rand() % 10); // 20-30°C
                break;
            case HUMIDITY_REG:
                *value = 40 + (rand() % 20); // 40-60%
                break;
            case LIGHT_REG:
                *value = rand() % 255; // 0-255
                break;
            case MOTION_REG:
                *value = (rand() % 100) > 80 ? 1 : 0; // 20% chance of motion
                break;
            default:
                *value = 0;
                break;
        }
        return true;
    }
    
    // Real I2C implementation would go here
    // For demo, always succeed
    return true;
}

bool i2c_write_register(i2c_sensor_t* sensor, uint8_t reg __attribute__((unused)), uint8_t value __attribute__((unused))) {
    if (!sensor || !sensor->initialized) {
        return false;
    }
    
    if (sensor->fd < 0) {
        // Mock mode - just return success
        return true;
    }
    
    // Real I2C implementation would go here
    // For demo, always succeed
    return true;
}

sensor_data_t i2c_read_sensor_data(i2c_sensor_t* sensor) {
    sensor_data_t data = {0};
    uint8_t temp_raw, humidity_raw, light_raw, motion_raw;
    
    if (i2c_read_register(sensor, TEMP_REG, &temp_raw)) {
        data.temperature = (float)temp_raw + (rand() % 5) - 2.5f; // Add some variation
    }
    
    if (i2c_read_register(sensor, HUMIDITY_REG, &humidity_raw)) {
        data.humidity = (float)humidity_raw + (rand() % 10) - 5.0f;
    }
    
    if (i2c_read_register(sensor, LIGHT_REG, &light_raw)) {
        data.light_level = light_raw;
    }
    
    if (i2c_read_register(sensor, MOTION_REG, &motion_raw)) {
        data.motion_detected = motion_raw > 0;
    }
    
    data.timestamp = time(NULL);
    
    return data;
}

void i2c_simulate_environment_changes(i2c_sensor_t* sensor) {
    if (!sensor || !sensor->initialized) {
        return;
    }
    
    static uint32_t simulation_counter = 0;
    simulation_counter++;
    
    // Simulate gradual temperature changes
    uint8_t new_temp = 20 + (sin(simulation_counter * 0.01) * 5);
    i2c_write_register(sensor, TEMP_REG, new_temp);
    
    // Simulate humidity changes
    uint8_t new_humidity = 50 + (cos(simulation_counter * 0.008) * 10);
    i2c_write_register(sensor, HUMIDITY_REG, new_humidity);
    
    // Simulate light level changes
    uint8_t new_light = 128 + (sin(simulation_counter * 0.02) * 100);
    i2c_write_register(sensor, LIGHT_REG, new_light);
}
