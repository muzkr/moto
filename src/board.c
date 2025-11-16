
#include "board.h"
#include "main.h"
#include "log.h"

#define MK_PIN(port, pin) ((uint32_t)((((uint32_t)(port)) << 16) | (0xffff & (pin))))
#define GET_PORT(pin) ((GPIO_TypeDef *)(IOPORT_BASE + ((pin) >> 16)))
#define GET_PIN_MASK(pin) (0xffff & (pin))

#define RESET_OUTPUT_PIN(pin) LL_GPIO_ResetOutputPin(GET_PORT(pin), GET_PIN_MASK(pin))
#define SET_OUTPUT_PIN(pin) LL_GPIO_SetOutputPin(GET_PORT(pin), GET_PIN_MASK(pin))
#define IS_INPUT_PIN_SET(pin) LL_GPIO_IsInputPinSet(GET_PORT(pin), GET_PIN_MASK(pin))

// PB10: PTT
#define PTT_PIN MK_PIN(GPIOB, LL_GPIO_PIN_10)
// PB15: Keypad row 1
#define KEYPAD_ROW1_PIN MK_PIN(GPIOB, LL_GPIO_PIN_15)
// PB14: Keypad row 2
#define KEYPAD_ROW2_PIN MK_PIN(GPIOB, LL_GPIO_PIN_14)
// PB6: Keypad col 1
#define KEYPAD_COL1_PIN MK_PIN(GPIOB, LL_GPIO_PIN_6)

// PC13: Flashlight
#define FLASHLIGHT_PIN MK_PIN(GPIOC, LL_GPIO_PIN_13)

void board_init()
{
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);

    RESET_OUTPUT_PIN(FLASHLIGHT_PIN);
    SET_OUTPUT_PIN(KEYPAD_COL1_PIN);

    LL_GPIO_InitTypeDef InitStruct = {0};
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    InitStruct.Pull = LL_GPIO_PULL_UP;

    // PTT & keypad rows
    InitStruct.Pin = GET_PIN_MASK(PTT_PIN) | GET_PIN_MASK(KEYPAD_ROW1_PIN) | GET_PIN_MASK(KEYPAD_ROW2_PIN);
    InitStruct.Mode = LL_GPIO_MODE_INPUT;
    LL_GPIO_Init(GPIOB, &InitStruct);
    // LL_GPIO_SetPinPull(GET_PORT(KEYPAD_ROW1_PIN), GET_PIN_MASK(KEYPAD_ROW1_PIN), LL_GPIO_PULL_NO);

    // Keypad col 1
    InitStruct.Pin = GET_PIN_MASK(KEYPAD_COL1_PIN);
    InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    LL_GPIO_Init(GPIOB, &InitStruct);

    // Flashlight
    InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    InitStruct.Pin = GET_PIN_MASK(FLASHLIGHT_PIN);
    LL_GPIO_Init(GPIOC, &InitStruct);
}

// Keypad ------

bool board_check_PTT()
{
    return !IS_INPUT_PIN_SET(PTT_PIN);
}

bool board_check_side_keys()
{
    SET_OUTPUT_PIN(KEYPAD_COL1_PIN);
    LL_mDelay(3);
    return !IS_INPUT_PIN_SET(KEYPAD_ROW1_PIN) || !IS_INPUT_PIN_SET(KEYPAD_ROW2_PIN);
}

bool board_check_M_key()
{
    RESET_OUTPUT_PIN(KEYPAD_COL1_PIN);
    LL_mDelay(3);
    bool b = !IS_INPUT_PIN_SET(KEYPAD_ROW1_PIN);
    SET_OUTPUT_PIN(KEYPAD_COL1_PIN);
    return b;
}

// Flashlight ----------

static struct
{
    uint32_t delay;
    uint32_t time;
} flashlight_state = {0};

void board_flashlight_on()
{
    SET_OUTPUT_PIN(FLASHLIGHT_PIN);
    flashlight_state.delay = 0;
}

void board_flashlight_off()
{
    RESET_OUTPUT_PIN(FLASHLIGHT_PIN);
    flashlight_state.delay = 0;
}

void board_flashlight_flash(uint32_t delay)
{
    flashlight_state.time = main_timestamp();
    flashlight_state.delay = delay / 2;
}

void board_flashlight_update()
{
    if (flashlight_state.delay > 0)
    {
        uint32_t t = main_timestamp();
        uint32_t dt = t - flashlight_state.time;
        if (dt >= flashlight_state.delay)
        {
            LL_GPIO_TogglePin(GET_PORT(FLASHLIGHT_PIN), GET_PIN_MASK(FLASHLIGHT_PIN));
            flashlight_state.time = t;
        }
    }
}
