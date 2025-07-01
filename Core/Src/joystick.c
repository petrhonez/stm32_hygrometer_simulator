#include "joystick.h"

void joystick_init(Joystick_HandleTypeDef *hjoy,
                   ADC_HandleTypeDef *hadc,
                   GPIO_TypeDef *btn_port, uint16_t btn_pin)
{
    hjoy->hadc = hadc;
    hjoy->btn_port = btn_port;
    hjoy->btn_pin = btn_pin;
    hjoy->threshold_low = 1300;
    hjoy->threshold_high = 2700;
    hjoy->deadzone = 300;
    hjoy->vrx = 0;
    hjoy->vry = 0;
}

void joystick_read(Joystick_HandleTypeDef *hjoy)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    // Lê eixo X (PA3 - ADC_CHANNEL_3)
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(hjoy->hadc, &sConfig);
    HAL_ADC_Start(hjoy->hadc);
    HAL_ADC_PollForConversion(hjoy->hadc, HAL_MAX_DELAY);
    hjoy->vrx = HAL_ADC_GetValue(hjoy->hadc);
    HAL_ADC_Stop(hjoy->hadc);

    // Lê eixo Y (PA4 - ADC_CHANNEL_4)
    sConfig.Channel = ADC_CHANNEL_4;
    HAL_ADC_ConfigChannel(hjoy->hadc, &sConfig);
    HAL_ADC_Start(hjoy->hadc);
    HAL_ADC_PollForConversion(hjoy->hadc, HAL_MAX_DELAY);
    hjoy->vry = HAL_ADC_GetValue(hjoy->hadc);
    HAL_ADC_Stop(hjoy->hadc);
}

JoystickDirection joystick_get_direction(Joystick_HandleTypeDef *hjoy)
{
    if (hjoy->vry > hjoy->threshold_high) return JOY_UP;
    if (hjoy->vry < hjoy->threshold_low) return JOY_DOWN;
    if (hjoy->vrx > hjoy->threshold_high) return JOY_RIGHT;
    if (hjoy->vrx < hjoy->threshold_low) return JOY_LEFT;
    return JOY_NONE;
}

uint8_t joystick_button_pressed(Joystick_HandleTypeDef *hjoy)
{
    return HAL_GPIO_ReadPin(hjoy->btn_port, hjoy->btn_pin) == GPIO_PIN_RESET;
}
