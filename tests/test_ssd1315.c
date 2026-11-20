#include "bsp_ssd1315.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static unsigned calls, pages, fail_at;
static uint8_t last_data[128], last_command;
static int token;
static int write_mock(void *context, uint8_t address, const uint8_t *data, size_t length)
{
    CHECK(context == &token);
    CHECK(address == 0x3C || address == 0x3D);
    CHECK(length == 2 || length == 129);
    CHECK(data[0] == (length == 2 ? 0x00 : 0x40));
    ++calls;
    if (calls == fail_at) return -5;
    if (length == 129) { ++pages; memcpy(last_data, data + 1, 128); }
    else last_command = data[1];
    return 0;
}
int main(void)
{
    BSP_SSD1315_Port port = {write_mock, &token, 0x3C};
    CHECK(BSP_SSD1315_Refresh() == BSP_SSD1315_ERROR);
    CHECK(BSP_SSD1315_Init(NULL) == BSP_SSD1315_ERROR);
    port.address = 0x78;
    CHECK(BSP_SSD1315_Init(&port) == BSP_SSD1315_ERROR);
    port.address = 0x3C;
    CHECK(BSP_SSD1315_Init(&port) == 0);
    CHECK(pages == 8 && last_command == 0xAF);
    unsigned before = calls;
    CHECK(BSP_SSD1315_Refresh() == 0 && calls == before);
    BSP_SSD1315_SetPixel(127, 63, 1);
    CHECK(BSP_SSD1315_Refresh() == 0);
    CHECK(pages == 9 && last_data[127] == 0x80);
    BSP_SSD1315_SetPixel(128, 64, 1);
    before = calls;
    CHECK(BSP_SSD1315_Refresh() == 0 && calls == before);
    for (unsigned offset = 1; offset <= 7; ++offset)
    {
        BSP_SSD1315_SetPixel(0, 0, 1);
        fail_at = calls + offset;
        CHECK(BSP_SSD1315_Refresh() == BSP_SSD1315_ERROR);
        fail_at = 0;
        before = pages;
        CHECK(BSP_SSD1315_Refresh() == 0 && pages == before + 1);
        CHECK(last_data[0] == 1);
    }
    CHECK(BSP_SSD1315_Clear() == 0);
    const uint8_t bitmap[] = {0xFF, 0x01};
    BSP_SSD1315_DrawBitmap(127, 7, 1, 9, bitmap);
    before = pages;
    CHECK(BSP_SSD1315_Refresh() == 0 && pages == before + 2);
    CHECK(last_data[127] == 0xFF);
    BSP_SSD1315_ClearArea(127, 8, 255, 255);
    CHECK(BSP_SSD1315_Refresh() == 0 && last_data[127] == 0);
    CHECK(BSP_SSD1315_Clear() == 0);
    BSP_SSD1315_DrawCircle(0, 0, 0);
    CHECK(BSP_SSD1315_Refresh() == 0 && last_data[0] == 1);
    BSP_SSD1315_DrawCircle(255, 255, 255);
    CHECK(BSP_SSD1315_Refresh() == 0);
    for (uint8_t i = 0; i < 4; ++i)
    {
        CHECK(BSP_SSD1315_DisplayTurn(i) == 0);
        CHECK(last_command == (uint8_t)(0xA0 | (i >> 1)));
    }
    fail_at = calls + 1;
    CHECK(BSP_SSD1315_Init(&port) == BSP_SSD1315_ERROR);
    fail_at = 0;
    port.address = 0x3D;
    CHECK(BSP_SSD1315_Init(&port) == 0);
    puts("ssd1315 tests passed");
    return 0;
}
