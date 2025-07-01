/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c_lcd.h"
#include "core_cm4.h"
#include "joystick.h"
#include <stdio.h>
#include <math.h>

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);


/* Definições IR ------------------------------------------------------------------*/
#define NEC_ON      0x00FFE01F
#define NEC_OFF      0x00FF609F

#define NEC_RED     0x00FF906F
#define NEC_GREEN   0x00FF10EF
#define NEC_BLUE    0x00FF50AF
#define NEC_WHITE   0x00FFD02F

#define NEC_R4   0x00FFB04F
#define NEC_G4   0x00FF30CF
#define NEC_B4   0x00FF708F

#define NEC_R3   0x00FFA857
#define NEC_G3   0x00FF28D7
#define NEC_B3   0x00FF6897

#define NEC_R2   0x00FF9867
#define NEC_G2   0x00FF18E7
#define NEC_B2   0x00FF58A7

#define NEC_R1   0x00FF8877
#define NEC_G1   0x00FF08F7
#define NEC_B1   0x00FF48B7


#define NEC_HEADER_MARK   9000
#define NEC_HEADER_SPACE  4500
#define NEC_BIT_MARK      562
#define NEC_ONE_SPACE     1687
#define NEC_ZERO_SPACE    562
#define NEC_END_MARK      562

/* Periféricos ------------------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
I2C_LCD_HandleTypeDef lcd1;
Joystick_HandleTypeDef hjoy;

uint16_t read = 0;
float read_f = 0;
float temp = 0;

#define NUM_LEDS 5
GPIO_TypeDef* LED_PORT[NUM_LEDS] = {GPIOA, GPIOA, GPIOA, GPIOA, GPIOA};
uint16_t LED_PIN[NUM_LEDS] = {GPIO_PIN_1, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12};


/* Menu ------------------------------------------------------------------*/
const char *menu_items[] = {
    "Acender Lampada",
	"Apagar Lampada",
    "Mostrar Temp C",
	"Mostrar Temp K",
	"Mostar Umidade",
	"Luz LCD OFF",
	"Luz LCD ON"
};
volatile int menu_index = 0;

/* Funções IR ------------------------------------------------------------------*/
void DWT_Delay_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(uint32_t microseconds) {
    uint32_t clk_start = DWT->CYCCNT;
    uint32_t cycles = microseconds * (HAL_RCC_GetHCLKFreq() / 1000000);
    while ((DWT->CYCCNT - clk_start) < cycles);
}

void send_bit(uint8_t bit) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    DWT_Delay_us(NEC_BIT_MARK);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
    DWT_Delay_us(bit ? NEC_ONE_SPACE : NEC_ZERO_SPACE);
}

void send_nec(uint32_t data) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    DWT_Delay_us(NEC_HEADER_MARK);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
    DWT_Delay_us(NEC_HEADER_SPACE);
    for (int i = 31; i >= 0; i--) send_bit((data >> i) & 0x01);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    DWT_Delay_us(NEC_END_MARK);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
}


/* Auxiliares ------------------------------------------------------------------*/
void clear_all_leds(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i], GPIO_PIN_RESET);
    }
}

void atualizar_menu(void) {
    lcd_clear(&lcd1);
    lcd_gotoxy(&lcd1, 0, 0);
    lcd_puts(&lcd1, "Menu:");
    lcd_gotoxy(&lcd1, 0, 1);
    lcd_puts(&lcd1, menu_items[menu_index]);
}

float TempC(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint16_t read = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float read_f = (read * 3.3f) / 4095.0f;
    return ((read_f / 3.3f) * 55.0f) - 5.0f;
    return read_f;
}

float TempK(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint16_t read = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float read_f = (read * 3.3f) / 4095.0f;
    return (((read_f / 3.3f) * 55.0f) - 5.0f) + 273.15f;
}

float Umd1(void) {
	ADC_ChannelConfTypeDef sConfig = {0};
	    sConfig.Channel = ADC_CHANNEL_5;
	    sConfig.Rank = 1;
	    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	    HAL_ADC_Start(&hadc1);
	    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	    uint16_t read = HAL_ADC_GetValue(&hadc1);
	    HAL_ADC_Stop(&hadc1);

    float read_f = (read * 3.3f) / 4095.0f;
    return ((read_f / 3.3f) * 100.0f) - 0.0f;
}

void LCDTempC(float temp) {
    char buffer[16];
    lcd_clear(&lcd1);
    lcd_gotoxy(&lcd1, 0, 0);
    lcd_puts(&lcd1, "Temperatura:");

    lcd_gotoxy(&lcd1, 0, 1);
    int t_int = (int)temp;
    int t_dec = (int)(fabsf(temp - t_int) * 100);  // pega o valor absoluto dos decimais
    sprintf(buffer, "%+3d.%02d C", t_int, t_dec);  // mostra o sinal explicitamente
    lcd_puts(&lcd1, buffer);
}
void LCDTempK(float tempk) {
    char buffer[16];
    lcd_clear(&lcd1);
    lcd_gotoxy(&lcd1, 0, 0);
    lcd_puts(&lcd1, "Temperatura:");

    lcd_gotoxy(&lcd1, 0, 1);
    int tk_int = (int)tempk;
    int tk_dec = (int)((tempk - tk_int) * 100);
    sprintf(buffer, "%2d.%02d K", tk_int, tk_dec);
    lcd_puts(&lcd1, buffer);
}
void LCDUmd(float umd) {
    char buffer[16];
    lcd_clear(&lcd1);
    lcd_gotoxy(&lcd1, 0, 0);
    lcd_puts(&lcd1, "Umidade:");

    lcd_gotoxy(&lcd1, 0, 1);
    int t_int = (int)umd;
    int t_dec = (int)(fabsf(umd - t_int) * 100);  // pega o valor absoluto dos decimais
    sprintf(buffer, "%2d.%02d %%", t_int, t_dec);  // mostra o sinal explicitamente
    lcd_puts(&lcd1, buffer);
}
void ledsTempC(float temp) {
    int num = 0;
    if (temp > 5) num++;
    if (temp > 15) num++;
    if (temp > 25) num++;
    if (temp > 35) num++;
    if (temp > 45) num++;
    for (int i = 0; i < NUM_LEDS; i++) {
        HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i], (i < num) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void ledsTempK(float tempk) {
    int num = 0;
    if (tempk > 5 + 273.15) num++;
    if (tempk > 15 + 273.15) num++;
    if (tempk > 25 + 273.15) num++;
    if (tempk > 35 + 273.15) num++;
    if (tempk > 45 + 273.15) num++;
    for (int i = 0; i < NUM_LEDS; i++) {
        HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i], (i < num) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void ledsUmd(float umd) {
    int num = 0;
    if (umd > 20) num++;
    if (umd > 40) num++;
    if (umd > 60) num++;
    if (umd > 80) num++;
    if (umd > 100) num++;
    for (int i = 0; i < NUM_LEDS; i++) {
        HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i], (i < num) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
/* Proteções ------------------------------------------------------------------*/

void protecao_umda(float umd) {
    static uint32_t t_inicial = 0;
    static uint8_t em_alerta = 0;
    static uint32_t fase_timer = 0;
    static uint8_t fase_pwm = 0;

    if (umd >= 30.0f) {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        em_alerta = 0;
        return;
    }

    uint32_t tempo_pwm = 2000;
    if (umd < 29) tempo_pwm = 500;
    if (umd < 28) tempo_pwm = 450;
    if (umd < 27) tempo_pwm = 400;
    if (umd < 26) tempo_pwm = 350;
    if (umd < 25) tempo_pwm = 300;
    if (umd < 24) tempo_pwm = 250;
    if (umd < 23) tempo_pwm = 200;
    if (umd < 22) tempo_pwm = 150;
    if (umd < 21) tempo_pwm = 100;
    if (umd < 20) tempo_pwm = 50;

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    if (HAL_GetTick() - fase_timer >= tempo_pwm) {
        int duty = (fase_pwm % 2 == 0) ? 10 : 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (__HAL_TIM_GET_AUTORELOAD(&htim3) * duty) / 100);
        fase_timer = HAL_GetTick();
        fase_pwm++;
    }

    if (umd <= 20.0f) {
        if (!em_alerta) {
            t_inicial = HAL_GetTick();
            em_alerta = 1;
        }

        if ((HAL_GetTick() - t_inicial) >= 10000) {
            if (Umd1() <= 20.0f) {
            	lcd_gotoxy(&lcd1, 0, 0);
				lcd_puts(&lcd1, "Umidade baixa:");
				lcd_gotoxy(&lcd1, 0, 1);
				lcd_puts(&lcd1, "Alerta!");

                while (Umd1() <= 25.0f) HAL_Delay(500);

                lcd_clear(&lcd1);
                lcd_puts(&lcd1, "Estabilizando");
                HAL_Delay(1000);
            } else {
                em_alerta = 0;
            }
        }
    } else {
        em_alerta = 0;
    }
}
void protecao_umdb(float umd) {
    static uint32_t t_inicial = 0;
    static uint8_t em_alerta = 0;
    static uint32_t fase_timer = 0;
    static uint8_t fase_pwm = 0;

    if (umd < 70.0f) {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
        em_alerta = 0;
        return;
    }

    uint32_t tempo_pwm = 2000;
    if (umd > 71) tempo_pwm = 500;
    if (umd > 72) tempo_pwm = 450;
    if (umd > 73) tempo_pwm = 400;
    if (umd > 74) tempo_pwm = 350;
    if (umd > 75) tempo_pwm = 300;
    if (umd > 76) tempo_pwm = 250;
    if (umd > 77) tempo_pwm = 200;
    if (umd > 78) tempo_pwm = 150;
    if (umd > 79) tempo_pwm = 100;
    if (umd > 80) tempo_pwm = 50;

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    if (HAL_GetTick() - fase_timer >= tempo_pwm) {
        int duty = (fase_pwm % 2 == 0) ? 10 : 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, (__HAL_TIM_GET_AUTORELOAD(&htim3) * duty) / 100);
        fase_timer = HAL_GetTick();
        fase_pwm++;
    }

    if (umd >= 80.0f) {
        if (!em_alerta) {
            t_inicial = HAL_GetTick();
            em_alerta = 1;
        }

        if ((HAL_GetTick() - t_inicial) >= 10000) {
            if (Umd1() >= 80.0f) {
						lcd_gotoxy(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Umidade alta:");
						lcd_gotoxy(&lcd1, 0, 1);
						lcd_puts(&lcd1, "Alerta!");
					while (Umd1() >= 75.0f) HAL_Delay(500);

					lcd_clear(&lcd1);
					lcd_puts(&lcd1, "Estabilizando");
					HAL_Delay(1000);

            } else {
                em_alerta = 0;
            }
        }
    } else {
        em_alerta = 0;
    }
}
void protecao_temperatura(float temp) {
    static uint32_t t_inicial = 0;
    static uint8_t em_alerta = 0;
    static uint32_t fase_timer = 0;
    static uint8_t fase_pwm = 0;

    if (temp <= 40.0f) {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        em_alerta = 0;
        return;
    }

    uint32_t tempo_pwm = 2000;
    if (temp > 41) tempo_pwm = 500;
    if (temp > 42) tempo_pwm = 450;
    if (temp > 43) tempo_pwm = 400;
    if (temp > 44) tempo_pwm = 350;
    if (temp > 45) tempo_pwm = 300;
    if (temp > 46) tempo_pwm = 250;
    if (temp > 47) tempo_pwm = 200;
    if (temp > 48) tempo_pwm = 150;
    if (temp > 49) tempo_pwm = 100;

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    if (HAL_GetTick() - fase_timer >= tempo_pwm) {
        int duty = (fase_pwm % 2 == 0) ? 10 : 100;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (__HAL_TIM_GET_AUTORELOAD(&htim3) * duty) / 100);
        fase_timer = HAL_GetTick();
        fase_pwm++;
    }

    if (temp >= 48.0f) {
        if (!em_alerta) {
            t_inicial = HAL_GetTick();
            em_alerta = 1;
        }

        if ((HAL_GetTick() - t_inicial) >= 10000) {
            if (TempC() >= 48.0f) {
                HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                lcd_clear(&lcd1);
                lcd_puts(&lcd1, "Superaquecimento!");
                lcd_backlight_off(&lcd1);
                send_nec(NEC_OFF);
                clear_all_leds();
                HAL_Delay(10000);

                lcd_clear(&lcd1);
                lcd_puts(&lcd1, "Aguardando...");
                while (TempC() >= 40.0f) HAL_Delay(500);

                lcd_clear(&lcd1);
                lcd_puts(&lcd1, "Sistema OK");
                HAL_Delay(1000);
                lcd_backlight_on(&lcd1);
                send_nec(NEC_ON);
            } else {
                em_alerta = 0;
            }
        }
    } else {
        em_alerta = 0;
    }
}
void protecao_kelvin(float tempk) {
    protecao_temperatura(tempk - 273.15f);
}
/* Lampada ------------------------------------------------------------------*/
void RGBC(float temp){
	if (temp < 0.0f) {send_nec(NEC_G1);}
	else if (temp < 5.0f) {send_nec(NEC_G2);}
	else if (temp < 10.0f) {send_nec(NEC_G3);}
	else if (temp < 15.0f) {send_nec(NEC_G4);}
	else if (temp < 20.0f) {send_nec(NEC_WHITE);}
	else if (temp < 25.0f) {send_nec(NEC_R1);}
	else if (temp < 30.0f) {send_nec(NEC_R2);}
	else if (temp < 35.0f) {send_nec(NEC_R3);}
	else if (temp < 40.0f) {send_nec(NEC_R4);}
	else if (temp < 45.0f) {send_nec(NEC_RED);}
}

void RGBK(float tempk){
	if (tempk < 0.0f + 273.15f) {send_nec(NEC_G1);}
	else if (tempk < 5.0f + 273.15f) {send_nec(NEC_G2);}
	else if (tempk < 10.0f + 273.15f) {send_nec(NEC_G3);}
	else if (tempk < 15.0f + 273.15f) {send_nec(NEC_G4);}
	else if (tempk < 20.0f + 273.15f) {send_nec(NEC_WHITE);}
	else if (tempk < 25.0f + 273.15f) {send_nec(NEC_R1);}
	else if (tempk < 30.0f + 273.15f) {send_nec(NEC_R2);}
	else if (tempk < 35.0f + 273.15f) {send_nec(NEC_R3);}
	else if (tempk < 40.0f + 273.15f) {send_nec(NEC_R4);}
	else if (tempk < 45.0f + 273.15f) {send_nec(NEC_RED);};
}

void RGBU(float umd){
	if (umd < 0.0f) {send_nec(NEC_R1);}
	else if (umd < 10.0f) {send_nec(NEC_R2);}
	else if (umd < 20.0f) {send_nec(NEC_R3);}
	else if (umd < 30.0f) {send_nec(NEC_R4);}
	else if (umd < 40.0f) {send_nec(NEC_RED);}
	else if (umd < 50.0f) {send_nec(NEC_B1);}
	else if (umd < 60.0f) {send_nec(NEC_B2);}
	else if (umd < 70.0f) {send_nec(NEC_B3);}
	else if (umd < 80.0f) {send_nec(NEC_B4);}
	else if (umd < 90.0f) {send_nec(NEC_BLUE);}
}

void executar_menu(int index) {
    float temp;
    float tempk;
    float umd;
    int em_modo_menu = 0;

    switch (index) {
        case 0:
            send_nec(NEC_ON);
            lcd_clear(&lcd1);
            lcd_puts(&lcd1, "Luz Acesa");
            while (1) {
                if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1) {
                    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1);
                    break;
                }
                HAL_Delay(100);
            }
            em_modo_menu = 1;
            break;
        case 1:
			send_nec(NEC_OFF);
			lcd_clear(&lcd1);
			lcd_puts(&lcd1, "Luz Apagada");
			while (1) {
				if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1) {
					while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1);
					break;
				}
			HAL_Delay(100);
			}
			em_modo_menu = 1;
			break;
        case 2:
            while (1) {
                if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1) {
                    while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1);
                    break;
                }
                temp = TempC();
                LCDTempC(temp);
                ledsTempC(temp);
                RGBC(temp);
                protecao_temperatura(temp);
                HAL_Delay(300);
            }
            em_modo_menu = 1;
            break;
        case 3:
            while (1) {
				if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1) {
					while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1);
					break;
				}
				tempk = TempK();
                LCDTempK(tempk);
                ledsTempK(tempk);
                RGBK(tempk);
                protecao_kelvin(tempk);
				HAL_Delay(300);
			}
			em_modo_menu = 1;
			break;
        case 4:
            while (1) {
				if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1) {
					while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 1);
					break;
				}
				umd = Umd1();
                LCDUmd(umd);
                ledsUmd(umd);
                RGBU(umd);
                protecao_umda(umd);
                protecao_umdb(umd);
				HAL_Delay(300);
			}
			em_modo_menu = 1;
			break;
        case 5:
                    lcd_clear(&lcd1);
                    lcd_backlight_off(&lcd1);
                    lcd_puts(&lcd1, "Backlight OFF");
                    em_modo_menu = 1;
                    break;
        case 6:
					lcd_clear(&lcd1);
					lcd_backlight_on(&lcd1);
					lcd_puts(&lcd1, "Backlight ON");
					em_modo_menu = 1;
					break;
    }

    if (em_modo_menu) atualizar_menu();
}
/*------------------------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_1 && GPIO_PIN_1 == 1) {
        executar_menu(menu_index);
    }
}

/* Main ------------------------------------------------------------------*/
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    DWT_Delay_Init();

    lcd1.hi2c = &hi2c1;
    lcd1.address = (0x27 << 1);
    lcd_init(&lcd1);
    lcd_clear(&lcd1);

    joystick_init(&hjoy, &hadc1, GPIOB, GPIO_PIN_2);
    atualizar_menu();

    while (1) {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == 0) {
            joystick_read(&hjoy);
            JoystickDirection dir = joystick_get_direction(&hjoy);
            if (dir == JOY_DOWN) {
                menu_index = (menu_index + 1) % 7;
                atualizar_menu();
                HAL_Delay(200);
            } else if (dir == JOY_UP) {
                menu_index = (menu_index + 6) % 7;
                atualizar_menu();
                HAL_Delay(200);
            }
        		 else if (dir == JOY_LEFT) {
                        menu_index = (menu_index + 1) % 7;
                        atualizar_menu();
                        HAL_Delay(200);
                    }
					 else if (dir == JOY_RIGHT) {
                        menu_index = (menu_index + 6) % 7;
                        atualizar_menu();
									HAL_Delay(200);
								}
        }

        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == 1) {
            executar_menu(menu_index);
        }
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 2526;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1263;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA9 PA10 PA11
                           PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
