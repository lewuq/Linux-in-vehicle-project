#!/bin/bash
# embedded_compliance_check.sh
# 车载终端嵌入式工程准则检查工具
# 用途：部署前检查代码是否遵守规范

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

echo "=========================================="
echo "    车载终端嵌入式工程准则合规性检查"
echo "=========================================="
echo ""

COMPLIANCE_SCORE=0
TOTAL_CHECKS=0

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

check_pass() {
    echo -e "${GREEN}✓ PASS${NC} $1"
    ((COMPLIANCE_SCORE++))
}

check_fail() {
    echo -e "${RED}✗ FAIL${NC} $1"
}

check_warn() {
    echo -e "${YELLOW}⚠ WARN${NC} $1"
}

# ============= 准则一：资源路径检查 =============
echo "【准则一】避免使用本地绝对路径"
echo "检查范围：所有 .cpp 和 .h 文件"
echo ""

HARDCODED_PATHS=$(grep -r "/home/zeller\|/usr/local\|/opt/" \
    --include="*.cpp" --include="*.h" "$PROJECT_ROOT" 2>/dev/null | grep -v ".qrc\|EMBEDDED_GUIDELINES.md\|Fonts/README" || true)

if [ -z "$HARDCODED_PATHS" ]; then
    check_pass "不存在硬编码绝对路径"
else
    check_fail "发现硬编码绝对路径："
    echo "$HARDCODED_PATHS"
fi
((TOTAL_CHECKS++))

# 检查是否使用 :/ 资源路径
RESOURCE_USAGE=$(grep -r '":/' \
    --include="*.cpp" --include="*.h" "$PROJECT_ROOT" 2>/dev/null | wc -l || true)

if [ "$RESOURCE_USAGE" -gt 5 ]; then
    check_pass "使用资源路径 (:/) 的比例合理"
else
    check_warn "使用资源路径 (:/) 较少，建议优化"
fi
((TOTAL_CHECKS++))

# 检查 .qrc 文件完整性
if [ -f "$PROJECT_ROOT/img.qrc" ]; then
    QRC_SIZE=$(wc -c < "$PROJECT_ROOT/img.qrc")
    if [ "$QRC_SIZE" -gt 1000 ]; then
        check_pass ".qrc 文件完整（大小 > 1KB）"
    else
        check_warn ".qrc 文件可能过小，检查是否包含足够资源"
    fi
else
    check_fail ".qrc 资源文件不存在"
fi
((TOTAL_CHECKS++))

# ============= 准则二：窗口管理检查 =============
echo ""
echo "【准则二】全屏与多窗口管理策略"
echo "检查范围：main.cpp 和 mainwindow.cpp"
echo ""

# 检查 configureEmbeddedDisplay 函数
if grep -q "configureEmbeddedDisplay\|setWindowFlags.*FramelessWindowHint" "$PROJECT_ROOT/main.cpp" 2>/dev/null; then
    check_pass "窗口已配置为无装饰模式（FramelessWindowHint）"
else
    check_fail "main.cpp 中未发现窗口无装饰配置"
fi
((TOTAL_CHECKS++))

# 检查窗口固定尺寸
if grep -q "setFixedSize.*1024.*600\|resize.*1024.*600" "$PROJECT_ROOT/main.cpp" 2>/dev/null \
   || grep -q "setFixedSize.*1024.*600\|resize.*1024.*600" "$PROJECT_ROOT/mainwindow.cpp" 2>/dev/null; then
    check_pass "窗口尺寸固定为 1024x600"
else
    check_warn "未发现窗口固定尺寸配置，检查是否在 configureEmbeddedDisplay 中"
fi
((TOTAL_CHECKS++))

# 检查是否避免了 QDialog
QDIALOG_COUNT=$(grep -r "QDialog\|new.*Dialog" \
    --include="*.h" --include="*.cpp" "$PROJECT_ROOT" 2>/dev/null | grep -v "EMBEDDED_GUIDELINES\|QFileDialog\|QMessageBox" | wc -l || true)

if [ "$QDIALOG_COUNT" -eq 0 ]; then
    check_pass "未在代码中使用独立 QDialog（推荐伪弹窗）"
else
    check_warn "检测到 $QDIALOG_COUNT 个 Dialog 类，建议改用伪弹窗"
fi
((TOTAL_CHECKS++))

# ============= 准则三：内存管理检查 =============
echo ""
echo "【准则三】内存约束与释放策略"
echo "检查范围：网络请求、图片加载"
echo ""

# 检查是否有 reply->deleteLater()
REPLY_DELETE=$(grep -r "reply->deleteLater()" \
    --include="*.cpp" "$PROJECT_ROOT" 2>/dev/null | wc -l || true)

if [ "$REPLY_DELETE" -gt 0 ]; then
    check_pass "网络请求中有 reply->deleteLater() 调用（$REPLY_DELETE 处）"
else
    check_warn "未发现 reply->deleteLater() 调用，检查网络内存释放逻辑"
fi
((TOTAL_CHECKS++))

# 检查是否使用 setScaledContents(false) 进行平滑缩放
SMOOTH_SCALE=$(grep -r "setScaledContents.*false\|SmoothTransformation" \
    --include="*.cpp" "$PROJECT_ROOT" 2>/dev/null | wc -l || true)

if [ "$SMOOTH_SCALE" -gt 2 ]; then
    check_pass "使用了平滑缩放（SmoothTransformation）"
else
    check_warn "平滑缩放使用较少，某些图标可能有锯齿"
fi
((TOTAL_CHECKS++))

# 检查是否使用了 new 动态分配替代栈分配
NEW_USAGE=$(grep -r "= new.*Widget\|= new.*Page" \
    --include="*.cpp" "$PROJECT_ROOT" 2>/dev/null | wc -l || true)

if [ "$NEW_USAGE" -gt 3 ]; then
    check_pass "多个 Widget 采用了动态分配（便于懒加载）"
else
    check_warn "动态分配较少，考虑优化为懒加载以节省内存"
fi
((TOTAL_CHECKS++))

# ============= 准则四：字体与触控检查 =============
echo ""
echo "【准则四】字体与触控的硬件适配"
echo "检查范围：main.cpp、QSS、按钮尺寸"
echo ""

# 检查字体加载函数
if grep -q "loadEmbeddedFonts\|QFontDatabase::addApplicationFont" "$PROJECT_ROOT/main.cpp" 2>/dev/null; then
    check_pass "main.cpp 中有自定义字体加载函数"
else
    check_fail "main.cpp 中未发现字体加载逻辑"
fi
((TOTAL_CHECKS++))

# 检查是否有字体文件
if [ -d "$PROJECT_ROOT/Fonts" ] && ls "$PROJECT_ROOT/Fonts"/*.ttf 2>/dev/null | grep -q .; then
    FONT_COUNT=$(ls "$PROJECT_ROOT/Fonts"/*.ttf 2>/dev/null | wc -l)
    check_pass "Fonts 目录中有 $FONT_COUNT 个字体文件"
else
    check_warn "Fonts 目录中未发现 .ttf 文件，建议放置思源黑体"
fi
((TOTAL_CHECKS++))

# 检查 QSS 中是否指定了 font-family
QSS_FONTFAMILY=$(grep -r "font-family\|font:" \
    --include="*.qss" "$PROJECT_ROOT" 2>/dev/null | wc -l || true)

if [ "$QSS_FONTFAMILY" -gt 0 ]; then
    check_pass "QSS 样式表中有字体定义"
else
    check_warn "QSS 中未发现明确的 font-family 声明"
fi
((TOTAL_CHECKS++))

# 检查按钮最小尺寸
MIN_SIZE_CHECK=$(grep -r "setFixedSize\|setMinimumSize" \
    --include="*.cpp" --include="*.h" "$PROJECT_ROOT" 2>/dev/null | grep -i "btn\|button\|push" | wc -l || true)

if [ "$MIN_SIZE_CHECK" -gt 0 ]; then
    check_pass "检测到按钮尺寸设置（需手动验证 ≥ 48×48px）"
else
    check_warn "未发现明确的按钮尺寸设定，建议设置最小尺寸"
fi
((TOTAL_CHECKS++))

# ============= 汇总报告 =============
echo ""
echo "=========================================="
echo "              检查结果汇总"
echo "=========================================="

PASS_RATE=$((COMPLIANCE_SCORE * 100 / TOTAL_CHECKS))

echo "通过检查项：$COMPLIANCE_SCORE / $TOTAL_CHECKS"
echo "合规率：$PASS_RATE%"
echo ""

if [ "$PASS_RATE" -ge 80 ]; then
    echo -e "${GREEN}✓ 合规等级：优秀${NC}"
    echo "项目基本符合嵌入式工程准则。"
else
    echo -e "${YELLOW}⚠ 合规等级：需要改进${NC}"
    echo "请根据上述建议进行优化。"
fi

echo ""
echo "=========================================="
echo "推荐后续步骤："
echo "=========================================="
echo "1. 根据 EMBEDDED_GUIDELINES.md 进行必要优化"
echo "2. 部署到 i.MX6ULL 硬件进行真实测试"
echo "3. 监控内存占用：top -p \$(pidof VehicleTerminal)"
echo "4. 检查日志输出，确保无异常警告"
echo ""
