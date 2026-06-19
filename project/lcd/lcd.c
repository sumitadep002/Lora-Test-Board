/*
 * lcd.c
 * * Dynamically supports both:
 * 1. RG1602A-19-I2C (Native I2C Controller - 0x3E)
 * 2. PCF8574T Backpack Variant (V13 Style - 0x27)
 *
 * Created on: Apr 5, 2026
 * Author: sumit
 */

#include "lcd.h"

// Standard header files
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Project header files
#include "main.h"

#include "cmsis_os.h"

// FreeRTOS Handles
static osMessageQueueId_t lcd_msg_queue = NULL;
static osThreadId_t lcd_task_handle = NULL;

// LCD Task Prototypes
static void lcd_task(void *argument);
static uint8_t lcd_hw_init(void);

// Externally defined I2C handle from main.c
extern I2C_HandleTypeDef hi2c1;

// --- Target Identification ---
#define LCD_ADDR_NATIVE (0x3E << 1)
#define LCD_ADDR_BACKPACK (0x27 << 1)

typedef enum
{
    LCD_TYPE_UNKNOWN = 0,
    LCD_TYPE_NATIVE,  // RG1602A-19 Direct Native I2C
    LCD_TYPE_BACKPACK // PCF8574T Backpack
} lcd_type_t;

typedef struct
{
    uint8_t i2c_addr;
    lcd_type_t type;
    uint8_t presence;
} lcd_ctrl_t;

static lcd_ctrl_t lcd_disp = {0, LCD_TYPE_UNKNOWN, 0};

// --- Native Controller Protocol Bytes ---
#define LCD_CTRL_CMD 0x00
#define LCD_CTRL_DATA 0x40

// LCD Commands
#define LCD_CMD_CLEAR 0x01
#define LCD_CMD_HOME 0x02
#define LCD_CMD_ENTRY_MODE 0x06
#define LCD_CMD_DISP_OFF 0x08
#define LCD_CMD_DISP_ON 0x0C
#define LCD_CMD_DISP_ON_CUR 0x0E
#define LCD_CMD_DISP_ON_BLK 0x0F

// Cursor Positioning
#define LCD_ROW_0 0x80
#define LCD_ROW_1 0xC0

// --- Error Logging Macro ---
#if defined(LCD_LOG_ENABLE)
#define LCD_LOG_ERR(...) printf("LCD [ERR]: " __VA_ARGS__)
#else
#define LCD_LOG_ERR(...)
#endif

// LCD Local functions declarations
static uint8_t lcd_scan(void);
static uint8_t lcd_compose_byte(uint8_t rs, uint8_t rw, uint8_t en, uint8_t bl, uint8_t byte_data, uint8_t nibble_type);
static uint8_t lcd_send_command(uint8_t cmd);
static uint8_t lcd_send_data(uint8_t data);
static uint8_t lcd_write_nibble(uint8_t rs, uint8_t data, uint8_t nibble_type);

/**
 * @brief Runtime evaluation scan to auto-detect hardware layout
 */
static uint8_t lcd_scan(void)
{
    // 1. Check for Native Controller (0x3E)
    if (HAL_I2C_IsDeviceReady(&hi2c1, LCD_ADDR_NATIVE, 1, LCD_I2C_TIMEOUT_MS) == HAL_OK)
    {
        lcd_disp.i2c_addr = LCD_ADDR_NATIVE;
        lcd_disp.type = LCD_TYPE_NATIVE;
        return 0;
    }

    // 2. Check for PCF8574 Backpack (0x27)
    if (HAL_I2C_IsDeviceReady(&hi2c1, LCD_ADDR_BACKPACK, 1, LCD_I2C_TIMEOUT_MS) == HAL_OK)
    {
        lcd_disp.i2c_addr = LCD_ADDR_BACKPACK;
        lcd_disp.type = LCD_TYPE_BACKPACK;
        return 0;
    }

    LCD_LOG_ERR("Unable to detect any valid LCD footprint on I2C bus\r\n");
    return 0xFF;
}

/**
 * @brief Initialize hardware dynamic initialization steps
 */
static uint8_t lcd_hw_init(void)
{
    if (lcd_scan() != 0)
    {
        LCD_LOG_ERR("Hardware initialization failed - device not found\r\n");
        return 0xFF;
    }

    osDelay(50); // Give hardware plenty of time to stabilize VCC
    lcd_disp.presence = 1;

    if (lcd_disp.type == LCD_TYPE_BACKPACK)
    {
        // Explicit 4-bit Handshake Sequence for PCF8574 Backpacks
        lcd_write_nibble(0, 0x30, 0);
        osDelay(5);
        lcd_write_nibble(0, 0x30, 0);
        osDelay(1);
        lcd_write_nibble(0, 0x30, 0);
        osDelay(1);
        lcd_write_nibble(0, 0x20, 0);
        osDelay(1); // Set to 4-bit interface

        lcd_send_command(0x28); // 4-bit execution mode, 2 rows, 5x8 font
    }
    else // LCD_TYPE_NATIVE
    {
        lcd_send_command(0x38); // Native mode configuration (8-bit bus)
    }

    // Common Configuration Parameters
    lcd_send_command(LCD_CMD_DISP_ON);
    lcd_send_command(LCD_CMD_ENTRY_MODE);
    lcd_clear();

    return 0;
}

uint8_t lcd_init(void)
{
    // 1. Create LCD Message Queue
    lcd_msg_queue = osMessageQueueNew(5, sizeof(lcd_msg_t), NULL);
    if (lcd_msg_queue == NULL)
    {
        LCD_LOG_ERR("Failed to create LCD message queue\r\n");
        return 0xEE;
    }

    // 2. Create LCD Manager Task (Consumer)
    const osThreadAttr_t lcd_task_attributes = {
        .name = "lcdTask",
        .stack_size = 256 * 4,
        .priority = (osPriority_t)osPriorityNormal,
    };
    lcd_task_handle = osThreadNew(lcd_task, NULL, &lcd_task_attributes);
    if (lcd_task_handle == NULL)
    {
        LCD_LOG_ERR("Failed to create LCD task\r\n");
        return 0xED;
    }

    return 0;
}

/**
 * @brief Consumer task that waits for messages and updates the LCD
 */
static void lcd_task(void *argument)
{
    lcd_msg_t msg;

    // Initialize hardware
    lcd_hw_init();

    for (;;)
    {
        if (osMessageQueueGet(lcd_msg_queue, &msg, NULL, osWaitForever) == osOK)
        {
            lcd_msg_middle(msg.str1, msg.str2);
        }
    }
}

uint8_t lcd_enqueue_msg(const char *str1, const char *str2)
{
    if (lcd_msg_queue == NULL)
    {
        return 0xEF;
    }

    lcd_msg_t msg;
    memset(&msg, 0, sizeof(lcd_msg_t));

    if (str1)
        strncpy(msg.str1, str1, 16);
    if (str2)
        strncpy(msg.str2, str2, 16);

    if (osMessageQueuePut(lcd_msg_queue, &msg, 0, 0) != osOK)
    {
        return 0xEB; // Queue full or error
    }

    return 0;
}

/**
 * @brief Map control bits to PCF8574 expansion pinout configuration
 */
static uint8_t lcd_compose_byte(uint8_t rs, uint8_t rw, uint8_t en, uint8_t bl, uint8_t byte_data, uint8_t nibble_type)
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

/**
 * @brief Helper for discrete 4-bit edge writes via the PCF8574
 */
static uint8_t lcd_write_nibble(uint8_t rs, uint8_t data, uint8_t nibble_type)
{
    uint8_t buffer[2];
    uint8_t bl = 1; // Keep backlight functional ON

    buffer[0] = lcd_compose_byte(rs, 0, 1, bl, data, nibble_type); // EN High
    buffer[1] = lcd_compose_byte(rs, 0, 0, bl, data, nibble_type); // EN Low Edge

    if (HAL_I2C_Master_Transmit(&hi2c1, lcd_disp.i2c_addr, buffer, 2, LCD_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return 0xFF;
    }
    osDelay(1); // Short delay to allow LCD to process nibble
    return 0;
}

uint8_t lcd_clear(void)
{
    uint8_t status = lcd_send_command(LCD_CMD_CLEAR);

    osDelay(2); // This shall not be removed

    return status;
}

/**
 * @brief Send a command byte to the LCD
 */
static uint8_t lcd_send_command(uint8_t cmd)
{
    if (lcd_disp.type == LCD_TYPE_BACKPACK)
    {
        if (lcd_write_nibble(0, cmd, 0) != 0)
            return 0xFF; // Upper
        if (lcd_write_nibble(0, cmd, 1) != 0)
            return 0xFF; // Lower
        return 0;
    }
    else // LCD_TYPE_NATIVE
    {
        uint8_t buffer[2] = {LCD_CTRL_CMD, cmd};
        if (HAL_I2C_Master_Transmit(&hi2c1, lcd_disp.i2c_addr, buffer, 2, LCD_I2C_TIMEOUT_MS) != HAL_OK)
        {
            LCD_LOG_ERR("I2C Transmit Error (CMD 0x%02X)\r\n", cmd);
            return 0xFF;
        }
        return 0;
    }
}

/**
 * @brief Send a data byte to the LCD
 */
static uint8_t lcd_send_data(uint8_t data)
{
    if (lcd_disp.type == LCD_TYPE_BACKPACK)
    {
        if (lcd_write_nibble(1, data, 0) != 0)
            return 0xFF; // Upper
        if (lcd_write_nibble(1, data, 1) != 0)
            return 0xFF; // Lower

        osDelay(1); // Short delay to allow data processing by LCD
        return 0;
    }
    else // LCD_TYPE_NATIVE
    {
        uint8_t buffer[2] = {LCD_CTRL_DATA, data};
        if (HAL_I2C_Master_Transmit(&hi2c1, lcd_disp.i2c_addr, buffer, 2, LCD_I2C_TIMEOUT_MS) != HAL_OK)
        {
            LCD_LOG_ERR("I2C Transmit Error (DATA 0x%02X)\r\n", data);
            return 0xFF;
        }
        return 0;
    }
}

uint8_t lcd_print_string(const char *str)
{
    if (str == NULL)
        return 0xFF;

    if (lcd_disp.type == LCD_TYPE_BACKPACK)
    {
        // PCF8574 must process and latch data byte-by-byte
        while (*str)
        {
            if (lcd_send_data((uint8_t)(*str++)) != 0)
                return 0xFF;
        }
    }
    else // LCD_TYPE_NATIVE
    {
        // Native controllers support direct block array streaming natively
        uint8_t buffer[17];
        uint8_t index = 0;

        buffer[index++] = LCD_CTRL_DATA;
        while (*str && index < sizeof(buffer))
        {
            buffer[index++] = (uint8_t)(*str++);
        }

        if (HAL_I2C_Master_Transmit(&hi2c1, lcd_disp.i2c_addr, buffer, index, LCD_I2C_TIMEOUT_MS) != HAL_OK)
        {
            LCD_LOG_ERR("I2C Transmit Error (STRING)\r\n");
            return 0xFF;
        }
    }

    return 0;
}

uint8_t lcd_msg_left(const char *str1, const char *str2)
{
    if (str1 == NULL || str2 == NULL || strlen(str1) > 16 || strlen(str2) > 16)
    {
        LCD_LOG_ERR("Msg Left Error - Invalid string lengths or NULL pointers\r\n");
        return 0xff;
    }

    if (lcd_clear() != 0)
    {
        return 0xfe;
    }

    if (lcd_send_command(LCD_ROW_0) != 0)
    {
        return 0xfd;
    }

    if (lcd_print_string(str1) != 0)
    {
        return 0xfc;
    }

    if (lcd_send_command(LCD_ROW_1) != 0)
    {
        return 0xfb;
    }

    if (lcd_print_string(str2) != 0)
    {
        return 0xfa;
    }

    return 0;
}

uint8_t lcd_msg_middle(const char *str1, const char *str2)
{
    if (str1 == NULL || str2 == NULL || strlen(str1) > 16 || strlen(str2) > 16)
    {
        LCD_LOG_ERR("Msg Middle Error - Invalid string lengths or NULL pointers\r\n");
        return 0xff;
    }

    if (lcd_clear() != 0)
    {
        return 0xfe;
    }

    if (lcd_send_command(LCD_ROW_0 + (16 - strlen(str1)) / 2) != 0)
    {
        return 0xfd;
    }

    if (lcd_print_string(str1) != 0)
    {
        return 0xfc;
    }

    if (lcd_send_command(LCD_ROW_1 + (16 - strlen(str2)) / 2) != 0)
    {
        return 0xfb;
    }

    if (lcd_print_string(str2) != 0)
    {
        return 0xfa;
    }

    return 0;
}
