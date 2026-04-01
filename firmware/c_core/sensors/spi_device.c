#include "spi_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <time.h>

#define SPI_MODE_0 0
#define SPI_SPEED_HZ 1000000

spi_device_t* spi_create(const char* device_path) {
    spi_device_t* spi = malloc(sizeof(spi_device_t));
    if (!spi) {
        return NULL;
    }
    
    memset(spi, 0, sizeof(spi_device_t));
    spi->device_path = strdup(device_path ? device_path : "/dev/spidev0.0");
    spi->fd = -1;
    spi->mode = SPI_MODE_0;
    spi->speed = SPI_SPEED_HZ;
    spi->bits_per_word = 8;
    
    return spi;
}

spi_device_t* spi_create_mock(void) {
    spi_device_t* spi = spi_create("/mock/spi");
    if (spi) {
        spi->initialized = true;
    }
    return spi;
}

bool spi_initialize(spi_device_t* spi, uint8_t mode, uint32_t speed) {
    if (!spi) {
        return false;
    }
    
    spi->mode = mode;
    spi->speed = speed;
    
    // Try to open real SPI device
    spi->fd = open(spi->device_path, O_RDWR);
    if (spi->fd < 0) {
        // Use mock mode
        spi->initialized = true;
        return true;
    }
    
    // Set SPI mode
    uint8_t mode_val = mode;
    if (ioctl(spi->fd, SPI_IOC_WR_MODE, &mode_val) < 0) {
        perror("Failed to set SPI mode");
        close(spi->fd);
        return false;
    }
    
    // Set SPI speed
    if (ioctl(spi->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to set SPI speed");
        close(spi->fd);
        return false;
    }
    
    // Set bits per word
    if (ioctl(spi->fd, SPI_IOC_WR_BITS_PER_WORD, &spi->bits_per_word) < 0) {
        perror("Failed to set bits per word");
        close(spi->fd);
        return false;
    }
    
    spi->initialized = true;
    return true;
}

void spi_destroy(spi_device_t* spi) {
    if (!spi) {
        return;
    }
    
    if (spi->fd >= 0) {
        close(spi->fd);
    }
    
    if (spi->device_path) {
        free(spi->device_path);
    }
    
    free(spi);
}

bool spi_transfer(spi_device_t* spi, uint8_t* tx_data, uint8_t* rx_data, int len) {
    if (!spi || !spi->initialized || !tx_data || !rx_data) {
        return false;
    }
    
    if (spi->fd < 0) {
        // Mock implementation - echo with some modification
        for (int i = 0; i < len; i++) {
            rx_data[i] = tx_data[i] ^ 0xFF; // Invert for mock response
        }
        return true;
    }
    
    // Real SPI implementation
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_data,
        .rx_buf = (unsigned long)rx_data,
        .len = len,
        .speed_hz = spi->speed,
        .delay_usecs = 0,
        .bits_per_word = spi->bits_per_word,
    };
    
    if (ioctl(spi->fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("SPI transfer failed");
        return false;
    }
    
    return true;
}

imu_data_t spi_read_imu_data(spi_device_t* spi) {
    imu_data_t data = {0};
    
    // Mock IMU data generation
    static uint32_t counter = 0;
    counter++;
    
    // Simulate accelerometer data (movement)
    data.accelerometer_x = 2048 + (sin(counter * 0.1) * 1024);
    data.accelerometer_y = 2048 + (cos(counter * 0.15) * 1024);
    data.accelerometer_z = 2048 + (sin(counter * 0.05) * 512);
    
    // Simulate gyroscope data (rotation)
    data.gyroscope_x = 2048 + (sin(counter * 0.2) * 1024);
    data.gyroscope_y = 2048 + (cos(counter * 0.25) * 1024);
    data.gyroscope_z = 2048 + (sin(counter * 0.1) * 512);
    
    // Simulate magnetometer data (orientation)
    data.magnetometer_x = 2048 + (sin(counter * 0.03) * 2048);
    data.magnetometer_y = 2048 + (cos(counter * 0.04) * 2048);
    data.magnetometer_z = 2048 + (sin(counter * 0.02) * 1024);
    
    data.timestamp = time(NULL);
    
    return data;
}

void spi_simulate_motion(spi_device_t* spi) {
    // This would trigger motion simulation in the IMU data
    // The actual simulation happens in spi_read_imu_data()
}
