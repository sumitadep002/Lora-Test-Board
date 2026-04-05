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

// Slave Address
#define I2C_ADDR (LCD_ADDRESS << 1)

// LCD Commands
#define LCD_CMD_CLEAR_DISPLAY 0x01

// LCD Presence
static uint8_t lcd_presence = 0;

// LCD Local functions declarations
static uint8_t lcd_scan();
static uint8_t lcd_compose_byte(uint8_t rs, uint8_t rw, uint8_t en, uint8_t bl, uint8_t byte_data, uint8_t nibble_type);
static uint8_t lcd_send_command(uint8_t cmd);

uint8_t lcd_init(void)
{
    if (lcd_scan() != 0)
    {
        return 0xFF;
    }

    lcd_presence = 1;

    lcd_send_command(0x38); // Set to 8 bit, 2 row configuration

    printf("LCD: Initialization complete\r\n");

    return 0;
}

uint8_t lcd_scan()
{
    // Check if device responds at this address
    if (HAL_I2C_IsDeviceReady(&hi2c1, I2C_ADDR, 1, LCD_I2C_TIMEOUT_MS) == HAL_OK)
    {
#if defined(LCD_LOG_ENABLE)
        printf("LCD: Detected at 0x%02X\r\n", LCD_ADDRESS);
        return 0;
#endif
    }
#if defined(LCD_LOG_ENABLE)
    printf("LCD: Unable to detect at 0x%02X\r\n", LCD_ADDRESS);
    return 0;
#endif

    return 0xff; // Return 0xFF if LCD is not found
}

/**
 * @brief Compose a byte to send to I2C LCD via PCF8574
 *
 * @param rs          RS bit (0=command, 1=data)
 * @param rw          RW bit (0=write, 1=read)
 * @param en          EN bit (0=low, 1=high for pulse)
 * @param bl          Backlight bit (0=off, 1=on)
 * @param byte_data   Full byte to send (command or data)
 * @param nibble_type 0=high nibble, 1=low nibble
 * @return uint8_t Byte to send over I2C
 */
uint8_t lcd_compose_byte(uint8_t rs, uint8_t rw, uint8_t en, uint8_t bl, uint8_t byte_data, uint8_t nibble_type)
{
    uint8_t byte = 0;

    // Map control bits to PCF8574 pins
    byte |= (rs & 0x01) << 0; // P0 = RS
    byte |= (rw & 0x01) << 1; // P1 = RW
    byte |= (en & 0x01) << 2; // P2 = EN
    byte |= (bl & 0x01) << 3; // P3 = Backlight

    // Select nibble
    uint8_t nibble = (nibble_type == 0) ? (byte_data >> 4) & 0x0F : byte_data & 0x0F;

    // Put nibble on P4–P7
    byte |= nibble << 4;

    return byte;
}

uint8_t lcd_clear(void)
{
    return lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
}

/**
 * @brief Send a command byte to the LCD
 *
 * @param cmd 8-bit command to send
 *
 * Sends the high nibble first, then low nibble, each with an EN pulse to latch.
 */
uint8_t lcd_send_command(uint8_t cmd)
{
    uint8_t buffer[2];
    buffer[0] = 0x00; // Control byte: RS = 0
    buffer[1] = cmd;  // The actual command

    if (HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR, buffer, sizeof(buffer), LCD_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return 0xFF;
    }
    return 0;
}