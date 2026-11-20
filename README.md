# SSD1315

面向 128×64 单色 OLED 的可移植 C99 驱动。核心仅依赖标准 C 头文件，不依赖 MCU SDK、HAL、RTOS 或字库。使用 1024 字节静态显存，按脏页刷新，无动态内存。

[![Seeed Studio Grove 0.96 英寸 SSD1315 OLED 显示模块](https://raw.githubusercontent.com/SeeedDocument/Grove-OLED-Display-0.96-SSD1315-/master/img/Grove-OLED-Displey-0.96-SSD1315-wiki.jpg)](https://wiki.seeedstudio.com/Grove-OLED-Display-0.96-SSD1315/)

## CMake support

### For subdirectory

在应用工程根目录执行：

```sh
git clone https://github.com/leejkee/ssd1315.git third_party/ssd1315
```

CMake接入示例：（在应用的 `CMakeLists.txt` 中添加）

```cmake
add_subdirectory(third_party/ssd1315)
target_link_libraries(my_app PRIVATE ssd1315::ssd1315)
```

### Use FetchContent

在应用的 `CMakeLists.txt` 中添加以下代码，CMake 会在配置阶段获取指定版本：

```cmake
include(FetchContent)
FetchContent_Declare(
    ssd1315
    GIT_REPOSITORY https://github.com/leejkee/ssd1315.git
    GIT_TAG a4fd30557675b69f0f31fa7e472825a0685a7cdf
)
FetchContent_MakeAvailable(ssd1315)
target_link_libraries(my_app PRIVATE ssd1315::ssd1315)
```

## 实现平台接口

用户负责初始化 I²C、GPIO、供电以及模块所需的复位和上电等待，然后提供同步发送函数：

```c
#include "bsp_ssd1315.h"

/* 用户实现：成功返回 0，失败返回非 0。 */
extern int board_i2c_write(void *bus, uint8_t address_7bit,
                           const uint8_t *bytes, size_t length);

int oled_start(void *bus)
{
    BSP_SSD1315_Port port = {board_i2c_write, bus, 0x3C};
    if (BSP_SSD1315_Init(&port) != BSP_SSD1315_OK)
        return BSP_SSD1315_ERROR;
    BSP_SSD1315_DrawLine(0, 0, 127, 63);
    return BSP_SSD1315_Refresh();
}
```

发送回调约定：

- 地址是 **7 位** `0x3C` 或 `0x3D`。底层 API 如果要求左移地址，由用户适配。
- 缓冲区已包含控制字节：命令 `0x00`，数据 `0x40`。整段数据必须在一次 I²C 事务中发送，最大 129 字节。
- 返回前必须完成发送，不能保存缓冲区指针后异步使用。超时及总线恢复由用户实现。
- 配置结构被复制，`context` 指向的对象需保持有效。驱动为单屏实例，不可重入；并发访问须由用户串行化。

初始化及通信接口返回 `BSP_SSD1315_OK` / `BSP_SSD1315_ERROR`。初始化失败后应重新调用 `Init`；刷新失败会保留未完成页的脏标记，可再次 `Refresh`。

## 绘图与字库

点、线、圆、矩形清除和位图接口只修改显存；绘制后调用 `Refresh`。`Clear` 立即清屏，`ClearBuffer` 只清显存。屏幕外像素会被裁剪。

`DrawBitmap(x, y, width, height, bitmap)` 支持任意 y 坐标。数据按页存储：`bitmap[(row / 8) * width + col]` 的第 `row % 8` 位对应像素，1 为亮，0 为灭（覆盖模式）。调用方提供至少 `width * ((height + 7) / 8)` 字节。中文、图标或其他字体只需查到对应点阵后调用该接口。

`examples/ascii/` 保留一套原项目的 ASCII 字库和使用示例，不参与核心构建。需要文字时编译该目录的两个 C 文件，调用 `SSD1315_ExampleShowString` 后刷新，或将端口配置传给 `SSD1315_Example` 显示完整演示。示例使用固定 8×16 字符，不自动换行。

## 构建与测试

```sh
cmake -S . -B build -DSSD1315_BUILD_EXAMPLE=ON -DSSD1315_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

交叉编译时通常关闭测试，使用自己的 CMake toolchain。主机测试通过模拟发送回调检查事务、刷新重试和绘图边界；实屏时序、供电及显示方向需要在目标板验证。

## 按需生成字库（推荐结合LLM使用）

在仓库根目录运行以下命令。ASCII 脚本仅需 Python，其他脚本需先安装 Pillow（`python -m pip install Pillow`）。可用 `--font` 为这些脚本指定包含目标字形的字体文件。

```sh
# ASCII 数字 0–9；省略范围时生成全部可打印 ASCII
python tools/FontTool/generate_ascii_font.py --first 0x30 --last 0x39 --out-dir generated

# 中文界面：把需要的汉字追加到内置字表
python tools/FontTool/generate_cn_font.py --no-scan --chars "温度" --out-dir generated

# 从文件名收集字符：将 ./media 换成自己的目录
python tools/FontTool/scan_dir_font.py ./media --out-dir generated

# 单码点图标：▶（U+25B6）和 ⏸（U+23F8）
python tools/FontTool/generate_emoji_font.py --codepoints U+25B6 U+23F8 --out-dir generated
```

**按 Unicode 码点选字**：图标脚本的 `--codepoints` 接受 `U+25B6`、`0x25B6` 或 `25B6`，多个码点用空格分隔。中文脚本的 `--chars` 接收实际汉字。若只有“温”和“度”的码点，可在 PowerShell 中先转成文字：

```powershell
$chars = python -c "print(chr(0x6E29) + chr(0x5EA6))"
python tools/FontTool/generate_cn_font.py --no-scan --chars "$chars" --out-dir generated
```

将生成的 `generated/src/` 中所需的 C 文件加入工程，并添加头文件路径 `generated/include/`。各脚本的完整参数见 `--help`，屏幕控制命令见 [SSD1315 命令表](docs/SSD1315_Command_Table.md)。

## 许可证

项目代码采用 [MIT](LICENSE)。字库示例、内嵌点阵及芯片资料的来源说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
