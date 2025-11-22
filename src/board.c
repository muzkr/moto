
#include "board.h"
#include "main.h"
#include "log.h"

// PB10: PTT
#define PTT_GPIOx GPIOB
#define PTT_PIN LL_GPIO_PIN_10
// PB15: Keypad row 1
#define KEYPAD_ROW1_GPIOx GPIOB
#define KEYPAD_ROW1_PIN LL_GPIO_PIN_15
// PB14: Keypad row 2
#define KEYPAD_ROW2_GPIOx GPIOB
#define KEYPAD_ROW2_PIN LL_GPIO_PIN_14

// PC13: Flashlight
#define FLASHLIGHT_GPIOx GPIOC
#define FLASHLIGHT_PIN LL_GPIO_PIN_13
// PF8: Backlight
#define BACKLIGHT_GPIOx GPIOF
#define BACKLIGHT_PIN LL_GPIO_PIN_8

// PA3: SPI flash CS
#define SPI_FLASH_CS_GPIOx GPIOA
#define SPI_FLASH_CS_PIN LL_GPIO_PIN_3
// PA6: LCD A0
#define LCD_A0_GPIOx GPIOA
#define LCD_A0_PIN LL_GPIO_PIN_6
// PB2: LCD CS
#define LCD_CS_GPIOx GPIOB
#define LCD_CS_PIN LL_GPIO_PIN_2

void board_init()
{
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB | LL_IOP_GRP1_PERIPH_GPIOC | LL_IOP_GRP1_PERIPH_GPIOF);

    LL_GPIO_ResetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
    LL_GPIO_ResetOutputPin(FLASHLIGHT_GPIOx, FLASHLIGHT_PIN);
    LL_GPIO_SetOutputPin(SPI_FLASH_CS_GPIOx, SPI_FLASH_CS_PIN);
    LL_GPIO_SetOutputPin(LCD_CS_GPIOx, LCD_CS_PIN);

    LL_GPIO_InitTypeDef InitStruct = {0};
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    InitStruct.Pull = LL_GPIO_PULL_UP;

    // Input ----------

    InitStruct.Mode = LL_GPIO_MODE_INPUT;

    // PTT & keypad rows
    InitStruct.Pin = PTT_PIN | KEYPAD_ROW1_PIN | KEYPAD_ROW2_PIN;
    LL_GPIO_Init(GPIOB, &InitStruct);

    // Output -----

    InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;

    // PA ------

    // SPI flash CS
    // LCD A0
    InitStruct.Pin = SPI_FLASH_CS_PIN | LCD_A0_PIN;
    LL_GPIO_Init(GPIOA, &InitStruct);

    // PB ----

    // LCD CS
    InitStruct.Pin = LCD_CS_PIN;
    LL_GPIO_Init(GPIOB, &InitStruct);

    // PC ----

    // Flashlight
    InitStruct.Pin = FLASHLIGHT_PIN;
    LL_GPIO_Init(GPIOC, &InitStruct);

    // PF ----

    // Backlight
    InitStruct.Pin = BACKLIGHT_PIN;
    LL_GPIO_Init(GPIOF, &InitStruct);
}

// Keypad ------

bool board_check_PTT()
{
    return !LL_GPIO_IsInputPinSet(PTT_GPIOx, PTT_PIN);
}

bool board_check_side_keys()
{
    LL_mDelay(3);
    return !LL_GPIO_IsInputPinSet(KEYPAD_ROW1_GPIOx, KEYPAD_ROW1_PIN) //
           || !LL_GPIO_IsInputPinSet(KEYPAD_ROW2_GPIOx, KEYPAD_ROW2_PIN);
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
    LL_GPIO_SetOutputPin(FLASHLIGHT_GPIOx, FLASHLIGHT_PIN);
    LL_GPIO_SetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
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
    LL_GPIO_ResetOutputPin(FLASHLIGHT_GPIOx, FLASHLIGHT_PIN);
    LL_GPIO_ResetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
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
            LL_GPIO_ResetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
            backlight_state.mode = BL_STATIC;
        }
        else // Flash
        {
            LL_GPIO_TogglePin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
            backlight_state.time = t;
        }
    }
}
