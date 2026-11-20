#include "ascii_example.h"
#include "font_en.h"
void SSD1315_ExampleShowString(uint8_t x, uint8_t y, const char *text)
{
    uint16_t column = x;
    if (text == NULL) return;
    while (*text && column < BSP_SSD1315_WIDTH)
    {
        unsigned char ch = (unsigned char)*text++;
        if (ch >= FONT_EN_FIRST && ch <= FONT_EN_LAST)
            BSP_SSD1315_DrawBitmap((uint8_t)column, y, 8, 16,
                                   font_en_8x16[ch - FONT_EN_FIRST]);
        column += 8;
    }
}
int SSD1315_Example(const BSP_SSD1315_Port *port)
{
    if (BSP_SSD1315_Init(port) != 0) return BSP_SSD1315_ERROR;
    SSD1315_ExampleShowString(0, 0, "Hello SSD1315!");
    BSP_SSD1315_DrawLine(0, 18, 127, 18);
    BSP_SSD1315_DrawCircle(64, 42, 16);
    return BSP_SSD1315_Refresh();
}
