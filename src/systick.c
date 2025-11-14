
#include "time.h"
#include "py32f0xx.h"

static volatile uint32_t timestamp;

void systick_init()
{
    // NVIC_EnableIRQ(SysTick_IRQn);
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 0);
}

uint32_t systick_timestamp()
{
    return timestamp;
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
    timestamp++;
}
