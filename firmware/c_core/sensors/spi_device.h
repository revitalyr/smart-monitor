#ifndef SPI_DEVICE_H
#define SPI_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool initialized;
    int fd;
    char* device_path;
    uint8_t mode;
    uint32_t speed;
    uint8_t bits_per_word;
} spi_device_t;

typedef struct {
    uint16_t accelerometer_x;
    uint16_t accelerometer_y;
    uint16_t accelerometer_z;
    uint16_t gyroscope_x;
    uint16_t gyroscope_y;
    uint16_t gyroscope_z;
    uint16_t magnetometer_x;
    uint16_t magnetometer_y;
    uint16_t magnetometer_z;
    uint32_t timestamp;
} imu_data_t;

// SPI device functions
spi_device_t* spi_create(const char* device_path);
bool spi_initialize(spi_device_t* spi, uint8_t mode, uint32_t speed);
void spi_destroy(spi_device_t* spi);
bool spi_transfer(spi_device_t* spi, uint8_t* tx_data, uint8_t* rx_data, int len);
imu_data_t spi_read_imu_data(spi_device_t* spi);

// Mock SPI functions
spi_device_t* spi_create_mock(void);
void spi_simulate_motion(spi_device_t* spi);

#endif // SPI_DEVICE_H
