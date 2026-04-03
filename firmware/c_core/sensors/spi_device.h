#ifndef SPI_DEVICE_H
#define SPI_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/smart_monitor_constants.h"

// SPI mode constants
#define SPI_MODE_0 0
#define SPI_SPEED_HZ 1000000

typedef struct {
    bool m_initialized;
    file_fd_t m_fd;
    char* m_device_path;
    uint8_t m_mode;
    uint32_t m_speed;
    uint8_t m_bits_per_word;
} spi_device_t;

typedef struct {
    accelerometer_data_t m_accelerometer_x;
    accelerometer_data_t m_accelerometer_y;
    accelerometer_data_t m_accelerometer_z;
    gyroscope_data_t m_gyroscope_x;
    gyroscope_data_t m_gyroscope_y;
    gyroscope_data_t m_gyroscope_z;
    magnetometer_data_t m_magnetometer_x;
    magnetometer_data_t m_magnetometer_y;
    magnetometer_data_t m_magnetometer_z;
    timestamp_t m_timestamp;
} imu_data_t;

// SPI device functions
spi_device_t* spi_create(const char* device_path);
bool spi_initialize(spi_device_t* spi, uint8_t mode, uint32_t speed);
void spi_destroy(spi_device_t* spi);
bool spi_transfer(spi_device_t* spi, uint8_t* tx_data, uint8_t* rx_data, sample_count_t len);
imu_data_t spi_read_imu_data(spi_device_t* spi);

// Mock SPI functions
spi_device_t* spi_create_mock(void);
void spi_simulate_motion(spi_device_t* spi);

#endif // SPI_DEVICE_H
