#include "bsp_ssd1315.h"
#include <stdlib.h>
#include <string.h>

static BSP_SSD1315_Port port;

static int BSP_SSD1315_WriteCmd(uint8_t command)
{
    const uint8_t data[2] = {0x00, command};
    if (port.write == NULL) return BSP_SSD1315_ERROR;
    return port.write(port.context, port.address, data, sizeof(data)) == 0
        ? BSP_SSD1315_OK : BSP_SSD1315_ERROR;
}

static int BSP_SSD1315_WriteData(const uint8_t *data, uint16_t len)
{
    uint8_t buf[129];
    if (port.write == NULL || len > 128) return BSP_SSD1315_ERROR;
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    return port.write(port.context, port.address, buf, (size_t)len + 1) == 0
        ? BSP_SSD1315_OK : BSP_SSD1315_ERROR;
}

/*
 * ============================================================================
 * GDDRAM 显存模型 (SSD1315, 128x64 单色 OLED)
 * ============================================================================
 *
 *  显存数组: uint8_t BSP_SSD1315_GRAM[8][128];  共 1024 字节
 *            [页 0~7][列 0~127]
 *
 *  物理映射 (默认: A0h + C0h, 未做重映射):
 *
 *    列方向: SEG0 (左) ────────────── SEG127 (右)
 *    行方向: COM0  (上) ────────────── COM63  (下)
 *
 *  内存布局 — 一维视角 (1024 字节连续存放):
 *
 *    byte   0 ── byte 127  │ byte 128 ── byte 255  │ ... │ byte 896 ── byte
 * 1023 ├─ PAGE0 × 128 列 ────┼─ PAGE1 × 128 列 ────┼─ ... ─┼─ PAGE7 × 128 列
 * ──┤ COM0~COM7             COM8~COM15                   COM56~COM63
 *
 *  每个字节 = 一列中同一页的 8 个像素:
 *
 *    BSP_SSD1315_GRAM[page][col]:
 *
 *      屏幕坐标 (col, page*8 + bit)
 *      ┌───┐
 *      │ █ │ ← bit 0 = COM(page*8+0) = 该页最上一行
 *      │ █ │ ← bit 1 = COM(page*8+1)
 *      │ █ │ ← bit 2 = COM(page*8+2)
 *      │ █ │ ← bit 3 = COM(page*8+3)
 *      │ █ │ ← bit 4 = COM(page*8+4)
 *      │ █ │ ← bit 5 = COM(page*8+5)
 *      │ █ │ ← bit 6 = COM(page*8+6)
 *      │ █ │ ← bit 7 = COM(page*8+7) = 该页最下一行
 *      └───┘
 *      SEG(col)
 *
 *  各页覆盖的 COM 范围:
 *
 *    PAGE0: COM0  ~ COM7   (屏幕第  1 ~  8 行)  — 左上角
 *    PAGE1: COM8  ~ COM15  (屏幕第  9 ~ 16 行)
 *    PAGE2: COM16 ~ COM23  (屏幕第 17 ~ 24 行)
 *    PAGE3: COM24 ~ COM31  (屏幕第 25 ~ 32 行)
 *    PAGE4: COM32 ~ COM39  (屏幕第 33 ~ 40 行)
 *    PAGE5: COM40 ~ COM47  (屏幕第 41 ~ 48 行)
 *    PAGE6: COM48 ~ COM55  (屏幕第 49 ~ 56 行)
 *    PAGE7: COM56 ~ COM63  (屏幕第 57 ~ 64 行)  — 右下角
 *
 *  重映射命令的影响:
 *
 *    命令      默认值    翻转后     效果
 *    ─────────────────────────────────────────
 *    A0h/A1h   SEG0 左   SEG127 左  左右镜像
 *    C0h/C8h   COM0 上   COM0 下    上下翻转
 *
 *  本工程 Init 中使用 A1h + C8h, 即 COM0 在屏幕下方, SEG127 在左。
 *  显存数组的读写方式不受重映射影响, 只改变物理像素和 GDDRAM 地址的对应关系。
 *
 *  像素坐标换算 (x = 列, y = 行):
 *
 *    page = y / 8;
 *    bit  = y % 8;
 *
 *    写像素: BSP_SSD1315_GRAM[page][x] |=  (1 << bit);  // 点亮
 *    擦像素: BSP_SSD1315_GRAM[page][x] &= ~(1 << bit);  // 熄灭
 *
 * ============================================================================
 */

static uint8_t BSP_SSD1315_GRAM[8][128];

static uint8_t dirty_pages;  // 1 字节，每个 bit 标记一页是否脏

int BSP_SSD1315_Init(const BSP_SSD1315_Port *config)
{
    static const uint8_t commands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6
    };
    if (config == NULL || config->write == NULL ||
        (config->address != 0x3C && config->address != 0x3D))
        return BSP_SSD1315_ERROR;
    port = *config;
    for (size_t i = 0; i < sizeof(commands); ++i)
        if (BSP_SSD1315_WriteCmd(commands[i]) != BSP_SSD1315_OK)
            return BSP_SSD1315_ERROR;
    if (BSP_SSD1315_Clear() != BSP_SSD1315_OK) return BSP_SSD1315_ERROR;
    return BSP_SSD1315_DisplayOn();
}

int BSP_SSD1315_ColorTurn(uint8_t i)
{
    return BSP_SSD1315_WriteCmd(0xA6 | (i & 1));
}

int BSP_SSD1315_DisplayTurn(uint8_t orientation)
{
    if (BSP_SSD1315_WriteCmd(0xC0 | ((orientation & 1) << 3)) != 0)
        return BSP_SSD1315_ERROR;
    return BSP_SSD1315_WriteCmd(0xA0 | ((orientation >> 1) & 1));
}

void BSP_SSD1315_ClearPoint(uint8_t x, uint8_t y)
{
    BSP_SSD1315_SetPixel(x, y, 0);  // 擦除像素点
}

int BSP_SSD1315_Clear(void)
{
    BSP_SSD1315_ClearBuffer();
    return BSP_SSD1315_Refresh();
}

void BSP_SSD1315_ClearBuffer(void)
{
    // 只清空显存数组，由调用方绘制完整帧后再刷新
    for (uint8_t page = 0; page < 8; page++)
    {
        for (uint8_t col = 0; col < 128; col++)
        {
            BSP_SSD1315_GRAM[page][col] = 0x00;
        }
    }
    dirty_pages = 0xFF;
}

void BSP_SSD1315_ClearArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint16_t x_end = (uint16_t)x + width;
    uint16_t y_end = (uint16_t)y + height;

    if ((width == 0U) || (height == 0U) || (x >= 128U) || (y >= 64U))
    {
        return;
    }
    if (x_end > 128U)
    {
        x_end = 128U;
    }
    if (y_end > 64U)
    {
        y_end = 64U;
    }

    for (uint16_t row = y; row < y_end; row++)
    {
        uint8_t page = (uint8_t)(row / 8U);
        uint8_t mask = (uint8_t)(1U << (row % 8U));

        for (uint16_t col = x; col < x_end; col++)
        {
            BSP_SSD1315_GRAM[page][col] &= (uint8_t)~mask;
        }
        dirty_pages |= (uint8_t)(1U << page);
    }
}

int BSP_SSD1315_DisplayOn(void)
{
    return BSP_SSD1315_WriteCmd(0xAF);  // 开显示
}

int BSP_SSD1315_DisplayOff(void)
{
    return BSP_SSD1315_WriteCmd(0xAE);  // 关显示
}

void BSP_SSD1315_DrawPoint(uint8_t x, uint8_t y)
{
    BSP_SSD1315_SetPixel(x, y, 1);  // 点亮像素点
}

void BSP_SSD1315_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    // 使用 Bresenham 算法绘制直线
    int dx  = abs(x2 - x1);
    int dy  = -abs(y2 - y1);
    int sx  = (x1 < x2) ? 1 : -1;
    int sy  = (y1 < y2) ? 1 : -1;
    int err = dx + dy;
    while (1)
    {
        BSP_SSD1315_SetPixel(x1, y1, 1);
        if (x1 == x2 && y1 == y2)
        {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

static void set_clipped(int x, int y)
{
    if (x >= 0 && x < 128 && y >= 0 && y < 64)
        BSP_SSD1315_SetPixel((uint8_t)x, (uint8_t)y, 1);
}

void BSP_SSD1315_DrawCircle(uint8_t x, uint8_t y, uint8_t r)
{
    int a = r, b = 0, error = 1 - (int)r;
    while (a >= b)
    {
        set_clipped(x+a, y+b); set_clipped(x+b, y+a);
        set_clipped(x-b, y+a); set_clipped(x-a, y+b);
        set_clipped(x-a, y-b); set_clipped(x-b, y-a);
        set_clipped(x+b, y-a); set_clipped(x+a, y-b);
        ++b;
        if (error < 0) error += 2*b + 1;
        else { --a; error += 2*(b-a) + 1; }
    }
}

void BSP_SSD1315_SetPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= 128 || y >= 64) return;
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    if (color)
    {
        BSP_SSD1315_GRAM[page][x] |= (1 << bit);
    }
    else
    {
        BSP_SSD1315_GRAM[page][x] &= ~(1 << bit);
    }
    dirty_pages |= (1 << page);
}

int BSP_SSD1315_Refresh(void)
{
    if (port.write == NULL) return BSP_SSD1315_ERROR;
    for (uint8_t p = 0; p < 8; ++p)
    {
        if (dirty_pages & (1U << p))
        {
            const uint8_t commands[] = {0x21, 0x00, 0x7F, 0x22, p, p};
            for (size_t i = 0; i < sizeof(commands); ++i)
                if (BSP_SSD1315_WriteCmd(commands[i]) != 0)
                    return BSP_SSD1315_ERROR;
            if (BSP_SSD1315_WriteData(BSP_SSD1315_GRAM[p], 128) != 0)
                return BSP_SSD1315_ERROR;
            dirty_pages &= (uint8_t)~(1U << p);
        }
    }
    return BSP_SSD1315_OK;
}

void BSP_SSD1315_DrawBitmap(uint8_t x, uint8_t y, uint8_t width,
                            uint8_t height, const uint8_t *bitmap)
{
    if (bitmap == NULL || x >= 128 || y >= 64) return;
    for (uint16_t row = 0; row < height && y + row < 64; ++row)
        for (uint16_t col = 0; col < width && x + col < 128; ++col)
            BSP_SSD1315_SetPixel((uint8_t)(x + col), (uint8_t)(y + row),
                (bitmap[(row / 8) * width + col] >> (row % 8)) & 1U);
}
