#ifndef BSP_SSD1315_H
#define BSP_SSD1315_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_SSD1315_WIDTH 128
#define BSP_SSD1315_HEIGHT 64
#define BSP_SSD1315_OK 0
#define BSP_SSD1315_ERROR (-1)

/* Blocking write: address is 7-bit; data includes the control byte.
 * Send all length bytes in ONE transaction. Return 0 on success, nonzero
 * on failure. Consume/copy data before returning; never retain its pointer.
 * The largest transaction is 129 bytes. Implement timeouts in the port. */
typedef int (*BSP_SSD1315_WriteFn)(void *context, uint8_t address,
                                   const uint8_t *data, size_t length);
typedef struct {
    BSP_SSD1315_WriteFn write;
    void *context;
    uint8_t address; /* 0x3C or 0x3D, NOT shifted */
} BSP_SSD1315_Port;

/* Single display instance, not reentrant. Initialize bus/power/reset first.
 * Config is copied; context must remain valid. Retry Init after failure. */
int BSP_SSD1315_Init(const BSP_SSD1315_Port *config);
int BSP_SSD1315_ColorTurn(uint8_t inverted);
/* bit 0: COM reverse; bit 1: SEG reverse. Init uses orientation 3. */
int BSP_SSD1315_DisplayTurn(uint8_t orientation);
int BSP_SSD1315_DisplayOn(void);
int BSP_SSD1315_DisplayOff(void);
int BSP_SSD1315_Clear(void);
/* Failed pages stay dirty and can be retried with Refresh. */
int BSP_SSD1315_Refresh(void);

/* Drawing only updates RAM. Call Refresh to submit the frame. */
void BSP_SSD1315_ClearBuffer(void);
void BSP_SSD1315_ClearArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void BSP_SSD1315_SetPixel(uint8_t x, uint8_t y, uint8_t color);
void BSP_SSD1315_DrawPoint(uint8_t x, uint8_t y);
void BSP_SSD1315_ClearPoint(uint8_t x, uint8_t y);
void BSP_SSD1315_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void BSP_SSD1315_DrawCircle(uint8_t x, uint8_t y, uint8_t r);
/* Page-major: bitmap[(row/8)*width+col], bit row%8; 1=on, 0=off.
 * Supply width * ceil(height/8) bytes. Arbitrary y supported; edges clipped. */
void BSP_SSD1315_DrawBitmap(uint8_t x, uint8_t y, uint8_t width,
                            uint8_t height, const uint8_t *bitmap);

#ifdef __cplusplus
}
#endif
#endif
