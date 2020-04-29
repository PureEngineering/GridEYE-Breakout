#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

// typedef unsigned char uint8_t;

// Global file descriptor used to talk to the I2C bus:
int i2c_fd = -1;
// device name for the I2C bus
const char *i2c_fname = "/dev/i2c-17";

#define I2C_SLAVE_ADDR 0x69

// Returns a new file descriptor for communicating with the I2C bus:
int i2c_init(void)
{
    if ((i2c_fd = open(i2c_fname, O_RDWR)) < 0)
    {
        char err[200];
        sprintf(err, "open('%s') in i2c_init, (modify i2c_fname in source)", i2c_fname);
        perror(err);
        return -1;
    }

    return i2c_fd;
}

// Write to an I2C slave device's register:
int i2c_write(uint8_t slave_addr, uint8_t reg, uint8_t data)
{
    int retval;
    uint8_t outbuf[2];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = reg;
    outbuf[1] = data;

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0)
    {
        perror("ioctl(I2C_RDWR) in i2c_write");
        return -1;
    }

    return 0;
}

// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(uint8_t slave_addr, uint8_t reg, uint8_t *result)
{
    int retval;
    uint8_t outbuf[1], inbuf[1];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = outbuf;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = 1;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = reg;

    inbuf[0] = 0;

    *result = 0;
    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0)
    {
        perror("ioctl(I2C_RDWR) in i2c_read");
        return -1;
    }

    *result = inbuf[0];
    return 0;
}

//to compile and run: gcc main.c ; sudo ./a.out
//modify i2c_fname to match i2c port
void main()
{
    int i, j;
    uint8_t lower_byte;
    uint8_t upper_byte;
    uint8_t buffer[64];
    int pixel;
    signed short image[64];
    signed short average[64];
    float temperature[64];
    float max_temperature = 0;
    float min_temperature = 99e99;

    for (i = 0; i < 64; i++)
    {
        average[i] = 80;
    }

    if (i2c_init() < 0)
    {
        exit(1);
    }

    i2c_write(I2C_SLAVE_ADDR, 0x01, 0x3f); //full reset

    i2c_write(I2C_SLAVE_ADDR, 0x07, 1<<5); //enable twice moving average 
    while (1)
    {

        for (i = 0; i < 64; i++)
        {
            i2c_read(I2C_SLAVE_ADDR, 0x80 + i * 2, &lower_byte);
            i2c_read(I2C_SLAVE_ADDR, 0x80 + i * 2 + 1, &upper_byte);

            image[i] = ((upper_byte << 8) | lower_byte);
            if (image[i] > 2047)
            {
                image[i] = image[i] - 4096;
            }
            temperature[i] = image[i] * .25;

            average[i] = (average[i] * 9 + image[i]) / 10;
        }

        printf("\033[H\033[J"); //clear screen
        printf("Raw Values\n");
        for (i = 0; i < 8; i++)
        {
            for (j = 7; j >= 0; j--)
            {
                pixel = image[i * 8 + j];
                printf("%*d ", 3, pixel);
            }
            printf("\n");
        }

        max_temperature = 0;
        min_temperature = 99e99;
        printf("\nTemperature\n");
        for (i = 0; i < 8; i++)
        {
            for (j = 7; j >= 0; j--)
            {
                printf("%6.2f ", temperature[i * 8 + j]);
                if (temperature[i * 8 + j] > max_temperature)
                {
                    max_temperature = temperature[i * 8 + j];
                }
                if (temperature[i * 8 + j] < min_temperature)
                {
                    min_temperature = temperature[i * 8 + j];
                }
            }
            printf("\n");
        }

        printf("\nMax Temperature: %6.2f C (%6.2f F)    Min Temperature: %6.2f C (%6.2f F) \n", max_temperature, max_temperature * 9 / 5, min_temperature, min_temperature * 9 / 5);

        printf("\nBackground Subtraction\n");
        for (i = 0; i < 8; i++)
        {
            for (j = 7; j >= 0; j--)
            {
                pixel = (image[i * 8 + j] - average[i * 8 + j]);
                printf("%*d ", 3, pixel);
            }
            printf("\n");
        }

        printf("\nDemo\n");
        for (i = 0; i < 8; i++)
        {
            for (j = 7; j >= 0; j--)
            {
                pixel = (image[i * 8 + j] - average[i * 8 + j]);
                if ((pixel) > 10)
                {
                    printf("%*d ", 3, image[i * 8 + j]);
                }
                else
                {
                    printf("    ");
                }
            }
            printf("\n");
        }
        usleep(110 * 1000);
    }
}
