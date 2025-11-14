
#include "flashlight.h"
#include "py32f071_ll_bus.h"
#include "py32f071_ll_gpio.h"
#include "systick.h"
#include "log.h"

// PC13
#define GPIOx GPIOC
#define GPIO_PIN LL_GPIO_PIN_13

static struct
{
    uint32_t delay;
    uint32_t time;
} flash_state = {0};

void flashlight_init()
{
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);

    LL_GPIO_InitTypeDef InitStruct = {0};
    InitStruct.Pin = GPIO_PIN;
    InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &InitStruct);

    LL_GPIO_ResetOutputPin(GPIOx, GPIO_PIN);
}

void flashlight_on()
{
    LL_GPIO_SetOutputPin(GPIOx, GPIO_PIN);
    flash_state.delay = 0;
}

void flashlight_off()
{
    LL_GPIO_ResetOutputPin(GPIOx, GPIO_PIN);
    flash_state.delay = 0;
}

void flashlight_flash(uint32_t delay)
{
    flash_state.time = systick_timestamp();
    flash_state.delay = delay / 2;
}

void flashlight_update()
{
    if (flash_state.delay > 0)
    {
        uint32_t t = systick_timestamp();
        uint32_t dt = t - flash_state.time;
        if (dt >= flash_state.delay)
        {
            LL_GPIO_TogglePin(GPIOx, GPIO_PIN);
            flash_state.time = t;
        }
    }
}
