/*
 * lcd.c
 *
 *  Created on: Apr 5, 2026
 *      Author: sumit
 */

// Standard hedaer files
#include "stdio.h"

// Project header files
#include "main.h"

#include "lcd.h"

// Externally defined I2C handle from main.c
extern I2C_HandleTypeDef hi2c1;

uint8_t lcd_init(void)
{
    // Init I2C1 before calling this function
    // Scan the bus for the LCD address (0x3E)
    for (uint8_t add = 1; add < 128; add++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (add << 1), 1, 10) == HAL_OK)
        {
            printf("LCD Detected at 0x%02X\r\n", add);
            return add;
        }
    }
    return 0; // Return 0 if LCD is not found
}