# Fonts 目录说明

## 目录用途
此目录存放嵌入式应用所需的自定义字体文件，确保 PC 和硬件上显示一致。

## 推荐字体：思源黑体（Source Han Sans CN）

### 为什么选择思源黑体？
- ✅ **开源免费**：Apache License 2.0
- ✅ **跨平台**：支持 Windows、macOS、Linux
- ✅ **完整中文支持**：包含 GBK 和 Unicode 全字符
- ✅ **车载友好**：清晰度高，易读性强
- ✅ **文件体积小**：Regular ≈ 12MB，Bold ≈ 12MB

vs 其他方案：
| 字体 | 开源 | 中文支持 | i.MX6ULL 可用 | 推荐度 |
|-----|------|--------|-------------|--------|
| **思源黑体** | ✅ | ✅ | ✅ | ⭐⭐⭐⭐⭐ |
| 微软雅黑 | ❌ | ✅ | ❌ | ⭐⭐ |
| Roboto | ✅ | ❌ | ✅ | ⭐⭐ |
| 文泉驿 | ✅ | ✅ | ✅ | ⭐⭐⭐ |

## 下载和设置步骤

### 1. 下载字体文件

```bash
# 方案 A：从官方 GitHub
git clone https://github.com/adobe-fonts/source-han-sans.git
cd source-han-sans/OTC

# 方案 B：直接下载（推荐国内）
# 从 Adobe 官方下载页：https://github.com/adobe-fonts/source-han-sans/releases
# 选择 SourceHanSansCN-{Regular,Bold}.ttf
```

### 2. 放置到项目目录

```bash
# 在项目根目录创建 Fonts 子目录
mkdir -p Fonts

# 复制字体文件
cp SourceHanSansCN-Regular.ttf Fonts/
cp SourceHanSansCN-Bold.ttf Fonts/
```

### 3. 添加到 img.qrc

编辑 `img.qrc`，在合适位置添加：

```xml
<qresource prefix="/fonts">
    <file>Fonts/SourceHanSansCN-Regular.ttf</file>
    <file>Fonts/SourceHanSansCN-Bold.ttf</file>
</qresource>
```

### 4. 修改 main.cpp

已在 `main.cpp` 中实现了 `loadEmbeddedFonts()` 函数，会自动加载上述字体。

## 验证字体加载

编译并运行程序，在控制台查看输出：

```
[启动] 字体加载...
已加载字体：("Source Han Sans CN",)
[启动] 嵌入式窗口配置完成: 1024x600 / 固定尺寸 / 无窗口装饰
```

如果看到 `加载失败` 的警告，检查：
1. 字体文件是否在 Fonts/ 目录
2. img.qrc 中是否正确添加了 file 条目
3. 是否重新运行了 `qmake` 和 `make`

## 许可证

思源黑体遵循 Apache License 2.0，可用于任何用途（包括商业应用）。

## 中文显示测试

运行程序后，查看以下文本是否正常显示：
- 当前温度（天气页）
- 地名（"惠州"）
- 按钮文本（"更新"、"返回"）

如果显示豆腐块（□□□），说明字体加载失败。

## 备用方案：轻量级开源字体

如果思源黑体文件过大，可以选择：

1. **思源黑体 Subset 版本**（仅包含中文和基础拉丁）
2. **文泉驿微米黑**（更小，但显示效果略差）
3. **Noto Sans CJK**（Google 出品，支持日韩中）

## 常见问题

**Q: 为什么需要自定义字体？**
A: PC 和硬件的系统字体库不同。硬件上可能没有微软雅黑，导致中文显示为豆腐块。自定义字体确保一致性。

**Q: 字体文件太大，会影响应用大小吗？**
A: 思源黑体 Regular ≈ 10MB，Bold ≈ 10MB。但这些是**静态资源**，.qrc 编译后不会显著增加二进制大小（取决于 compress 设置）。

**Q: 能否同时使用多个字体？**
A: 可以。在 QSS 或代码中指定 font-family fallback：
```css
font-family: "Source Han Sans CN", "Microsoft YaHei", sans-serif;
```

---

**设置完成后，删除此 README 文件即可。**
