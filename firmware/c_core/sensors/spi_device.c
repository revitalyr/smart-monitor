#include "spi_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <math.h>

#define SPI_SPEED_HZ 1000000

spi_device_t* spi_create(const char* device_path) {
    spi_device_t* spi = malloc(sizeof(spi_device_t));
    if (!spi) {
        return NULL;
    }
    
    memset(spi, 0, sizeof(spi_device_t));
    spi->m_device_path = strdup(device_path ? device_path : "/dev/spidev0.0");
    spi->m_fd = -1;
    spi->m_mode = SPI_MODE_0;
    spi->m_speed = SPI_SPEED_HZ;
    spi->m_bits_per_word = 8;
    
    return spi;
}

spi_device_t* spi_create_mock(void) {
    spi_device_t* spi = spi_create("/mock/spi");
    if (spi) {
        spi->m_initialized = true;
    }
    return spi;
}

bool spi_initialize(spi_device_t* spi, uint8_t mode, uint32_t speed) {
    if (!spi) {
        return false;
    }
    
    spi->m_mode = mode;
    spi->m_speed = speed;
    
    // Try to open real SPI device
    spi->m_fd = open(spi->m_device_path, O_RDWR);
    if (spi->m_fd < 0) {
        // Use mock mode
        spi->m_initialized = true;
        return true;
    }
    
    // Set SPI mode
    uint8_t mode_val = mode;
    if (ioctl(spi->m_fd, SPI_IOC_WR_MODE, &mode_val) < 0) {
        perror("Failed to set SPI mode");
        close(spi->m_fd);
        return false;
    }
    
    // Set SPI speed
    if (ioctl(spi->m_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to set SPI speed");
        close(spi->m_fd);
        return false;
    }
    
    // Set bits per word
    if (ioctl(spi->m_fd, SPI_IOC_WR_BITS_PER_WORD, &spi->m_bits_per_word) < 0) {
        perror("Failed to set bits per word");
        close(spi->m_fd);
        return false;
    }
    
    spi->m_initialized = true;
    return true;
}

void spi_destroy(spi_device_t* spi) {
    if (!spi) {
        return;
    }
    
    if (spi->m_fd >= 0) {
        close(spi->m_fd);
    }
    
    if (spi->m_device_path) {
        free(spi->m_device_path);
    }
    
    free(spi);
}

bool spi_transfer(spi_device_t* spi, uint8_t* tx_data, uint8_t* rx_data, sample_count_t len) {
    if (!spi || !spi->m_initialized || !tx_data || !rx_data) {
        return false;
    }
    
    if (spi->m_fd < 0) {
        // Mock implementation - echo with some modification
        for (sample_count_t i = 0; i < len; i++) {
            rx_data[i] = tx_data[i] ^ 0xFF; // Invert for mock response
        }
        return true;
    }
    
    // Real SPI implementation
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_data,
        .rx_buf = (unsigned long)rx_data,
        .len = len,
        .speed_hz = spi->m_speed,
        .delay_usecs = 0,
        .bits_per_word = spi->m_bits_per_word,
    };
    
    if (ioctl(spi->m_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("SPI transfer failed");
        return false;
    }
    
    return true;
}

imu_data_t spi_read_imu_data(spi_device_t* spi) {
    (void)spi; // Suppress unused parameter warning
    imu_data_t data = {0};
    
    // Mock IMU data generation
    static uint32_t counter = 0;
    counter++;
    
    // Simulate accelerometer data (movement)
    data.m_accelerometer_x = (accelerometer_data_t)(2048 + (sin(counter * 0.1) * 1024));
    data.m_accelerometer_y = (accelerometer_data_t)(2048 + (cos(counter * 0.15) * 1024));
    data.m_accelerometer_z = (accelerometer_data_t)(2048 + (sin(counter * 0.05) * 512));
    
    // Simulate gyroscope data (rotation)
    data.m_gyroscope_x = (gyroscope_data_t)(2048 + (sin(counter * 0.2) * 1024));
    data.m_gyroscope_y = (gyroscope_data_t)(2048 + (cos(counter * 0.25) * 1024));
    data.m_gyroscope_z = (gyroscope_data_t)(2048 + (sin(counter * 0.1) * 512));
    
    // Simulate magnetometer data (orientation)
    data.m_magnetometer_x = (magnetometer_data_t)(2048 + (sin(counter * 0.03) * 2048));
    data.m_magnetometer_y = (magnetometer_data_t)(2048 + (cos(counter * 0.04) * 2048));
    data.m_magnetometer_z = (magnetometer_data_t)(2048 + (sin(counter * 0.02) * 1024));
    
    data.m_timestamp = time(NULL);
    
    return data;
}

void spi_simulate_motion(spi_device_t* spi) {
    (void)spi; // Suppress unused parameter warning
    // This would trigger motion simulation in the IMU data
    // The actual simulation happens in spi_read_imu_data()
}
