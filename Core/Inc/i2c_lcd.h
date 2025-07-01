#ifndef I2C_LCD_H
#define I2C_LCD_H

#include <stdint.h>

/**
 * @brief Includes the HAL driver present in the project
 */
#if __has_include("stm32f4xx_hal.h")
	#include "stm32f4xx_hal.h"
#elif __has_include("stm32c0xx_hal.h")
	#include "stm32c0xx_hal.h"
#elif __has_include("stm32g4xx_hal.h")
	#include "stm32g4xx_hal.h"
#endif

/**
 * @brief Structure to hold LCD instance information
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;     // I2C handler for communication
    uint8_t address;            // I2C address of the LCD
    uint8_t backlight; 			// novo campo backlight  0x08 se ligado, 0x00 se desligado

} I2C_LCD_HandleTypeDef;

/**
 * @brief Initializes the LCD.
 * @param lcd: Pointer to the LCD handle
 */
void lcd_init(I2C_LCD_HandleTypeDef *lcd);

/**
 * @brief Sends a command to the LCD.
 * @param lcd: Pointer to the LCD handle
 * @param cmd: Command byte to send
 */
void lcd_send_cmd(I2C_LCD_HandleTypeDef *lcd, char cmd);

/**
 * @brief Sends data (character) to the LCD.
 * @param lcd: Pointer to the LCD handle
 * @param data: Data byte to send
 */
void lcd_send_data(I2C_LCD_HandleTypeDef *lcd, char data);

/**
 * @brief Sends a single character to the LCD.
 * @param lcd: Pointer to the LCD handle
 * @param ch: Character to send
 */
void lcd_putchar(I2C_LCD_HandleTypeDef *lcd, char ch);

/**
 * @brief Sends a string to the LCD.
 * @param lcd: Pointer to the LCD handle
 * @param str: Null-terminated string to send
 */
void lcd_puts(I2C_LCD_HandleTypeDef *lcd, char *str);


/**
 * @brief Moves the cursor to a specific position on the LCD.
 * @param lcd: Pointer to the LCD handle
 * @param col: Column number (0-15)
 * @param row: Row number (0 or 1)
 */
void lcd_gotoxy(I2C_LCD_HandleTypeDef *lcd, int col, int row);

/**
 * @brief Clears the LCD display.
 * @param lcd: Pointer to the LCD handle
 */
void lcd_clear(I2C_LCD_HandleTypeDef *lcd);
/**
 * @brief Turns off the LCD display (content remains in memory).
 * @param lcd: Pointer to the LCD handle
 */
void lcd_display_off(I2C_LCD_HandleTypeDef *lcd);

/**
 * @brief Turns off the LCD display (content remains).
 * @param lcd: Pointer to the LCD handle
 */
void lcd_display_on(I2C_LCD_HandleTypeDef *lcd);

/**
 * @brief Turns on the LCD display (content remains).
 * @param lcd: Pointer to the LCD handle
 */
void lcd_backlight_off(I2C_LCD_HandleTypeDef *lcd);
void lcd_backlight_on(I2C_LCD_HandleTypeDef *lcd);

#endif /* I2C_LCD_H */
