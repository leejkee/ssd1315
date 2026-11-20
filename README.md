# SSD1315

面向 128×64 单色 OLED 的可移植 C99 驱动。核心仅依赖标准 C 头文件，不依赖 MCU SDK、HAL、RTOS 或字库。使用 1024 字节静态显存，按脏页刷新，无动态内存。

[![Seeed Studio Grove 0.96 英寸 SSD1315 OLED 显示模块](https://raw.githubusercontent.com/SeeedDocument/Grove-OLED-Display-0.96-SSD1315-/master/img/Grove-OLED-Displey-0.96-SSD1315-wiki.jpg)](https://wiki.seeedstudio.com/Grove-OLED-Display-0.96-SSD1315/)

## 接入

将 `src/bsp_ssd1315.c` 加入工程，并添加 `include/` 头文件路径；CMake 项目可以按下面两种方式引入，统一链接 `ssd1315::ssd1315`。

### CMake：本地 clone 后导入

将本项目 clone 到应用工程的 `third_party/ssd1315`，在应用的 `CMakeLists.txt` 中写入：

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES C)

add_subdirectory(third_party/ssd1315)
add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE ssd1315::ssd1315)
```

如果 clone 在工程目录之外，使用 `add_subdirectory(/path/to/ssd1315 ${CMAKE_CURRENT_BINARY_DIR}/ssd1315)` 指定独立构建目录。

### CMake：FetchContent 自动下载导入

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES C)
include(FetchContent)

# 替换为实际发布的仓库地址及固定的 tag 或 commit。
FetchContent_Declare(
    ssd1315
    GIT_REPOSITORY https://github.com/<owner>/ssd1315.git
    GIT_TAG <tag-or-commit>
)
FetchContent_MakeAvailable(ssd1315)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE ssd1315::ssd1315)
```

仓库尚未配置远程地址，上面的地址和版本是占位符。首次配置需要 Git 和仓库网络访问；离线使用已 clone 的目录时，可传入 `-DFETCHCONTENT_SOURCE_DIR_SSD1315=/path/to/ssd1315`。

两种方式都会自动传递头文件路径和 C99 编译要求，无需全局 `include_directories` 或手动添加驱动源码。库使用父工程的工具链，不指定 MCU、编译器或全局编译选项。

`SSD1315_BUILD_EXAMPLE` 和 `SSD1315_BUILD_TESTS` 默认均为 `OFF`。若需要 ASCII 示例，在导入前设置 `set(SSD1315_BUILD_EXAMPLE ON)`，再链接 `ssd1315::ascii`，它会自动链接核心驱动。`ssd1315` 和 `ssd1315_ascii` 原 target 名称仍然可用。

### 实现平台接口

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

## 从旧版本迁移

- `BSP_SSD1315_Init()` 改为 `BSP_SSD1315_Init(&port)`，移除 `main.h`、`i2c.h`、`hi2c1` 依赖。
- 通信函数由 `void` 改为 `int`，应检查结果。
- 核心移除 `ShowChar`、`ShowString`、`ShowNum`、`ShowChinese`、`ShowIcon` 和 `CnFontType`；文本排版、数字格式化和字库查找由应用层处理，使用通用 `DrawBitmap` 绘制。
- 移除原先仅声明、未实现的 `ScrollDisplay`。
- `DisplayTurn` 的 bit 0 控制 COM 反向，bit 1 控制 SEG 反向，初始化方向为 3。

## 构建与测试

```sh
cmake -S . -B build -DSSD1315_BUILD_EXAMPLE=ON -DSSD1315_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

交叉编译时通常关闭测试，使用自己的 CMake toolchain。主机测试通过模拟发送回调检查事务、刷新重试和绘图边界；实屏时序、供电及显示方向需要在目标板验证。

## 字库工具与资料

`tools/FontTool/` 保留原项目生成工具，非驱动运行依赖。使用时显式指定输出目录；ASCII 工具无第三方 Python 依赖，其他工具需要 Pillow。

```sh
python tools/FontTool/generate_ascii_font.py --out-dir generated
python tools/FontTool/generate_cn_font.py --no-scan --chars "播放暂停" --out-dir generated
python tools/FontTool/scan_dir_font.py --chars "歌曲名称" --output-basename font_file_cn --out-dir generated
python tools/FontTool/generate_emoji_font.py --small-size 12 --large-size 16 --out-dir generated
```

生成器在输出目录下创建 `include/` 和 `src/`。生成的新字库由应用自行接入。`docs/FontTool.md` 为原工具说明，其中原项目路径仅供背景参考；控制命令说明见 `docs/SSD1315_Command_Table.md`。

## 许可证

项目代码采用 [MIT](LICENSE)。字库示例、内嵌点阵及芯片资料的来源说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
