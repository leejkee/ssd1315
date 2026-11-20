#ifndef SSD1315_ASCII_EXAMPLE_H
#define SSD1315_ASCII_EXAMPLE_H
#include "bsp_ssd1315.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fixed 8x16 ASCII, no wrapping. Call Refresh after drawing. */
void SSD1315_ExampleShowString(uint8_t x, uint8_t y, const char *text);
/* Bus, reset and power must already be initialized by the caller. */
int SSD1315_Example(const BSP_SSD1315_Port *port);
#ifdef __cplusplus
}
#endif
#endif
