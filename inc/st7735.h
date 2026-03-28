/*******************************************************************************
 * @file        st7735.h
 * @author      Sarath S
 * @date        2026-01-02
 * @version     1.0
 * @brief       ST7735 TFT display driver interface
 *
 * @details
 * This module provides a simple and portable driver interface for the ST7735
 * TFT display controller. It is designed to be platform-independent by using
 * abstraction layers for SPI communication and OS services (OSAL).
 *
 * The driver exposes a set of operations (via function pointers) that allow
 * drawing pixels, filling regions, and controlling display orientation.
 *
 * @note
 * Hardware-specific implementations (SPI, GPIO, delay) must be provided
 * through the OSAL, SPI bus, and pin interfaces.
 *
 * @contact     elektronikaembedded@gmail.com
 * @website     https://elektronikaembedded.wordpress.com
 ******************************************************************************/

#ifndef ST7735_H
#define ST7735_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "osal.h"
#include "bus_spi.h"
#include "st7735_pin.h"

/*******************************************************************************
 * Defines
 *******************************************************************************/

/** Default display resolution (common 1.8" ST7735) */
#define ST7735_PIXEL_WIDTH_HORIZONTAL   128
#define ST7735_PIXEL_HEIGHT_VERTICAL   160

/** Maximum supported width (used for internal buffer) */
#define ST7735_MAX_WIDTH               160

/*******************************************************************************
 * ST7735 Command Set (partial, commonly used)
 *******************************************************************************/
/* Basic commands */
#define ST7735_CMD_SWRESET     		0x01U
#define ST7735_CMD_SLPOUT      		0x11U
#define ST7735_CMD_DISPON      		0x29U

/* Addressing */
#define ST7735_CMD_CASET       		0x2AU
#define ST7735_CMD_RASET       		0x2BU
#define ST7735_CMD_RAMWR       		0x2CU

/* Control */
#define ST7735_CMD_MADCTL      		0x36U
#define ST7735_CMD_COLMOD      		0x3AU

/* Timing */
#define ST7735_FRAME_RATE_CTRL1   	0xB1U
#define ST7735_FRAME_RATE_CTRL2   	0xB2U
#define ST7735_FRAME_RATE_CTRL3   	0xB3U
#define ST7735_FRAME_INVERSION_CTRL 0xB4U
/*******************************************************************************
 * Types
 *******************************************************************************/

/**
 * @brief Driver return status
 */
typedef enum
{
    ST7735_OK = 0,
    ST7735_ERR_NULL_PTR = -1,
    ST7735_ERR_INVALID_ARGS = -2,
    ST7735_ERR_ZERO_LENGTH = -3,
    ST7735_ERR_BAD_LENGTH = -4
} st7735_err_t;

/**
 * @brief Display rotation
 */
typedef enum
{
    ST7735_ROT_0,     /**< Default orientation */
    ST7735_ROT_90,    /**< Rotate 90 degrees */
    ST7735_ROT_180,   /**< Rotate 180 degrees */
    ST7735_ROT_270    /**< Rotate 270 degrees */
} st7735_rotation_t;

/**
 * @brief RGB565 color format
 */
typedef uint16_t st7735_color_t;

/* Forward declaration */
typedef struct st7735_dev st7735_t;

/**
 * @brief Driver operations table
 *
 * Provides a flexible interface to interact with the display.
 * All drawing operations are exposed through this structure.
 */
typedef struct
{
    st7735_err_t (*init)(st7735_t *dev);
    st7735_err_t (*de_init)(st7735_t *dev);

    st7735_err_t (*set_orientation)(st7735_t *dev,
                                    st7735_rotation_t orient);

    st7735_err_t (*draw_pixel)(st7735_t *dev,
                               uint16_t xpos,
                               uint16_t ypos,
                               st7735_color_t color);

    st7735_err_t (*fill_rect)(st7735_t *dev,
                              uint16_t xpos,
                              uint16_t ypos,
                              uint16_t w,
                              uint16_t h,
                              st7735_color_t color);

    st7735_err_t (*fill_color)(st7735_t *dev,
                               st7735_color_t color);

} st7735_opts_t;

/*******************************************************************************
 * Device Structure
 *******************************************************************************/

/**
 * @brief ST7735 device instance
 *
 * Holds all runtime information required to control a display.
 * Multiple instances can be created for multiple displays.
 */
struct st7735_dev
{
    const st7735_opts_t *opts;   /**< Driver operations */

    st7735_pin_t *pin;           /**< GPIO control (CS, DC, RESET) */
    osal_t *osal;                /**< OS abstraction layer */
    bus_spi_t *bus_spi;          /**< SPI communication interface */

    uint16_t width;              /**< Physical width */
    uint16_t height;             /**< Physical height */

    uint16_t screen_width;       /**< Active width (after rotation) */
    uint16_t screen_height;      /**< Active height (after rotation) */

    uint16_t x_offset;           /**< X offset (panel dependent) */
    uint16_t y_offset;           /**< Y offset (panel dependent) */

    st7735_rotation_t orientation; /**< Current rotation */

    uint8_t line_buf[ST7735_MAX_WIDTH * 2]; /**< Internal buffer for fast fill */
};

/*******************************************************************************
 * API
 *******************************************************************************/

/**
 * @brief Initialize ST7735 driver instance
 *
 * This function sets up the driver context and links required interfaces.
 * Hardware initialization is performed via the driver ops.
 *
 * @param dev       Pointer to device instance
 * @param osal      OS abstraction layer
 * @param spi_bus   SPI bus interface
 * @param pin       pin/gpio interface
 *
 * @return ST7735_OK on success
 */
st7735_err_t st7735_init(st7735_t *dev,
                         osal_t *osal,
                         bus_spi_t *spi, st7735_pin_t *pin);

st7735_err_t st7735_start(st7735_t *dev);
st7735_err_t st7735_set_orientation(st7735_t *dev, st7735_rotation_t rot);
st7735_err_t st7735_fill_color(st7735_t *dev, st7735_color_t color);


/**
 * @brief Get driver operations table
 *
 * @return Pointer to constant ops structure
 */
const st7735_opts_t *get_st7735_opts(void);

#ifdef __cplusplus
}
#endif

#endif /* ST7735_H */

/****************************** End of file ***********************************/
