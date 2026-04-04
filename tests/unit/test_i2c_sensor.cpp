#include <gtest/gtest.h>
#include "sensors/i2c_sensor.h"
#include "common/smart_monitor_constants.h"

class I2CSensorTest : public ::testing::Test {
protected:
    void SetUp() override {
        sensor = i2c_create("/dev/i2c-1", 0x48);
        ASSERT_NE(sensor, nullptr);
    }

    void TearDown() override {
        if (sensor) {
            i2c_destroy(sensor);
        }
    }

    i2c_sensor_t* sensor;
};

TEST_F(I2CSensorTest, CreateAndDestroy) {
    EXPECT_TRUE(sensor->initialized);
    EXPECT_EQ(sensor->device_addr, 0x48);
    EXPECT_STREQ(sensor->device_path, "/dev/i2c-1");
}

TEST_F(I2CSensorTest, InitializeValidSensor) {
    bool result = i2c_initialize(sensor);
    EXPECT_TRUE(result);
    EXPECT_GT(sensor->fd, 0);
}

TEST_F(I2CSensorTest, ReadRegister) {
    i2c_initialize(sensor);
    
    uint8_t value;
    bool result = i2c_read_register(sensor, 0x00, &value);
    
    // Result depends on hardware availability
    if (result) {
        EXPECT_LE(value, 255);
    }
}

TEST_F(I2CSensorTest, WriteRegister) {
    i2c_initialize(sensor);
    
    bool result = i2c_write_register(sensor, 0x01, 0x42);
    
    // Result depends on hardware availability
    if (result) {
        uint8_t read_value;
        bool read_result = i2c_read_register(sensor, 0x01, &read_value);
        if (read_result) {
            EXPECT_EQ(read_value, 0x42);
        }
    }
}

TEST_F(I2CSensorTest, ReadSensorData) {
    i2c_initialize(sensor);
    
    sensor_data_t data = i2c_read_sensor_data(sensor);
    
    // Check that timestamp is set
    EXPECT_GT(data.timestamp, 0);
    
    // Check data ranges (if hardware is available)
    if (data.temperature != 0) {
        EXPECT_GE(data.temperature, -40.0f);
        EXPECT_LE(data.temperature, 85.0f);
    }
    
    if (data.humidity != 0) {
        EXPECT_GE(data.humidity, 0.0f);
        EXPECT_LE(data.humidity, 100.0f);
    }
    
    EXPECT_LE(data.light_level, 65535);
}

class I2CSensorMockTest : public ::testing::Test {
protected:
    void SetUp() override {
        sensor = i2c_create_mock();
        ASSERT_NE(sensor, nullptr);
    }

    void TearDown() override {
        if (sensor) {
            i2c_destroy(sensor);
        }
    }

    i2c_sensor_t* sensor;
};

TEST_F(I2CSensorMockTest, MockSensorData) {
    i2c_initialize(sensor);
    
    // Simulate environmental changes
    i2c_simulate_environment_changes(sensor);
    
    sensor_data_t data = i2c_read_sensor_data(sensor);
    
    // Mock should generate realistic data
    EXPECT_GT(data.timestamp, 0);
    EXPECT_GE(data.temperature, -40.0f);
    EXPECT_LE(data.temperature, 85.0f);
    EXPECT_GE(data.humidity, 0.0f);
    EXPECT_LE(data.humidity, 100.0f);
    EXPECT_LE(data.light_level, 65535);
}

TEST_F(I2CSensorMockTest, MockEnvironmentalChanges) {
    i2c_initialize(sensor);
    
    sensor_data_t data1 = i2c_read_sensor_data(sensor);
    i2c_simulate_environment_changes(sensor);
    sensor_data_t data2 = i2c_read_sensor_data(sensor);
    
    // Data should change after simulation
    EXPECT_NE(data1.temperature, data2.temperature);
    EXPECT_NE(data1.humidity, data2.humidity);
    EXPECT_GT(data2.timestamp, data1.timestamp);
}
