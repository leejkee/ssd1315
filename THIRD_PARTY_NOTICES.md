# 第三方资料与字库

驱动及工具从 MiniAudioPlayerST 提取。项目代码采用根目录 MIT 许可证。

`examples/ascii/font_en.c` 是原项目生成的 8×16 ASCII 字库示例，
`tools/FontTool/generate_ascii_font.py` 内嵌了同源点阵。
原脚本将其描述为标准 VGA/public-domain 字库，但未附可追溯的原始来源及授权文件；
此处保留来源说明，不将该描述视为已核实的授权结论。该字库不参与核心驱动的默认构建。
发布包含该示例或内嵌点阵的分发包前，应补齐其来源证明或替换为授权明确的数据。

字库工具生成的其他字体数据遵循输入字体各自的授权；本仓库不包含 TTF/OTF 文件。
`docs/SSD1315.pdf` 是第三方芯片资料，不适用本项目 MIT 许可证。
