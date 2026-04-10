/*
 * lcd.h
 *
 *  Created on: Apr 5, 2026
 *      Author: sumit
 */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include <stddef.h>

#define LCD_LOG_ENABLE
#define LCD_ADDRESS 0x3E
#define LCD_I2C_TIMEOUT_MS 1000

typedef struct {
    char str1[17];
    char str2[17];
} lcd_msg_t;

uint8_t lcd_init(void);
uint8_t lcd_clear(void);
uint8_t lcd_msg_left(const char *str1, const char *str2);
uint8_t lcd_msg_middle(const char *str1, const char *str2);
uint8_t lcd_enqueue_msg(const char *str1, const char *str2);

#endif /* LCD_H_ */
