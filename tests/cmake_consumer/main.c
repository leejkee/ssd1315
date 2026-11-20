#include "bsp_ssd1315.h"
#ifdef TEST_ASCII
#include "ascii_example.h"
#endif

static int write_bus(void *context, uint8_t address,
                     const uint8_t *data, size_t length)
{
    (void)context;
    return address == 0x3C && data != NULL && length > 0 ? 0 : -1;
}

int main(void)
{
    BSP_SSD1315_Port port = {write_bus, NULL, 0x3C};
#ifdef TEST_ASCII
    return SSD1315_Example(&port) == BSP_SSD1315_OK ? 0 : 1;
#else
    return BSP_SSD1315_Init(&port) == BSP_SSD1315_OK ? 0 : 1;
#endif
}
