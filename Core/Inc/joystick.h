#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "stm32f4xx_hal.h"

typedef enum {
    JOY_NONE,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT
} JoystickDirection;

typedef struct {
    ADC_HandleTypeDef *hadc;
    GPIO_TypeDef *btn_port;
    uint16_t btn_pin;
    uint16_t threshold_low;
    uint16_t threshold_high;
    uint16_t deadzone;
    uint32_t vrx;
    uint32_t vry;
} Joystick_HandleTypeDef;

void joystick_init(Joystick_HandleTypeDef *hjoy,
                   ADC_HandleTypeDef *hadc,
                   GPIO_TypeDef *btn_port, uint16_t btn_pin);

void joystick_read(Joystick_HandleTypeDef *hjoy);
JoystickDirection joystick_get_direction(Joystick_HandleTypeDef *hjoy);
uint8_t joystick_button_pressed(Joystick_HandleTypeDef *hjoy);

#endif
