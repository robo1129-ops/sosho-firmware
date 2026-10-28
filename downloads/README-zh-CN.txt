SO-SHO 本地验证固件

本 ZIP 包含 2026-10-28 在本地构建的六种 UF2 配置。
通常使用右侧 Central 的 sosho_R_trackball.uf2 和左侧 Peripheral 的
sosho_L_trackpad.uf2。请使用匹配的一对固件。

通过 USB 刷写时，只将右侧进入引导模式。右下最右端的实体按键保留为右侧
Central 的引导模式按键，不能在 Web 编辑器中改成普通按键。左侧相同位置不执行引导模式动作。

运行时休眠、键位和宏写入需要 SO-SHO 扩展 RPC 固件。已确认这些本地构建和 UF2
生成成功；在 Web 编辑器中看到重新读取一致之前，不声明实机保存成功。
