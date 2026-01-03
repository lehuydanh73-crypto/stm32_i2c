#include "main.h"
#include "stm32f1xx_it.h"

extern volatile bool read_flag;
extern volatile uint32_t isr_count;

void SysTick_Handler(void)
{
    HAL_IncTick();
    osSystickHandler();
}

/* EXTI from PCF8574 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        read_flag = true;
        isr_count++;
    }
}
