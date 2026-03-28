/*******************************************************************************
 * @file    st7735_pin.h
 * @brief   ST7735 GPIO control interface
 *
 * @details
 * This module provides an abstraction layer for controlling the ST7735
 * display pins (CS, DC, RESET).
 *
 * It allows the driver to remain platform-independent by delegating
 * GPIO operations to user-provided implementations.
 ******************************************************************************/

#ifndef ST7735_PIN_H
#define ST7735_PIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Forward Declaration
 *******************************************************************************/
typedef struct st7735_pin st7735_pin_t;

/*******************************************************************************
 * Pin Operations
 *******************************************************************************/

/**
 * @brief GPIO control operations for ST7735
 *
 * These functions must be implemented per platform (STM32, ESP32, etc.)
 */
typedef struct
{
    int (*cs_pin_low)(st7735_pin_t *pin);
    int (*cs_pin_high)(st7735_pin_t *pin);

    int (*dc_pin_low)(st7735_pin_t *pin);
    int (*dc_pin_high)(st7735_pin_t *pin);

    int (*reset_pin_low)(st7735_pin_t *pin);
    int (*reset_pin_high)(st7735_pin_t *pin);

} st7735_pin_ops_t;

/*******************************************************************************
 * Device Structure
 *******************************************************************************/

/**
 * @brief ST7735 pin control handle
 */
struct st7735_pin
{
    const st7735_pin_ops_t *ops; /**< GPIO operations */
};

/*******************************************************************************
 * API
 *******************************************************************************/

/**
 * @brief Get default pin operations
 *
 * @return Pointer to operations table
 */
const st7735_pin_ops_t *get_st7735_pin_ops(void);

/**
 * @brief Initialize pin interface
 *
 * @param st7735_pin Pointer to pin handle
 * @param ops        Platform-specific operations
 *
 * @return 0 on success
 */
int st7735_pin_init(st7735_pin_t *st7735_pin,
                    const st7735_pin_ops_t *ops);

#ifdef __cplusplus
}
#endif

#endif /* ST7735_PIN_H */

/****************************** End of file ***********************************/
