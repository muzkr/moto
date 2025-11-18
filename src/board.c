
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
// #define FLASHLIGHT_PIN MK_PIN(GPIOC, LL_GPIO_PIN_13)
// PF8: Backlight
#define BACKLIGHT_PIN MK_PIN(GPIOF, LL_GPIO_PIN_8)

// PA3: SPI flash CS
#define SPI_FLASH_CS_PIN MK_PIN(GPIOA, LL_GPIO_PIN_3)

// PB2: LCD CS
#define LCD_CS_PIN MK_PIN(GPIOB, LL_GPIO_PIN_2)
// PA6: LCD A0
#define LCD_A0_PIN MK_PIN(GPIOA, LL_GPIO_PIN_6)

void board_init()
{
    // LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB | LL_IOP_GRP1_PERIPH_GPIOC);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB | LL_IOP_GRP1_PERIPH_GPIOF);

    RESET_OUTPUT_PIN(BACKLIGHT_PIN);
    SET_OUTPUT_PIN(KEYPAD_COL1_PIN);
    SET_OUTPUT_PIN(SPI_FLASH_CS_PIN);
    SET_OUTPUT_PIN(LCD_CS_PIN);

    LL_GPIO_InitTypeDef InitStruct = {0};
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    InitStruct.Pull = LL_GPIO_PULL_UP;

    // Input ----------

    InitStruct.Mode = LL_GPIO_MODE_INPUT;

    // PTT & keypad rows
    InitStruct.Pin = GET_PIN_MASK(PTT_PIN) | GET_PIN_MASK(KEYPAD_ROW1_PIN) | GET_PIN_MASK(KEYPAD_ROW2_PIN);
    LL_GPIO_Init(GPIOB, &InitStruct);
    // LL_GPIO_SetPinPull(GET_PORT(KEYPAD_ROW1_PIN), GET_PIN_MASK(KEYPAD_ROW1_PIN), LL_GPIO_PULL_NO);

    // Output -----

    InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;

    // Keypad col 1
    // LCD CS
    InitStruct.Pin = GET_PIN_MASK(KEYPAD_COL1_PIN) | GET_PIN_MASK(LCD_CS_PIN);
    LL_GPIO_Init(GPIOB, &InitStruct);

    // Backlight
    InitStruct.Pin = GET_PIN_MASK(BACKLIGHT_PIN);
    LL_GPIO_Init(GET_PORT(BACKLIGHT_PIN), &InitStruct);

    // SPI flash CS
    // LCD A0
    InitStruct.Pin = GET_PIN_MASK(SPI_FLASH_CS_PIN) | GET_PIN_MASK(LCD_A0_PIN);
    LL_GPIO_Init(GPIOA, &InitStruct);
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

// Backlight ----------

enum
{
    BL_STATIC,
    BL_DELAY,
    BL_FLASH,
};

static struct
{
    uint8_t mode;
    uint32_t delay;
    uint32_t time;
} backlight_state = {0};

void board_backlight_on(uint32_t delay)
{
    SET_OUTPUT_PIN(BACKLIGHT_PIN);
    if (0 == delay)
    {
        backlight_state.mode = BL_STATIC;
    }
    else
    {
        backlight_state.mode = BL_DELAY;
        backlight_state.delay = delay;
        backlight_state.time = main_timestamp();
    }
}

void board_backlight_off()
{
    RESET_OUTPUT_PIN(BACKLIGHT_PIN);
    backlight_state.mode = BL_STATIC;
}

void board_backlight_flash(uint32_t delay)
{
    backlight_state.mode = BL_FLASH;
    backlight_state.time = main_timestamp();
    backlight_state.delay = delay / 2;
}

void board_backlight_update()
{
    if (BL_STATIC == backlight_state.mode)
    {
        return;
    }

    uint32_t t = main_timestamp();
    uint32_t dt = t - backlight_state.time;
    if (dt >= backlight_state.delay)
    {
        if (BL_DELAY == backlight_state.mode)
        {
            RESET_OUTPUT_PIN(BACKLIGHT_PIN);
            backlight_state.mode = BL_STATIC;
        }
        else // Flash
        {
            LL_GPIO_TogglePin(GET_PORT(BACKLIGHT_PIN), GET_PIN_MASK(BACKLIGHT_PIN));
            backlight_state.time = t;
        }
    }
}
