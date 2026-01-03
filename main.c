#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

/* ================= PCF8574 ================= */
#define PCF8574_ADDR(x)   ((0x20 + (x)) << 1)

/* ================= HANDLES ================= */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* ================= GLOBAL ================= */
volatile bool read_flag = false;
volatile uint32_t isr_count = 0;

/* ================= PROTOTYPES ================= */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
void task1(void *argument);

/* ================= UART PRINTF ================= */
void uart_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}

/* ================= TASK1 ================= */
void task1(void *argument)
{
    uint8_t addr = PCF8574_ADDR(0);
    unsigned line = 0;
    uint16_t value = 0;
    uint8_t byte = 0xFF;

    for (;;)
    {
        uart_printf("\nI2C Demo Begins (STM32)\n\n");

        while (1)
        {
            read_flag = false;

            /* Upper 4 bits input, lower 4 bits output */
            value = (value & 0x0F) | 0xF0;

            uart_printf("Writing 0x%02X to I2C @ 0x%02X\n",
                        value, addr >> 1);

            /* WRITE */
            if (HAL_I2C_Master_Transmit(&hi2c1, addr,
                                        (uint8_t*)&value, 1,
                                        HAL_MAX_DELAY) != HAL_OK)
            {
                uart_printf("I2C WRITE ERROR\n");
                break;
            }

            /* REPEATED START + READ */
            if (HAL_I2C_Master_Receive(&hi2c1, addr,
                                       &byte, 1,
                                       HAL_MAX_DELAY) != HAL_OK)
            {
                uart_printf("I2C READ ERROR\n");
                break;
            }

            /* PRINT RESULT */
            if (read_flag)
            {
                if (byte & 0x80)
                    uart_printf("%04u: BUTTON RELEASED: 0x%02X wrote 0x%02X ISR %lu\n",
                                ++line, byte, value, isr_count);
                else
                    uart_printf("%04u: BUTTON PRESSED: 0x%02X wrote 0x%02X ISR %lu\n",
                                ++line, byte, value, isr_count);
            }
            else
            {
                uart_printf("%04u: Read 0x%02X wrote 0x%02X ISR %lu\n",
                            ++line, byte, value, isr_count);
            }

            value = (value + 1) & 0x0F;
            osDelay(300);
        }
    }
}

/* ================= MAIN ================= */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    osThreadDef(task1, task1, osPriorityNormal, 0, 512);
    osThreadCreate(osThread(task1), NULL);

    osKernelStart();

    while (1) {}
}
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PCF8574 INT -> PA0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
