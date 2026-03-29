/*******************************************************************************
 * @file        st7735.c
 * @author      Sarath S
 * @date        2026-01-02
 * @version     1.0
 * @brief       ST7735 TFT display driver implementation
 *
 * @details
 * This file implements the ST7735 display driver. It handles:
 *  - Initialization sequence
 *  - SPI communication (command/data)
 *  - Drawing operations (pixel, rectangle, screen fill)
 *
 * The driver is platform-independent and relies on:
 *  - OSAL (for delays)
 *  - SPI abstraction layer
 *  - GPIO pin interface (CS, DC, RESET)
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include "st7735.h"

#ifdef OSAL_H
#include "osal.h"
#else
#error "osal.h not found!"
#endif

#ifdef BUS_SPI_H
#include "bus_spi.h"
#else
#error "bus_spi.h not found!"
#endif

/*******************************************************************************
 * Macros
 *******************************************************************************/

/**
 * @brief Validate device pointer and dependencies
 */
#define ST7735_CHECK_DEV(dev)                         \
    do {                                             \
        if ((dev == NULL) ||                         \
            (dev->opts == NULL) ||                   \
            (dev->osal == NULL) ||                   \
            (dev->bus_spi == NULL) ||                \
            (dev->pin == NULL))                      \
        {                                            \
            return ST7735_ERR_NULL_PTR;              \
        }                                            \
    } while (0)

/**
 * @brief Check return code and propagate error
 */
#define ST7735_CHECK_RC(x)                           \
    do {                                             \
        st7735_err_t rc = (x);                       \
        if (rc != ST7735_OK) return rc;              \
    } while (0)

/*******************************************************************************
 * Static Function Prototypes
 *******************************************************************************/
static st7735_err_t st7735_hw_init(st7735_t *dev);
static st7735_err_t st7735_set_window(st7735_t *dev,
                                      uint16_t xs, uint16_t ys,
                                      uint16_t xe, uint16_t ye);

static st7735_err_t st7735_write_cmd(st7735_t *dev, uint8_t cmd);
static st7735_err_t st7735_write_data(st7735_t *dev, uint8_t data);
static st7735_err_t st7735_write_bytes(st7735_t *dev,
                                       const uint8_t *data,
                                       uint16_t len);

/* Ops functions */
static st7735_err_t fn_init(st7735_t *dev);
static st7735_err_t fn_deinit(st7735_t *dev);
static st7735_err_t fn_set_orientation(st7735_t *dev, st7735_rotation_t orient);
static st7735_err_t fn_draw_pixel(st7735_t *dev,
                                 uint16_t x,
                                 uint16_t y,
                                 st7735_color_t color);
static st7735_err_t fn_fill_rect(st7735_t *dev,
                                uint16_t x,
                                uint16_t y,
                                uint16_t w,
                                uint16_t h,
                                st7735_color_t color);
static st7735_err_t fn_fill_screen(st7735_t *dev,
                                  st7735_color_t color);

/*******************************************************************************
 * Driver Ops Table
 *******************************************************************************/
static const st7735_opts_t st7735_opts =
{
    .init = fn_init,
    .de_init = fn_deinit,
    .set_orientation = fn_set_orientation,
	.set_window = st7735_set_window,
    .draw_pixel = fn_draw_pixel,
    .fill_rect = fn_fill_rect,
    .fill_color = fn_fill_screen
};

/*******************************************************************************
 * Public API
 *******************************************************************************/

/**
 * @brief Initialize driver context
 *
 * Links OSAL and SPI interfaces and prepares the device structure.
 * Hardware initialization is done separately via opts->init().
 */
st7735_err_t st7735_init(st7735_t *dev,
                         osal_t *osal,
                         bus_spi_t *spi, st7735_pin_t *pin)
{
    if ((dev == NULL) || (osal == NULL) || (spi == NULL))
        return ST7735_ERR_NULL_PTR;

    dev->osal = osal;
    dev->bus_spi = spi;
    dev->opts = &st7735_opts;
    dev->pin = pin;

    dev->width  = ST7735_PIXEL_WIDTH_HORIZONTAL;
    dev->height = ST7735_PIXEL_HEIGHT_VERTICAL;

    dev->screen_width  = dev->width;
    dev->screen_height = dev->height;

    dev->x_offset = 0;
    dev->y_offset = 0;

    //dev->orientation = ST7735_ROT_0;

    return ST7735_OK;
}

/**
 * @brief Get driver operations table
 */
const st7735_opts_t *get_st7735_opts(void)
{
    return &st7735_opts;
}

/*******************************************************************************
 * Ops Implementation
 *******************************************************************************/

/**
 * @brief Perform hardware initialization sequence
 */
static st7735_err_t fn_init(st7735_t *dev)
{
    return st7735_hw_init(dev);
}

/**
 * @brief Deinitialize driver (placeholder)
 */
static st7735_err_t fn_deinit(st7735_t *dev)
{
    ST7735_CHECK_DEV(dev);
    return ST7735_OK;
}

/**
 * @brief Set display orientation
 */
static st7735_err_t fn_set_orientation(st7735_t *dev,
                                       st7735_rotation_t orient)
{
    ST7735_CHECK_DEV(dev);

    uint8_t madctl = 0;

    dev->pin->ops->cs_pin_low(dev->pin);
    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_MADCTL));

    switch (orient)
    {
        case ST7735_ROT_0:
            madctl = 0x00; // RGB
            dev->screen_width  = dev->width;
            dev->screen_height = dev->height;

            dev->x_offset = 0;
            dev->y_offset = 0;
            break;

        case ST7735_ROT_90:
            madctl = 0x60; // MV | MX

            dev->screen_width  = dev->height;
            dev->screen_height = dev->width;

            dev->x_offset = 0;
            dev->y_offset = 0;
            break;

        case ST7735_ROT_180:
            madctl = 0xC0; // MX | MY

            dev->screen_width  = dev->width;
            dev->screen_height = dev->height;

            dev->x_offset = 0;
            dev->y_offset = 0;
            break;

        case ST7735_ROT_270:
            madctl = 0xA0; // MV | MY

            dev->screen_width  = dev->height;
            dev->screen_height = dev->width;

            dev->x_offset = 0;
            dev->y_offset = 0;
            break;
    }

    ST7735_CHECK_RC(st7735_write_data(dev, madctl));

    dev->orientation = orient;

    dev->pin->ops->cs_pin_high(dev->pin);

    return ST7735_OK;
}

/**
 * @brief Draw a single pixel
 */
static st7735_err_t fn_draw_pixel(st7735_t *dev,
                                 uint16_t x,
                                 uint16_t y,
                                 st7735_color_t color)
{
    return fn_fill_rect(dev, x, y, 1, 1, color);
}

/**
 * @brief Fill rectangular area with a color
 */
static st7735_err_t fn_fill_rect(st7735_t *dev,
                                uint16_t x,
                                uint16_t y,
                                uint16_t w,
                                uint16_t h,
                                st7735_color_t color)
{
    ST7735_CHECK_DEV(dev);

    if ((x >= dev->screen_width) || (y >= dev->screen_height))
        return ST7735_OK;

    if ((x + w) > dev->screen_width)  w = dev->screen_width - x;
    if ((y + h) > dev->screen_height) h = dev->screen_height - y;

    if ((w == 0) || (h == 0))
        return ST7735_OK;

    uint16_t x1 = x + w - 1;
    uint16_t y1 = y + h - 1;

//    uint8_t hi = color >> 8;
//    uint8_t lo = color & 0xFF;

    uint16_t c = color;

    /* Swap RED and BLUE channels */
    uint16_t r = (c & 0xF800) >> 11;
    uint16_t g = (c & 0x07E0);
    uint16_t b = (c & 0x001F) << 11;

    uint16_t fixed = b | g | r;

    uint8_t hi = fixed >> 8;
    uint8_t lo = fixed & 0xFF;


    /* Prepare one line buffer */
    for (uint16_t i = 0; i < w; i++)
    {
        dev->line_buf[i * 2]     = hi;
        dev->line_buf[i * 2 + 1] = lo;
    }

    dev->pin->ops->cs_pin_low(dev->pin);

    ST7735_CHECK_RC(st7735_set_window(dev, x, y, x1, y1));

    dev->pin->ops->dc_pin_high(dev->pin);

    for (uint16_t row = 0; row < h; row++)
    {
        ST7735_CHECK_RC(st7735_write_bytes(dev,
                                           dev->line_buf,
                                           w * 2));
    }

    dev->pin->ops->cs_pin_high(dev->pin);
    return ST7735_OK;
}

/**
 * @brief Fill entire screen with a color
 */
static st7735_err_t fn_fill_screen(st7735_t *dev,
                                  st7735_color_t color)
{
    return fn_fill_rect(dev,
                        0,
                        0,
                        dev->screen_width,
                        dev->screen_height,
                        color);
}

/*******************************************************************************
 * Hardware Init
 *******************************************************************************/

/**
 * @brief Send initialization sequence to display
 */
static st7735_err_t st7735_hw_init(st7735_t *dev)
{
    ST7735_CHECK_DEV(dev);

    uint8_t buf[6];

    dev->pin->ops->cs_pin_high(dev->pin);
    dev->pin->ops->reset_pin_high(dev->pin);
    dev->osal->ops->delay_ms(5);

    dev->pin->ops->reset_pin_low(dev->pin);
    dev->osal->ops->delay_ms(5);
    dev->pin->ops->reset_pin_high(dev->pin);
    dev->osal->ops->delay_ms(5);

    dev->pin->ops->cs_pin_low(dev->pin);

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_SLPOUT));
    dev->osal->ops->delay_ms(20);

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_FRAME_RATE_CTRL1));
    buf[0] = 0x05; buf[1] = 0x3C; buf[2] = 0x3C;
    ST7735_CHECK_RC(st7735_write_bytes(dev, buf, 3));

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_FRAME_RATE_CTRL2));
    ST7735_CHECK_RC(st7735_write_bytes(dev, buf, 3));

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_FRAME_RATE_CTRL3));
    buf[3] = 0x05; buf[4] = 0x3C; buf[5] = 0x3C;
    ST7735_CHECK_RC(st7735_write_bytes(dev, buf, 6));

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_FRAME_INVERSION_CTRL));
    ST7735_CHECK_RC(st7735_write_data(dev, 0x03));

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_COLMOD));
    ST7735_CHECK_RC(st7735_write_data(dev, 0x05)); /* RGB565 */

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_DISPON));
    dev->osal->ops->delay_ms(10);

    dev->pin->ops->cs_pin_high(dev->pin);

    return ST7735_OK;
}

/*******************************************************************************
 * Low-Level Functions
 *******************************************************************************/

/**
 * @brief Set drawing window
 */
static st7735_err_t st7735_set_window(st7735_t *dev,
                                      uint16_t xs, uint16_t ys,
                                      uint16_t xe, uint16_t ye)
{
    uint8_t buf[4];

    xs += dev->x_offset;
    xe += dev->x_offset;
    ys += dev->y_offset;
    ye += dev->y_offset;

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_CASET));
    buf[0] = xs >> 8; buf[1] = xs;
    buf[2] = xe >> 8; buf[3] = xe;
    dev->pin->ops->dc_pin_high(dev->pin);
    ST7735_CHECK_RC(st7735_write_bytes(dev, buf, 4));

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_RASET));
    buf[0] = ys >> 8; buf[1] = ys;
    buf[2] = ye >> 8; buf[3] = ye;
    dev->pin->ops->dc_pin_high(dev->pin);
    ST7735_CHECK_RC(st7735_write_bytes(dev, buf, 4));

    ST7735_CHECK_RC(st7735_write_cmd(dev, ST7735_CMD_RAMWR));

    return ST7735_OK;
}

/**
 * @brief Send command byte
 */
static st7735_err_t st7735_write_cmd(st7735_t *dev, uint8_t cmd)
{
    dev->pin->ops->dc_pin_low(dev->pin);
    return dev->bus_spi->ops->write(dev->bus_spi, &cmd, 1);
}

/**
 * @brief Send single data byte
 */
static st7735_err_t st7735_write_data(st7735_t *dev, uint8_t data)
{
    dev->pin->ops->dc_pin_high(dev->pin);
    return dev->bus_spi->ops->write(dev->bus_spi, &data, 1);
}

/**
 * @brief Send multiple data bytes
 */
static st7735_err_t st7735_write_bytes(st7735_t *dev,
                                       const uint8_t *data,
                                       uint16_t len)
{
    return dev->bus_spi->ops->write(dev->bus_spi, data, len);
}


st7735_err_t st7735_start(st7735_t *dev)
{
    if ((dev == NULL) || (dev->opts == NULL) || (dev->opts->init == NULL))
        return ST7735_ERR_NULL_PTR;

    return dev->opts->init(dev);
}

st7735_err_t st7735_set_orientation(st7735_t *dev, st7735_rotation_t rot)
{
    if ((dev == NULL) || (dev->opts == NULL) || (dev->opts->set_orientation == NULL))
        return ST7735_ERR_NULL_PTR;

    return dev->opts->set_orientation(dev, rot);
}

st7735_err_t st7735_fill_color(st7735_t *dev, st7735_color_t color)
{
    if ((dev == NULL) || (dev->opts == NULL) || (dev->opts->fill_color == NULL))
        return ST7735_ERR_NULL_PTR;

    return dev->opts->fill_color(dev, color);
}



