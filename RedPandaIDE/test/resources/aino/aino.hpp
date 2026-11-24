// =============================================================================
//  aino.hpp — 单头文件 C++17 桌面 UI 库
//
//  立即模式 API：应用持有状态，Stack 负责顺序布局，Theme 提供语义颜色。
//  几何图形在 CPU 缓冲内光栅化；文字使用 GDI、FreeType/X11 或 CoreText。
//  当前分支采用 macOS 风格，背景材质以模糊近似。
//
//  用法：仅在一个编译单元定义 AINO_IMPLEMENTATION，其余编译单元直接包含。
//      #define AINO_IMPLEMENTATION
//      #include "aino.hpp"
//
//  编译：
//      MSVC: cl /std:c++17 /utf-8 /EHsc /MT main.cpp user32.lib gdi32.lib ole32.lib windowscodecs.lib
//      GCC: g++ -std=c++17 main.cpp -lX11 -ldl
//      Wayland: g++ -std=c++17 -DAINO_WAYLAND main.cpp -ldl -pthread
//      Clang: clang++ -std=c++17 main.cpp -framework Cocoa -framework CoreText -framework CoreGraphics -framework ImageIO
//
//  源码为 UTF-8，MSVC 使用 /utf-8。
//  静态运行时与平台依赖说明见 docs/wiki/01-quickstart.md。
//
//  作者：sudoevolve <sudoevolve@gmail.com>
//  许可：Apache-2.0（见 LICENSE）
// =============================================================================

#ifndef AINO_HPP
#define AINO_HPP

// 窄字面量统一使用 UTF-8；其他编译单元仍需使用 /utf-8。
#if defined(_MSC_VER)
#  pragma execution_character_set("utf-8")
#endif

#define AINO_VERSION_MAJOR 0
#define AINO_VERSION_MINOR 5
#define AINO_VERSION_PATCH 4
#define AINO_VERSION "0.5.4"

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <functional>

// -----------------------------------------------------------------------------
//  平台探测
// -----------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
#  define AINO_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
#  define AINO_PLATFORM_MACOS 1
#elif defined(__linux__) || defined(__unix__)
#  if defined(AINO_WAYLAND)
#    define AINO_PLATFORM_WAYLAND 1
#  else
#    define AINO_PLATFORM_X11 1
#  endif
#  define AINO_PLATFORM_UNIX 1
#else
#  error "aino: 未支持的平台"
#endif

namespace aino {

// =============================================================================
//  一、基础类型
// =============================================================================

// 8 位通道颜色。内部混合在 sRGB 空间做（GUI 不追求物理正确的线性混合）。
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    Color() = default;
    Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}

    // 0xRRGGBB
    static Color hex(uint32_t rgb, uint8_t alpha = 255) {
        return Color(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb), alpha);
    }
    static Color rgba(int r_, int g_, int b_, int a_ = 255) {
        return Color(uint8_t(r_), uint8_t(g_), uint8_t(b_), uint8_t(a_));
    }
    // 全透明，也叫"未设置"哨兵 —— Style 里用它表示"沿用主题默认值"
    static Color none() { return Color(0, 0, 0, 0); }
    static Color white() { return Color(255, 255, 255); }
    static Color black() { return Color(0, 0, 0); }
    static Color transparent() { return Color(0, 0, 0, 0); }

    bool is_none() const { return a == 0; }

    Color with_alpha(uint8_t alpha) const { return Color(r, g, b, alpha); }
    // 按倍率缩放当前 alpha（0..1）—— 名字刻意与 with_alpha 区分，避免 int 实参二义
    Color scaled_alpha(float factor) const {
        float v = float(a) * factor;
        if (v < 0.0f) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        return Color(r, g, b, uint8_t(v + 0.5f));
    }
    // alpha 直接给 0..255
    Color alpha255(int value) const {
        int v = value < 0 ? 0 : (value > 255 ? 255 : value);
        return Color(r, g, b, uint8_t(v));
    }

    // t=0 返回自身，t=1 返回 other
    Color mix(const Color& other, float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return other;
        float inv = 1.0f - t;
        return Color(uint8_t(r * inv + other.r * t + 0.5f),
                     uint8_t(g * inv + other.g * t + 0.5f),
                     uint8_t(b * inv + other.b * t + 0.5f),
                     uint8_t(a * inv + other.a * t + 0.5f));
    }
    Color lighten(float amount) const {
        return Color(uint8_t(r + (255 - r) * amount + 0.5f),
                     uint8_t(g + (255 - g) * amount + 0.5f),
                     uint8_t(b + (255 - b) * amount + 0.5f), a);
    }
    Color darken(float amount) const {
        return Color(uint8_t(r * (1.0f - amount) + 0.5f),
                     uint8_t(g * (1.0f - amount) + 0.5f),
                     uint8_t(b * (1.0f - amount) + 0.5f), a);
    }
    // 将本色合成到不透明 base 上；半透明颜色判断前景对比度前应先合成。
    Color over(const Color& base) const {
        if (a == 255) return *this;
        const float t = float(a) / 255.0f;
        const float u = 1.0f - t;
        return Color(uint8_t(float(r) * t + float(base.r) * u + 0.5f),
                     uint8_t(float(g) * t + float(base.g) * u + 0.5f),
                     uint8_t(float(b) * t + float(base.b) * u + 0.5f), 255);
    }

    // 感知亮度 0..1，用于自动判断该配深色还是浅色前景
    // 注意：不含 alpha。半透明色要先 over(底色) 再量（见上）。
    float luminance() const {
        return (0.2126f * r + 0.7152f * g + 0.0722f * b) / 255.0f;
    }
    uint32_t rgb24() const { return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }

    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

// 解码后的 RGBA 像素（非预乘 alpha）；加载一次，跨帧复用。
struct Image {
    int width = 0, height = 0;
    std::vector<uint8_t> pixels;
    bool load(const std::string& path, std::string* error = nullptr);
    bool load_resource(int resource_id, std::string* error = nullptr);
    bool valid() const {
        return width > 0 && height > 0 && width <= 16384 && height <= 16384 &&
               uint64_t(width) * uint64_t(height) <= 67108864u &&
               pixels.size() == size_t(width) * size_t(height) * 4u;
    }
};

/** @brief 当前可执行文件的 UTF-8 目录，含末尾分隔符；无法取得时返回空字符串。
 */
std::string executable_directory();

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
};

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}
    static Rect from_ltrb(float l, float t, float r, float b) { return Rect(l, t, r - l, b - t); }
    static Rect centered(float cx, float cy, float w, float h) {
        return Rect(cx - w * 0.5f, cy - h * 0.5f, w, h);
    }

    float left()   const { return x; }
    float top()    const { return y; }
    float right()  const { return x + w; }
    float bottom() const { return y + h; }
    float cx()     const { return x + w * 0.5f; }
    float cy()     const { return y + h * 0.5f; }

    bool empty() const { return w <= 0.0f || h <= 0.0f; }
    bool contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    bool contains(const Vec2& p) const { return contains(p.x, p.y); }
    bool intersects(const Rect& o) const {
        return !(o.x >= right() || o.right() <= x || o.y >= bottom() || o.bottom() <= y);
    }
    Rect inset(float d) const { return Rect(x + d, y + d, w - 2 * d, h - 2 * d); }
    Rect inset(float dx, float dy) const { return Rect(x + dx, y + dy, w - 2 * dx, h - 2 * dy); }
    // 向外扩张，等价于 inset(-d)。
    Rect outset(float d) const { return Rect(x - d, y - d, w + 2 * d, h + 2 * d); }
    // 交集。完全不相交时返回宽高都为 0 的矩形（empty() 为真）。
    // 裁剪区的交叠、以及"这个控件还剩多少可见"都靠它。
    Rect intersect(const Rect& o) const {
        const float x0 = x > o.x ? x : o.x;
        const float y0 = y > o.y ? y : o.y;
        const float x1 = right() < o.right() ? right() : o.right();
        const float y1 = bottom() < o.bottom() ? bottom() : o.bottom();
        return Rect(x0, y0, x1 > x0 ? x1 - x0 : 0.0f, y1 > y0 ? y1 - y0 : 0.0f);
    }
    // 在当前矩形内居中指定尺寸的子矩形，不修改自身。
    Rect center_y(float h_) const { return Rect(x, y + (h - h_) * 0.5f, w, h_); }
    Rect center_x(float w_) const { return Rect(x + (w - w_) * 0.5f, y, w_, h); }
    Rect center(float w_, float h_) const { return Rect(x + (w - w_) * 0.5f, y + (h - h_) * 0.5f, w_, h_); }

    Rect offset(float dx, float dy) const { return Rect(x + dx, y + dy, w, h); }
    Rect offset(const Vec2& d) const { return Rect(x + d.x, y + d.y, w, h); }
    Rect expand(float d) const { return inset(-d); }
    // 返回切下的条带，并从自身移除对应区域。
    Rect cut_left(float amount) { Rect r(x, y, amount, h); x += amount; w -= amount; return r; }
    Rect cut_right(float amount) { w -= amount; return Rect(x + w, y, amount, h); }
    Rect cut_top(float amount) { Rect r(x, y, w, amount); y += amount; h -= amount; return r; }
    Rect cut_bottom(float amount) { h -= amount; return Rect(x, y + h, w, amount); }
    // 水平 / 垂直内边距后的可用区
};

enum class Align  { Left, Center, Right };
enum class VAlign { Top, Middle, Bottom };

// 鼠标光标形状
enum class Cursor { Arrow, Hand, IBeam, ResizeH, ResizeV, Text };

/** @brief 系统符号：Windows 使用 Segoe Fluent Icons，macOS 使用 SF Symbols。 */
enum class SystemIcon {
    Pointer, Text, Toggle, Slider, Segment, Menu, Calendar, Palette, Bell, Table,
    Chart, Loader, Card, Navigation, List, Copy, Star, More, Upload, Download,
    Edit, Trash, Prev, Next, Moon, Sun, Auto, Close, Eye, Plus, Folder,
    Search, ChevronDown, ChevronUp, Check, Minus, Settings, Network, Image, Info, Warning, People,
    Play, Pause, PreviousTrack, NextTrack, Shuffle, RepeatOne, RepeatAll, Timer, Count
};

// 键盘修饰键 / 常用键位（与平台无关的抽象码）
enum Key {
    KEY_NONE = 0,
    KEY_BACKSPACE = 8, KEY_TAB = 9, KEY_ENTER = 13, KEY_ESC = 27, KEY_SPACE = 32,
    KEY_DELETE = 127,
    KEY_LEFT = 0x1000, KEY_RIGHT, KEY_UP, KEY_DOWN,
    KEY_HOME, KEY_END, KEY_PAGEUP, KEY_PAGEDOWN,
    KEY_SHIFT, KEY_CTRL, KEY_ALT, KEY_SUPER,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_COUNT
};

// =============================================================================
//  二、主题 —— macOS 风格语义色
//
//  label/fill/tint 描述文字、表面与强调色，所有组件共用同一组语义字段，
//  由主题工厂映射。修改强调色应调用 set_tint()，同步前景、焦点环与选中行颜色。
//
//  设计资料与实现映射见 docs/macos27/README.md。
// =============================================================================
struct Theme {
    // ---- macOS 原生语义色 ----
    Color tint;                  // controlAccentColor（系统强调色，默认蓝）
    Color tint_foreground;       // 强调色上的文字（alternateSelectedControlText）

    Color label;                 // labelColor        85% 前景
    Color label2;                // secondaryLabelColor  50~55%
    Color label3;                // tertiaryLabelColor   25%
    Color label4;                // quaternaryLabelColor 10%

    Color separator;             // separatorColor    10%
    Color window_bg;             // windowBackgroundColor
    Color control_bg;            // controlBackgroundColor（表格/浏览器等大面）
    Color sidebar_bg;            // 侧边栏材质（本库不做玻璃，取不透明等价色）

    Color label5;                // quinaryLabelColor   5%

    // 控件表面的半透明填充 —— Apple 的 Fills - Opaque 五级（10 / 8 / 5 / 3 / 2%）
    // 标准按钮面 = fill（10%），悬停/按下靠往上叠一层 fill3 / fill4，
    // 这样任意底色上都成立，和系统控件的行为一致。
    Color fill;                  // Fills - Opaque/Primary   10%
    Color fill2;                 // Secondary                 8%
    Color fill3;                 // Tertiary                  5%
    Color fill4;                 // Quaternary                3%
    Color fill5;                 // Quinary                   2%
    Color fill_hover;
    Color fill_press;
    Color track;                 // 进度条/滑块轨道
    Color shadow_color;          // 弹层投影

    // ---- 其余系统面 ----
    Color title_bg;              // 标题栏（Materials - Controls/Title Bar）
    Color popover_bg;            // 弹层底（Materials/Thick）
    Color tooltip_bg;            // 气泡底
    Color tooltip_stroke;        // 气泡内描边（暗色才有）
    Color scrollbar;             // 滚动条滑块
    Color table_sel;             // 表格选中行（Active）
    Color table_sel_inactive;    // 表格选中行（失焦）
    Color row_alt;               // 表格隔行底色
    Color overlay;               // 模态遮罩

    // ---- 强调色板（macOS 27 Accents 变量集）----
    Color blue, red, orange, yellow, green, mint, teal, cyan, indigo, purple, pink, brown, gray;

    Color card_bg;               // 分组卡片表面
    Color on_red;                // 危险色上的文字
    Color input_border;
    Color focus_ring;

    // 度量 —— macOS 27 控件刻度（参考资料 Sizes 变量集，Medium 为默认档）
    //   圆角：Mini~Medium 是圆角矩形（4/5/6），Large/XL 的按钮与开关是胶囊（半径 1000）
    //   字号：macOS 正文 13pt（不是 iOS 的 17pt），行高 16 = size + 3
    //   控件高度：Global/Height = 16 / 20 / 24 / 28 / 36
    //   按钮内边距：水平 7/10/16/16/16，垂直 2/3/4/6/10
    float radius      = 6.0f;   // 标准控件圆角矩形（Medium）
    float radius_sm   = 4.0f;   // 小徽标 / 迷你控件（Mini 4、Small 5、Medium 6）
    float radius_lg   = 10.0f;  // 面板 / 菜单 / 弹层（同心 = 父半径 − 内边距）
    float radius_xl   = 12.0f;  // Group Box / Sheet（Figma 的 Group Box 是 r12）
    float radius_full = 9999.0f;

    float font_size   = 13.0f;  // Body
    float line_height = 16.0f;

    float pad_x       = 16.0f;  // Button/Padding - Horizontal (Medium)
    float pad_y       = 4.0f;   // Button/Vertical Padding (Medium)

    // 5 档控制尺寸（Mini / Small / Medium / Large / XL），组件按尺寸查表用。
    // 下标 0=mini 1=small 2=medium 3=large 4=xl
    float size_h[5]     = {16.0f, 20.0f, 24.0f, 28.0f, 36.0f};
    float size_radius[5]= {4.0f, 5.0f, 6.0f, 7.0f, 9.0f};
    float size_font[5]  = {10.0f, 11.0f, 13.0f, 13.0f, 13.0f};
    float size_line[5]  = {13.0f, 14.0f, 16.0f, 16.0f, 16.0f};
    float size_padx[5]  = {7.0f, 10.0f, 16.0f, 16.0f, 16.0f};
    float size_pady[5]  = {2.0f, 3.0f, 4.0f, 6.0f, 10.0f};

    // 连续圆角强度（Apple / Figma cornerSmoothing）。0 = 纯圆弧，1 = 最"方"的连续圆角。
    // macOS 的控件、面板、菜单一律 0.6 —— 这一项不改，色值全对也还是 Web 味。
    float corner_smoothing = 0.6f;

    // 凹槽底（Segmented 轨道、输入框内壁）：浅色下压一层 8% 黑，深色下必须换
    // 8% 白 —— 深色窗口本身已经接近纯黑，再压黑等于什么都没画，控件会"消失"。
    Color track_well() const {
        return window_bg.luminance() < 0.5f ? Color(255, 255, 255, 20) : Color(0, 0, 0, 20);
    }
    bool is_dark() const { return window_bg.luminance() < 0.5f; }

    std::string font_family;    // 空 = 用平台默认无衬线字体
    std::string font_family_mono;

    // 值比较：Window::set_theme 用它判断"主题真的变了吗"。
    // 没有它，在帧回调里每帧调用 set_theme（很自然的写法：切主题后让本帧就用新配色）
    // 会无条件要帧 → 主循环再也回不到 WaitMessage → 空闲吃满一个核。
    bool same_as(const Theme& o) const {
        auto ceq = [](const Color& a, const Color& b) {
            return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
        };
        for (int i = 0; i < 5; ++i) {
            if (size_h[i] != o.size_h[i] || size_radius[i] != o.size_radius[i]
                || size_font[i] != o.size_font[i] || size_line[i] != o.size_line[i]
                || size_padx[i] != o.size_padx[i] || size_pady[i] != o.size_pady[i]) return false;
        }
        return ceq(card_bg, o.card_bg) && ceq(on_red, o.on_red)
            && ceq(input_border, o.input_border) && ceq(focus_ring, o.focus_ring)
            && ceq(tint, o.tint) && ceq(tint_foreground, o.tint_foreground)
            && ceq(label, o.label) && ceq(label2, o.label2)
            && ceq(label3, o.label3) && ceq(label4, o.label4)
            && ceq(separator, o.separator) && ceq(window_bg, o.window_bg)
            && ceq(control_bg, o.control_bg) && ceq(sidebar_bg, o.sidebar_bg)
            && ceq(fill, o.fill) && ceq(fill_hover, o.fill_hover) && ceq(fill_press, o.fill_press)
            && ceq(track, o.track) && ceq(shadow_color, o.shadow_color)
            && ceq(label5, o.label5) && ceq(fill2, o.fill2) && ceq(fill3, o.fill3)
            && ceq(fill4, o.fill4) && ceq(fill5, o.fill5)
            && ceq(title_bg, o.title_bg) && ceq(popover_bg, o.popover_bg)
            && ceq(tooltip_bg, o.tooltip_bg) && ceq(tooltip_stroke, o.tooltip_stroke)
            && ceq(scrollbar, o.scrollbar) && ceq(table_sel, o.table_sel)
            && ceq(table_sel_inactive, o.table_sel_inactive) && ceq(row_alt, o.row_alt)
            && ceq(overlay, o.overlay)
            && ceq(blue, o.blue) && ceq(red, o.red) && ceq(orange, o.orange)
            && ceq(yellow, o.yellow) && ceq(green, o.green) && ceq(mint, o.mint)
            && ceq(teal, o.teal) && ceq(cyan, o.cyan) && ceq(indigo, o.indigo)
            && ceq(purple, o.purple) && ceq(pink, o.pink) && ceq(brown, o.brown)
            && ceq(gray, o.gray)
            && radius == o.radius && radius_sm == o.radius_sm
            && radius_lg == o.radius_lg && radius_xl == o.radius_xl
            && radius_full == o.radius_full
            && font_size == o.font_size && line_height == o.line_height
            && pad_x == o.pad_x && pad_y == o.pad_y
            && font_family == o.font_family && font_family_mono == o.font_family_mono
            && corner_smoothing == o.corner_smoothing;
    }

    // 同步强调色、前景、焦点环和选中行。
    // 由应用保存选择，再对当前 light()/dark() 主题调用，避免外观切换丢失强调色。
    Theme& set_tint(const Color& color) {
        tint = color;
        tint_foreground = color.luminance() > 0.55f ? Color(0, 0, 0, 217) : Color::white();
        focus_ring = tint.with_alpha(is_dark() ? 140 : 128);
        table_sel = tint;
        return *this;
    }

    // 浅色：窗口底 #FFFFFF，文字 85% 黑，强调色蓝
    static Theme light();
    // 深色：窗口底 #1E1E1E，文字 85% 白
    static Theme dark();
};

// =============================================================================
//  三、Style —— 组件修饰器（可选参数），链式调用
//    每种组件会读取它关心的字段；未被设置的字段（Color::none / 负数值）
//    表示"沿用主题默认"。
// =============================================================================
struct Style {
    // --- 通用外观 ---
    Color bg      = Color::none();
    Color fg      = Color::none();
    Color border_color = Color::none();
    float border_width = -1.0f;   // <0 = 不描边（除非组件自身有默认描边）
    float radius  = -1.0f;
    float pad_x   = -1.0f;
    float pad_y   = -1.0f;
    float font_size = -1.0f;
    int   font_weight = -1;       // 100..900, -1 = 默认；>=600 走粗体
    bool  italic  = false;
    bool  password = false;      // 单行输入以圆点显示，编辑值保持原文
    bool  disabled = false;

    // --- 组件语义变体 ---
    enum Variant { Default, Secondary, Outline, Ghost, Destructive, Link, Standard };
    int variant = -1;             // -1 = 由组件决定默认变体

    // --- 文本 ---
    Align  align  = Align::Left;
    VAlign valign = VAlign::Middle;
    bool   align_set  = false;    // 区分"用户显式指定"与"组件的默认值"
    bool   valign_set = false;
    bool   muted  = false;        // 用 muted_foreground 作为前景色
    std::string placeholder;      // input 用
    // 多行：`\n` 永远生效；wrap = true 时再按 box 宽度自动折行。
    // 折行规则：CJK 逐字断，西文优先退到最近的空格 —— 没有空格才硬断。
    bool  wrap = false;
    float line_gap = 0.0f;        // 行距附加量（0 = 直接用字体的 line_height）

    // spinner 可显式持续播放；skeleton 开启后仍仅在悬停时播放。
    bool  animated = false;

    // 行底线：列表里相邻的 item 之间画一条 1px 分隔线（macOS 的 source list /
    // 表格行都有，但菜单和单独一张卡片没有）。默认关，由调用方点名要。
    bool  row_sep = false;

    // 背景材质的模糊半径（逻辑像素，-1 = 组件默认）；不模拟折射或色散。
    float blur_radius = -1.0f;

    // --- 交互 ---
    float press_scale = 1.0f;     // 按压缩放，1.0 = 不缩放
    uint64_t identity = 0;        // 非零稳定身份，在本 Ui / state_scope 内唯一，与坐标无关

    // -------- 链式修饰 --------
    Style& color(const Color& c) { fg = c; return *this; }
    Style& background(const Color& c) { bg = c; return *this; }
    Style& stroke(float w, const Color& c) { border_width = w; border_color = c; return *this; }
    Style& corner(float r) { radius = r; return *this; }
    Style& padded(float x, float y) { pad_x = x; pad_y = y; return *this; }
    Style& padding(float v) { pad_x = v; pad_y = v; return *this; }
    Style& font(float size) { font_size = size; return *this; }
    // 列表行之间画分隔线（见 row_sep 的注释）
    Style& row_lines() { row_sep = true; return *this; }
    Style& weight(int w) { font_weight = w; return *this; }
    Style& bold() { font_weight = 700; return *this; }
    Style& semi_bold() { font_weight = 600; return *this; }
    Style& medium() { font_weight = 500; return *this; }
    Style& normal_weight() { font_weight = 400; return *this; }
    Style& italicized() { italic = true; return *this; }
    Style& left()   { align = Align::Left;   align_set = true; return *this; }
    Style& center() { align = Align::Center; align_set = true; return *this; }
    Style& right()  { align = Align::Right;  align_set = true; return *this; }
    Style& top()    { valign = VAlign::Top;    valign_set = true; return *this; }
    Style& middle() { valign = VAlign::Middle; valign_set = true; return *this; }
    Style& bottom() { valign = VAlign::Bottom; valign_set = true; return *this; }
    Style& muted_text() { muted = true; return *this; }
    Style& hint(const std::string& h) { placeholder = h; return *this; }
    Style& off() { disabled = true; return *this; }
    Style& secret(bool on = true) { password = on; return *this; }
    Style& scale_on_press(float s) { press_scale = s; return *this; }
    Style& id(uint64_t value) { identity = value; return *this; }
    Style& wrap_text(bool v = true) { wrap = v; return *this; }
    Style& line_spacing(float g) { line_gap = g; return *this; }
    Style& animate(bool v = true) { animated = v; return *this; }
    // 玻璃面板的模糊半径（见 blur_radius）
    Style& blur(float b) { blur_radius = b; return *this; }

    // 语义变体
    Style& primary()     { variant = Default;     return *this; }
    /** @brief 中性实心按钮；Secondary 使用半透明填充。 */
    Style& standard()    { variant = Standard;    return *this; }
    Style& secondary()   { variant = Secondary;   return *this; }
    Style& outline()     { variant = Outline;     return *this; }
    Style& ghost()       { variant = Ghost;       return *this; }
    Style& destructive() { variant = Destructive; return *this; }
    Style& link()        { variant = Link;        return *this; }
};

/** @brief 普通页签的可选关闭、状态标记及宽度限制；指针仅在 tabs 调用期间借用。 */
struct TabOptions {
    /** @brief 输出本帧请求关闭的索引，未请求时写入 -1；由调用方移除对应数据。 */
    int* close_index = nullptr;
    /** @brief 每项的状态色；缺少对应项或颜色为 none 时不画标记。 */
    const std::vector<Color>* indicators = nullptr;
    /** @brief 单项最大宽度，0 表示按文字自然宽度排列。 */
    float max_width = 0.0f;
};

// =============================================================================
//  四、输入状态 —— 每帧刷新，立即模式读取它
// =============================================================================
struct Input {
    Vec2 mouse;                 // 逻辑坐标（已除以 DPI 缩放）
    Vec2 mouse_prev;
    Vec2 mouse_delta;
    float wheel = 0.0f;         // 本帧滚轮增量，向上为正

    bool down[3]     = {false, false, false};   // 左中右是否按住
    bool pressed[3]  = {false, false, false};   // 本帧按下
    bool released[3] = {false, false, false};   // 本帧抬起
    bool double_click = false;
    int  click_count = 0;

    // 键盘
    bool key_down[KEY_COUNT] = {};
    bool key_pressed[KEY_COUNT] = {};
    // key_repeat：按住不放时系统自动重复触发的"再按一次"。
    //
    // 为什么单列一个数组而不是把 key_pressed 直接置真：key_pressed 的语义是
    // "按下那一刻"。很多地方靠它做一次性动作（切换开关、提交表单、Ctrl+Z 撤销），
    // 让它自带重复会误触发。但终端、文本编辑这类"按住该连续动"的场景又必须要重复，
    // 所以把重复单独暴露出来，由调用方自己决定要不要理它（Input::key_hit 是二者取或）。
    //
    // 记**次数**而不是 bool：一帧里可能挤进好几次自动重复（帧掉到 20fps、系统重复
    // 率又高的时候），bool 会把这几次压成一次 —— 终端里就是"长按方向键偶尔少走一格"。
    unsigned char key_repeat[KEY_COUNT] = {};
    // 每个键**按下那一刻**的修饰键状态。为什么需要单独存：同一批消息里
    // Ctrl 的 WM_KEYUP 可能排在 WM_KEYDOWN('Z') 之后、但在这帧之前被处理掉，
    // 于是帧里读 ctrl() 已经是 false —— Ctrl+Z 就收不到了（实测快速按键必现）。
    bool key_ctrl[KEY_COUNT]  = {};
    bool key_shift[KEY_COUNT] = {};
    bool key_alt[KEY_COUNT]   = {};
    std::string text_input;     // 本帧输入的 UTF-8 文本（多行编辑可能包含 Tab）
    bool shift() const { return key_down[KEY_SHIFT]; }
    bool ctrl()  const { return key_down[KEY_CTRL]; }
    bool alt()   const { return key_down[KEY_ALT]; }
    bool super() const { return key_down[KEY_SUPER]; }

    // "这一帧这个键生效了吗" = 首次按下 或 按住后的自动重复。
    // 需要连发的场景（终端方向键、长按退格、游戏按键）用它；只该触发一次的
    // 场景（开关、提交）继续用 key_pressed[k]。
    bool key_hit(int k) const { return k >= 0 && k < KEY_COUNT && (key_pressed[k] || key_repeat[k] > 0); }

    // 这一帧这个键总共应该生效几次（首按 1 次 + 本帧收到的自动重复次数）。
    // 终端方向键、退格这种"按几次就该发几次"的地方用它，别用 key_hit 循环。
    int key_times(int k) const {
        if (k < 0 || k >= KEY_COUNT) return 0;
        return (key_pressed[k] ? 1 : 0) + int(key_repeat[k]);
    }

    // 组合键判定：ctrl_of(k) = "本帧按下的 k 当时 Ctrl 是按住状态"。
    // 注意它**自带** key_pressed[k] 判定 —— 写成"key_pressed[k] && ctrl_of(k)"
    // 是多余的，而写成 ctrl_of(k) 单独用也不会误触发（曾经退回 live 状态，
    // 结果按住 Ctrl 的任意一帧都会当成按了 V，直接粘一段空剪贴板）。
    bool ctrl_of(int k)  const { return key_pressed[k] && key_ctrl[k]; }
    bool alt_of(int k)   const { return key_pressed[k] && key_alt[k]; }
    bool shift_of(int k) const { return key_pressed[k] ? key_shift[k] : key_down[KEY_SHIFT]; }
    bool any_ctrl_key()  const {
        for (int i = 0; i < KEY_COUNT; ++i) if (key_pressed[i] && key_ctrl[i]) return true;
        return false;
    }
    bool any_alt_key()   const {
        for (int i = 0; i < KEY_COUNT; ++i) if (key_pressed[i] && key_alt[i]) return true;
        return false;
    }

    Vec2 mouse_drag_start;
    bool dragging = false;
};

// =============================================================================
//  五、Stack —— 顺序布局器
//
//  从指定矩形沿主方向依次切分空间，调用方负责元素尺寸。
//
//      Stack col = ui.vbox({x, y, w, h}, 10);   // 纵向，元素间距 10
//      ui.heading("标题", col.next(32));
//      ui.button("确定",  col.next(36));
//      ui.button("更多",  col.fill());          // 吃掉剩余
//
//  gap 只加在元素之间；next_frac / part 提供比例与等分，子布局可继承 gap。
//  Stack 可按值复制，不拥有控件状态；不自动换行或计算内容高度。
// =============================================================================
class Ui;

/** @brief 临时选择控件状态的拥有者；析构恢复之前的作用域，不销毁状态。
 * 只在所属 Ui 存活期间、同一 UI 线程内按嵌套顺序使用。
 */
class StateScope {
public:
    StateScope(const StateScope&) = delete;
    StateScope& operator=(const StateScope&) = delete;
    StateScope(StateScope&& other) noexcept;
    StateScope& operator=(StateScope&&) = delete;
    ~StateScope();
private:
    friend class Ui;
    StateScope(Ui* ui, uint64_t previous) : ui_(ui), previous_(previous) {}
    Ui* ui_ = nullptr;
    uint64_t previous_ = 0;
};

/** @brief 滚动视口的作用域句柄；析构或 close() 时恢复裁剪与布局并绘制滚动条。
 * 在所属 Ui 存活期间、同一帧内按嵌套顺序关闭；不要覆盖仍活跃的句柄。
 */
struct ScrollScope {
    Ui*      ui         = nullptr;
    uint64_t id         = 0;
    uint64_t state_scope_id = 0;   // 偏移属于创建句柄时的拥有者
    Rect     box;
    float    content_h  = 0.0f;
    float    max_offset = 0.0f;
    float    offset     = 0.0f;     // 本帧生效的偏移
    bool     active     = false;

    ScrollScope() = default;
    ScrollScope(const ScrollScope&) = delete;
    ScrollScope& operator=(const ScrollScope&) = delete;
    ScrollScope(ScrollScope&& o) noexcept
        : ui(o.ui), id(o.id), state_scope_id(o.state_scope_id), box(o.box), content_h(o.content_h),
          max_offset(o.max_offset), offset(o.offset), active(o.active) {
        o.active = false; o.ui = nullptr;
    }
    ScrollScope& operator=(ScrollScope&&) = delete;
    ~ScrollScope();

    void  close();              // 收尾（幂等；也可等析构）
    void  content(float h);     // 改内容高度（先排后量的场景）
    void  to(float v);          // 程序化滚动
    float y() const { return offset; }
};

struct Stack {
    Ui*   ui   = nullptr;
    Rect  area;                  // 剩余可用区
    float gap  = 0.0f;           // 元素间距

    Stack() = default;
    Stack(Ui* u, const Rect& box, bool vertical, float gap_)
        : ui(u), area(box), gap(gap_), vertical_(vertical) {}

    bool  vertical() const { return vertical_; }
    // 剩余区域 —— 已结算"待垫的 gap"，即"下一个元素的起始位置"；只读不消费
    Rect  rest()     const;
    float remain()   const { const Rect r = rest(); return vertical_ ? r.h : r.w; }
    bool  valid()    const { return area.w > 0.0f && area.h > 0.0f; }

    // ---- 沿主方向切走一块 ----
    Rect next(float size);                  // 固定尺寸；超出剩余时收窄，不溢出
    Rect next_frac(float fraction);         // 按剩余空间的比例
    Rect next_text(const std::string& s, float extra = 0.0f,
                   float font_size = 0.0f, int weight = -1);   // 按文字宽度 + 额外留白
    Rect fill();                            // 吃掉剩余全部
    void skip(float amount);                // 只推进游标，不产出

    // ---- 不改游标：把剩余区等分成 count 份，取第 index 份（份间留 gap）----
    Rect part(int index, int count) const;

    // ---- 在指定区域里开子布局；g < 0 表示继承当前 gap ----
    Stack vbox(const Rect& box, float g = -1.0f) const;
    Stack hbox(const Rect& box, float g = -1.0f) const;
    Stack vbox(float size, float g = -1.0f);    // 切一块并立刻开子布局
    Stack hbox(float size, float g = -1.0f);

private:
    bool vertical_ = true;       // true = 沿 y 推进
    bool pending_  = false;      // 已有元素产出，下一个之前要垫一个 gap
    void advance(float d);
};

// =============================================================================
//  五之二、动画与变换
//
//  tween 按目标推进并在完成后停止续帧；smooth 是不申请帧的指数平滑函数。
//  push_transform / pop_transform 只影响绘制，不影响布局与命中坐标。
// =============================================================================
using Easing = float (*)(float);

namespace easing {
inline float linear(float t) { return t; }
inline float out_cubic(float t) { const float u = 1.0f - t; return 1.0f - u * u * u; }
inline float out_quint(float t) { const float u = 1.0f - t; return 1.0f - u * u * u * u * u; }
inline float in_out_cubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - 4.0f * (1.0f - t) * (1.0f - t) * (1.0f - t);
}
inline float out_expo(float t) {
    return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
// 轻微过冲（1.0 之后回弹一点点），做"弹一下"的强调动效用
inline float out_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}
} // namespace easing

// 仿射变换参数。语义与 CSS transform 一致：
//   p' = translate + rotate(rotate°) * scale * (p - origin)
struct Transform {
    Vec2  translate{0.0f, 0.0f};   // 逻辑单位的平移
    float scale = 1.0f;            // 缩放（绕 origin）
    float rotate = 0.0f;           // 旋转角度，**度**，顺时针为正（屏幕坐标）
    Vec2  origin{0.0f, 0.0f};      // 缩放/旋转的中心（逻辑坐标）
};

// menubar() 无选择时 menu 与 item 均为 -1，可通过 valid() 查询。
struct MenuPick {
    int menu = -1;   // 第几个顶级菜单（-1 = 本帧没有选择）
    int item = -1;   // 该菜单里的第几项
    bool valid() const { return menu >= 0 && item >= 0; }
};

// =============================================================================
//  六、Ui —— 每帧的绘制与组件入口
// =============================================================================
/** @brief 绘制、布局与交互入口；在所属 UI 线程使用。 */
class Ui {
public:
    Ui();
    ~Ui();
    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;
    Ui(Ui&&) = delete;
    Ui& operator=(Ui&&) = delete;

    // ---------------- 帧生命周期（由 Window::run 调用，通常不用手动调）-------
    void begin_frame(float width, float height, float dt, float dpi_scale);
    void end_frame();

    /** @brief 为页面、会话或动态控件指定稳定的状态拥有者；隐藏时保留状态。
     * id 在本 Ui 内唯一，0 表示原有未分组状态；嵌套作用域使用各自独立的 id。
     * 返回的句柄只恢复当前作用域，永久销毁拥有者时调用 release_state_scope(id)。
     */
    [[nodiscard]] StateScope state_scope(uint64_t id);
    /** @brief 明确销毁指定拥有者的内部状态；不修改应用持有的业务数据。
     * 仅 UI 线程调用。帧内延迟到所有 deferred 结束后释放，帧外立即释放。
     * 隐藏、切页、离屏不应调用；0 和测量期间的调用忽略，其他作用域不受影响。
     * 销毁后不要继续使用该拥有者；若复用 id，须等本帧 end_frame() 完成。
     */
    void release_state_scope(uint64_t id);

    // ---------------- 环境查询 ----------------
    /** @brief 当前调用层可见的输入；被浮层或裁剪挡住时屏蔽鼠标按键、滚轮和拖拽。
     * 鼠标坐标和键盘保留；自绘 hover 使用 pointer_available() 或 hovered()。
     * 引用只在当前交互作用域内使用，不要带入 deferred 回调。
     */
    const Input& input() const;
    /** @brief 原始帧输入，仅供全局快捷键、弹层外部点击检测等输入管理代码使用。 */
    const Input& raw_input() const;
    /** @brief 当前调用层的指针是否未被浮层、裁剪或测量模式屏蔽。 */
    bool pointer_available() const;
    const Theme& theme() const;
    void  set_theme(const Theme& t);
    float dpi_scale() const;              // 例如 1.5
    float width() const;                  // 逻辑宽
    float height() const;                 // 逻辑高
    float time() const;                   // 秒
    float frame_dt() const;

    /** @brief 主线程在帧外请求一次更新；帧内、测量期间或未绑定窗口时返回 false。 */
    bool request_next_frame();

    /** @brief 自定义动画在当前帧申请一个后续帧；帧外或测量期间返回 false。
     * 每帧仅在动画仍需推进时调用；停止调用即停止续帧，不改变全局动画开关。
     */
    bool request_animation_frame();
    /** @brief 当前帧登记一次延迟重绘；下一帧若仍需要，应重新登记。仅 UI 线程调用。 */
    void request_frame_in(float seconds);

    // ---- 布局测量 ----
    // 进入后所有图元与命中测试只记账、不落地（不画、不响应、不排帧）。
    // 立即模式没有隐式自动高度，而卡片 / 面板往往要在**画之前**知道内容多高，
    // 于是用同一份绘制代码先空跑一遍量出外接矩形：
    //     ui.begin_measure();
    //     d.draw(ui, a, box);            // 拿到任意 box 都能量
    //     const float need = ui.end_measure().h;
    //     // 再用 max(声明高度, need) 画真正的卡片
    // 代价是这段绘制代码每帧跑两遍（测量那遍不碰像素），换来高度不再靠手填。
    void begin_measure();
    Rect end_measure();
    bool measuring() const;
    // 显式启停连续动画；启用期间必须在动画结束时关闭。
    void set_animating(bool on);
    bool animating() const;

    void set_cursor(Cursor c);
    Cursor cursor() const;

    // 焦点与剪贴板（输入框用；剪贴板由平台后端提供）
    void* focused_id() const;
    void  set_focus(void* id);

    // ---------------- 图元 ----------------
    void clear(const Color& c);
    void fill_rect(const Rect& r, const Color& c, float radius = 0.0f);
    void stroke_rect(const Rect& r, const Color& c, float radius = 0.0f, float width = 1.0f);
    // 显式指定连续圆角强度（smooth < 0 表示用主题里的 0.6）
    void fill_rect_s(const Rect& r, const Color& c, float radius, float smooth);
    void stroke_rect_s(const Rect& r, const Color& c, float radius, float width, float smooth);
    // 竖直渐变填充（上 → 下）。macOS 的边框、进度、标题栏都是这种两色竖直渐变。
    void fill_rect_grad(const Rect& r, const Color& top, const Color& bottom,
                        float radius = 0.0f, float smooth = -1.0f);
    // 只描上边（bezel 的顶部高光）：把描边裁到顶部 height 像素内
    void stroke_rect_top(const Rect& r, const Color& c, float radius, float width, float height,
                         float smooth = -1.0f);
    // 内阴影：从内边缘向里衰减的一圈暗带。macOS 的输入框 / 复选框 / 表格靠它"凹"下去。
    void inner_shadow(const Rect& r, const Color& c, float radius, float blur, float dy = 0.0f,
                      float smooth = -1.0f);
    void fill_circle(float cx, float cy, float radius, const Color& c);
    // 月牙（图标用）：圆 (cx,cy,r) 减去圆 (cut_cx,cut_cy,cut_r)。
    // **不是**用背景色挖洞 —— 减法是几何上的，底下是什么色都不会穿帮。
    void fill_crescent(float cx, float cy, float radius,
                       float cut_cx, float cut_cy, float cut_radius, const Color& c);
    void draw_line(float x0, float y0, float x1, float y1, const Color& c, float width = 1.0f);
    /** @brief 将已解码图像拉伸到 box，遵守当前裁剪与变换。 */
    void draw_image(const Image& image, const Rect& box);
    // 画文本；Style 支持字号、字重、斜体、对齐、折行与行距。
    void draw_text(const std::string& text, const Rect& box, const Color& c, const Style& st = Style());
    /** @brief 在指定区域居中绘制系统图标；尊重当前裁剪、缩放和颜色。 */
    bool system_icon(SystemIcon icon, const Rect& box, const Color& color);
    /** @brief 注册可唤醒重绘的悬停区；被裁剪或浮层遮住时返回 false。 */
    bool hovered(const Rect& box);
    // 度量
    float text_width(const std::string& text, float font_size, int weight = -1) const;
    float line_height(float font_size) const;

    // ---------------- 等宽文本（终端 / 日志 / 表格） ----------------
    // draw_text 走比例字体：一行里每个字符的步进都不同，画不出"列对得齐"的东西。
    // 终端、十六进制视图、固定宽度表格要的是**每个半角字符同宽**的网格：
    //
    //     const float fs  = 13.0f;
    //     const float adv = ui.mono_advance(fs);        // 半角步进（量出来的，不是估的）
    //     const float asc = ui.mono_ascent(fs);         // 顶到基线的距离
    //     for (int row = 0; row < rows; ++row)
    //         for (int col = 0; col < cols; ++col)
    //             ui.draw_mono_text(cell_text, x + col * adv, y + row * lh, color, fs);
    //
    // draw_mono_text 的 (x, y_top) 是文字**左上角**（和 draw_text 的 box 定位同义），
    // 并且自动裁到当前 push_clip 的区域内 —— 滚动区只要裁一次，不用自己切行。
    float mono_width(const std::string& text, float font_size, int weight = 400) const;
    float mono_advance(float font_size, int weight = 400) const;   // 单个半角字符步进
    float mono_ascent(float font_size, int weight = 400) const;    // 顶到基线的距离
    void  draw_mono_text(const std::string& text, float x, float y_top, const Color& c,
                         float font_size, int weight = 400);
    // 按 st.wrap / '\n' 的真实折行结果算出这段文字要占多高（逻辑单位）。
    // 和 text_width 对称：aino 不做"高度自适应"，但高度是**可以量**的 ——
    // 折行规则和执行绘制时用的是同一套（layout_wrapped_lines），不是估算。
    // 所以给组件留高度时可以 width = text_width(...)、height = text_height(...)。
    float text_height(const std::string& text, float width, const Style& st = Style()) const;
    // alert 在给定宽度下需要多高（含内边距与标题行）。给 alert 留高度就用它。
    float alert_height(const std::string& title, const std::string& description,
                       float width, const Style& st = Style()) const;

    // 渐变 / 阴影（可选，不依赖即可）
    void drop_shadow(const Rect& r, float radius, float blur, const Color& c, float dy = 1.0f);

    // 对已绘制的后备缓冲做三趟可分离盒式模糊，按圆角区域写回；不模拟折射或色散。
    // 调用时机：必须先画背景、再调它、最后画面板内容（它只处理调用这一刻的像素）。
    void backdrop_blur(const Rect& r, float radius, float blur);
    // 玻璃面板 = backdrop_blur + 材质 + 高光边 + 外描边 + 投影，一条调用画完。
    //   ui.glass(rect, 12.0f);                       // 默认 12px 模糊
    //   ui.glass(rect, 12.0f, Style().background(...));  // 指定材质色
    void glass(const Rect& r, float radius = 12.0f, const Style& st = Style());

    // ---------------- 动画 ----------------
    // 目标驱动补间：duration 秒内从"当前显示值"过渡到 target，缓动为 e。
    //   float t = ui.tween(make_id("w", rect), open ? 240.0f : 0.0f, 0.24f);
    //   ui.fill_rect(Rect(x, y, t, 4), ui.theme().tint);
    // 运行期间自动要帧；到达目标后不再要帧（空闲 CPU 依旧 0%）。
    float tween(uint64_t id, float target, float duration = 0.22f, Easing e = easing::out_cubic);
    Color tween_color(uint64_t id, const Color& target, float duration = 0.22f,
                      Easing e = easing::out_cubic);
    // 纯函数：指数平滑，和帧率无关（rate 越大跟得越紧）。没有终点时用它。
    float smooth(float current, float target, float rate = 14.0f) const;
    // 显隐淡入淡出：visible 为 true 归 1，false 归 0，返回本帧的值（0..1）
    float fade(uint64_t id, bool visible, float duration = 0.18f, Easing e = easing::out_cubic);

    // ---------------- 变换 ----------------
    // 压入一个仿射变换，作用范围到对应的 pop_transform()。
    // 只影响绘制（几何 + 文字），不影响布局坐标与命中判定。
    // 支持嵌套：内层在外层的局部坐标里再变换一次。
    void push_transform(const Transform& t);
    void pop_transform();
    Transform transform() const;            // 当前累计变换（近似值，R/S/T 已合成）
    bool  transformed() const;              // 当前是否有非单位变换

    // ---------------- 组件 ----------------
    // 文本类
    void label(const std::string& text, const Rect& box, const Style& st = Style());
    void heading(const std::string& text, const Rect& box, const Style& st = Style());
    void subheading(const std::string& text, const Rect& box, const Style& st = Style());
    void muted_text(const std::string& text, const Rect& box, const Style& st = Style());
    void code_text(const std::string& text, const Rect& box, const Style& st = Style());
    // 只读代码块：等宽字体、行号、C++ 语法着色。**不滚动**，超出行数的部分被裁掉，
    // 所以调用方应当用 code_block_height() 算高度，而不是给个固定值。
    void  code_block(const std::string& code, const Rect& box, const Style& st = Style());
    float code_block_height(const std::string& code, const Style& st = Style()) const;

    // 容器类
    void panel(const Rect& box, const Style& st = Style());          // 卡片 / 面板
    void separator(const Rect& box, const Style& st = Style());      // 水平或垂直细线
    void badge(const std::string& text, const Rect& box, const Style& st = Style());
    void progress(float value, const Rect& box, const Style& st = Style());   // value 0..1
    /** @brief 环形进度，value 范围 0..1，使用现有主题轨道与强调色。 */
    void progress_ring(float value, const Rect& box, const Style& st = Style());
    void skeleton(const Rect& box, const Style& st = Style());       // 占位骨架，.animate() 后悬停才播放

    // 交互类 —— 返回 true 表示本帧发生了"值变化"或"点击"
    bool button(const std::string& text, const Rect& box, const Style& st = Style());
    bool icon_button(const std::string& glyph, const Rect& box, const Style& st = Style());
    // 侧边栏条目：左对齐 + 选中底色。button 是居中的，拿它做导航一行行都会飘。
    bool nav_item(const std::string& label, bool active, const Rect& box, const Style& st = Style());
    bool input(std::string* value, const Rect& box, const Style& st = Style());
    // 搜索框（NSSearchField）：胶囊输入框 + 框内左侧放大镜 + 有内容时的清除按钮。
    // 就是 input 的胶囊皮加两枚符号，所以光标/选区/IME 全部复用同一套逻辑。
    bool search_field(std::string* value, const Rect& box, const Style& st = Style());
    // 颜色池（NSColorWell）：48×24 的圆角色块，点击返回 true，由调用方弹选择器。
    bool color_well(const Color& c, const Rect& box, const Style& st = Style());
    // 颜色选择器：触发器 + RGB 三滑杆浮层，保留输入颜色的 alpha。
    // 浮层在帧末更新 *value 并调用 on_change；value/open 须存活至本帧结束。
    void color_picker(Color* value, bool* open, const Rect& box,
                      const std::function<void(Color)>& on_change = {}, const Style& st = Style());
    // 图像池：中性填充、细边、系统图片符号与说明；不处理文件拖放。
    void image_well(const Rect& box, const Style& st = Style());
    bool checkbox(bool* value, const std::string& label, const Rect& box, const Style& st = Style());
    bool toggle(bool* value, const Rect& box, const Style& st = Style());
    bool slider(float* value, float min_value, float max_value, const Rect& box, const Style& st = Style());
    bool tabs(const std::vector<std::string>& labels, int* index, const Rect& box, const Style& st = Style());
    /** @brief 带关闭请求和状态标记的普通页签；溢出时提供翻页按钮。 */
    bool tabs(const std::vector<std::string>& labels, int* index, const Rect& box,
              const TabOptions& options, const Style& st);
    bool segmented(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st = Style());
    bool select(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st = Style());

    // --- 表单补充 ---
    // 单选组：纵向等分 box.h，每行一个圆点 + 标签
    bool radio_group(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st = Style());
    // 数值步进器：[−] 数值 [+] 三段式，点击左右按钮 ±step
    bool stepper(float* value, float step, float min_value, float max_value, const Rect& box, const Style& st = Style());
    // 多行文本框：换行由 '\n' 决定，不做软换行（超宽的行横向裁掉）
    bool textarea(std::string* value, const Rect& box, const Style& st = Style());
    // 星级评分：点击第 i 颗星 → *value = i+1；再点当前值可清零
    bool rating(int* value, int max_stars, const Rect& box, const Style& st = Style());

    // --- 展示补充 ---
    void avatar(const std::string& text, const Rect& box, const Style& st = Style());   // 圆形头像，取首字
    void spinner(const Rect& box, const Style& st = Style());                           // 旋转指示器
    void alert(const std::string& title, const std::string& description, const Rect& box, const Style& st = Style());
    void breadcrumb(const std::vector<std::string>& items, const Rect& box, const Style& st = Style());
    void kbd(const std::string& text, const Rect& box, const Style& st = Style());       // 按键提示
    bool accordion(const std::string& title, bool* open, const Rect& box, const Style& st = Style());
    void table(const std::vector<std::string>& headers,
               const std::vector<std::vector<std::string>>& rows,
               const Rect& box, const Style& st = Style());
    void sparkline(const std::vector<float>& values, const Rect& box, const Style& st = Style());
    void bars(const std::vector<float>& values, const Rect& box, const Style& st = Style());

    // --- 分组按钮、复合输入、列表与日期 ---

    // Toggle / Toggle Group：按下态按钮（不是开关！开关是 toggle()）
    bool toggle_button(const std::string& label, bool* value, const Rect& box, const Style& st = Style());
    // 单选（*index = -1 表示一个都不选）／多选（*mask 的第 i 位）
    bool toggle_group(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st = Style());
    bool toggle_group_multi(const std::vector<std::string>& items, std::uint32_t* mask,
                            const Rect& box, const Style& st = Style());

    // Button Group：共用轨道的动作按钮，各自独立触发，无持久选中态
    bool button_group(const std::vector<std::string>& items, int* clicked_index,
                      const Rect& box, const Style& st = Style());

    // Input Group：左右挂前缀/后缀（单位、币种、@域名）
    bool input_group(std::string* value, const std::string& prefix, const std::string& suffix,
                     const Rect& box, const Style& st = Style());
    // Input OTP：length 个格子，自动聚焦、只收数字、退格回退、方向键移动
    bool input_otp(std::string* value, int length, const Rect& box, const Style& st = Style());

    // Field：标签 + 控件槽 + 描述/错误文案。control 拿到的是控件该占的矩形
    void field(const std::string& label, const std::string& hint, const std::string& error,
               const Rect& box, const std::function<void(const Rect&)>& control,
               const Style& st = Style());
    // Field 的高度估算（和 field() 用同一套数值，避免"算的"和"画的"对不上）
    float field_height(const std::string& label, const std::string& hint, const std::string& error,
                       float control_h, const Style& st = Style()) const;

    // Item：一行条目（标题 + 描述 + 尾部文字），整行可点
    bool item(const std::string& title, const std::string& description, const std::string& trailing,
              const Rect& box, const Style& st = Style());
    // Empty：居中的空状态标题与描述
    void empty_state(const std::string& title, const std::string& description,
                     const Rect& box, const Style& st = Style());

    // Pagination：页码器，自动省略号；返回本帧是否翻页
    bool pagination(int* page, int total_pages, const Rect& box, const Style& st = Style());

    // Calendar：月历（纯整数日期运算，不依赖 <ctime>，三平台同一结果）
    bool calendar(int* year, int* month, int* day, const Rect& box, const Style& st = Style());

    // --- 浮层类（自己负责盖在其它组件之上）---
    // 悬停提示：鼠标进入 anchor 时在它上/下方弹一个气泡（帧末绘制）
    void tooltip(const Rect& anchor, const std::string& text, const Style& st = Style());
    // 下拉菜单：box 是触发按钮，点开后弹列表；选中某项时返回 true 并写入 *clicked_index
    bool menu(const std::string& label, const std::vector<std::string>& items,
              int* clicked_index, const Rect& box, const Style& st = Style());
    // 菜单栏：横向一排顶级菜单，点开一个之后把鼠标划到隔壁会自动切换
    // （桌面菜单栏的标准行为，Web 上一般做不到）。labels 与 items 一一对应。
    // 返回本帧选中的是哪一项，没选到就 MenuPick{}。
    MenuPick menubar(const std::vector<std::string>& labels,
                     const std::vector<std::vector<std::string>>& items,
                     const Rect& box, const Style& st = Style());
    // 模态面板：box 处一块浮起的面（r10 + Primary 投影，**不铺整屏黑幕** ——
    // macOS 的模态只是窗口失去焦点，不会被压暗），返回"本帧是否处于打开态"。
    // 自己的按钮紧接着 dialog() 之后画即可，层级由 gate_claim 保证在面板之上。
    bool dialog(const std::string& title, const std::string& body, bool* open,
                const Rect& box, const Style& st = Style());
    // 工作表（sheet）：贴在窗口标题栏下方、**上平下圆**的模态面板。窗口其余部分
    // 只失去焦点、不压黑（整屏黑幕是 Web Dialog 的 overlay，mac 的 sheet 没有）。
    // 按钮和 dialog 一样由调用方在它之后续画。
    bool sheet(const std::string& title, const std::string& body, bool* open,
               const Rect& box, const Style& st = Style());
    // 轻提示条（通常自己摆在右下角；生命周期由调用方用 bool 控制）
    void toast(const std::string& text, const Rect& box, const Style& st = Style());

    // Popover：返回调用时的打开状态；draw 在帧末执行，收到内容区矩形。
    // open 及回调引用的数据须存活至本帧结束；回调关闭面板不改变已返回的值。
    bool popover(const std::string& label, bool* open, const Rect& box, Vec2 content,
                 const std::function<void(const Rect&)>& draw, const Style& st = Style());
    // Hover Card：悬停时在帧末绘制只读内容；回调引用的数据须存活至本帧结束。
    void hover_card(const Rect& anchor, Vec2 content, const std::function<void(const Rect&)>& draw,
                    const Style& st = Style());
    // Alert Dialog：带确认/取消的模态框。返回 true 表示本帧点了确认。
    // 传 Style().destructive() 则确认键使用中性底和危险色文字。
    bool alert_dialog(const std::string& title, const std::string& body,
                      const std::string& confirm, const std::string& cancel,
                      bool* open, const Rect& box, const Style& st = Style());
    // Drawer：位置由 box 指定的模态面板。
    bool drawer(const std::string& title, bool* open, const Rect& box,
                const std::function<void(const Rect&)>& draw, const Style& st = Style());
    // Context Menu：在 area 内右键弹出（不占布局，弹层在鼠标位置）
    bool context_menu(const Rect& area, const std::vector<std::string>& items,
                      int* clicked_index, const Style& st = Style());
    // Combobox：可搜索的下拉选择（输入即过滤，上下键 + 回车）
    bool combobox(const std::vector<std::string>& items, int* index,
                  const Rect& box, const Style& st = Style());

    // 可拖动分栏：ratio 是分隔线位置（0..1），拖动时更新
    bool split_pane(float* ratio, const Rect& box, Rect* first, Rect* second,
                    bool vertical = true, float min_ratio = 0.15f, const Style& st = Style());

    // 将回调推迟到 end_frame()；按值捕获临时值，引用的数据须存活至帧末。
    void deferred(std::function<void()> fn);

    // 布局辅助（立即模式下的轻量自动布局）
    // 按 ratio = 宽/高，从宽度算出高度。
    // 立即模式里没有"容器组件"，所以它就是一个算矩形的小工具：
    //   Rect r = Ui::aspect_rect(cell, 16.0f / 9.0f);  ui.fill_rect(r, bg);
    static Rect aspect_rect(const Rect& box, float ratio);
    // 顺序布局器
    Stack vbox(const Rect& box, float gap = 0.0f, float pad = 0.0f);
    Stack hbox(const Rect& box, float gap = 0.0f, float pad = 0.0f);

    // 裁剪（几何与文字都会被限制在区域内）
    void push_clip(const Rect& r);
    void pop_clip();

    // ---- 滚动 ----
    // 只有一件事：**一个视口 + 一段内容高度**。虚拟列表不是第二个 API，只是
    // "内容高度 = 行数 × 行高"的特例：可见区间能 O(1) 算出来，于是不可见项连
    // 构建都跳过。两者共用同一份偏移（按 box 记忆）、同一根滚动条、同一套裁剪。
    //
    //   通用（内容高度自己给，内部随便排）：
    //     { auto sc = ui.scroll(viewport, content_h);
    //         ui.label(...); ui.row(24); ...
    //     }                                   // 析构自动收尾
    //
    //   虚拟（固定行高，只对可见行调回调）：
    //     int drawn = ui.scroll(list, 100000, 26.0f, [&](int i, const Rect& row) {
    //         ui.label("第 " + std::to_string(i + 1) + " 行", row);
    //     });
    //
    // 滚轮只在鼠标位于 box 内时生效；返回的 drawn 可以直接断言"只画了一屏"。
    ScrollScope scroll(const Rect& box, float content_h);
    template <class F>
    int scroll(const Rect& box, int count, float item_h, F&& draw) {
        ScrollScope sc = scroll(box, float(std::max(count, 0)) * item_h);
        int drawn = 0;
        if (count > 0 && item_h > 0.01f) {
            int i0 = int(std::floor(sc.offset / item_h));
            int i1 = int(std::ceil((sc.offset + box.h) / item_h)) - 1;
            if (i0 < 0) i0 = 0;
            if (i0 > count - 1) i0 = count - 1;
            if (i1 > count - 1) i1 = count - 1;
            for (int i = i0; i <= i1; ++i, ++drawn)
                draw(i, Rect(box.x, box.y + float(i) * item_h - sc.offset, box.w, item_h));
        }
        return drawn;
    }

    // 低层零件：自己算可见区间、自己画，只要偏移与滚动条
    float scroll_visible(const Rect& box, int count, float item_h, int* first, int* last);
    void  scroll_bar(const Rect& box, float content_h);
    float scroll_offset(const Rect& box) const;
    void  scroll_to_index(const Rect& box, int index, float item_h, int count);

    struct Impl;
    Impl* impl = nullptr;

private:
    bool tabs_impl(const std::vector<std::string>& labels, int* index, const Rect& box,
                   const TabOptions& options, bool segmented_control, const Style& st);
    void end_scroll();
    void repaint();
    friend struct ScrollScope;
};

// 带可拖动标题区的玻璃面板。每帧传入 bounds 作为默认位置；draw() 返回实际绘制位置，
// 子控件基于返回值定位。radius、Style().blur(...) 和 Style().background(...) 可调节材质。
//   GlassPanel panel(64.0f);
//   Rect r = panel.draw(ui, bounds, 18.0f, Style().blur(14.0f).background(tint));
class GlassPanel {
public:
    explicit GlassPanel(float drag_height = 72.0f) : drag_height_(drag_height) {}

    Rect draw(Ui& ui, const Rect& bounds, float radius = 12.0f,
              const Style& st = Style());
    void set_drag_height(float height) { drag_height_ = std::max(0.0f, height); }
    void reset() { offset_x_ = offset_y_ = 0.0f; dragging_ = false; }

private:
    float drag_height_ = 72.0f;
    float offset_x_ = 0.0f, offset_y_ = 0.0f;
    float drag_offset_x_ = 0.0f, drag_offset_y_ = 0.0f;
    bool dragging_ = false;
};

// =============================================================================
//  七、Window —— 窗口与主循环
// =============================================================================

// 窗口配置。C++17 用 WindowConfig cfg; cfg.width = ...; 按需覆盖默认值。
struct WindowConfig {
    int         width  = 1180;
    int         height = 760;
    std::string title  = "aino";

    // ---- 图标 ----
    // Windows: .ico; macOS: image or .icns for Dock; X11: _NET_WM_ICON。
    // Wayland 通过 app_id 和桌面 .desktop 文件匹配图标。
    std::string icon_path;

    // ---- 字体 ----
    // font_path 是字体**文件**（.ttf / .otf），库会先把它注册进本进程，
    // 再按文件里 `name` 表写着的家族名排版 —— 所以不用手工填家族名。
    // 只填 font_family 则使用系统已安装的家族名。两个都填时：先注册文件，
    // 家族名以文件为准；注册失败才退回 font_family。
    // Linux 不注册字体文件；解析出的家族名仍通过 fontconfig 匹配。
    std::string font_path;
    std::string font_family;            // 空 = 平台默认无衬线
    std::string font_family_mono;       // 空 = Consolas / 平台等宽

    // ---- 尺寸约束（逻辑像素；0 = 不限制）----
    // 只填 min 就是"不许拖得比这更小"，两个都不填就是随便拖。
    int min_width  = 0;
    int min_height = 0;
    int max_width  = 0;
    int max_height = 0;

    // 关掉后去掉可拖拽的边框与最大化按钮（X11 上只能靠 WM 尊重 size hints）。
    bool resizable = true;

    // 首次显示时居中到当前显示器的工作区（不含任务栏）。
    bool center = true;

    // Windows：关掉后不声明进程 DPI 感知，由系统缩放；只有第一个窗口说了算。
    // Linux：关掉后固定以 1 倍栅格绘制；Wayland 的输出缩放由 compositor 协调。
    bool high_dpi = true;

    /** @brief Wayland 的桌面标识，应与 .desktop 文件名一致；其他后端忽略。 */
    std::string app_id = "aino";
};

/** @brief 原生窗口与事件循环；除 wake()/close() 外，在所属 UI 线程操作。
 * macOS 必须使用主线程；后台调用者须在窗口析构前结束。
 */
class Window {
public:
    explicit Window(const WindowConfig& cfg);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    bool ok() const;                   // 创建是否成功
    Ui&  ui();

    void set_theme(const Theme& t);
    void set_title(const std::string& title);
    void set_size(int width, int height);
    int  width() const;
    int  height() const;
    float dpi_scale() const;
    bool should_close() const;
    void close();

    // 请求下一帧。帧回调内会忽略此调用；帧外的主线程更新可使用它。
    // 跨线程叫醒主循环请使用 wake()。
    void request_repaint();

    // 唤醒阻塞的主循环并请求一帧，可跨线程调用；应用数据仍须自行同步。
    // 不调用 Xlib/AppKit；后台线程须在窗口析构前结束，不要分离捕获窗口引用的线程。
    void wake();

    // 常驻动画（spinner / 光标闪烁 / hover 插值）的重绘帧率上限，默认 60fps。
    // 软件光栅化下，一个"一直在动"的组件会让整窗每帧重画；窗口越大越贵。
    // 把它调到 30 或 20 能近乎线性地砍掉这部分 CPU（代价：动画略顿）。
    void set_anim_fps(float fps);

    // 指针移动时的悬停反馈帧率上限，默认跟着 set_anim_fps（60fps）。
    //
    // 注意"帧率"在这里的含义：aino 没有渲染循环，空闲时一帧都不画（0% CPU）。
    // 只有指针真的移动、而且**悬停目标变了**才会排帧 —— 在同一个按钮上抖鼠标
    // 是 0 帧，扫过一排条目是满刷新率。所以这个上限平时根本碰不到，只有
    // "鼠标持续高速划过大量控件"时才会生效。想更省电可以调到 30。
    void set_move_fps(float fps);

    // 关掉"指针在同一控件内移动就不重绘"的优化。默认开启。
    // 只有一种应用需要关：帧回调里画了跟着指针跑、又不经过任何控件的东西
    // （自定义准星、拖影、画板笔迹）。
    void set_hover_skip(bool on);

    // 主循环：事件驱动。有事件就处理，没事件就阻塞在内核等待 —— 空闲 0% CPU。
    // frame 回调只在"需要重绘"时被调用，所以它可以放心地重建整个 UI。
    void run(const std::function<void(Ui&)>& frame);
    /** @brief 注册同线程子窗口的绘制回调，由该线程正在运行的 run() 派发。
     * @note 按值持有回调。窗口及回调状态必须在所属 UI 线程销毁，不能在自身回调中销毁窗口。
     * macOS 的窗口与事件循环必须位于主线程；清空回调可停止自动绘制。
     */
    void set_frame_callback(std::function<void(Ui&)> frame);

    // 单步：给需要自己控制循环的人用
    bool poll();                       // 处理一批消息，返回是否仍存活
    void render();
    void render(const std::function<void(Ui&)>& frame); // 同步绘制一帧，含 begin/end/present

    // 剪贴板（input 组件的复制粘贴用）
    /** @brief 写入系统剪贴板；失败返回 false，调用者应保留原始内容。 */
    bool try_set_clipboard(const std::string& text);
    /** @brief 在窗口线程交付剪贴板内容；不阻塞等待 X11/Wayland 数据，失败交付空串。
     * 每个窗口只保留一个请求；销毁窗口会取消回调。
     */
    void request_clipboard(std::function<void(std::string)> complete);
    std::string clipboard() const;

    struct Impl;
    Impl* impl = nullptr;
};

/**
 * @brief 在创建窗口之前调用；处理短时系统图标解码子进程，并为普通启动启用隔离。
 * @return -1 表示继续应用启动，非负值表示工作进程已处理，应立即作为退出码返回。
 * @note Linux 复用当前可执行文件；其他平台返回 -1。未接入的应用保持进程内解码。
 */
int system_icon_worker_entry(int argc, char** argv);

} // namespace aino

#endif // AINO_HPP

// =============================================================================
//                          实 现 部 分 开 始
// =============================================================================
#ifdef AINO_IMPLEMENTATION
#ifndef AINO_IMPLEMENTED
#define AINO_IMPLEMENTED

// -----------------------------------------------------------------------------
//  平台头文件（只在实现单元里引入，头文件本身保持干净）
// -----------------------------------------------------------------------------
#ifdef AINO_PLATFORM_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <windowsx.h>
#  include <wincodec.h>
#elif defined(AINO_PLATFORM_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xresource.h>
#  include <X11/cursorfont.h>   // XC_left_ptr / XC_hand2 ...
#  include <X11/keysym.h>
#  include <X11/Xatom.h>
#endif
#if defined(AINO_PLATFORM_UNIX)
#  include <unistd.h>
#  include <sys/mman.h>
#  include <sys/socket.h>
#  include <pthread.h>
#  include <clocale>
#  include <cerrno>       // XOpenDisplay 失败时要报 errno（EAGAIN=11 是"现在别连"）
// 文字渲染：FreeType + fontconfig（运行时 dlopen），核心 XLFD 字体兜底。
//
// 为什么不能用核心 X 字体：它只有 **1bit 点阵字体**（xfonts-base 里的 6x13 /
// 8x13 那几套），既没有抗锯齿，也没有 12/13/14 这种连续字号 —— 请求 13px 时
// 服务端会把 12px 的位图放大到 13px，中文更是直接挑一套全角点阵。结果就是
// "每一个字都在抖、字距莫名其妙、标题和正文一样细"。
//
// 为什么也不用 Xft：XftDrawString 是画到 **X server** 上的，而这里每一帧都是
// 先在客户端缓冲里画完、帧末 XPutImage 整块覆盖 —— 文字要么被盖掉，要么得走
// Pixmap + XGetImage 的同步往返（几百段文字 = 几百次往返，卡）；而且 Xft 只认
// 单个字体，没有逐字符兜底，中文会全是 tofu 方块。
//
// **不 #include 任何第三方头、也不加 -l***：freetype / fontconfig 的运行时包在
// 普通桌面上必然存在（GTK / Qt / Electron 都依赖），但开发包不一定 —— 要求用户
// 装 libfreetype-dev 才看得见正常字，等于没修。见下方 FtApi / FcApi。
#  include <dlfcn.h>
#  include <time.h>
#  if defined(__linux__)
#    include <spawn.h>
#    include <sys/stat.h>
#    include <sys/wait.h>
#    include <fcntl.h>
#    include <signal.h>
#    include <filesystem>
extern char** environ;
#  endif
#endif
#if defined(AINO_PLATFORM_MACOS)
// MacTypes.h 会往**全局**塞 ::Rect 和 ::Style（QuickDraw 遗产），和 aino::Rect /
// aino::Style 撞名 —— 用户代码里 `using aino::Rect;` 会变成二义性。包含系统头之前
// 先把这两个名字改掉，让全局只剩 MacRect / MacStyle（SDL 处理 QuickDraw 也是这招）。
#  define Rect  MacRect
#  define Style MacStyle
#  include <ApplicationServices/ApplicationServices.h>
#  include <CoreText/CoreText.h>
#  include <ImageIO/ImageIO.h>
#  include <objc/objc.h>
#  include <objc/message.h>
#  include <objc/runtime.h>
#  include <mach-o/dyld.h>
#  include <pthread.h>
#  undef Rect
#  undef Style
#  include <unistd.h>
#  include <time.h>
#endif

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <algorithm>
#include <chrono>
#include <list>
#include <limits>
#include <unordered_map>
#include <type_traits>
#include <deque>
#include <memory>
#include <fstream>
#include <atomic>
#include <mutex>
#if !defined(AINO_PLATFORM_WINDOWS)
#  include <poll.h>
#  include <fcntl.h>
#  include <cerrno>
#endif

namespace aino {
namespace detail {

template<class Fn> struct ScopeExit {
    Fn fn;
    explicit ScopeExit(Fn action) : fn(std::move(action)) {}
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ~ScopeExit() { fn(); }
};

// =============================================================================
//  8.1  基础工具
// =============================================================================

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// 表格单元格是不是"数字"：可选正负号 + 数字 + 小数点 + 千分位 + 结尾的 %/单位。
// macOS 的表格里数字列一律右对齐，靠这个判断，调用方不用为每列再传一次对齐方式。
inline bool looks_numeric(const std::string& s) {
    size_t i = 0, n = s.size();
    while (i < n && (s[i] == ' ' || s[i] == '\t')) ++i;
    if (i < n && (s[i] == '-' || s[i] == '+')) ++i;
    bool digit = false, dot = false;
    for (; i < n; ++i) {
        const char c = s[i];
        if (c >= '0' && c <= '9') { digit = true; continue; }
        if (c == '.' && !dot) { dot = true; continue; }
        if (c == ',' || c == ' ' || c == '\'') continue;          // 千分位
        if (c == '%' || c == '\xC2' /* ¥ 的前导字节 */) continue;
        if (c == '$' || c == '\xE2' /* € 的前导字节 */) continue;
        if (c == 'Y' || c == 'K' || c == 'M') continue;            // ¥ / KB / MB 之类
        break;
    }
    if (!digit) return false;
    // 尾部允许单位（ms / 秒 / px / KB），但不允许再出现数字 —— "第 3 项" 不是数字列
    for (size_t k = i; k < n; ++k)
        if ((s[k] >= '0' && s[k] <= '9') || s[k] == '.') return false;
    return true;
}
inline int   clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// FNV-1a 64 位：给立即模式的组件生成稳定 ID
inline uint64_t hash_string(const char* s) {
    uint64_t h = 1469598103934665603ull;
    while (s && *s) { h ^= uint8_t(*s++); h *= 1099511628211ull; }
    return h;
}
inline uint64_t hash_combine(uint64_t a, uint64_t b) {
    return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2));
}
// 用组件名 + 矩形坐标合成 ID —— 立即模式里没有控件对象，ID 必须可复现
inline uint64_t make_id(const char* kind, const Rect& r, uint64_t extra = 0) {
    uint64_t h = hash_string(kind);
    int ix = int(r.x * 2.0f), iy = int(r.y * 2.0f);
    h = hash_combine(h, uint64_t(uint32_t(ix)) << 32 | uint32_t(iy));
    if (extra) h = hash_combine(h, extra);
    return h;
}

// -----------------------------------------------------------------------------
//  UTF-8 <-> 码点（文本编辑必须按码点走，否则中文会被截断）
// -----------------------------------------------------------------------------
inline int utf8_decode_n(const char* s, size_t avail, uint32_t* out) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
    if (avail == 0) { *out = 0; return 0; }
    if (p[0] < 0x80) { *out = p[0]; return 1; }
    if ((p[0] & 0xE0) == 0xC0) {
        *out = avail >= 2 ? (((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu)) : 0xFFFD;
        return 2;
    }
    if ((p[0] & 0xF0) == 0xE0) {
        *out = avail >= 3 ? (((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu)) : 0xFFFD;
        return 3;
    }
    if ((p[0] & 0xF8) == 0xF0) {
        *out = avail >= 4 ? (((p[0] & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12)
                            | ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu)) : 0xFFFD;
        return 4;
    }
    *out = 0xFFFD; return 1;
}
inline int utf8_encode(uint32_t cp, char* out) {
    if (cp < 0x80) { out[0] = char(cp); return 1; }
    if (cp < 0x800) { out[0] = char(0xC0 | (cp >> 6)); out[1] = char(0x80 | (cp & 0x3F)); return 2; }
    if (cp < 0x10000) {
        out[0] = char(0xE0 | (cp >> 12)); out[1] = char(0x80 | ((cp >> 6) & 0x3F)); out[2] = char(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = char(0xF0 | (cp >> 18)); out[1] = char(0x80 | ((cp >> 12) & 0x3F));
    out[2] = char(0x80 | ((cp >> 6) & 0x3F)); out[3] = char(0x80 | (cp & 0x3F));
    return 4;
}
inline std::vector<uint32_t> utf8_codepoints(const std::string& s) {
    std::vector<uint32_t> cps;
    size_t i = 0;
    while (i < s.size()) {
        uint32_t cp = 0;
        int n = utf8_decode_n(s.c_str() + i, s.size() - i, &cp);
        cps.push_back(cp);
        i += size_t(n < 1 ? 1 : n);
    }
    return cps;
}
inline std::string utf8_from_codepoints(const std::vector<uint32_t>& cps, size_t from = 0, size_t count = SIZE_MAX) {
    std::string out;
    char buf[8];
    size_t end = (count == SIZE_MAX) ? cps.size() : std::min(cps.size(), from + count);
    for (size_t i = from; i < end; ++i) {
        int n = utf8_encode(cps[i], buf);
        out.append(buf, size_t(n));
    }
    return out;
}
// 码点计数：单遍扫描，**不分配**。老实现是 utf8_codepoints(s).size()，每调一次
// 就要为整串分配一个 uint32 数组 —— 输入框/多行文本框每帧都调它，20 万字的文档
// 每帧白分配 800 KB。前进步长只由首字节决定，与 utf8_codepoints 的计数完全一致。
inline int utf8_count(const std::string& s) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    const size_t n = s.size();
    int count = 0;
    size_t i = 0;
    while (i < n) {
        const unsigned char c = p[i];
        size_t step = 1;
        if ((c & 0xE0u) == 0xC0u) step = 2;
        else if ((c & 0xF0u) == 0xE0u) step = 3;
        else if ((c & 0xF8u) == 0xF0u) step = 4;
        i += step;
        ++count;
    }
    return count;
}
// 取第 idx 个码点之前的字节偏移
inline size_t utf8_byte_offset(const std::string& s, int idx) {
    size_t i = 0; int k = 0;
    while (i < s.size() && k < idx) { uint32_t cp; i += size_t(utf8_decode_n(s.c_str() + i, s.size() - i, &cp)); ++k; }
    return i;
}

// -----------------------------------------------------------------------------
//  从字体文件里读家族名（sfnt 的 `name` 表）
// -----------------------------------------------------------------------------
// WindowConfig::font_path 只给了文件路径，可 GDI / CoreText 都要**家族名**
// 才能选字体。三个平台各查一遍太脏，而且 Windows 的 GetFontResourceInfoW
// 只有 MSVC 的头文件里有声明，mingw 直接编不过。
//
// `name` 表是 TrueType / OpenType 的公共部分（.ttf 和 .otf 结构一致），
// 自己解三十行就够，还跨平台。要的是 nameID=1（家族名），退而求其次 4
// （全名）；字符串优先取 platformID=3（Windows / UTF-16BE），没有再取
// platformID=1（Mac Roman，当 Latin-1 处理）。
inline std::string font_family_from_file(const std::string& path) {
#ifdef AINO_PLATFORM_WINDOWS
    const int path_units = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), int(path.size()), nullptr, 0);
    std::wstring wide(size_t(std::max(path_units, 0)), L'\0');
    if (path_units <= 0 || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), int(path.size()), wide.data(), path_units)) return {};
    std::FILE* f = _wfopen(wide.c_str(), L"rb");
#else
    std::FILE* f = std::fopen(path.c_str(), "rb");
#endif
    if (!f) return std::string();
    std::vector<uint8_t> buf;
    {
        std::fseek(f, 0, SEEK_END);
        const long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        // 字体文件最大也就几十 MB，超过 64MB 的认成"不是字体"，别把内存吃光
        if (n > 0 && n < 64L * 1024 * 1024) {
            buf.resize(size_t(n));
            if (std::fread(buf.data(), 1, size_t(n), f) != size_t(n)) buf.clear();
        }
    }
    std::fclose(f);
    if (buf.size() < 12) return std::string();

    auto u16 = [&](size_t o) -> uint32_t {
        return o + 2 <= buf.size() ? uint32_t(buf[o]) << 8 | uint32_t(buf[o + 1]) : 0u;
    };
    auto u32 = [&](size_t o) -> uint32_t {
        return o + 4 <= buf.size() ? uint32_t(buf[o]) << 24 | uint32_t(buf[o + 1]) << 16 |
                                     uint32_t(buf[o + 2]) << 8 | uint32_t(buf[o + 3]) : 0u;
    };

    // 表目录：numTables 在 4；每条记录 16 字节（tag/checksum/offset/length）
    const uint32_t num_tables = u16(4);
    size_t name_off = 0, name_len = 0;
    for (uint32_t i = 0; i < num_tables && i < 64; ++i) {
        const size_t rec = 12 + size_t(i) * 16;
        if (rec + 16 > buf.size()) break;
        if (buf[rec] == 'n' && buf[rec + 1] == 'a' && buf[rec + 2] == 'm' && buf[rec + 3] == 'e') {
            name_off = u32(rec + 8);
            name_len = u32(rec + 12);
            break;
        }
    }
    if (!name_off || !name_len || name_off + 6 > buf.size()) return std::string();

    const uint32_t count = u16(name_off + 2);
    const size_t str_base = name_off + u16(name_off + 4);
    std::string fam_win, full_win, best_mac;
    for (uint32_t i = 0; i < count && i < 512; ++i) {
        const size_t rec = name_off + 6 + size_t(i) * 12;
        if (rec + 12 > buf.size()) break;
        const uint32_t plat = u16(rec);
        const uint32_t name_id = u16(rec + 6);
        const uint32_t len = u16(rec + 8);
        const size_t off = str_base + u16(rec + 10);
        if ((name_id != 1 && name_id != 4) || !len || off + len > buf.size()) continue;

        if (plat == 3 && (len & 1u) == 0) {           // UTF-16BE
            std::string& dst = (name_id == 1) ? fam_win : full_win;
            if (!dst.empty()) continue;
            char mb[4];
            for (uint32_t k = 0; k + 1 < len; k += 2) {
                const int n = utf8_encode(uint32_t(buf[off + k]) << 8 | uint32_t(buf[off + k + 1]), mb);
                dst.append(mb, size_t(n));
            }
        } else if (plat == 1 && name_id == 1 && best_mac.empty()) {   // Mac Roman ≈ Latin-1
            best_mac.assign(reinterpret_cast<const char*>(&buf[off]), len);
        }
    }
    if (!fam_win.empty()) return fam_win;          // 家族名优先
    if (!full_win.empty()) return full_win;
    return best_mac;
}

// =============================================================================
//  8.2  像素画布 + CPU 光栅化
//     内存布局：4 字节/像素。Windows / Linux 是 BGRA，macOS 是 RGBA。
// =============================================================================
#ifdef AINO_PLATFORM_MACOS
static const int CH_R = 0, CH_G = 1, CH_B = 2, CH_A = 3;
#else
static const int CH_B = 0, CH_G = 1, CH_R = 2, CH_A = 3;
#endif

// 小端机上"4 字节像素"可以整个当 uint32 写 —— 比逐通道赋值快得多，
// 而且是 BGRA / RGBA 都适用（通道只是字节偏移，不关心谁在前）。
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#  define AINO_BIG_ENDIAN 1
#else
#  define AINO_BIG_ENDIAN 0
#endif

// 把不透明色打包成整像素值（仅在 !AINO_BIG_ENDIAN 时用于快速填充）
inline uint32_t pack_opaque(const Color& c) {
    uint8_t b[4];
    b[CH_R] = c.r; b[CH_G] = c.g; b[CH_B] = c.b; b[CH_A] = 255;
    uint32_t v = 0;
    std::memcpy(&v, b, 4);
    return v;
}

struct Canvas {
    uint8_t* px = nullptr;
    int w = 0, h = 0;
    int stride = 0;                 // 字节
    void*  platform_dc = nullptr;   // Windows: HDC; macOS: CGContextRef; X11: Display*
    void*  platform_gc = nullptr;   // X11: GC

    // 裁剪区（物理像素）。clipped=false 时是整块画布。
    bool clipped = false;
    int clip_x0 = 0, clip_y0 = 0, clip_x1 = 0, clip_y1 = 0;

    inline void set_clip(int x0, int y0, int x1, int y1) {
        clipped = true;
        clip_x0 = clampi(x0, 0, w); clip_y0 = clampi(y0, 0, h);
        clip_x1 = clampi(x1, 0, w); clip_y1 = clampi(y1, 0, h);
    }
    inline void clear_clip() { clipped = false; }

    inline void blend(int x, int y, const Color& c, float coverage) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        if (clipped && (x < clip_x0 || x >= clip_x1 || y < clip_y0 || y >= clip_y1)) return;
        float sa = (float(c.a) * (1.0f / 255.0f)) * coverage;
        if (sa <= 0.0015f) return;
        uint8_t* p = px + size_t(y) * size_t(stride) + size_t(x) * 4u;
        if (sa >= 0.9985f) {
            p[CH_R] = c.r; p[CH_G] = c.g; p[CH_B] = c.b; p[CH_A] = 255;
            return;
        }
        float inv = 1.0f - sa;
        p[CH_R] = uint8_t(c.r * sa + p[CH_R] * inv + 0.5f);
        p[CH_G] = uint8_t(c.g * sa + p[CH_G] * inv + 0.5f);
        p[CH_B] = uint8_t(c.b * sa + p[CH_B] * inv + 0.5f);
        p[CH_A] = 255;   // 不透明窗口：alpha 恒为 255，避免与 GDI 文字合成时出问题
    }

    // 不透明色整行快填（尊重裁剪区）。常用于圆角矩形的实心内区。
    inline void fill_row_fast(int y, int x0, int x1, const Color& c) {
        if (y < 0 || y >= h) return;
        if (clipped) {
            if (y < clip_y0 || y >= clip_y1) return;
            if (x0 < clip_x0) x0 = clip_x0;
            if (x1 > clip_x1) x1 = clip_x1;
        }
        if (x0 < 0) x0 = 0;
        if (x1 > w) x1 = w;
        if (x1 <= x0) return;
        uint8_t* base = px + size_t(y) * size_t(stride);
        if (!AINO_BIG_ENDIAN) {
            uint32_t* p = reinterpret_cast<uint32_t*>(base) + size_t(x0);
            const uint32_t v = pack_opaque(c);
            for (int i = x1 - x0; i > 0; --i, ++p) *p = v;
            return;
        }
        uint8_t* p = base + size_t(x0) * 4u;
        for (int x = x0; x < x1; ++x, p += 4) {
            p[CH_R] = c.r; p[CH_G] = c.g; p[CH_B] = c.b; p[CH_A] = 255;
        }
    }

    // 半透明整行填充。
    //
    // 行裁剪、边界判断、颜色量化全部只做一次，循环里只剩三次整数乘加。
    // 为什么值得单独写：圆角矩形/圆形的"实心内区"对半透明色会落在这条路径上，
    // 而 Group Box 的 2% 底、代码块底、侧边栏材质都是几十万像素的大面积半透明 ——
    // 逐像素调 blend() 时那些判断和浮点换算几乎占掉一半时间。
    //
    // 用 0..256 的定点 alpha（而不是 0..255）是为了让"全覆盖"能整除干净：
    // 反色分量乘 (256-sa)，结果右移 8 位正好还原，不需要再判一次 sa==255。
    inline void blend_row(int y, int x0, int x1, const Color& c, float coverage = 1.0f) {
        if (y < 0 || y >= h) return;
        if (clipped) {
            if (y < clip_y0 || y >= clip_y1) return;
            if (x0 < clip_x0) x0 = clip_x0;
            if (x1 > clip_x1) x1 = clip_x1;
        }
        if (x0 < 0) x0 = 0;
        if (x1 > w) x1 = w;
        if (x1 <= x0) return;
        int sa = int(float(c.a) * coverage * (256.0f / 255.0f) + 0.5f);
        if (sa <= 0) return;
        if (sa >= 256) { fill_row_fast(y, x0, x1, c); return; }
        const int inv = 256 - sa;
        const int sr = int(c.r) * sa, sg = int(c.g) * sa, sb = int(c.b) * sa;
        uint8_t* p = px + size_t(y) * size_t(stride) + size_t(x0) * 4u;
        for (int i = x1 - x0; i > 0; --i, p += 4) {
            p[CH_R] = uint8_t((sr + p[CH_R] * inv + 128) >> 8);
            p[CH_G] = uint8_t((sg + p[CH_G] * inv + 128) >> 8);
            p[CH_B] = uint8_t((sb + p[CH_B] * inv + 128) >> 8);
            p[CH_A] = 255;
        }
    }

    // 覆盖率恒为 1 的整行填充。只有不透明色才能走 memcpy 式的快填 ——
    // 半透明色覆盖率 1 也还是"混合"，必须逐像素 blend，否则会把颜色写实。
    inline void fill_row_opaque(int y, int x0, int x1, const Color& c) {
        if (c.a == 255) { fill_row_fast(y, x0, x1, c); return; }
        blend_row(y, x0, x1, c, 1.0f);
    }

    // 预乘了覆盖率的掩膜整行合成（图标：8 位；投影：16 位）。
    //
    // 存在的理由：图标与投影的覆盖率是**预先算好的常量**，每帧只需要"把掩膜贴上去"。
    // 走 blend() 的话每个像素都要重判边界与裁剪区 —— 一个 24×24 图标 576 次判断，
    // 一块弹窗投影几十万次。这里由调用方保证 [x0,x1) 已经裁到画布与裁剪区内，
    // 循环里只剩混合本身；算式与 blend() 逐项一致，输出逐像素相同。
    inline void blend_mask_row8(int y, int x0, int x1, const Color& c, const uint8_t* a) {
        if (x1 <= x0) return;
        uint8_t* p = px + size_t(y) * size_t(stride) + size_t(x0) * 4u;
        // 与 blend() 同序计算：sa = (c.a/255) * (alpha/255)，不要先合并常数，
        // 否则浮点结合顺序变了，个别像素会差 1。
        const float ca = float(c.a) * (1.0f / 255.0f);
        for (int i = x1 - x0; i > 0; --i, p += 4, ++a) {
            const float sa = ca * (float(*a) * (1.0f / 255.0f));
            if (sa <= 0.0015f) continue;
            if (sa >= 0.9985f) { p[CH_R] = c.r; p[CH_G] = c.g; p[CH_B] = c.b; p[CH_A] = 255; continue; }
            const float inv = 1.0f - sa;
            p[CH_R] = uint8_t(c.r * sa + p[CH_R] * inv + 0.5f);
            p[CH_G] = uint8_t(c.g * sa + p[CH_G] * inv + 0.5f);
            p[CH_B] = uint8_t(c.b * sa + p[CH_B] * inv + 0.5f);
            p[CH_A] = 255;
        }
    }
    inline void blend_mask_row16(int y, int x0, int x1, const Color& c, const uint16_t* a) {
        if (x1 <= x0) return;
        uint8_t* p = px + size_t(y) * size_t(stride) + size_t(x0) * 4u;
        const float ca = float(c.a) * (1.0f / 255.0f);
        for (int i = x1 - x0; i > 0; --i, p += 4, ++a) {
            const float sa = ca * (float(*a) * (1.0f / 65535.0f));
            if (sa <= 0.0015f) continue;
            if (sa >= 0.9985f) { p[CH_R] = c.r; p[CH_G] = c.g; p[CH_B] = c.b; p[CH_A] = 255; continue; }
            const float inv = 1.0f - sa;
            p[CH_R] = uint8_t(c.r * sa + p[CH_R] * inv + 0.5f);
            p[CH_G] = uint8_t(c.g * sa + p[CH_G] * inv + 0.5f);
            p[CH_B] = uint8_t(c.b * sa + p[CH_B] * inv + 0.5f);
            p[CH_A] = 255;
        }
    }

    void clear(const Color& c) {
        if (w <= 0 || h <= 0 || !px) return;
        // 先铺满第 0 行的 w 个像素，其余行整块拷贝 —— 3 MB 的 memcpy 比
        // 逐像素循环快一个量级（1080×700 从 ~3 ms 降到 ~0.2 ms）。
        uint8_t* row0 = px;
        if (!AINO_BIG_ENDIAN) {
            uint32_t* p = reinterpret_cast<uint32_t*>(row0);
            const uint32_t v = pack_opaque(c);
            for (int x = 0; x < w; ++x) p[x] = v;
        } else {
            for (int x = 0; x < w; ++x) {
                row0[size_t(x) * 4u + CH_R] = c.r; row0[size_t(x) * 4u + CH_G] = c.g;
                row0[size_t(x) * 4u + CH_B] = c.b; row0[size_t(x) * 4u + CH_A] = 255;
            }
        }
        const size_t row_bytes = size_t(w) * 4u;
        for (int y = 1; y < h; ++y)
            std::memcpy(px + size_t(y) * size_t(stride), row0, row_bytes);
    }};

// --- 圆角矩形在给定 y 处的横跨区间 -------------------------------------------
struct Span { float xl, xr; };

// -----------------------------------------------------------------------------
// 连续圆角（Apple continuous corner / Figma cornerSmoothing）
//
// 普通圆角是四段 1/4 圆弧直接接直线，曲率在切点处从 0 跳到 1/R —— 眼睛看得出来"发尖"，
// 这也是"Web 味"最主要的来源。Apple 的圆角是**曲率连续**的：拐角由
//     三次贝塞尔 → 圆弧 → 三次贝塞尔
// 三段拼成，切点落在 (1+s)·R 处，圆弧只占 90(1−s) 度。macOS 控件一律 s = 0.6。
//
// 下面的常数与 Figma 的 figma-squircle 实现逐项对齐（已用 R=10,s=0.6 的参考资料路径校验）：
//     p  = (1+s)R                       切点离拐角的距离
//     a  = 2b,  b = (p − 弧长 − c − d)/3
//     c  = R·tan(α/2)·cos(β),  d = c·tan(β),   α = β = 45s°
//     弧长 = sin(45(1−s)°)·R·√2
// 局部坐标：拐角在原点，u 沿水平边向里为负、v 沿竖直边向里为正。
// -----------------------------------------------------------------------------
struct CornerGeo {
    float R = 0, s = 0;
    float p = 0;        // 拐角带总高
    float a = 0, b = 0, c = 0, d = 0;
    float v1 = 0, v2 = 0;        // 圆弧起止的 v
    float p1x = 0, p1y = 0;      // 第一段贝塞尔终点
    float p2x = 0, p2y = 0;      // 圆弧终点
    bool  ok = false;
};

inline CornerGeo corner_geo(float R, float s) {
    CornerGeo g;
    if (R <= 0.001f || s <= 0.001f) return g;
    const float D2R = 3.14159265358979f / 180.0f;
    g.R = R; g.s = s;
    const float p      = (1.0f + s) * R;
    const float am     = 90.0f * (1.0f - s) * D2R;          // 圆弧跨度
    const float arcLen = std::sin(am * 0.5f) * R * 1.41421356f;
    const float alpha  = 45.0f * s * D2R;
    const float beta   = alpha;
    const float p3p4   = R * std::tan(alpha * 0.5f);
    const float c      = p3p4 * std::cos(beta);
    const float d      = c * std::tan(beta);
    const float b      = (p - arcLen - c - d) / 3.0f;
    if (!(b >= 0.0f) || !(c >= 0.0f) || !(d >= 0.0f)) return g;   // 数值退化就退回圆弧
    g.p = p; g.a = 2.0f * b; g.b = b; g.c = c; g.d = d;
    const float th1 = -(45.0f + 90.0f * (1.0f - s) * 0.5f) * D2R;
    const float th2 = -(45.0f - 90.0f * (1.0f - s) * 0.5f) * D2R;
    g.v1 = R + R * std::sin(th1);
    g.v2 = R + R * std::sin(th2);
    g.p1x = -R + R * std::cos(th1); g.p1y = g.v1;
    g.p2x = -R + R * std::cos(th2); g.p2y = g.v2;
    g.ok = true;
    return g;
}

// 距顶部切边 v 处，边界离竖直切边的水平内缩量
inline float corner_inset(float v, const CornerGeo& g) {
    if (v <= 0.0f) return g.p;
    if (v >= g.p)  return 0.0f;
    if (v <= g.v1) {
        // 第一段：三个控制点 y 都是 0 → y(t) = p1y·t³，可以直接开立方求 t
        const float t = std::cbrt(std::max(v / std::max(g.p1y, 1e-6f), 0.0f));
        const float mt = 1.0f - t;
        const float x = mt * mt * mt * (-g.p)
                      + 3.0f * mt * mt * t * (-g.p + g.a)
                      + 3.0f * mt * t * t * (-g.p + g.a + g.b)
                      + t * t * t * g.p1x;
        return -x;
    }
    if (v <= g.v2) {
        const float dy = v - g.R;
        return g.R - std::sqrt(std::max(0.0f, g.R * g.R - dy * dy));
    }
    // 第三段：y(t) 单调递增，二分 14 次足够（亚像素）
    const float cy1 = g.p2y + g.c;
    const float cy2 = g.p2y + g.c + g.b;
    float lo = 0.0f, hi = 1.0f;
    for (int i = 0; i < 14; ++i) {
        const float t = (lo + hi) * 0.5f, mt = 1.0f - t;
        const float y = mt * mt * mt * g.p2y
                      + 3.0f * mt * mt * t * cy1
                      + 3.0f * mt * t * t * cy2
                      + t * t * t * g.p;
        if (y < v) lo = t; else hi = t;
    }
    const float t = (lo + hi) * 0.5f, mt = 1.0f - t;
    const float x = mt * mt * mt * g.p2x
                  + 3.0f * mt * mt * t * (g.p2x + g.d)
                  + 3.0f * mt * t * t * (g.p2x + g.d)
                  + t * t * t * 0.0f;
    return -x;
}

// 预计算好的圆角矩形几何（每帧每个矩形算一次，不要放进扫描线循环）
struct RRectGeo {
    float x0 = 0, x1 = 0, y0 = 0, y1 = 0;
    float rad = 0, p = 0;
    CornerGeo cg;
    bool smooth = false;
};

inline RRectGeo make_rrgeo(const Rect& r, float rad, float smooth) {
    RRectGeo g;
    g.x0 = r.x; g.x1 = r.right(); g.y0 = r.y; g.y1 = r.bottom();
    if (rad <= 0.0f || r.w <= 0.0f || r.h <= 0.0f) { g.rad = 0.0f; return g; }
    g.rad = std::min(rad, std::min(r.w, r.h) * 0.5f);
    if (smooth > 0.001f) {
        // 拐角带不能超过半高：超出了就压低 smoothing（保持半径不变，形状不会突变）
        float s = smooth;
        const float half = std::min(r.w, r.h) * 0.5f;
        if ((1.0f + s) * g.rad > half) s = std::max(0.0f, half / g.rad - 1.0f);
        g.cg = corner_geo(g.rad, s);
        if (g.cg.ok) { g.smooth = true; g.p = g.cg.p; }
    }
    return g;
}

inline Span rr_span(float y, const RRectGeo& g) {
    Span s{g.x0, g.x1};
    if (y < g.y0 || y > g.y1) { s.xl = 1.0f; s.xr = 0.0f; return s; }
    if (g.rad <= 0.0f) return s;
    if (g.smooth) {
        float dx = 0.0f;
        if (y < g.y0 + g.p)              dx = corner_inset(y - g.y0, g.cg);
        else if (y > g.y1 - g.p)         dx = corner_inset(g.y1 - y, g.cg);
        s.xl = g.x0 + dx; s.xr = g.x1 - dx;
        return s;
    }
    const float top_center = g.y0 + g.rad;
    const float bot_center = g.y1 - g.rad;
    if (y < top_center) {
        const float dy = top_center - y;
        const float dx = g.rad - std::sqrt(std::max(0.0f, g.rad * g.rad - dy * dy));
        s.xl = g.x0 + dx; s.xr = g.x1 - dx;
    } else if (y > bot_center) {
        const float dy = y - bot_center;
        const float dx = g.rad - std::sqrt(std::max(0.0f, g.rad * g.rad - dy * dy));
        s.xl = g.x0 + dx; s.xr = g.x1 - dx;
    }
    return s;
}


// 4 倍垂直子采样 × 水平解析覆盖 —— 半径 8px 的圆角能得到干净的边缘
static const int AINO_SS = 4;

// 在 outer 区域铺色，若 inner 非空则挖掉 inner（用于描边）
// 竖直渐变：一列一色，所以整行仍然是同一个颜色 —— 快路径（整行铺色）照样能用。
struct RowPaint {
    Color c0, c1;
    float y0 = 0, y1 = 0;
    bool  grad = false;
    Color at(float py) const {
        if (!grad) return c0;
        const float t = clampf((py + 0.5f - y0) / std::max(y1 - y0, 0.001f), 0.0f, 1.0f);
        return c0.mix(c1, t);
    }
};

inline void raster_rrect_paint(Canvas& cv, const Rect& r, const RowPaint& paint, float rad, float smooth);

inline void raster_rrect(Canvas& cv, const Rect& r, const Color& c, float rad, float smooth = 0.0f) {
    RowPaint rp; rp.c0 = c;
    raster_rrect_paint(cv, r, rp, rad, smooth);
}

inline void raster_rrect_paint(Canvas& cv, const Rect& r, const RowPaint& paint, float rad, float smooth) {
    int y0 = (int)std::floor(r.y), y1 = (int)std::ceil(r.bottom());
    if (y1 > cv.h) y1 = cv.h;
    if (y0 < 0) y0 = 0;
    if (y1 <= y0) return;
    // 半径按短边折半，和 rr_span 内部一致；先算好才能推出"必定被覆盖"的内区
    const RRectGeo geo = make_rrgeo(r, rad, smooth);

    for (int py = y0; py < y1; ++py) {
        if (cv.clipped && (py < cv.clip_y0 || py >= cv.clip_y1)) continue;
        const Color c = paint.at(float(py));
        if (c.a == 0) continue;
        float xls[AINO_SS], xrs[AINO_SS];
        float mn = 1e9f, mx = -1e9f;
        float inl = -1e9f, inr = 1e9f;      // 4 条子采样线都覆盖到的区间
        bool all_inside = true;             // 4 条子采样线是否都落在矩形内
        for (int s = 0; s < AINO_SS; ++s) {
            float sy = float(py) + (float(s) + 0.5f) / float(AINO_SS);
            Span sp = rr_span(sy, geo);
            xls[s] = sp.xl; xrs[s] = sp.xr;
            // rr_span 对"行在矩形外"返回空哨兵 (xl=1, xr=0)。哨兵会把
            // min/max 带偏，所以必须用「是否真落在矩形内」显式判定。
            if (!(sp.xr > sp.xl)) all_inside = false;
            if (sp.xl < mn) mn = sp.xl;
            if (sp.xr > mx) mx = sp.xr;
            if (sp.xl > inl) inl = sp.xl;
            if (sp.xr < inr) inr = sp.xr;
        }
        if (mx <= mn) continue;
        int px0 = std::max(int(std::floor(mn)), 0);
        int px1 = std::min(int(std::ceil(mx)), cv.w);
        if (px1 <= px0) continue;

        // ---- 实心内区：整段覆盖率恒为 1，整行快填 ----
        // 只有在 4 条子采样线都真落在矩形内时，[ceil(inl), floor(inr)) 才必定
        // 被完整覆盖；有任何一条在外（顶/底边缘行）就不能走这条快路径。
        // 这一条几乎吃掉了所有面积（面板/卡片内部、radius=0 的普通矩形全部命中），
        // 只有左右各 1~2 个边缘像素才需要走 4 倍子采样。
        const int ix0 = int(std::ceil(inl));
        const int ix1 = int(std::floor(inr));
        const int fx0 = std::max(ix0, px0);
        const int fx1 = std::min(ix1, px1);
        const bool band = all_inside && (fx1 > fx0);
        if (band) cv.fill_row_opaque(py, fx0, fx1, c);

        // ---- 两端边缘：逐像素 4 倍垂直子采样 × 水平解析覆盖 ----
        // 注意：两段 AA 区间必须互不相交、且合起来正好是 [px0,px1) 减掉实心带。
        auto edge = [&](int x) {
            float cov = 0.0f;
            const float a = float(x), b = float(x + 1);
            for (int s = 0; s < AINO_SS; ++s) {
                float ov = std::min(b, xrs[s]) - std::max(a, xls[s]);
                if (ov > 0.0f) cov += ov;
            }
            cov *= (1.0f / float(AINO_SS));
            if (cov > 0.0005f) cv.blend(x, py, c, clampf(cov, 0.0f, 1.0f));
        };
        const int ax1 = band ? fx0 : px1;      // 左段 [px0, ax1)
        for (int x = px0; x < ax1; ++x) edge(x);
        if (band) for (int x = fx1; x < px1; ++x) edge(x);
    }
}

// 描边：outer 覆盖 - inner 覆盖
// 同样有"恒 1 区"快路径：左右两条竖边里，coverage 恒为 1（外框整覆盖、内框不沾边），
// 一整条 1px 边框按逐像素 4 倍子采样跑会非常贵 —— 大面板的边框就是靠这条吃掉的。
// 注意 rr_span 对"行在矩形外"返回空哨兵 (xl=1, xr=0)，直接用 min/max 会被带偏
// （内框在某行为空时 inr 会变成 0，右竖边下界退化成 0 → 整行被当成边框填满），
// 所以下面一律按「有效样本」分别求边界。
inline void raster_rrect_stroke(Canvas& cv, const Rect& r, const Color& c, float rad, float width, float smooth = 0.0f) {
    float w = std::max(width, 0.5f);
    Rect inner = r.inset(w);
    const RRectGeo go = make_rrgeo(r, rad, smooth);
    const RRectGeo gi = make_rrgeo(inner, std::max(0.0f, go.rad - w), smooth);
    const bool has_inner = inner.w > 0.0f && inner.h > 0.0f;
    int y0 = (int)std::floor(r.y), y1 = (int)std::ceil(r.bottom());
    if (y1 > cv.h) y1 = cv.h;
    if (y0 < 0) y0 = 0;
    if (y1 <= y0) return;

    for (int py = y0; py < y1; ++py) {
        if (cv.clipped && (py < cv.clip_y0 || py >= cv.clip_y1)) continue;
        float ol[AINO_SS], orr[AINO_SS], il[AINO_SS], irr[AINO_SS];
        float mn = 1e9f, mx = -1e9f;
        bool outer_all = true;          // 4 条子采样线都落在外框内
        bool inner_all = has_inner;     // 4 条子采样线都落在内框内
        float oin_l = -1e9f, oin_r = 1e9f;   // 外框：4 线都覆盖到的区间
        float iin_l = -1e9f, iin_r = 1e9f;   // 内框：4 线都覆盖到的区间
        float inl_min = 1e9f, inr_max = -1e9f;   // 内框有效样本的 min 左 / max 右
        for (int s = 0; s < AINO_SS; ++s) {
            float sy = float(py) + (float(s) + 0.5f) / float(AINO_SS);
            Span so = rr_span(sy, go);
            const bool ok = (so.xr > so.xl);
            ol[s] = so.xl; orr[s] = so.xr;
            if (!ok) outer_all = false;
            if (so.xl < mn) mn = so.xl;
            if (so.xr > mx) mx = so.xr;
            if (ok) {
                if (so.xl > oin_l) oin_l = so.xl;
                if (so.xr < oin_r) oin_r = so.xr;
            }
            Span si = has_inner ? rr_span(sy, gi) : Span{1.0f, 0.0f};
            const bool iok = has_inner && (si.xr > si.xl);
            il[s] = si.xl; irr[s] = si.xr;
            if (!iok) { inner_all = false; }
            else {
                if (si.xl < inl_min) inl_min = si.xl;
                if (si.xr > inr_max) inr_max = si.xr;
                if (si.xl > iin_l) iin_l = si.xl;
                if (si.xr < iin_r) iin_r = si.xr;
            }
        }
        if (mx <= mn) continue;
        int px0 = std::max(int(std::floor(mn)), 0);
        int px1 = std::min(int(std::ceil(mx)), cv.w);
        if (px1 <= px0) continue;

        // 恒覆盖（cov==1）的整段：[s0a,s1a) 左竖边、[s0b,s1b) 右竖边。
        // 若这一行根本没有有效的内框（内框为空，或描边宽到把内框吃掉了），
        // ci 恒为 0，此时只有"整条外框实心带"是恒 1 的，左右边缘仍要 AA。
        int s0a = 0, s1a = 0, s0b = 0, s1b = 0;
        bool has_a = false, has_b = false;
        if (outer_all) {
            const int oi0 = int(std::ceil(oin_l));      // 外框左边界：右侧 co==1
            const int oi1 = int(std::floor(oin_r));     // 外框右边界：左侧 co==1
            if (inl_min < 1e8f && inr_max > -1e8f) {
                // 左竖边：x+1 <= 内框最小的左边界 → ci==0
                const int ahi = int(std::floor(inl_min - 1.0f)) + 1;
                if (ahi > oi0) { s0a = std::max(px0, oi0); s1a = std::min(ahi, px1); has_a = s1a > s0a; }
                // 右竖边：x >= 内框最大的右边界 → ci==0
                const int blo = int(std::ceil(inr_max));
                if (oi1 > blo) { s0b = std::max(px0, blo); s1b = std::min(oi1, px1); has_b = s1b > s0b; }
            } else if (oi1 > oi0) {
                s0a = std::max(px0, oi0); s1a = std::min(oi1, px1); has_a = s1a > s0a;
            }
        }
        if (has_a) cv.fill_row_opaque(py, s0a, s1a, c);
        if (has_b) cv.fill_row_opaque(py, s0b, s1b, c);

        // 两条竖边之间是"外框内、内框内"→ 覆盖率恒为 0，整段跳过。
        // 只有 4 条子采样线都落在内框内时，ci 才可能恒为 1。
        int z0 = px0, z1 = px0;
        if (outer_all && inner_all) {
            z0 = std::max(px0, std::max(int(std::ceil(oin_l)), int(std::ceil(iin_l))));
            z1 = std::min(px1, std::min(int(std::floor(oin_r)), int(std::floor(iin_r))));
        }

        // 剩下的（左右 1~2 px 边缘 + 圆角带）才走 4 倍子采样；单次线性扫描，不重复混合
        auto edge = [&](int x) {
            float co = 0.0f, ci = 0.0f;
            const float a = float(x), b = float(x + 1);
            for (int s = 0; s < AINO_SS; ++s) {
                float ovo = std::min(b, orr[s]) - std::max(a, ol[s]);
                if (ovo > 0.0f) co += ovo;
                float ovi = std::min(b, irr[s]) - std::max(a, il[s]);
                if (ovi > 0.0f) ci += ovi;
            }
            co *= (1.0f / float(AINO_SS));
            ci *= (1.0f / float(AINO_SS));
            float cov = clampf(co, 0.0f, 1.0f) - clampf(ci, 0.0f, 1.0f);
            if (cov > 0.0005f) cv.blend(x, py, c, cov);
        };
        for (int x = px0; x < px1; ) {
            if (has_a && x >= s0a && x < s1a) { x = s1a; continue; }
            if (has_b && x >= s0b && x < s1b) { x = s1b; continue; }
            if (z1 > z0 && x >= z0 && x < z1) { x = z1; continue; }
            edge(x);
            ++x;
        }
    }
}

// 多边形填充（奇偶规则）—— 任意简单多边形，凹的也行，星形就靠它。
// 做法与 raster_rrect 完全一致：4 倍垂直子采样把"是否在内部"变成覆盖率，
// 每条扫描线先解析求出所有交点，再按奇偶配对成实心段。所以小尺寸下边缘
// 与圆角矩形一样干净，而实现只有 40 行、不引入任何新的数据结构。
inline void raster_poly(Canvas& cv, const std::vector<Vec2>& pts, const Color& c) {
    const size_t n = pts.size();
    if (n < 3 || c.a == 0) return;
    float ymin = pts[0].y, ymax = pts[0].y;
    for (size_t i = 1; i < n; ++i) {
        ymin = std::min(ymin, pts[i].y);
        ymax = std::max(ymax, pts[i].y);
    }
    const int y0 = std::max(0, int(std::floor(ymin)));
    const int y1 = std::min(cv.h, int(std::ceil(ymax)) + 1);
    if (y1 <= y0) return;

    std::vector<float> xs;            // 本子采样行的交点
    std::vector<Vec2>  spans;         // 配对后的实心段 (x=xl, y=xr)
    std::vector<float> cov;           // 本扫描线的水平覆盖累加
    for (int py = y0; py < y1; ++py) {
        spans.clear();
        float mn = 1e9f, mx = -1e9f;
        for (int s = 0; s < AINO_SS; ++s) {
            const float sy = float(py) + (float(s) + 0.5f) / float(AINO_SS);
            xs.clear();
            for (size_t i = 0; i < n; ++i) {
                const Vec2& a = pts[i];
                const Vec2& b = pts[(i + 1) % n];
                // 半开区间判定，避免顶点被重复计数（否则奇偶会翻转错）
                if ((a.y <= sy && b.y > sy) || (b.y <= sy && a.y > sy)) {
                    const float t = (sy - a.y) / (b.y - a.y);
                    xs.push_back(a.x + t * (b.x - a.x));
                }
            }
            if (xs.size() < 2) continue;
            std::sort(xs.begin(), xs.end());
            for (size_t k = 0; k + 1 < xs.size(); k += 2) {
                if (xs[k + 1] <= xs[k]) continue;
                spans.push_back(Vec2(xs[k], xs[k + 1]));
                mn = std::min(mn, xs[k]);
                mx = std::max(mx, xs[k + 1]);
            }
        }
        if (mx <= mn) continue;
        const int px0 = std::max(0, int(std::floor(mn)));
        const int px1 = std::min(cv.w, int(std::ceil(mx)));
        if (px1 <= px0) continue;

        // 恒覆盖带：所有实心段都完整盖住 [x, x+1] 的那一段。
        // 覆盖率是"各段重叠量之和 / 子采样数"，所以被全部段完整覆盖时它恒为
        // max(1, 段数/4) —— 段数 ≥ 4（= 4 条子采样线都贡献了段）时它必然是 1，
        // 于是这一整段可以直接整行快填，不用逐像素累加再混合。
        // 条形图/面积填充的内部几乎全是这一段，是热点所在。
        int bx0 = px0, bx1 = px0;
        if (spans.size() >= size_t(AINO_SS)) {
            float cl = -1e9f, cr = 1e9f;
            for (const Vec2& sp : spans) {
                if (sp.x > cl) cl = sp.x;
                if (sp.y < cr) cr = sp.y;
            }
            if (cr > cl) {
                bx0 = std::max(px0, int(std::ceil(cl)));
                bx1 = std::min(px1, int(std::floor(cr - 1.0f)) + 1);
                if (bx1 > bx0) cv.fill_row_opaque(py, bx0, bx1, c);
            }
        }

        cov.assign(size_t(px1 - px0), 0.0f);
        // 有了快填带就只需要累加带外的像素，省掉绝大部分浮点乘加
        if (bx1 <= bx0) {
            for (const Vec2& sp : spans) {
                for (int x = px0; x < px1; ++x) {
                    const float ov = std::min(float(x + 1), sp.y) - std::max(float(x), sp.x);
                    if (ov > 0.0f) cov[size_t(x - px0)] += ov;
                }
            }
        } else {
            for (const Vec2& sp : spans) {
                for (int x = px0; x < bx0; ++x) {
                    const float ov = std::min(float(x + 1), sp.y) - std::max(float(x), sp.x);
                    if (ov > 0.0f) cov[size_t(x - px0)] += ov;
                }
                for (int x = bx1; x < px1; ++x) {
                    const float ov = std::min(float(x + 1), sp.y) - std::max(float(x), sp.x);
                    if (ov > 0.0f) cov[size_t(x - px0)] += ov;
                }
            }
        }
        const float inv = 1.0f / float(AINO_SS);
        for (int x = px0; x < px1; ) {
            if (bx1 > bx0 && x >= bx0 && x < bx1) { x = bx1; continue; }   // 已整段填过
            const float v = cov[size_t(x - px0)] * inv;
            if (v > 0.0005f) cv.blend(x, py, c, clampf(v, 0.0f, 1.0f));
            ++x;
        }
    }
}

// 生成一个正 n 角星的顶点（外/内半径交替），第一个尖朝正上方。
inline std::vector<Vec2> star_points(float cx, float cy, int spikes, float r_out, float r_in) {
    std::vector<Vec2> pts;
    if (spikes < 3) return pts;
    pts.reserve(size_t(spikes) * 2);
    const float pi  = 3.14159265358979f;
    const float step = pi / float(spikes);
    float ang = -pi * 0.5f;
    for (int i = 0; i < spikes * 2; ++i) {
        const float r = (i % 2 == 0) ? r_out : r_in;
        pts.push_back(Vec2(cx + std::cos(ang) * r, cy + std::sin(ang) * r));
        ang += step;
    }
    return pts;
}

// 圆：距离场近似（1px 过渡带）
inline void raster_circle(Canvas& cv, float cx, float cy, float radius, const Color& c) {
    int y0 = std::max(0, int(std::floor(cy - radius - 1)));
    int y1 = std::min(cv.h, int(std::ceil(cy + radius + 1)));
    for (int y = y0; y < y1; ++y) {
        float dy = (float(y) + 0.5f) - cy;
        float half = std::sqrt(std::max(0.0f, radius * radius - dy * dy));
        int x0 = std::max(0, int(std::floor(cx - half - 1)));
        int x1 = std::min(cv.w, int(std::ceil(cx + half + 1)));
        for (int x = x0; x < x1; ++x) {
            float dx = (float(x) + 0.5f) - cx;
            float d = std::sqrt(dx * dx + dy * dy);
            float cov = clampf(radius + 0.5f - d, 0.0f, 1.0f);
            if (cov > 0.0005f) cv.blend(x, y, c, cov);
        }
    }
}

// 月牙 = 圆 A 减去圆 B。
//
// 为什么单独做一个原语：常见画法是"先画实心圆，再用背景色画一个圆挖掉"——
// 那个"挖"的圆是**不透明的底色**，于是控件一换背景（悬停高亮、别的主题、
// 图片/渐变底）就露出一块错的色块，也就是"穿帮"。这里改成距离场的 CSG 差集：
//
//     dA = |p - A| - rA        （A 内部为负）
//     dB = |p - B| - rB        （B 内部为负）
//     d  = max(dA, -dB)        （A 且 非 B）
//
// 再做 1px 过渡带 → 每个像素只 blend 一次，**完全不需要知道背景色**。
// 代价是一次包围盒扫描（只扫 A 的框），和画一个实心圆一个量级。
inline void raster_crescent(Canvas& cv, float cx, float cy, float radius,
                            float cut_cx, float cut_cy, float cut_radius, const Color& c) {
    if (c.a == 0 || radius <= 0.0f) return;
    int y0 = std::max(0, int(std::floor(cy - radius - 1)));
    int y1 = std::min(cv.h, int(std::ceil(cy + radius + 1)));
    for (int y = y0; y < y1; ++y) {
        float dy = (float(y) + 0.5f) - cy;
        int x0 = std::max(0, int(std::floor(cx - radius - 1)));
        int x1 = std::min(cv.w, int(std::ceil(cx + radius + 1)));
        for (int x = x0; x < x1; ++x) {
            float dx = (float(x) + 0.5f) - cx;
            const float dA = std::sqrt(dx * dx + dy * dy) - radius;
            if (dA > 0.5f) continue;                       // 在 A 外面，省掉第二次开方
            float d = dA;
            if (cut_radius > 0.0f) {
                float kx = (float(x) + 0.5f) - cut_cx;
                float ky = (float(y) + 0.5f) - cut_cy;
                const float dB = std::sqrt(kx * kx + ky * ky) - cut_radius;
                d = std::max(dA, -dB);
            }
            float cov = clampf(0.5f - d, 0.0f, 1.0f);
            if (cov > 0.0005f) cv.blend(x, y, c, cov);
        }
    }
}

// 圆环 / 圆弧：一次遍历环的包围盒，用「径向距离」求覆盖率、「极角」求 alpha，
// 每个像素只 blend 一次 —— 于是根本不存在接缝。
//
// 为什么不用「N 段短线拼一圈」：raster_line 的两端是圆头，相邻段在接缝处相互
// 重叠，同一个像素被 blend 两次、颜色被叠深。段数越多接缝越密，最后看起来
// 就是一节一节的"拼接块"，弧越长越明显。而且 N 段 = N 次独立包围盒扫描，
// 成本也是 N 倍（实测拼 84 段比这版慢 8 倍）。
//
//   a_sweep <= 0  → 整圈（底环）。角度不参与计算，alpha 恒为 c.a。
//   a_sweep >  0  → 弧：从 a_start 起跨 a_sweep 弧度；两端各补一个半圆帽。
//   a_from        → 尾端（t=0）的 alpha 比例。
//   a_fade        → 尾部渐隐占弧长的比例（默认 1 = 整条弧线性渐变）。
//                   取 0.4 就是"尾巴 40% 淡出，前面 60% 恒定满色"——
//                   loading 环要的就是这个：尾巴柔和消失，弧身不再有长渐变带。
inline void raster_arc(Canvas& cv, float cx, float cy, float radius, float width,
                       const Color& c, float a_start, float a_sweep, float a_from,
                       float a_fade = 1.0f) {
    if (c.a == 0 || radius <= 0.0f) return;
    const float hw    = std::max(width, 0.5f) * 0.5f;
    const float reach = radius + hw + 1.0f;
    const int y0 = std::max(0, int(std::floor(cy - reach)));
    const int y1 = std::min(cv.h, int(std::ceil(cy + reach)));
    const int x0 = std::max(0, int(std::floor(cx - reach)));
    const int x1 = std::min(cv.w, int(std::ceil(cx + reach)));
    if (y0 >= y1 || x0 >= x1) return;

    const float TAU   = 6.2831853f;
    const bool  full  = a_sweep <= 0.0f;
    const float sweep = full ? TAU : std::min(a_sweep, TAU);
    float st = std::fmod(a_start, TAU);
    if (st < 0.0f) st += TAU;

    // 弧两端的圆心（补半圆帽用），以及"靠近端点"的角度窗口
    float cap0x = 0.0f, cap0y = 0.0f, cap1x = 0.0f, cap1y = 0.0f;
    float cap_win = 0.0f;
    if (!full) {
        cap0x = cx + std::cos(st) * radius;
        cap0y = cy + std::sin(st) * radius;
        cap1x = cx + std::cos(st + sweep) * radius;
        cap1y = cy + std::sin(st + sweep) * radius;
        cap_win = hw / std::max(radius, 0.001f) + 0.02f;
    }

    for (int y = y0; y < y1; ++y) {
        const float py = (float(y) + 0.5f) - cy;
        for (int x = x0; x < x1; ++x) {
            const float px = (float(x) + 0.5f) - cx;
            const float d  = std::sqrt(px * px + py * py);
            float cov = clampf(hw + 0.5f - std::fabs(d - radius), 0.0f, 1.0f);
            if (cov <= 0.0005f) continue;

            float t = 1.0f;                       // alpha 在弧上的插值位置 0..1
            if (!full) {
                float rel = std::atan2(py, px);
                if (rel < 0.0f) rel += TAU;
                rel -= st;
                if (rel < 0.0f) rel += TAU;
                if (rel <= sweep) {
                    t = rel / sweep;
                    // 端点附近补圆帽：环带被角度切断是平头，加上圆形才能和
                    // 直立的"胶囊"端点看起来一致
                    if (rel <= cap_win || rel >= sweep - cap_win) {
                        const float ax = px - (cap0x - cx), ay = py - (cap0y - cy);
                        const float bx = px - (cap1x - cx), by = py - (cap1y - cy);
                        const float c0 = clampf(hw + 0.5f - std::sqrt(ax * ax + ay * ay), 0.0f, 1.0f);
                        const float c1 = clampf(hw + 0.5f - std::sqrt(bx * bx + by * by), 0.0f, 1.0f);
                        cov = std::max(cov, std::max(c0, c1));
                        if (c1 > c0) t = 1.0f;
                    }
                } else {
                    // 不在角度区间内：只有两端的半圆帽还能覆盖到
                    const float ax = px - (cap0x - cx), ay = py - (cap0y - cy);
                    const float bx = px - (cap1x - cx), by = py - (cap1y - cy);
                    const float c0 = clampf(hw + 0.5f - std::sqrt(ax * ax + ay * ay), 0.0f, 1.0f);
                    const float c1 = clampf(hw + 0.5f - std::sqrt(bx * bx + by * by), 0.0f, 1.0f);
                    if (c0 <= 0.0005f && c1 <= 0.0005f) continue;
                    cov = std::max(c0, c1);
                    t = (c1 > c0) ? 1.0f : 0.0f;
                }
            }

            Color cc = c;
            if (a_from < 1.0f) {
                // 把 t 压到 [0,a_fade] 区间里做插值：a_fade=1 时就是整弧线性，
                // a_fade=0.4 时前 60% 直接饱和到满色，只有尾巴在淡出。
                const float u = (a_fade >= 1.0f)
                              ? clampf(t, 0.0f, 1.0f)
                              : clampf(t / std::max(a_fade, 0.001f), 0.0f, 1.0f);
                cc.a = uint8_t(float(c.a) * (a_from + (1.0f - a_from) * u));
            }
            cv.blend(x, y, cc, cov);
        }
    }
}

// 拖尾环要逐像素求极角 + 余弦。直接调 atan2/cos 的话，128px 的 spinner 单帧
// 要 785µs —— 全部耗在这两个三角函数上（环带约 6000 像素 × 130ns）。
// 这两张表把单帧压到 ~230µs：
//   atan_  → atan(i/256)，i=0..256，0..π/4
//   cos_   → cos(i/1024 * 2π)，i=0..1024
// atan 用八象限折叠（|x|,|y| 先折到第一象限），误差 < 0.002 rad ≈ 0.12px。
struct RingLut {
    float atan_[257];
    float cos_[1025];
    RingLut() {
        for (int i = 0; i <= 256; ++i)  atan_[i] = std::atan(float(i) * (1.0f / 256.0f));
        for (int i = 0; i <= 1024; ++i) cos_[i]  = std::cos(float(i) * (6.2831853f / 1024.0f));
    }
};
inline const RingLut& ring_lut() { static const RingLut t; return t; }

// 整圈「拖尾环」：**没有端点**，亮度沿角度按周期函数循环。
//
// 为什么 spinner 不能用带端点的弧：端点无论做成圆头、平头还是渐隐，都是一个
// "断口"。转到起点时尾端重新长出来，视觉上就是断一下；圆头还会在环上留下
// 一个明显的"头"，看起来像两个圆截出来的月牙。
// 连续几何的正确解法是**不画端点**：整圈都画，用沿角度连续分布的 alpha 表示
// 方向。COS 型分布是周期的 —— 最亮处与最暗处首尾相接且数值相等，所以绕多少圈
// 都不会出现接缝或断口。
//
//   a_phase  最亮处所在的极角
//   a_min    最暗处的 alpha 比例。给 0 会让环在那一段彻底消失（看起来像缺口），
//            所以默认留一点底，环始终是完整的一圈。
inline void raster_ring_tail(Canvas& cv, float cx, float cy, float radius, float width,
                             const Color& c, float a_phase, float a_min) {
    if (c.a == 0 || radius <= 0.0f) return;
    const float hw    = std::max(width, 0.5f) * 0.5f;
    const float reach = radius + hw + 1.0f;
    const int y0 = std::max(0, int(std::floor(cy - reach)));
    const int y1 = std::min(cv.h, int(std::ceil(cy + reach)));
    const int x0 = std::max(0, int(std::floor(cx - reach)));
    const int x1 = std::min(cv.w, int(std::ceil(cx + reach)));
    if (y0 >= y1 || x0 >= x1) return;

    const float TAU = 6.2831853f;
    // Bayer 4x4：alpha 是 8bit 的，连续的渐变会被量化成一条条等值线（马赫带），
    // 小尺寸下看着就像"拼接的色块"。加 ±0.5 级的抖动把台阶打散成噪点 ——
    // 只在这条渐变路径上抖，不动 Canvas::blend 的通用路径。
    static const int bayer[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    const RingLut& lut = ring_lut();

    for (int y = y0; y < y1; ++y) {
        const float py = (float(y) + 0.5f) - cy;
        for (int x = x0; x < x1; ++x) {
            const float px = (float(x) + 0.5f) - cx;
            const float d  = std::sqrt(px * px + py * py);
            const float cov = clampf(hw + 0.5f - std::fabs(d - radius), 0.0f, 1.0f);
            if (cov <= 0.0005f) continue;

            // 极角：八象限折叠后查表（见 RingLut 的注释），不调 atan2
            const float ax = std::fabs(px), ay = std::fabs(py);
            float a;
            if (ax >= ay) a = lut.atan_[int(ay / (ax + 1e-9f) * 256.0f)];
            else          a = 1.5707963f - lut.atan_[int(ax / (ay + 1e-9f) * 256.0f)];
            if (px < 0.0f) a = 3.14159265f - a;
            if (py < 0.0f) a = 6.2831853f - a;

            float rel = a - a_phase;
            if (rel < 0.0f) rel += TAU;
            int ci = int(rel * (1024.0f / TAU));
            if (ci > 1024) ci = 1024;
            const float cs = 0.5f + 0.5f * lut.cos_[ci];     // 1 在头，0 在正对面
            const float a01 = cs * cs;                       // 压一下，让亮区更集中
            const float af  = a_min + (1.0f - a_min) * a01;

            const int dith = bayer[((y & 3) << 2) | (x & 3)];
            Color cc = c;
            cc.a = uint8_t(clampf(af * float(c.a) + (float(dith) * 0.0625f - 0.5f), 0.0f, 255.0f));
            cv.blend(x, y, cc, cov);
        }
    }
}

// 线段：距离场 + 圆头，任意宽度都抗锯齿
inline void raster_line(Canvas& cv, float x0, float y0, float x1, float y1, const Color& c, float width) {
    float hw = std::max(width, 0.5f) * 0.5f;
    float ex = hw + 0.5f;
    int bx0 = std::max(0, int(std::floor(std::min(x0, x1) - ex)));
    int bx1 = std::min(cv.w, int(std::ceil(std::max(x0, x1) + ex)));
    int by0 = std::max(0, int(std::floor(std::min(y0, y1) - ex)));
    int by1 = std::min(cv.h, int(std::ceil(std::max(y0, y1) + ex)));
    float dx = x1 - x0, dy = y1 - y0;
    float len2 = dx * dx + dy * dy;
    for (int y = by0; y < by1; ++y) {
        for (int x = bx0; x < bx1; ++x) {
            float pxc = float(x) + 0.5f, pyc = float(y) + 0.5f;
            float t = len2 > 0.0001f ? clampf(((pxc - x0) * dx + (pyc - y0) * dy) / len2, 0.0f, 1.0f) : 0.0f;
            float qx = x0 + dx * t, qy = y0 + dy * t;
            float d = std::sqrt((pxc - qx) * (pxc - qx) + (pyc - qy) * (pyc - qy));
            float cov = clampf(hw + 0.5f - d, 0.0f, 1.0f);
            if (cov > 0.0005f) cv.blend(x, y, c, cov);
        }
    }
}

// 圆角矩形的"外侧有符号距离"（内部 <= 0，边界 = 0）。
// 这就是 SDF 的标准写法：先把点折到第一象限的"圆心矩形"上，再减半径。
inline float rrect_sdf(float px, float py, const Rect& r, float rad) {
    const float rr = std::min(rad, std::min(r.w, r.h) * 0.5f);
    const float qx = std::fabs(px - r.cx()) - (r.w * 0.5f - rr);
    const float qy = std::fabs(py - r.cy()) - (r.h * 0.5f - rr);
    const float mx = std::max(qx, 0.0f), my = std::max(qy, 0.0f);
    return std::sqrt(mx * mx + my * my) + std::min(std::max(qx, qy), 0.0f) - rr;
}

// 内阴影：沿内边缘向里衰减的一圈暗带。
// macOS 的输入框、复选框、表格底都是"凹"下去的 —— 靠的就是这种内阴影，
// 只描一圈边框是画不出凹陷感的（那只会像 Web 的 input）。
inline void raster_rrect_inner(Canvas& cv, const Rect& r, float rad, float blur,
                               const Color& c, float dy) {
    if (c.a == 0 || blur <= 0.01f) return;
    int y0 = std::max(0, (int)std::floor(r.y)), y1 = std::min(cv.h, (int)std::ceil(r.bottom()));
    int x0 = std::max(0, (int)std::floor(r.x)), x1 = std::min(cv.w, (int)std::ceil(r.right()));
    if (cv.clipped) {
        x0 = std::max(x0, cv.clip_x0); x1 = std::min(x1, cv.clip_x1);
        y0 = std::max(y0, cv.clip_y0); y1 = std::min(y1, cv.clip_y1);
    }
    if (y1 <= y0 || x1 <= x0) return;
    const float inset = std::max(blur, std::min(rad, std::min(r.w, r.h) * 0.5f));
    const int middle0 = clampi(int(std::ceil(r.x + inset - 0.5f)), x0, x1);
    const int middle1 = clampi(int(std::ceil(r.right() - inset - 0.5f)), middle0, x1);
    for (int py = y0; py < y1; ++py) {
        const float sample_y = float(py) + 0.5f - dy;
        const bool skip_middle = sample_y >= r.y + inset && sample_y <= r.bottom() - inset;
        auto draw_range = [&](int first, int last) {
            for (int px = first; px < last; ++px) {
                const float d = -rrect_sdf(float(px) + 0.5f, sample_y, r, rad);
                if (d <= -1.0f) continue;
                const float edge = clampf(d + 1.0f, 0.0f, 1.0f);
                const float t = clampf(1.0f - std::max(d, 0.0f) / blur, 0.0f, 1.0f);
                const float a = t * t * edge;
                if (a > 0.002f) cv.blend(px, py, c, a);
            }
        };
        if (skip_middle) {
            draw_range(x0, middle0);
            draw_range(middle1, x1);
        } else {
            draw_range(x0, x1);
        }
    }
}

// 阴影轮廓边界的覆盖率为 50%；blur 对应高斯 σ = blur/2。使用 logistic 查表近似累积分布，避免逐像素 exp。
//
// 查表放在函数外的 MAGIC STATIC 里：C++11 起它的初始化是线程安全的。
// 早先用手工 `init` 标志，两个线程（画廊的素材窗口就走这条路）同时首次调用会竞争。
struct ShadowCurve {
    float tab[65];
    ShadowCurve() {
        for (int i = 0; i < 65; ++i) {
            const float x = -2.0f + 4.0f * float(i) / 64.0f;   // u ∈ [-2, 2]
            tab[i] = 1.0f / (1.0f + std::exp(3.404f * x));
        }
    }
};
inline const ShadowCurve& shadow_curve() {
    static const ShadowCurve t;
    return t;
}

inline float shadow_cov(float u) {
    const float* tab = shadow_curve().tab;
    const float t = (u * 0.25f + 0.5f) * 64.0f;                // u ∈ [-2,2] → [0,64]
    if (t <= 0.0f) return 1.0f;
    if (t >= 64.0f) return 0.0f;
    const int   i = int(t);
    const float f = t - float(i);
    return tab[i] + (tab[i + 1] - tab[i]) * f;
}

// -----------------------------------------------------------------------------
//  投影掩膜缓存
//
//  掩膜里每个像素只取决于三件事：矩形在包围盒内的**亚像素相位**、矩形宽高、
//  圆角与模糊半径。与颜色、与屏幕位置都无关（距离场是平移不变的）。所以一个
//  弹窗的掩膜从第二帧起可以整块复用 —— 挡住的是逐像素的 sqrt。
//
//  为什么值得做：level2 投影（dialog/sheet/drawer 用 blur 90）在 480×300 上
//  实测 3.92 ms/帧，是整帧最大单项；而它每帧的形状一个像素都没变。
//
//  为什么不存 8 位：软阴影的 alpha 跨度只有 0..≈1，8 位量化在渐变段会让整块
//  阴影出现台阶（马赫带）。16 位把误差压到输出端 1 LSB 以内，实测与直接计算
//  的逐像素最大差 ≤1。
//
//  内存：按 16 位存。480×300 / blur90 的掩膜是 723×543×2 ≈ 785 KiB。
//  单块超过 kMaskCap 就退回逐像素直算（整窗尺寸的投影属于罕见路径，不值得为它
//  常驻几 MB）；总量超过 kBudget 按最久未用淘汰。
// -----------------------------------------------------------------------------
struct ShadowMask {
    int w = 0, h = 0;
    float px = 0, py = 0;           // 矩形在包围盒内的相位（0..1）
    float rw = 0, rh = 0;           // 矩形宽高
    float rad = 0, blur = 0;
    uint64_t stamp = 0;
    std::vector<uint16_t> a;

    bool same_shape(float px_, float py_, float rw_, float rh_, float rad_, float blur_) const {
        return px == px_ && py == py_ && rw == rw_ && rh == rh_ && rad == rad_ && blur == blur_;
    }
};

struct ShadowCache {
    static const size_t kSlots   = 4;
    static const size_t kMaskCap = size_t(2u << 20);    // 单块上限 2 MiB
    static const size_t kBudget  = size_t(4u << 20);    // 总量上限 4 MiB
    std::vector<ShadowMask> slots;
    uint64_t tick = 0;
};

inline ShadowCache& shadow_cache() {
    // 每个渲染线程一份：画廊的素材窗口在独立线程里画自己的帧。
    static thread_local ShadowCache c;
    return c;
}

// 直算路径：掩膜太大不进缓存时走这里（原实现，逐像素求距离场）。
inline void raster_shadow_direct(Canvas& cv, const Rect& s, float rad, float blur, const Color& c) {
    const float inv_blur = 1.0f / blur;
    // 拖尾截断到 ±1.35·blur：该处覆盖率为 1%，肉眼不可见。
    const float ext = blur * 1.35f;
    const int x0 = std::max(0, int(std::floor(s.x - ext)));
    const int x1 = std::min(cv.w, int(std::ceil(s.right() + ext)));
    const int y0 = std::max(0, int(std::floor(s.y - ext)));
    const int y1 = std::min(cv.h, int(std::ceil(s.bottom() + ext)));
    if (x0 >= x1 || y0 >= y1) return;
    for (int y = y0; y < y1; ++y) {
        const float pyc = float(y) + 0.5f;
        for (int x = x0; x < x1; ++x) {
            const float d = rrect_sdf(float(x) + 0.5f, pyc, s, rad);
            const float cov = shadow_cov(d * inv_blur);
            if (cov < 0.004f) continue;
            cv.blend(x, y, c, cov);   // alpha 由 blend 里的 c.a 负责
        }
    }
}

// 阴影：对"外扩 blur"的包围盒逐像素求到圆角矩形边界的有符号距离 d，
// 再按高斯覆盖率曲线取 alpha。距离场一次遍历就够，曲线不随 blur 变味。
inline void raster_shadow(Canvas& cv, const Rect& r, float rad, float blur, const Color& c, float dy) {
    if (blur <= 0.5f || c.a == 0) return;
    const Rect s = r.offset(0.0f, dy);
    const float ext = blur * 1.35f;

    // 掩膜包围盒：不裁到画布 —— 裁了相位就变了，同一块弹窗在窗口边缘会算出
    // 另一张掩膜，缓存直接失效。裁剪留到合成那一步做。
    const int bx0 = int(std::floor(s.x - ext));
    const int by0 = int(std::floor(s.y - ext));
    const int bx1 = int(std::ceil(s.right() + ext));
    const int by1 = int(std::ceil(s.bottom() + ext));
    const int bw = bx1 - bx0, bh = by1 - by0;
    if (bw <= 0 || bh <= 0) return;
    if (bx1 <= 0 || by1 <= 0 || bx0 >= cv.w || by0 >= cv.h) return;   // 完全在画布外

    const float phx = s.x - float(bx0), phy = s.y - float(by0);
    const size_t bytes = size_t(bw) * size_t(bh) * sizeof(uint16_t);
    if (bytes > ShadowCache::kMaskCap) { raster_shadow_direct(cv, s, rad, blur, c); return; }

    ShadowCache& cache = shadow_cache();
    ShadowMask* hit = nullptr;
    for (ShadowMask& m : cache.slots) {
        if (m.w == bw && m.h == bh && m.same_shape(phx, phy, s.w, s.h, rad, blur)) { hit = &m; break; }
    }

    if (!hit) {
        // 腾位置：先按最久未用淘汰，再插入。插入前就把这块算出来 —— 算的时候
        // 不持任何锁，也不看画布。
        size_t used = 0;
        for (const ShadowMask& m : cache.slots) used += m.a.size() * sizeof(uint16_t);
        while (!cache.slots.empty() && (cache.slots.size() >= ShadowCache::kSlots
                                        || used + bytes > ShadowCache::kBudget)) {
            size_t oldest = 0;
            for (size_t i = 1; i < cache.slots.size(); ++i)
                if (cache.slots[i].stamp < cache.slots[oldest].stamp) oldest = i;
            used -= cache.slots[oldest].a.size() * sizeof(uint16_t);
            cache.slots.erase(cache.slots.begin() + long(oldest));
        }
        cache.slots.emplace_back();
        ShadowMask& m = cache.slots.back();
        m.w = bw; m.h = bh;
        m.px = phx; m.py = phy; m.rw = s.w; m.rh = s.h; m.rad = rad; m.blur = blur;
        m.a.resize(size_t(bw) * size_t(bh));
        // 与直算路径逐项相同：像素中心 (bx0+i+0.5, by0+j+0.5) 到圆角矩形的距离。
        const float inv_blur = 1.0f / blur;
        for (int j = 0; j < bh; ++j) {
            const float pyc = float(by0 + j) + 0.5f;
            uint16_t* row = m.a.data() + size_t(j) * size_t(bw);
            for (int i = 0; i < bw; ++i) {
                const float d = rrect_sdf(float(bx0 + i) + 0.5f, pyc, s, rad);
                const float cov = shadow_cov(d * inv_blur);
                row[i] = uint16_t(cov * 65535.0f + 0.5f);
            }
        }
        hit = &m;
    }
    hit->stamp = ++cache.tick;

    // ---- 合成：只把落在画布与裁剪区内的那一段贴上去 ----
    const int cx0 = std::max(bx0, 0), cx1 = std::min(bx1, cv.w);
    const int cy0 = std::max(by0, 0), cy1 = std::min(by1, cv.h);
    for (int y = cy0; y < cy1; ++y) {
        if (cv.clipped && (y < cv.clip_y0 || y >= cv.clip_y1)) continue;
        int x0 = cx0, x1 = cx1;
        if (cv.clipped) {
            if (x0 < cv.clip_x0) x0 = cv.clip_x0;
            if (x1 > cv.clip_x1) x1 = cv.clip_x1;
        }
        if (x1 <= x0) continue;
        const uint16_t* src = hit->a.data() + size_t(y - by0) * size_t(bw) + size_t(x0 - bx0);
        cv.blend_mask_row16(y, x0, x1, c, src);
    }
}

// -----------------------------------------------------------------------------
//  背景模糊与材质近似
// -----------------------------------------------------------------------------
// 做法：降采样 → 三趟可分离盒式模糊（≈ 高斯）→ 双线性放大 + 圆角遮罩写回。
//
// 为什么是盒式而不是"真高斯"：Figma 的 blur 是半径，对应 σ = blur/2，高斯核要
// ±3σ；blur=16 时就是逐像素 48 像素半径的卷积，代价 O(w·h·r)。三趟盒式把代价
// 压到 O(w·h)，以三趟盒式模糊近似高斯分布。
//
// 为什么先降采样：模糊本身就是低通，1/2 或 1/4 分辨率上算完再放大，视觉上
// 没有区别，像素数直接除以 4 / 16。1180×760 的窗口上，一整条 940×300 的玻璃
// 实测 0.7ms（1/4 采样），比一次圆角填充还便宜。
inline void blur_backdrop(Canvas& cv, const Rect& p, float rad, float blur) {
    if (blur < 0.8f || p.w < 2.0f || p.h < 2.0f) return;

    // 采样范围比目标矩形外扩 1.5·blur：边缘像素也要有"外面的内容"可模糊，
    // 否则玻璃边内侧会出现一圈色带（等于把边界 clamp 拉进来了）。
    const float ext = blur * 1.5f;
    const int rx0 = std::max(0, int(std::floor(p.x)));
    const int ry0 = std::max(0, int(std::floor(p.y)));
    const int rx1 = std::min(cv.w, int(std::ceil(p.right())));
    const int ry1 = std::min(cv.h, int(std::ceil(p.bottom())));
    const int sx0 = std::max(0, int(std::floor(p.x - ext)));
    const int sy0 = std::max(0, int(std::floor(p.y - ext)));
    const int sx1 = std::min(cv.w, int(std::ceil(p.right() + ext)));
    const int sy1 = std::min(cv.h, int(std::ceil(p.bottom() + ext)));
    if (rx1 <= rx0 || ry1 <= ry0 || sx1 <= sx0 || sy1 <= sy0) return;

    const int S = blur >= 10.0f ? 4 : 2;                 // 降采样倍数
    const int bw = (sx1 - sx0 + S - 1) / S;
    const int bh = (sy1 - sy0 + S - 1) / S;
    if (bw < 3 || bh < 3) return;

    // 缓冲复用：这条路径每帧都可能走（玻璃面板），不能每次 malloc。
    // ⚠ 变量名不能叫 small —— Windows 的 rpcndr.h 里有 `#define small char`，
    // 撞上就是一堆"char 前面语法错误"，而且只在包含 windows.h 的编译单元里炸。
    thread_local std::vector<uint8_t> blur_a, blur_b;
    blur_a.assign(size_t(bw) * size_t(bh) * 3u, 0);
    blur_b.assign(size_t(bw) * size_t(bh) * 3u, 0);

    // ---- 1. 均值降采样（SxS 盒就是最便宜的低通）----
    for (int by = 0; by < bh; ++by) {
        const int y0 = sy0 + by * S, y1 = std::min(y0 + S, sy1);
        for (int bx = 0; bx < bw; ++bx) {
            const int x0 = sx0 + bx * S, x1 = std::min(x0 + S, sx1);
            int r = 0, g = 0, b = 0, n = 0;
            for (int y = y0; y < y1; ++y) {
                const uint8_t* row = cv.px + size_t(y) * size_t(cv.stride);
                for (int x = x0; x < x1; ++x) {
                    const uint8_t* q = row + size_t(x) * 4u;
                    r += q[CH_R]; g += q[CH_G]; b += q[CH_B]; ++n;
                }
            }
            if (!n) n = 1;
            uint8_t* o = &blur_a[(size_t(by) * size_t(bw) + size_t(bx)) * 3u];
            o[0] = uint8_t(r / n); o[1] = uint8_t(g / n); o[2] = uint8_t(b / n);
        }
    }

    // ---- 2. 三趟盒式（行列各一趟算一趟）----
    // 盒宽 w = 2r+1 的方差是 (w²−1)/12；三趟叠加后 σ ≈ w/2。要 σ = blur/2，
    // 所以 w ≈ blur（在降采样坐标里再除以 S）。
    int rb = int(std::lround(blur / float(S) * 0.5f));
    rb = std::max(1, std::min(rb, std::min(bw, bh) / 2));
    auto box_h = [&](const uint8_t* src, uint8_t* dst) {
        const int win = 2 * rb + 1;
        for (int y = 0; y < bh; ++y) {
            const uint8_t* s = src + size_t(y) * size_t(bw) * 3u;
            uint8_t* d = dst + size_t(y) * size_t(bw) * 3u;
            int acc[3] = {0, 0, 0};
            for (int k = -rb; k <= rb; ++k) {
                const int x = clampi(k, 0, bw - 1);
                acc[0] += s[x * 3]; acc[1] += s[x * 3 + 1]; acc[2] += s[x * 3 + 2];
            }
            for (int x = 0; x < bw; ++x) {
                d[x * 3]     = uint8_t(acc[0] / win);
                d[x * 3 + 1] = uint8_t(acc[1] / win);
                d[x * 3 + 2] = uint8_t(acc[2] / win);
                const int xa = clampi(x - rb, 0, bw - 1);
                const int xb = clampi(x + rb + 1, 0, bw - 1);
                acc[0] += s[xb * 3]     - s[xa * 3];
                acc[1] += s[xb * 3 + 1] - s[xa * 3 + 1];
                acc[2] += s[xb * 3 + 2] - s[xa * 3 + 2];
            }
        }
    };
    auto box_v = [&](const uint8_t* src, uint8_t* dst) {
        const int win = 2 * rb + 1;
        const size_t rowb = size_t(bw) * 3u;
        for (int x = 0; x < bw; ++x) {
            int acc[3] = {0, 0, 0};
            for (int k = -rb; k <= rb; ++k) {
                const int y = clampi(k, 0, bh - 1);
                const uint8_t* s = src + size_t(y) * rowb + size_t(x) * 3u;
                acc[0] += s[0]; acc[1] += s[1]; acc[2] += s[2];
            }
            for (int y = 0; y < bh; ++y) {
                uint8_t* d = dst + size_t(y) * rowb + size_t(x) * 3u;
                d[0] = uint8_t(acc[0] / win);
                d[1] = uint8_t(acc[1] / win);
                d[2] = uint8_t(acc[2] / win);
                const int ya = clampi(y - rb, 0, bh - 1);
                const int yb = clampi(y + rb + 1, 0, bh - 1);
                const uint8_t* sa = src + size_t(ya) * rowb + size_t(x) * 3u;
                const uint8_t* sb = src + size_t(yb) * rowb + size_t(x) * 3u;
                acc[0] += sb[0] - sa[0];
                acc[1] += sb[1] - sa[1];
                acc[2] += sb[2] - sa[2];
            }
        }
    };
    for (int pass = 0; pass < 3; ++pass) {
        box_h(blur_a.data(), blur_b.data());
        box_v(blur_b.data(), blur_a.data());
    }

    // ---- 3. 双线性放大 + 圆角遮罩写回 ----
    //
    // 这一段是整条路径的热点（每像素一次）。两处优化都值钱：
    //  · 圆角遮罩只在**四个角的方块**里算 SDF（sqrt 很贵）；其余像素必在内部，
    //    覆盖率直接取 1。半径 12 时，95% 以上的像素走的是快路径。
    //  · 双线性用 16.16 定点，整数乘加比浮点快一倍，且没有 float→int 转换开销。
    // 8.8 定点：两级插值各 1/256 精度，肉眼足够；关键是乘积留在 32 位里
    // （16.16 的两段相乘要到 2^48，只能用 64 位乘法，慢一倍）。
    const int step = 256 / S;                   // 输出 1px 在缩小图上的步进
    const int channels[3] = {CH_R, CH_G, CH_B};
    const float frad = rad;
    for (int y = ry0; y < ry1; ++y) {
        if (cv.clipped && (y < cv.clip_y0 || y >= cv.clip_y1)) continue;
        uint8_t* row = cv.px + size_t(y) * size_t(cv.stride);
        const int fy = ((y * 2 + 1) - 2 * sy0) * step / 2 - 128;
        int fy0 = fy >> 8;
        int wy = fy & 0xFF;
        if (fy0 < 0) { fy0 = 0; wy = 0; }
        if (fy0 > bh - 2) { fy0 = bh - 2; wy = 256; }
        const int y0 = fy0, y1 = clampi(fy0 + 1, 0, bh - 1);
        const uint8_t* row0 = &blur_a[size_t(y0) * size_t(bw) * 3u];
        const uint8_t* row1 = &blur_a[size_t(y1) * size_t(bw) * 3u];
        const float yc = float(y) + 0.5f;
        const bool y_flat = (yc >= p.y + frad) && (yc < p.bottom() - frad);
        // 行内也要分两段：中间是一条**不带遮罩**的快路径（cov 恒 1），只有左右
        // 两个圆角方块里的像素才需要 SDF。这样绝大多数像素只剩"取 4 个点 + 插值"。
        const int x_in0 = std::max(rx0, int(std::floor(p.x + frad)));
        const int x_in1 = std::min(rx1, int(std::ceil(p.right() - frad)));
        auto put = [&](int x, float cov) {
            const int fx = ((x * 2 + 1) - 2 * sx0) * step / 2 - 128;
            int fx0 = fx >> 8;
            int wx = fx & 0xFF;
            if (fx0 < 0) { fx0 = 0; wx = 0; }
            if (fx0 > bw - 2) { fx0 = bw - 2; wx = 256; }
            const int x0 = fx0, x1 = clampi(fx0 + 1, 0, bw - 1);
            const int iw = 256 - wx;
            const uint8_t* s00 = row0 + size_t(x0) * 3u;
            const uint8_t* s10 = row0 + size_t(x1) * 3u;
            const uint8_t* s01 = row1 + size_t(x0) * 3u;
            const uint8_t* s11 = row1 + size_t(x1) * 3u;
            uint8_t* q = row + size_t(x) * 4u;
            const int ih = 256 - wy;
            for (int ch = 0; ch < 3; ++ch) {
                const int top = int(s00[ch]) * iw + int(s10[ch]) * wx;
                const int bot = int(s01[ch]) * iw + int(s11[ch]) * wx;
                const int v = (top * ih + bot * wy) >> 16;
                const int channel = channels[ch];
                q[channel] = (cov >= 0.999f)
                            ? uint8_t(v)
                            : uint8_t(float(q[channel]) + (float(v) - float(q[channel])) * cov + 0.5f);
            }
        };
        if (y_flat) {
            for (int x = rx0; x < x_in0; ++x) {            // 左圆角
                const float d = rrect_sdf(float(x) + 0.5f, yc, p, frad);
                if (d <= 0.75f) put(x, clampf(0.5f - d, 0.0f, 1.0f));
            }
            for (int x = x_in0; x < x_in1; ++x) put(x, 1.0f);   // 直边中段
            for (int x = x_in1; x < rx1; ++x) {            // 右圆角
                const float d = rrect_sdf(float(x) + 0.5f, yc, p, frad);
                if (d <= 0.75f) put(x, clampf(0.5f - d, 0.0f, 1.0f));
            }
        } else {
            for (int x = rx0; x < rx1; ++x) {
                const float d = rrect_sdf(float(x) + 0.5f, yc, p, frad);
                if (d <= 0.75f) put(x, clampf(0.5f - d, 0.0f, 1.0f));
            }
        }
    }
}

// =============================================================================
//  8.3  文字渲染接口
//    几何图形我们自绘（要抗锯齿），但文字必须交给系统 —— 只有系统字体引擎
//    才能正确覆盖中日韩、阿拉伯文的字形与回退链，且不增加二进制体积。
// =============================================================================
struct TextRenderer {
    virtual ~TextRenderer() {}
    virtual void  init(float dpi_scale, const std::string& family, const std::string& mono) = 0;
    virtual float measure(const std::string& utf8, float pixel_size, int weight) = 0;
    virtual float ascent(float pixel_size, int weight) = 0;
    virtual float line_height(float pixel_size, int weight) = 0;
    // x / y_top 已是物理像素；y_top 是行盒顶部；clip 为物理像素裁剪区（w<0 表示不裁）
    virtual void  draw(Canvas& cv, const std::string& utf8, float x, float y_top,
                       float pixel_size, int weight, const Color& c, const Rect& clip) = 0;
    // 等宽变体：代码块靠它按列对齐。后端未实现时退化为比例字体 ——
    // 字形仍然正确，只是列不对齐，比缺字好。
    virtual float measure_mono(const std::string& utf8, float pixel_size, int weight) {
        return measure(utf8, pixel_size, weight);
    }
    virtual float ascent_mono(float pixel_size, int weight) {
        return ascent(pixel_size, weight);
    }
    virtual void  draw_mono(Canvas& cv, const std::string& utf8, float x, float y_top,
                            float pixel_size, int weight, const Color& c, const Rect& clip) {
        draw(cv, utf8, x, y_top, pixel_size, weight, c, clip);
    }
    // 旋转文字：(cx, cy) 是**行盒中心**（物理像素），degrees 顺时针为正。
    // 默认实现退化成不旋转的 draw() —— 其它后端没实现旋转时至少不会缺字，
    // 只是位置会落在中心左上角那半边。GDI 后端用 lfEscapement 原生旋转。
    virtual void  draw_rot(Canvas& cv, const std::string& utf8, float cx, float cy,
                           float pixel_size, int weight, const Color& c, const Rect& clip,
                           float degrees) {
        (void)degrees;
        const float w = measure(utf8, pixel_size, weight);
        const float lh = line_height(pixel_size, weight);
        draw(cv, utf8, cx - w * 0.5f, cy - lh * 0.5f, pixel_size, weight, c, clip);
    }
    virtual void caret_positions(const std::string& utf8, float size, int weight, std::vector<float>& out) {
        out.assign(1, 0.0f);
        std::string prefix;
        for (size_t i = 0; i < utf8.size();) {
            uint32_t cp;
            const size_t n = std::min(size_t(utf8_decode_n(utf8.data() + i, utf8.size() - i, &cp)), utf8.size() - i);
            prefix.append(utf8, i, n); i += n;
            out.push_back(measure(prefix, size, weight));
        }
    }
    virtual void  release() = 0;
    virtual void reconfigure(float dpi, const std::string& family, const std::string& mono) {
        release(); init(dpi, family, mono);
    }
};

#ifdef AINO_PLATFORM_WINDOWS
// -----------------------------------------------------------------------------
//  Windows: GDI 文字
// -----------------------------------------------------------------------------
// 复用调用方缓冲，避免每次绘制文字都分配一个 wstring。
inline void to_wide_into(const std::string& s, std::wstring& out) {
    out.clear();
    if (s.empty() || s.size() > size_t(0x7fffffff)) return;
    const int count = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    if (count <= 0) return;
    out.resize(size_t(count));
    if (!MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), out.data(), count)) out.clear();
}

inline std::wstring to_wide(const std::string& s) {
    std::wstring out;
    to_wide_into(s, out);
    return out;
}

// Windows 字体依次尝试 Segoe UI Variable Text、Segoe UI、系统消息字体，中文由 GDI 字体链接回退。
inline bool font_face_installed(const wchar_t* face) {
    LOGFONTW lf{};
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    std::wcsncpy(lf.lfFaceName, face, LF_FACESIZE - 1);
    bool found = false;
    HDC dc = GetDC(nullptr);
    if (dc) {
        EnumFontFamiliesExW(dc, &lf,
            [](const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM p) -> int {
                *reinterpret_cast<bool*>(p) = true;
                return 0;                       // 找到一个就够了
            },
            reinterpret_cast<LPARAM>(&found), 0);
        ReleaseDC(nullptr, dc);
    }
    return found;
}

inline std::string from_wide(const std::wstring& s) {
    if (s.empty()) return std::string();
    int need = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), int(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(size_t(need), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), int(s.size()), &out[0], need, nullptr, nullptr);
    return out;
}

inline std::string default_ui_font_family() {
    static const char* kCandidates[] = {
        "Segoe UI Variable Text",   // Win11 正文
        "Segoe UI Variable",        // Win11 旧命名
        "Segoe UI",                 // Win7+ 一定有
    };
    for (const char* c : kCandidates) {
        const std::wstring w = to_wide(c);
        if (font_face_installed(w.c_str())) return std::string(c);
    }
    // 兜底：问系统它自己的消息字体（简中机器上多半就是 Microsoft YaHei UI）
    NONCLIENTMETRICSW ncm{};
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)
        && ncm.lfMessageFont.lfFaceName[0]) {
        return from_wide(ncm.lfMessageFont.lfFaceName);
    }
    return "Microsoft YaHei UI";
}

class GdiTextRenderer : public TextRenderer {
public:
    void init(float dpi, const std::string& family, const std::string& mono) override {
        dpi_ = dpi <= 0.0f ? 1.0f : dpi;
        family_ = family.empty() ? default_ui_font_family() : family;
        mono_   = mono.empty() ? std::string("Consolas") : mono;
        metric_dc_ = CreateCompatibleDC(nullptr);
        SetBkMode(metric_dc_, TRANSPARENT);
    }
    // 当前 DC 上"已经设置好"的 GDI 状态。文字是每帧调用最多的图元（一页上百段），
    // 每段都做 SelectObject + SetBkMode + SetTextColor 再逐个还原，光这 6 次
    // 往返就占掉每段 30µs 里的一大半 —— 而相邻两段文字 90% 概率用同一个字体
    // 和同一种颜色。记住上次设过什么，相同就跳过。
    //
    // 不还原是有意的：这个 DC 是后备缓冲的内部 DC，Canvas 的图形全是纯内存
    // 光栅（不碰 GDI 状态），所以留给下一段文字复用是安全的。换了 DC（resize
    // 重建 DIB）就整体失效重设。
    HDC      cur_dc_ = nullptr;
    HFONT    cur_font_ = nullptr;
    COLORREF cur_color_ = 0xFFFFFFFF;
    int      cur_bk_ = -1;

    void invalidate_dc_state() { cur_font_ = nullptr; cur_color_ = 0xFFFFFFFF; cur_bk_ = -1; }

    void release() override {
        if (cur_dc_ && GetObjectType(cur_dc_)) SelectObject(cur_dc_, GetStockObject(SYSTEM_FONT));
        cur_dc_ = nullptr;
        invalidate_dc_state();
        if (mask_dc_) { DeleteDC(mask_dc_); mask_dc_ = nullptr; }
        if (mask_bitmap_) { DeleteObject(mask_bitmap_); mask_bitmap_ = nullptr; }
        mask_pixels_ = nullptr; mask_w_ = mask_h_ = 0;
        for (auto& kv : fonts_) if (kv.second) DeleteObject(kv.second);
        fonts_.clear();
        measure_cache_.clear();
        lh_cache_.clear();
        if (metric_dc_) { DeleteDC(metric_dc_); metric_dc_ = nullptr; }
    }

    float measure(const std::string& utf8, float size, int weight) override {
        return measure_impl(utf8, size, weight, false);
    }
    float measure_mono(const std::string& utf8, float size, int weight) override {
        return measure_impl(utf8, size, weight, true);
    }
    void caret_positions(const std::string& utf8, float size, int weight, std::vector<float>& out) override {
        const auto wide = to_wide(utf8);
        std::vector<int> advances(wide.size());
        const auto old = SelectObject(metric_dc_, font(size, weight, 0, false));
        SIZE extent{};
        const bool ok = wide.empty() || GetTextExtentExPointW(metric_dc_, wide.data(), int(wide.size()),
            0, nullptr, advances.data(), &extent);
        SelectObject(metric_dc_, old);
        if (!ok) { TextRenderer::caret_positions(utf8, size, weight, out); return; }
        out.assign(1, 0.0f);
        for (size_t i = 0; i < wide.size(); ++i) {
            if (wide[i] >= 0xd800 && wide[i] <= 0xdbff && i + 1 < wide.size()
                && wide[i + 1] >= 0xdc00 && wide[i + 1] <= 0xdfff) ++i;
            out.push_back(float(advances[i]));
        }
    }
    float measure_impl(const std::string& utf8, float size, int weight, bool mono) {
        if (utf8.empty()) return 0.0f;
        // 同一帧里同一段文字会被反复测量（居中/右对齐/省略号试排），跨帧更是
        // 完全重复。缓存掉这次 GDI 往返，命中率接近 100%。
        const uint64_t fk = font_key(size, weight, 0, mono);
        const uint64_t h = hash_key(fk, utf8);
        auto it = measure_cache_.find(h);
        if (it != measure_cache_.end() && it->second.first == utf8)
            return it->second.second;
        HFONT f = font(size, weight, 0, mono);
        HGDIOBJ old = SelectObject(metric_dc_, f);
        std::wstring& w = wide_scratch();
        to_wide_into(utf8, w);
        SIZE sz{0, 0};
        GetTextExtentPoint32W(metric_dc_, w.c_str(), int(w.size()), &sz);
        SelectObject(metric_dc_, old);
        if (utf8.size() > 4096) return float(sz.cx); // 限制每项字节，条目数预算才有意义。
        // 分批腾出空间，避免整表清空；unordered_map 的遍历顺序不是 LRU。
        if (measure_cache_.size() >= 2048) {
            auto it = measure_cache_.begin();
            for (int i = 0; i < 1024 && it != measure_cache_.end(); ++i) it = measure_cache_.erase(it);
        }
        measure_cache_.emplace(h, std::make_pair(utf8, float(sz.cx)));
        return float(sz.cx);
    }

    float ascent(float size, int weight) override { return ascent_impl(size, weight, false); }
    float ascent_mono(float size, int weight) override { return ascent_impl(size, weight, true); }
    float ascent_impl(float size, int weight, bool mono) {
        TEXTMETRICW tm{};
        HFONT f = font(size, weight, 0, mono);
        HGDIOBJ old = SelectObject(metric_dc_, f);
        GetTextMetricsW(metric_dc_, &tm);
        SelectObject(metric_dc_, old);
        return float(tm.tmAscent);
    }
    float line_height(float size, int weight) override {
        const uint64_t fk = font_key(size, weight, 0, false);
        auto it = lh_cache_.find(fk);
        if (it != lh_cache_.end()) return it->second;
        TEXTMETRICW tm{};
        HFONT f = font(size, weight, 0, false);
        HGDIOBJ old = SelectObject(metric_dc_, f);
        GetTextMetricsW(metric_dc_, &tm);
        SelectObject(metric_dc_, old);
        const float v = float(tm.tmHeight + tm.tmExternalLeading);
        if (lh_cache_.size() >= 256) lh_cache_.erase(lh_cache_.begin());
        lh_cache_[fk] = v;
        return v;
    }

    void draw(Canvas& cv, const std::string& utf8, float x, float y_top,
              float size, int weight, const Color& c, const Rect& clip) override {
        draw_impl(cv, utf8, x, y_top, size, weight, c, clip, false);
    }
    void draw_mono(Canvas& cv, const std::string& utf8, float x, float y_top,
                   float size, int weight, const Color& c, const Rect& clip) override {
        draw_impl(cv, utf8, x, y_top, size, weight, c, clip, true);
    }
    void draw_impl(Canvas& cv, const std::string& utf8, float x, float y_top,
                   float size, int weight, const Color& c, const Rect& clip, bool mono) {
        if (utf8.empty() || c.a == 0) return;
        if (c.a < 255) { draw_alpha(cv, utf8, x, y_top, size, weight, c, clip, mono, 0); return; }
        HDC dc = reinterpret_cast<HDC>(cv.platform_dc);
        if (!dc) return;
        HFONT f = font(size, weight, 0, mono);
        if (dc != cur_dc_) { cur_dc_ = dc; invalidate_dc_state(); }
        if (f != cur_font_) { SelectObject(dc, f); cur_font_ = f; }
        if (cur_bk_ != (int)TRANSPARENT) { SetBkMode(dc, TRANSPARENT); cur_bk_ = (int)TRANSPARENT; }
        const COLORREF cr = RGB(c.r, c.g, c.b);
        if (cr != cur_color_) { SetTextColor(dc, cr); cur_color_ = cr; }

        std::wstring& w = wide_scratch();
        to_wide_into(utf8, w);
        const int tx = int(std::floor(x + 0.5f)), ty = int(std::floor(y_top + 0.5f));
        if (clip.w >= 0.0f) {
            // ETO_CLIPPED 让 GDI 自己裁 —— 省掉 SaveDC/IntersectClipRect/RestoreDC
            // 三次 DC 往返（文字是每帧调用最多的图元，这三次很有感）。
            RECT rc{ int(std::floor(clip.x)), int(std::floor(clip.y)),
                     int(std::ceil(clip.right())), int(std::ceil(clip.bottom())) };
            ExtTextOutW(dc, tx, ty, ETO_CLIPPED, &rc, w.c_str(), UINT(w.size()), nullptr);
        } else {
            ExtTextOutW(dc, tx, ty, 0, nullptr, w.c_str(), UINT(w.size()), nullptr);
        }
        // 状态刻意留着给下一段文字复用（见 cur_* 的注释）。
    }

    // 旋转文字：GDI 用 lfEscapement 原生旋转（比"画到位图再采样"干净得多，
    // 边缘仍是灰度 AA，不会糊）。
    //   · 角度取负：屏幕 y 向下，而 lfEscapement 以 y 向上的逆时针为正，
    //     要让"顺时针为正"成立就得取反；
    //   · ExtTextOut 的参考点是**行盒左上角**，且该点在文字坐标系里定义，
    //     所以先把中心沿旋转后的 x/y 轴各退半宽半高，得到那个角。
    void draw_rot(Canvas& cv, const std::string& utf8, float cx, float cy,
                  float size, int weight, const Color& c, const Rect& clip, float degrees) override {
        if (utf8.empty() || c.a == 0) return;
        HDC dc = reinterpret_cast<HDC>(cv.platform_dc);
        if (!dc) return;
        const float w  = measure(utf8, size, weight);
        const float lh = line_height(size, weight);
        const float rad = degrees * 3.14159265358979f / 180.0f;
        const float cs = std::cos(rad), sn = std::sin(rad);
        const float hw = w * 0.5f, hh = lh * 0.5f;
        const float ax = cx - (hw * cs - hh * sn);
        const float ay = cy - (hw * sn + hh * cs);
        if (c.a < 255) { draw_alpha(cv, utf8, ax, ay, size, weight, c, clip, false, degrees); return; }

        const int esc10 = int(-degrees * 10.0f);
        HGDIOBJ old_font = SelectObject(dc, font(size, weight, esc10));
        int old_mode = SetBkMode(dc, TRANSPARENT);
        COLORREF old_color = SetTextColor(dc, RGB(c.r, c.g, c.b));
        // 旋转路径仍然完整还原：它用的是带 escapement 的临时字体，而且调用稀疏，
        // 缓存收益为零、还把"当前状态"搞脏（这里必须让缓存作废）。
        cur_dc_ = dc; invalidate_dc_state();

        std::wstring& ws = wide_scratch();
        to_wide_into(utf8, ws);
        RECT rc{ int(std::floor(clip.x)), int(std::floor(clip.y)),
                 int(std::ceil(clip.right())), int(std::ceil(clip.bottom())) };
        const UINT flags = clip.w >= 0.0f ? ETO_CLIPPED : 0;
        ExtTextOutW(dc, int(std::floor(ax + 0.5f)), int(std::floor(ay + 0.5f)),
                    flags, clip.w >= 0.0f ? &rc : nullptr, ws.c_str(), UINT(ws.size()), nullptr);

        SetTextColor(dc, old_color);
        SetBkMode(dc, old_mode);
        SelectObject(dc, old_font);
    }

private:
    // GDI 只提供覆盖率；透明文字在 CPU 上与任意背景合成。掩膜分块且复用，
    // 不为每段文字分配原生对象，也不把 RGB 文字直接烘焙成不透明色。
    void draw_alpha(Canvas& cv, const std::string& utf8, float x, float y, float size,
                    int weight, const Color& color, const Rect& clip, bool mono, float degrees) {
        if (!cv.px) return;
        const float width = measure_impl(utf8, size, weight, mono);
        const float height = line_height(size, weight);
        const float radians = degrees * 3.14159265358979f / 180.0f;
        const float cs = std::cos(radians), sn = std::sin(radians);
        float left = x, right = x, top = y, bottom = y;
        for (int i = 1; i < 4; ++i) {
            const float dx = (i & 1) ? width : 0, dy = (i & 2) ? height : 0;
            const float px = x + dx * cs - dy * sn, py = y + dx * sn + dy * cs;
            left = std::min(left, px); right = std::max(right, px);
            top = std::min(top, py); bottom = std::max(bottom, py);
        }
        // 字形可能在行盒外有 overhang；最终裁剪始终尊重调用方与 Canvas。
        const float margin = std::max(2.0f, size);
        Rect visible = Rect(left - margin, top - margin, right - left + 2 * margin,
                            bottom - top + 2 * margin).intersect(Rect(0, 0, float(cv.w), float(cv.h)));
        if (clip.w >= 0) visible = visible.intersect(clip);
        if (cv.clipped) visible = visible.intersect(Rect(float(cv.clip_x0), float(cv.clip_y0),
            float(cv.clip_x1 - cv.clip_x0), float(cv.clip_y1 - cv.clip_y0)));
        if (visible.empty()) return;
        const int x0 = int(std::floor(visible.x)), y0 = int(std::floor(visible.y));
        const int x1 = int(std::ceil(visible.right())), y1 = int(std::ceil(visible.bottom()));
        const int mw = std::min(4096, x1 - x0), mh = std::min(512, y1 - y0);
        if (!mask_dc_ || mask_w_ < mw || mask_h_ < mh) {
            if (mask_dc_) DeleteDC(mask_dc_);
            if (mask_bitmap_) DeleteObject(mask_bitmap_);
            mask_bitmap_ = nullptr; mask_pixels_ = nullptr;
            mask_dc_ = CreateCompatibleDC(nullptr);
            mask_w_ = mw; mask_h_ = mh;
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = mw; info.bmiHeader.biHeight = -mh;
            info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
            if (mask_dc_) mask_bitmap_ = CreateDIBSection(mask_dc_, &info, DIB_RGB_COLORS,
                reinterpret_cast<void**>(&mask_pixels_), nullptr, 0);
            if (mask_bitmap_) SelectObject(mask_dc_, mask_bitmap_);
        }
        if (!mask_dc_ || !mask_bitmap_ || !mask_pixels_) return;
        const HFONT selected = font(size, weight, int(-degrees * 10.0f), mono);
        const HGDIOBJ old_font = SelectObject(mask_dc_, selected);
        detail::ScopeExit restore_font([&] { SelectObject(mask_dc_, old_font); });
        SetTextColor(mask_dc_, RGB(255, 255, 255)); SetBkMode(mask_dc_, TRANSPARENT);
        const std::wstring text = to_wide(utf8);
        GdiFlush();
        for (int ty = y0; ty < y1; ty += mask_h_) for (int tx = x0; tx < x1; tx += mask_w_) {
            std::memset(mask_pixels_, 0, size_t(mask_w_) * size_t(mask_h_) * 4);
            SetViewportOrgEx(mask_dc_, -tx, -ty, nullptr);
            ExtTextOutW(mask_dc_, int(std::floor(x + .5f)), int(std::floor(y + .5f)), 0,
                nullptr, text.c_str(), UINT(text.size()), nullptr);
            GdiFlush();
            const int rows = std::min(mask_h_, y1 - ty), cols = std::min(mask_w_, x1 - tx);
            for (int row = 0; row < rows; ++row) for (int col = 0; col < cols; ++col) {
                const unsigned coverage = mask_pixels_[(size_t(row) * mask_w_ + col) * 4];
                if (coverage) cv.blend(tx + col, ty + row, color, float(coverage) / 255.0f);
            }
        }
    }
    static std::wstring& wide_scratch() {
        thread_local std::wstring b;
        return b;
    }
    static uint64_t hash_key(uint64_t fk, const std::string& s) {
        uint64_t h = 1469598103934665603ull ^ fk;
        for (uint8_t ch : s) { h ^= ch; h *= 1099511628211ull; }
        return h;
    }
    uint64_t font_key(float size, int weight, int esc10 = 0, bool mono = false) const {
        int q = int(size * 4.0f + 0.5f);
        if (q < 4) q = 4;
        return (uint64_t(uint32_t(q)) << 32)
             | (uint64_t(uint32_t(esc10 & 0xFFF)) << 16)      // 旋转角度（1/10 度）
             | (mono ? (uint64_t(1) << 28) : uint64_t(0))     // 等宽标记（28..31 空着）
             | uint32_t(weight < 0 ? 400 : weight);
    }
    HFONT font(float size, int weight, int esc10 = 0, bool mono = false) {
        const uint64_t key = font_key(size, weight, esc10, mono);
        auto it = fonts_.find(key);
        if (it != fonts_.end()) return it->second;
        if (fonts_.size() >= 256) {
            for (auto old = fonts_.begin(); old != fonts_.end(); ++old) {
                if (old->second != cur_font_ && DeleteObject(old->second)) {
                    fonts_.erase(old);
                    break;
                }
            }
        }
        int px = int(std::floor(float(int32_t(key >> 32)) / 4.0f + 0.5f));
        HFONT hf = CreateFontW(-px, 0, esc10, esc10, weight < 0 ? FW_NORMAL : weight,
                               FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                               // 灰度 AA，不是 ClearType。试过 CLEARTYPE_QUALITY：
                               // 在这条"画进 32bpp DIB 再 BitBlt"的路径上，GDI 给出的
                               // 次像素边纹过重 —— 实测 21px 中文的最大通道差 174、
                               // 8.7% 的像素带明显橙蓝彩边，比系统的 ClearType 脏得多。
                               // 纯灰度反而干净。
                               ANTIALIASED_QUALITY,
                               mono ? (FIXED_PITCH | FF_MODERN) : (DEFAULT_PITCH | FF_DONTCARE),
                               to_wide(mono ? mono_ : family_).c_str());
        fonts_[key] = hf;
        return hf;
    }
    HDC metric_dc_ = nullptr;
    HDC mask_dc_ = nullptr;
    HBITMAP mask_bitmap_ = nullptr;
    uint8_t* mask_pixels_ = nullptr;
    int mask_w_ = 0, mask_h_ = 0;
    float dpi_ = 1.0f;
    std::string family_, mono_;
    std::unordered_map<uint64_t, HFONT> fonts_;
    std::unordered_map<uint64_t, std::pair<std::string, float>> measure_cache_;
    std::unordered_map<uint64_t, float> lh_cache_;
};
#endif // AINO_PLATFORM_WINDOWS

} // namespace detail
} // namespace aino

// 从这里开始回到 aino 的顶层：主题、图标、Ui、Window 都是公开 API 层级
namespace aino {

// Canvas / TextRenderer 在 detail 里实现，但它们是可替换的扩展点，所以提上来
using detail::Canvas;
using detail::TextRenderer;
using detail::utf8_codepoints;
using detail::utf8_from_codepoints;
using detail::utf8_count;
using detail::utf8_byte_offset;
using detail::clampf;
using detail::lerpf;

namespace detail {
// 平台后端的入口在下面才定义，这里先声明（Window 构造函数要用）
// 参数是完整的 WindowConfig：后端要读图标、字体、最小/最大尺寸、是否可缩放。
#ifdef AINO_PLATFORM_WINDOWS
void w32_create(Window* self, const WindowConfig& cfg);
void w32_destroy(Window* self);
#elif defined(AINO_PLATFORM_X11)
void x11_create(Window* self, const WindowConfig& cfg);
void x11_destroy(Window* self);
#elif defined(AINO_PLATFORM_WAYLAND)
void wayland_create(Window* self, const WindowConfig& cfg);
void wayland_destroy(Window* self);
#elif defined(AINO_PLATFORM_MACOS)
void cocoa_create(Window* self, const WindowConfig& cfg);
void cocoa_destroy(Window* self);
#endif
} // namespace detail

// =============================================================================
//  8.4  主题与图标
// =============================================================================
namespace detail {
// alert 的内边距：绘制与 alert_height 必须读同一个数，否则折行宽度对不上。
inline constexpr float kAlertPad = 12.0f;
} // namespace detail

Theme Theme::light() {
    Theme t;
    // ---- 标签 ----
    t.label             = Color::rgba(0, 0, 0, 217);         // Labels/Primary      85%
    t.label2            = Color::rgba(0, 0, 0, 128);         // Labels/Secondary    50%
    t.label3            = Color::rgba(0, 0, 0, 64);          // Labels/Tertiary     25%
    t.label4            = Color::rgba(0, 0, 0, 26);          // Labels/Quaternary   10%
    t.label5            = Color::rgba(0, 0, 0, 13);          // Labels/Quinary       5%

    // ---- 叠加填充（Fills - Opaque）----
    t.fill              = Color::rgba(0, 0, 0, 26);          // Primary     10%
    t.fill2             = Color::rgba(0, 0, 0, 20);          // Secondary    8%
    t.fill3             = Color::rgba(0, 0, 0, 13);          // Tertiary     5%
    t.fill4             = Color::rgba(0, 0, 0, 8);           // Quaternary   3%
    t.fill5             = Color::rgba(0, 0, 0, 5);           // Quinary      2%
    t.fill_hover        = Color::rgba(0, 0, 0, 38);          // 悬停 = fill 叠 fill3 (≈14.5%)
    t.fill_press        = Color::rgba(0, 0, 0, 44);          // 按下 = fill 叠 fill2 (≈17%)
    t.track             = Color::rgba(0, 0, 0, 18);          // Progress Bars/Track - Stroke
    t.shadow_color      = Color::rgba(0, 0, 0, 64);

    // ---- 表面 ----
    t.window_bg         = Color::hex(0xFFFFFF);              // Window Background
    t.control_bg        = Color::hex(0xFFFFFF);
    t.sidebar_bg        = Color::hex(0xEAEAEA);              // Materials - Controls/Sidebar
    t.title_bg          = Color::hex(0xEAEAEA);              // Materials - Controls/Title Bar
    t.popover_bg        = Color::hex(0xF6F6F6);              // Materials/Thick
    t.tooltip_bg        = Color::hex(0xF6F6F6);
    t.tooltip_stroke    = Color::rgba(255, 255, 255, 0);
    t.scrollbar         = Color::rgba(0, 0, 0, 128);         // Miscellaneous/Scrollbar
    t.table_sel         = Color::hex(0x0165E2);
    t.table_sel_inactive= Color::rgba(0, 0, 0, 36);
    t.row_alt           = Color::rgba(0, 0, 0, 13);
    t.overlay           = Color::rgba(171, 171, 171, 153);   // Miscellaneous/Alerts/Overlay

    // ---- 强调色（按 macOS 官方界面参考图取值）----
    t.blue   = Color::hex(0x0084F7); t.red    = Color::hex(0xFF383C);
    t.orange = Color::hex(0xFF8D28); t.yellow = Color::hex(0xFFCC00);
    t.green  = Color::hex(0x34C759); t.mint   = Color::hex(0x00C8B3);
    t.teal   = Color::hex(0x00C3D0); t.cyan   = Color::hex(0x00C0E8);
    t.indigo = Color::hex(0x6155F5); t.purple = Color::hex(0xCB30E0);
    t.pink   = Color::hex(0xFF2D55); t.brown  = Color::hex(0xAC7F5E);
    t.gray   = Color::hex(0x8E8E93);
    t.tint              = t.blue;
    t.tint_foreground   = Color::hex(0xFFFFFF);              // Labels/White
    t.separator         = Color::rgba(0, 0, 0, 26);          // Column Separator

    // 分组表面与焦点
    t.card_bg = t.control_bg;
    t.on_red = Color::white();
    t.input_border = Color::rgba(0, 0, 0, 26);
    t.focus_ring = t.tint.with_alpha(128);
    return t;
}

Theme Theme::dark() {
    Theme t;
    // ---- 标签 ----
    t.label             = Color::rgba(255, 255, 255, 217);
    t.label2            = Color::rgba(255, 255, 255, 140);   // 55%
    t.label3            = Color::rgba(255, 255, 255, 64);
    t.label4            = Color::rgba(255, 255, 255, 26);
    t.label5            = Color::rgba(255, 255, 255, 13);

    // ---- 叠加填充 ----
    t.fill              = Color::rgba(255, 255, 255, 26);
    t.fill2             = Color::rgba(255, 255, 255, 20);
    t.fill3             = Color::rgba(255, 255, 255, 13);
    t.fill4             = Color::rgba(255, 255, 255, 8);
    t.fill5             = Color::rgba(255, 255, 255, 5);
    t.fill_hover        = Color::rgba(255, 255, 255, 38);
    t.fill_press        = Color::rgba(255, 255, 255, 44);
    t.track             = Color::rgba(255, 255, 255, 10);
    t.shadow_color      = Color::rgba(0, 0, 0, 150);

    // ---- 表面 ----
    t.window_bg         = Color::hex(0x1E1E1E);
    t.control_bg        = Color::hex(0x282828);
    t.sidebar_bg        = Color::hex(0x1E1E1E);
    t.title_bg          = t.window_bg;
    t.popover_bg        = Color::hex(0x282828);
    t.tooltip_bg        = Color::hex(0x282828);
    t.tooltip_stroke    = Color::rgba(255, 255, 255, 38);    // #FFFFFF26
    t.scrollbar         = Color::rgba(255, 255, 255, 140);
    t.table_sel         = Color::hex(0x0158D2);
    t.table_sel_inactive= Color::rgba(255, 255, 255, 46);
    t.row_alt           = Color::rgba(255, 255, 255, 13);
    t.overlay           = Color::rgba(23, 23, 23, 158);

    // ---- 强调色 ----
    t.blue   = Color::hex(0x0A95FF); t.red    = Color::hex(0xFF4245);
    t.orange = Color::hex(0xFF9230); t.yellow = Color::hex(0xFFD600);
    t.green  = Color::hex(0x30D158); t.mint   = Color::hex(0x00DAC3);
    t.teal   = Color::hex(0x00D2E0); t.cyan   = Color::hex(0x3CD3FE);
    t.indigo = Color::hex(0x6D7CFF); t.purple = Color::hex(0xDB34F2);
    t.pink   = Color::hex(0xFF375F); t.brown  = Color::hex(0xB78A66);
    t.gray   = Color::hex(0x98989F);
    t.tint              = t.blue;
    t.tint_foreground   = Color::hex(0xFFFFFF);
    t.separator         = Color::rgba(255, 255, 255, 26);

    // 分组表面与焦点
    t.card_bg = Color::hex(0x252525);
    t.on_red = Color::white();
    t.input_border = Color::rgba(255, 255, 255, 26);
    t.focus_ring = t.tint.with_alpha(140);
    return t;
}

namespace detail {
// 交互态配色：亮底压暗、暗底提亮 —— 两条路径都保证对比度单调变化
inline Color hover_color(const Color& c) { return c.luminance() > 0.5f ? c.darken(0.07f) : c.lighten(0.14f); }
inline Color press_color(const Color& c) { return c.luminance() > 0.5f ? c.darken(0.13f) : c.lighten(0.24f); }
} // namespace detail

// =============================================================================
//  8.5  Ui 内部状态
// =============================================================================

namespace detail {
// 分项计时的时基。AINO_PROF=1 时才被调用，平时一次 QueryPerformanceCounter 都不发。
inline uint64_t prof_now() {
#if defined(_WIN32)
    static LARGE_INTEGER freq = [] { LARGE_INTEGER f{}; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER c{}; QueryPerformanceCounter(&c);
    return uint64_t(double(c.QuadPart) * 1e6 / double(freq.QuadPart));
#else
    // clock() 是**进程 CPU 时间**，不是墙钟：在 Linux/macOS 上把"这一帧花了
    // 多久"和"等了多久"混在一起，数字没有可比性。用 steady_clock。
    using namespace std::chrono;
    return uint64_t(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
#endif
}

// RAII 打点：函数里有多少个 return 都不用管。
struct ProfTick {
    uint64_t* acc;
    uint32_t* cnt;
    uint64_t  t0;
    bool      live;
    ProfTick(uint64_t* a, uint32_t* c, bool on) : acc(a), cnt(c), t0(0), live(on) {
        if (live) t0 = prof_now();
    }
    ~ProfTick() {
        if (!live || !acc) return;
        *acc += prof_now() - t0;
        ++*cnt;
    }
};
// 以 (拥有者, 控件键) 隔离状态。未绘制无法区分隐藏和删除，因此不按帧数淘汰；
// 只有拥有者明确销毁时释放整组，避免误删离屏输入框的撤销历史或滚动位置。
template<class T>
class WidgetStateTable {
    struct Key {
        uint64_t scope, id;
        bool operator==(const Key& other) const { return scope == other.scope && id == other.id; }
    };
    struct Hash {
        size_t operator()(const Key& key) const {
            return size_t(key.scope ? hash_combine(key.scope, key.id) : key.id);
        }
    };
    std::unordered_map<Key, T, Hash> entries_;
    std::unordered_map<Key, T, Hash> measured_entries_;
    uint64_t scope_ = 0;
    bool measuring_ = false;

public:
    void set_scope(uint64_t scope) { scope_ = scope; }
    // 测量使用写时副本：既不能留下新节点，也不能改变已有滚动、动画和编辑状态。
    void set_measuring(bool on) {
        measuring_ = on;
        if (!on) {
            measured_entries_.clear();
            if (measured_entries_.bucket_count() > 64) decltype(measured_entries_)().swap(measured_entries_);
        }
    }
    T& operator[](uint64_t id) {
        const Key key{scope_, id};
        if (measuring_) {
            auto found = measured_entries_.find(key);
            if (found != measured_entries_.end()) return found->second;
            auto original = entries_.find(key);
            return original == entries_.end() ? measured_entries_.try_emplace(key).first->second
                : measured_entries_.emplace(key, original->second).first->second;
        }
        return entries_.try_emplace(key).first->second;
    }
    T& get_or_insert(uint64_t id, const T& initial) {
        const Key key{scope_, id};
        if (measuring_) {
            if (entries_.find(key) != entries_.end()) return (*this)[id];
            return measured_entries_.try_emplace(key, initial).first->second;
        }
        auto result = entries_.try_emplace(key, initial);
        return result.first->second;
    }
    T* get_if(uint64_t id) {
        if (measuring_) {
            auto found = measured_entries_.find(Key{scope_, id});
            if (found != measured_entries_.end()) return &found->second;
            return entries_.find(Key{scope_, id}) == entries_.end() ? nullptr : &(*this)[id];
        }
        auto it = entries_.find(Key{scope_, id});
        return it == entries_.end() ? nullptr : &it->second;
    }
    const T* peek(uint64_t id) const {
        if (measuring_) {
            auto found = measured_entries_.find(Key{scope_, id});
            if (found != measured_entries_.end()) return &found->second;
        }
        auto it = entries_.find(Key{scope_, id});
        return it == entries_.end() ? nullptr : &it->second;
    }
    size_t erase(uint64_t id) {
        return measuring_ ? measured_entries_.erase(Key{scope_, id}) : entries_.erase(Key{scope_, id});
    }
    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }
    size_t bucket_count() const { return entries_.bucket_count(); }

    template<class Cost, class Age>
    void trim_bytes(size_t budget, uint64_t keep, Cost cost, Age age) {
        if (measuring_) return;
        size_t total = 0;
        for (const auto& entry : entries_) total += cost(entry.second);
        if (total <= budget) return;
        std::vector<std::pair<uint64_t, Key>> candidates;
        candidates.reserve(entries_.size());
        for (const auto& entry : entries_) {
            if (entry.first.scope == scope_ && entry.first.id == keep) continue;
            candidates.push_back({age(entry.second), entry.first});
        }
        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& candidate : candidates) {
            if (total <= budget) break;
            auto found = entries_.find(candidate.second);
            total -= cost(found->second);
            entries_.erase(found);
        }
    }
    template<class Cost> size_t bytes(Cost cost) const {
        size_t total = 0;
        for (const auto& entry : entries_) total += cost(entry.second);
        return total;
    }

    void release_scope(uint64_t scope) {
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->first.scope == scope) it = entries_.erase(it);
            else ++it;
        }
        // erase 只释放节点。大页面关闭后，还要收缩高水位留下的 bucket 数组；
        // 留四倍余量与小表底线，避免普通增删反复触发 rehash。
        if (entries_.bucket_count() > 64 && entries_.size() < entries_.bucket_count() / 4) {
            // 部分标准库的 rehash 不缩容。转移节点到小表后交换，明确释放旧桶，
            // 不复制字符串、撤销历史，也不改变存活节点的值或引用。
            decltype(entries_) compact;
            compact.reserve(entries_.size());
            compact.merge(entries_);
            entries_.swap(compact);
        }
    }
};
} // namespace detail

struct Ui::Impl {
    Theme theme = Theme::light();
    Input input;
    Input masked_input;
    float width = 0.0f, height = 0.0f;
    float dt = 1.0f / 60.0f;
    float scale = 1.0f;
    float clock = 0.0f;
    bool  animating = false;
    bool  frame_open = false;
    Cursor cursor = Cursor::Arrow;
    // AINO_PROF=1 时按图元分项累计耗时，定位"这一帧花在哪"。
    bool     prof_on = false;
    uint64_t pf_fill_us = 0, pf_stroke_us = 0, pf_text_us = 0,
             pf_circle_us = 0, pf_shadow_us = 0, pf_measure_us = 0;
    uint32_t pf_fill_n = 0, pf_stroke_n = 0, pf_text_n = 0,
             pf_circle_n = 0, pf_shadow_n = 0, pf_measure_n = 0;
    // 面积/字符数：用来分"面积大"还是"每像素慢" —— 只有耗时没有面积，
    // 没法判断该优化算法还是该少画。
    uint64_t pf_fill_px = 0, pf_stroke_px = 0, pf_text_ch = 0;

    Canvas* canvas = nullptr;
    TextRenderer* text = nullptr;

    // ---- 布局测量模式（见 Ui::begin_measure）----
    // 置位后图元与命中测试都只把 box 并进 measure_acc，不落地。
    // 两重保险：图元与命中测试在入口判 measure_on；同时 begin_measure 会把
    // canvas 摘成 null —— 绕过图元直接 raster_* 的控件（rating / spinner /
    // sparkline 的 area 填充）只要判了空就不会落地。sparkline 当初就是只靠
    // "每个控件自己记得判 measure_on"漏掉的：它在屏幕左上角留下了一块折线。
    bool measure_on = false;
    bool measure_any = false;
    Rect measure_acc{};
    Canvas* measure_canvas = nullptr;   // begin_measure 期间暂存的画布
    Input measure_input;
    size_t measure_clip_depth = 0;

    void measure_add(const Rect& r) {
        Rect q = r;
        // 只应用测量期间新增的裁剪，外层视口不能裁掉待测的屏幕外内容。
        for (size_t i = measure_clip_depth; i < clip_logic.size(); ++i) {
            const Rect& clip = clip_logic[i];
            const float x0 = std::max(q.x, clip.x), y0 = std::max(q.y, clip.y);
            const float x1 = std::min(q.right(), clip.right()), y1 = std::min(q.bottom(), clip.bottom());
            if (x1 <= x0 || y1 <= y0) return;
            q = Rect(x0, y0, x1 - x0, y1 - y0);
        }
        // 滚动区里的内容只算**视口内**的部分：内层列表画了 20 行、视口只有 164px，
        // 全算进来会把卡片撑成 600px（滚动区的内容天生超出，这正是它存在的理由）。
        if (!scroll_stack.empty()) {
            const Rect& cb = scroll_stack.back().box;
            const float x0 = std::max(q.x, cb.x), y0 = std::max(q.y, cb.y);
            const float x1 = std::min(q.right(), cb.right()), y1 = std::min(q.bottom(), cb.bottom());
            if (x1 <= x0 || y1 <= y0) return;
            q = Rect(x0, y0, x1 - x0, y1 - y0);
        }
        if (q.w <= 0.0f && q.h <= 0.0f) return;
        if (!measure_any) { measure_acc = q; measure_any = true; return; }
        const float ax0 = std::min(measure_acc.x, q.x);
        const float ay0 = std::min(measure_acc.y, q.y);
        const float ax1 = std::max(measure_acc.right(), q.right());
        const float ay1 = std::max(measure_acc.bottom(), q.bottom());
        measure_acc = Rect(ax0, ay0, ax1 - ax0, ay1 - ay0);
    }

    // 立即模式交互状态（跨帧存活）
    uint64_t hot = 0;             // 本帧鼠标悬停的控件
    uint64_t active = 0;          // 正在被按住的控件
    uint64_t active_scope_id = 0;
    uint64_t hot_scope_id = 0;
    // 本帧的按下已经被谁领走了。一帧里可能有多个控件同时包含指针位置
    // （叠放的卡片、滚出视口但还压在页脚下面的列表行），而"按下"这个事件
    // 只属于其中一个 —— 否则一次点击会被画两次、动两个状态。
    bool press_taken = false;
    uint64_t next_hot = 0;
    uint64_t next_hot_scope_id = 0;
    // 仅在悬停目标变化时请求重绘，避免控件内部指针移动产生无效帧。
    Rect next_hot_rect{};
    Rect hot_rect{};
    bool hot_rect_valid = false;
    // 上一帧所有可交互矩形。指针落在空白处移动时，用它确认"确实没东西可 hover"。
    std::vector<Rect> hit_rects;
    std::vector<Rect> hit_rects_next;

    // 指针移到 (x, y) 时，悬停状态会不会变？不会变就不必重绘。
    // 判定只用**上一帧**的信息（布局每帧基本一致，而且真有变化的那一帧本来
    // 就会自己排帧）。
    bool hover_would_change(float x, float y) const {
        if (active != 0 || input.down[0] || input.down[1] || input.down[2])
            return true;                              // 控件或自定义拖动中：位置会影响画面
        if (hot_rect_valid) return !hot_rect.contains(x, y);
        for (const Rect& r : hit_rects)
            if (r.contains(x, y)) return true;        // 从空白进了某个控件
        return false;                                 // 空白 → 空白
    }

    // 有些控件只看指针位置、根本不产生 interact：spinner / skeleton 的悬停动画、
    // tooltip / hover_card 的锚点、split_pane 的分隔条。它们不登记的话，
    // "指针移动不重绘"会连它们一起冻住 —— 鼠标压上去转圈也不转。
    // 登记成"悬停区域"就行：进出这些矩形会排帧，区域内部移动仍然跳过。
    bool next_region_hit = false;
    void hover_region(const Rect& r) {
        if (measure_on || r.w <= 0.0f || r.h <= 0.0f) return;
        // 被裁剪掉的控件没有悬停可言 —— 看不见的地方不该因为鼠标划过就排帧。
        const Rect reg = clip_logic_on ? r.intersect(clip_logic_cur) : r;
        if (reg.empty()) return;
        if (hit_rects_next.size() < 4096) hit_rects_next.push_back(reg);
        if (active == 0 && reg.contains(input.mouse.x, input.mouse.y)) {
            next_hot_rect = reg;
            next_region_hit = true;
            next_hot_scope_id = state_scope_id;
        }
    }
    void* focus = nullptr;        // 当前获得键盘焦点的控件
    uint64_t focus_id = 0;
    uint64_t focus_scope_id = 0;
    // 焦点是**鼠标点出来**的还是键盘走出来的。macOS 只有键盘焦点画焦点环：
    // 点一下按钮就留一圈蓝（Web 的 :focus-visible 之外的默认行为）很有"网页感"，
    // 而文本类控件点进去本来就有环，所以它们照旧画。
    bool focus_by_mouse = false;

    // 文本编辑状态
    int   caret = 0;              // 光标位置（码点索引）
    int   sel_anchor = -1;        // 选区锚点
    float caret_blink = 0.0f;
    bool timed_repaint = false;
    std::chrono::steady_clock::time_point repaint_at;

    void schedule_caret() {
        if (measure_on) return;
        const float phase = std::fmod(caret_blink, 1.06f);
        const int delay = std::max(1, int(std::ceil(((phase < 0.62f ? 0.62f : 1.06f) - phase) * 1000.0f)));
        const auto at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
        if (!timed_repaint || at < repaint_at) repaint_at = at;
        timed_repaint = true;
    }
    int repaint_timeout_ms() const {
        if (!timed_repaint) return -1;
        const auto remaining = repaint_at - std::chrono::steady_clock::now();
        return std::max(0, int(std::chrono::ceil<std::chrono::milliseconds>(remaining).count()));
    }

    // 组合键后短暂抑制字符输入，避免输入法将快捷键同时提交为文本。
    float text_veto = 0.0f;

    // 撤销栈（每个编辑控件一份，按 id 记忆）
    struct EditSnap { std::string text; int caret = 0; int anchor = -1; };
    struct EditStack {
        std::deque<EditSnap> undo, redo;
        double last = -1e9;       // steady_clock 时间，与动画步长无关。
        uint64_t kind = 0;
    };
    detail::WidgetStateTable<EditStack> edit_stack;

    // hover 等状态的插值动画表
    detail::WidgetStateTable<float> anim;
    // 常驻动画的**局部相位**（spinner 的转角、skeleton 的呼吸）。和 clock 的区别：
    // 只在"该动"的时候按 dt 累加，停下就冻结 —— 所以鼠标移开再移回来是
    // "接着动"，而不是跳到全局时钟对应的相位。
    detail::WidgetStateTable<float> phase;
    // 离散的跨帧状态（如 select 是否展开）
    detail::WidgetStateTable<float> flags;
    // 组件私有的字符串状态（目前只有 combobox 的搜索词用）。
    detail::WidgetStateTable<std::string> strings;
    // 多行文本框的切行结果。切行是 O(全文)，而光标闪烁期间每帧都要重画 ——
    // 大文档静止时不该每帧重新切一遍。命中判据是"全文 memcmp 相等"，
    // 所以程序改内容、长度不变也能立刻发现，不会画出上一版的行。
    struct TextLines {
        std::string src;                    // 上一帧的全文，用于校验
        std::vector<std::string> lines;
        std::vector<int> cps_n, cp_base;
        uint64_t used = 0;
        size_t bytes() const {
            size_t total = sizeof(TextLines) + 64 + src.capacity() + lines.capacity() * sizeof(std::string)
                + (cps_n.capacity() + cp_base.capacity()) * sizeof(int);
            for (const auto& line : lines) total += line.capacity() + 1;
            return total;
        }
    };
    detail::WidgetStateTable<TextLines> text_lines;
    uint64_t text_lines_serial = 0;

    // 本帧 interact() 的调用序号。立即模式下"调用顺序 = 绘制顺序 = 层级"，
    // 序号小的先画、在下面，所以它可以直接当 z 序用。每帧归零重数。
    //
    // 帧末的 deferred 弹层是个例外：它们视觉上在最上面，但序号落在**所有**普通
    // 控件之后，光比大小分不清"弹层自己的内容"和"弹层之后画的普通控件"（后者
    // 也在弹层下面，理应被挡住）。所以 end_frame() 起跑 deferred 之前把 seq 抬到
    // Z_FRAME_END，给帧末层划出一段独立的序号区：
    //   普通控件 [1, Z_FRAME_END)      帧末弹层内容 [Z_FRAME_END, ...)
    // 弹层登记闸门时用 Z_FRAME_END 当层级号，于是普通控件"怎么画都"低于弹层，
    // 而弹层内容自己永远在上面。见 gate_claim() 的注释。
    static constexpr uint64_t Z_FRAME_END = 1ull << 40;
    uint64_t seq = 0;

    // 有些弹层的项是"帧内 interact、帧末才画"（combobox / menubar / context_menu：
    // 交互要当帧出结果，绘制排到帧末才盖得住别人）。它们视觉上属于帧末层，
    // 序号却落在普通区 —— 用这个作用域把交互期间的序号临时切到帧末区，
    // 闸门才会把它们当"弹层内容"放行。
    struct SeqFrameEnd {
        Impl* im;
        uint64_t saved;
        explicit SeqFrameEnd(Impl* i) : im(i), saved(i->seq) { im->seq = Z_FRAME_END; }
        ~SeqFrameEnd() { im->seq = saved; }
        SeqFrameEnd(const SeqFrameEnd&) = delete;
        SeqFrameEnd& operator=(const SeqFrameEnd&) = delete;
    };

    // ---- 浮层输入闸门 ----
    // 立即模式的绘制顺序决定视觉层级，交互却按调用顺序处理：最后画的对话框
    // 其实最晚拿到点击，于是点遮罩会连带点到底下的按钮（穿透）。
    // 这里反过来 —— 上一帧登记过的浮层决定本帧谁能接收输入。
    // 浮层可能在普通控件之后调用，需使用上一帧记录提前阻挡输入。
    // 槽位数：一次只能有 8 个"可交互区"被放行。一个浮层通常占 2~3 个
    // （触发器 + 面板 + 面板里的按钮），所以这够 popover + dialog + drawer
    // 同时开着。超出的 claim 会被丢弃 —— 表现为那个区域点不动，不会崩。
    static constexpr int GATE_SLOTS = 8;
    bool gate_on = false;
    bool gate_next = false;
    Rect gate_allow[GATE_SLOTS] = {};
    uint64_t gate_allow_z[GATE_SLOTS] = {};    // 该浮层"第一个属于自己的控件"的序号
    uint64_t gate_allow_scope[GATE_SLOTS] = {};
    int  gate_allow_n = 0;
    Rect gate_next_allow[GATE_SLOTS] = {};
    uint64_t gate_next_allow_z[GATE_SLOTS] = {};
    uint64_t gate_next_allow_scope[GATE_SLOTS] = {};
    int  gate_next_allow_n = 0;

    // 判定 = 位置 + 层级。两条都要，缺一条就漏：
    //   · 落在所有浮层区域之外          → 挡住（遮罩）
    //   · 落在浮层区域内，但控件的 z 比该浮层小 → 挡住（压在面板底下的东西）
    // 第二条是关键：只按位置白名单放行的话，面板几何范围内的一切都算"允许"，
    // 被面板盖住的按钮照样吃 hover / click —— 那还是穿透。
    bool gate_blocked(uint64_t z, float x, float y) const {
        if (!gate_on) return false;
        uint64_t need = 0;
        bool inside = false;
        for (int i = 0; i < gate_allow_n; ++i) {
            if (!gate_allow[i].contains(x, y)) continue;
            inside = true;
            if (gate_allow_z[i] > need) need = gate_allow_z[i];  // 最上层那个说了算
        }
        return !inside || z < need;
    }

    // 给"不产生 interact 的读取"用：tooltip / spinner / skeleton / 滚轮 / 键盘。
    // 取调用点当下的 seq —— 页面控件的 seq 小、浮层自己的内容 seq 大，判断一致。
    bool gate_blocked_here(const Rect& r) const {
        if (!gate_on) return false;
        return gate_blocked(seq, r.cx(), r.cy());
    }
    bool gate_blocked_pointer() const {
        if (!gate_on) return false;
        return gate_blocked(seq, input.mouse.x, input.mouse.y);
    }

    // 浮层在帧内登记"下一帧谁能交互"（触发器、面板、下拉列表都算）。
    //
    // z 的取法分两种，选错了不是漏挡就是自己挡自己：
    //   · 触发器、以及 dialog 这类**当帧直接画**的面板内容 → 传函数入口处的
    //     seq+1。它们和普通控件同在一个序号区，比大小才有意义。
    //   · deferred 到帧末画的面板（select/menu/popover/combobox/context_menu…）
    //     → 传 Z_FRAME_END。面板里的控件在帧末才 interact，序号从这一段起，
    //     用同一个基准比较才能既放行自己、又挡住"排在浮层之后画的普通控件"。
    //     传入口的 seq+1 就漏挡：侧栏先画 select、紧接着画列表项，那些列表项
    //     的序号更大，会被判成"在弹层上面"，点击直接穿到列表项上。
    void gate_claim(const Rect& r, uint64_t z) {
        if (gate_next_allow_n < GATE_SLOTS) {
            gate_next_allow[gate_next_allow_n] = r;
            gate_next_allow_z[gate_next_allow_n] = z;
            gate_next_allow_scope[gate_next_allow_n] = state_scope_id;
            ++gate_next_allow_n;
        }
        gate_next = true;
    }


    // ---- 文本折行缓存 ----
    // 折行是 O(字符数) 的逐字量宽。一段 20 万字的文档，每帧重排一次就是
    // 十几毫秒 —— 于是"大块文本"在立即模式里必然卡。这里把**整段**折行结果
    // 按 (文本地址, 长度, 宽度, 字号, 字重, wrap) 缓存起来，命中就是一次
    // 哈希 + 一次 memcmp。
    //
    // 为什么用地址当键：以内容做键要先把整串哈希一遍（20 万字 ≈ 200µs），
    // 每帧都付不起。地址在调用方的 string 不扩容时是稳定的，配合 entry 里
    // 存的原文本做 memcmp 校验，碰撞/复用地址都不会出错（只是退化成重排）。
    struct WrapEntry {
        std::string text;
        float max_w = 0.0f, pfs = 0.0f;
        int weight = 0;
        bool wrap = false;
        std::vector<std::string> lines;
        std::list<uint64_t>::iterator recency;
        size_t bytes = 0;
    };
    std::unordered_map<uint64_t, WrapEntry> wrap_cache;
    std::list<uint64_t> wrap_lru; // 首部最近使用，尾部可 O(1) 淘汰。
    std::vector<std::string> wrap_scratch; // 超预算结果只保留到下次折行调用。
    size_t   wrap_bytes = 0;
    size_t   wrap_evictions = 0;      // 统计：被淘汰的段数（探针用）

    // 单字宽度缓存：首次折行时每个码点只问一次 GDI。
    std::unordered_map<uint64_t, float> adv_cache;

    struct DeferredCall { uint64_t scope; std::function<void()> fn; };
    std::deque<DeferredCall> defer;

    // 布局栈
    std::vector<Rect> clips;
    // 裁剪区的**逻辑坐标**副本。clips 存的是物理矩形（已经过变换与 DPI 缩放），
    // 而命中测试用的是逻辑坐标，两套对不上，所以并行记一份。
    //
    // 为什么命中必须看裁剪：滚动区把内容裁到视口，可是**裁剪只发生在绘制阶段** ——
    // 列表溢出视口的那些行照样按坐标参与 interact()。gallery 的侧边栏一旦滚到中段，
    // 页脚（主题按钮）那一片下面就压着十几行看不见的导航项，点按钮会被最下面那行
    // 抢走：页面莫名切走，按钮一点反应没有。
    std::vector<Rect> clip_logic;
    Rect clip_logic_cur;
    bool clip_logic_on = false;

    void recompute_clip_logic() {
        if (clip_logic.empty()) { clip_logic_on = false; clip_logic_cur = Rect(); return; }
        Rect c = clip_logic[0];
        for (size_t i = 1; i < clip_logic.size(); ++i) {
            const float x0 = std::max(c.x, clip_logic[i].x);
            const float y0 = std::max(c.y, clip_logic[i].y);
            const float x1 = std::min(c.right(), clip_logic[i].right());
            const float y1 = std::min(c.bottom(), clip_logic[i].bottom());
            c = Rect(x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0));
        }
        clip_logic_cur = c;
        clip_logic_on = true;
    }
    bool clip_excludes(float x, float y) const {
        return clip_logic_on && !clip_logic_cur.contains(x, y);
    }
    // 滚动作用域的嵌套上下文。
    struct ScrollCtx {
        uint64_t id = 0, state_scope_id = 0;
        Rect box;
        float offset = 0.0f, max_offset = 0.0f;
    };
    std::vector<ScrollCtx> scroll_stack;
    // ID 由滚动区内的内容位置生成，屏幕位移不会重置控件状态或串用相邻控件的动画。
    uint64_t widget_id(const char* kind, const Rect& box, uint64_t extra = 0,
                       uint64_t stable = 0) const {
        if (stable) {
            const uint64_t id = detail::hash_combine(detail::hash_string(kind), stable);
            return state_scope_id ? detail::hash_combine(state_scope_id, id) : id;
        }
        if (scroll_stack.empty()) {
            const uint64_t id = detail::make_id(kind, box, extra);
            return state_scope_id ? detail::hash_combine(state_scope_id, id) : id;
        }
        const auto& parent = scroll_stack.back();
        Rect local = box.offset(-parent.box.x, parent.offset - parent.box.y);
        // ID 使用半像素精度；四舍五入避免小数滚动抵消后 120 变成 119.99999 而换 ID。
        local.x = std::round(local.x * 2.f) * .5f;
        local.y = std::round(local.y * 2.f) * .5f;
        const uint64_t id = detail::hash_combine(parent.id, detail::make_id(kind, local, extra));
        return state_scope_id ? detail::hash_combine(state_scope_id, id) : id;
    }

    // ---- 滚轮归属 ----
    // 滚轮事件是"整窗一个"，而滚动区可以嵌套（内容区里再放一个列表）。只按
    // box.contains(mouse) 判定的话，鼠标在内层列表上滚，外层内容区也一起滚 ——
    // 两个滚动条同时动。这里记下**每一帧所有滚动区的矩形与嵌套深度**，下一帧
    // 只有"最深且含指针"的那个吃滚轮；第一帧没有记录，退化成老行为（可接受）。
    struct ScrollRectMemo { Rect box; int depth = 0; };
    std::vector<ScrollRectMemo> scroll_memo;      // 上一帧的
    std::vector<ScrollRectMemo> scroll_memo_next; // 本帧正在收集
    bool deeper_scroll_has_pointer(int my_depth, float x, float y) const {
        for (const auto& m : scroll_memo)
            if (m.depth > my_depth && m.box.contains(x, y)) return true;
        return false;
    }
    Rect effective_clip = Rect(0, 0, -1, -1);   // 物理像素；w<0 表示不裁

    void recompute_clip() {
        if (clips.empty()) { effective_clip = Rect(0, 0, -1, -1); }
        else {
            Rect c = clips[0];
            for (size_t i = 1; i < clips.size(); ++i) {
                float x0 = std::max(c.x, clips[i].x), y0 = std::max(c.y, clips[i].y);
                float x1 = std::min(c.right(), clips[i].right()), y1 = std::min(c.bottom(), clips[i].bottom());
                c = Rect(x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0));
            }
            effective_clip = c;
        }
        if (canvas) {
            if (effective_clip.w < 0.0f) canvas->clear_clip();
            else canvas->set_clip(int(effective_clip.x), int(effective_clip.y),
                                  int(effective_clip.right()), int(effective_clip.bottom()));
        }
    }

    // 平台回调
    std::function<void()> request_repaint;
    std::function<bool(const std::string&)> try_set_clipboard;
    std::function<void(std::function<void(std::string)>)> get_clipboard_async;
    struct PasteRequest {
        uint64_t id = 0, scope = 0;
        std::string before, text;
        int caret = 0, anchor = -1;
        bool ready = false;
    };
    std::shared_ptr<PasteRequest> paste;
    std::string paste_text(uint64_t id, std::string& value, bool begin) {
        if (paste && paste->id == id && paste->ready) {
            auto result = std::move(paste); paste.reset();
            if (result->scope != state_scope_id || result->before != value
                || result->caret != caret || result->anchor != sel_anchor) return {};
            return std::move(result->text);
        }
        if (!begin) return {};
        if (!get_clipboard_async) return {};
        auto request = std::make_shared<PasteRequest>();
        request->id = id; request->scope = state_scope_id; request->before = value;
        request->caret = caret; request->anchor = sel_anchor; paste = request;
        get_clipboard_async([request](std::string text) { request->text = std::move(text); request->ready = true; });
        if (request->ready) {
            paste.reset();
            return std::move(request->text);
        }
        return {};
    }
    bool copy_to_clipboard(const std::string& value) {
        return try_set_clipboard && try_set_clipboard(value);
    }
    // 把输入法的组合窗 / 候选窗挪到光标处（参数是**物理客户区**像素：
    // caret_x, 输入框上沿, 输入框下沿）。平台后端绑定；未绑定就什么都不做。
    std::function<void(float, float, float)> ime_pos;
    std::function<void(bool, bool)> ime_content_type;
    // IME 正在组字的串（拼音/注音…，UTF-8）。这是**预编辑**文本：上屏前它不属于
    // value，一个字节都不该写进去（写进去就等于"打拼音时正文被污染"）。
    // input / textarea 在聚焦时把它画在 caret 处、带下划线，caret 顺延到它后面。
    // Windows 后端从 WM_IME_COMPOSITION 的 GCS_COMPSTR 取；未绑定输入法的平台恒为空。
    std::string ime_comp;
    // 本帧有没有输入框在画组字串（input / textarea 每次把 IME 钉到光标处都会置位）。
    // 后端靠它判断"系统那个浮动组合窗要不要藏"：有输入框接管才藏，
    // 否则焦点落在按钮之类的地方时，藏了就等于把拼音吞掉。
    bool ime_owner = false;

    // 把逻辑坐标矩形转成物理像素矩形
    Rect phys(const Rect& r) const {
        return Rect(r.x * scale, r.y * scale, r.w * scale, r.h * scale);
    }
    float px(float v) const { return v * scale; }

    // ---- 仿射变换（只作用于绘制）----
    // x' = a*x + c*y + e ; y' = b*x + d*y + f   （之后统一乘 DPI 缩放）
    struct Affine { float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0; };
    Affine xf;
    std::vector<Affine> xf_stack;

    bool xf_identity() const {
        return xf.a == 1.0f && xf.b == 0.0f && xf.c == 0.0f && xf.d == 1.0f && xf.e == 0.0f && xf.f == 0.0f;
    }
    bool  xf_rotated() const { return xf.b != 0.0f || xf.c != 0.0f; }
    float xf_scale()   const { return std::sqrt(std::fabs(xf.a * xf.d - xf.b * xf.c)); }
    float xf_angle()   const { return std::atan2(xf.b, xf.a); }        // 弧度，顺时针

    Vec2 xf_map(float x, float y) const {
        return Vec2((xf.a * x + xf.c * y + xf.e) * scale,
                    (xf.b * x + xf.d * y + xf.f) * scale);
    }
    // 逻辑矩形 → 物理像素矩形。无旋转时精确，有旋转时返回包围盒（裁剪/阴影用）
    Rect xf_phys(const Rect& r) const {
        if (xf_identity()) return phys(r);
        if (!xf_rotated()) {
            const Vec2 p0 = xf_map(r.x, r.y), p1 = xf_map(r.right(), r.bottom());
            return Rect(p0.x, p0.y, p1.x - p0.x, p1.y - p0.y);
        }
        const Vec2 p0 = xf_map(r.x, r.y),       p1 = xf_map(r.right(), r.y);
        const Vec2 p2 = xf_map(r.right(), r.bottom()), p3 = xf_map(r.x, r.bottom());
        const float x0 = std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x));
        const float x1 = std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x));
        const float y0 = std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y));
        const float y1 = std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y));
        return Rect(x0, y0, x1 - x0, y1 - y0);
    }
    float xf_len(float v) const { return v * xf_scale() * scale; }

    // ---- 补间状态 ----
    struct Tween {
        float from = 0.0f, to = 0.0f;
        float t = 0.0f, dur = 0.2f;
        bool  init = false;
        Easing e = nullptr;
    };
    detail::WidgetStateTable<Tween> tweens;

    struct ColorTween {
        Color from{}, to{};
        float t = 0.0f, dur = 0.2f;
        bool  init = false;
        Easing e = nullptr;
    };
    detail::WidgetStateTable<ColorTween> color_tweens;

    uint64_t state_scope_id = 0;
    std::vector<uint64_t> released_state_scopes;
    template<class Fn> void each_widget_state(Fn fn) {
        fn(anim); fn(phase); fn(flags); fn(strings); fn(edit_stack); fn(text_lines);
        fn(tweens); fn(color_tweens);
    }
    void set_state_scope(uint64_t id) {
        state_scope_id = id;
        each_widget_state([&](auto& table) { table.set_scope(id); });
    }
    void release_widget_state(uint64_t id) {
        if (paste && paste->scope == id) paste.reset();
        each_widget_state([&](auto& table) { table.release_scope(id); });
        if (active_scope_id == id) { active = 0; active_scope_id = 0; }
        if (focus_scope_id == id) {
            focus = nullptr; focus_id = 0; focus_scope_id = 0; ime_comp.clear();
            ime_owner = false;
        }
        if (hot_scope_id == id) { hot = 0; hot_scope_id = 0; hot_rect_valid = false; }
        if (next_hot_scope_id == id) {
            next_hot = 0; next_hot_scope_id = 0; next_hot_rect = Rect{}; next_region_hit = false;
        }
        auto remove_gates = [id](Rect* rects, uint64_t* z, uint64_t* owners, int& count) {
            int kept = 0;
            for (int i = 0; i < count; ++i) {
                if (owners[i] == id) continue;
                rects[kept] = rects[i]; z[kept] = z[i]; owners[kept] = owners[i]; ++kept;
            }
            count = kept;
        };
        remove_gates(gate_allow, gate_allow_z, gate_allow_scope, gate_allow_n);
        remove_gates(gate_next_allow, gate_next_allow_z, gate_next_allow_scope, gate_next_allow_n);
        gate_on = gate_allow_n > 0;
        gate_next = gate_next_allow_n > 0;
    }

    float tween_now(const Tween& tw) const {
        Easing e = tw.e ? tw.e : easing::out_cubic;
        const float k = tw.dur > 0.0001f ? clampf(tw.t / tw.dur, 0.0f, 1.0f) : 1.0f;
        return tw.from + (tw.to - tw.from) * e(k);
    }
    void abort_frame() noexcept {
        defer.clear();
        if (measure_on) {
            canvas = measure_canvas; measure_canvas = nullptr; measure_on = false;
            input = std::move(measure_input);
            each_widget_state([](auto& table) { table.set_measuring(false); });
        }
        scroll_stack.clear();
        clip_logic.clear();
        xf_stack.clear();
        frame_open = false;
        active = 0;
        gate_on = gate_next = false;
        gate_allow_n = gate_next_allow_n = 0;
        input.text_input.clear();
        input.wheel = 0;
        for (int i = 0; i < KEY_COUNT; ++i) input.key_pressed[i] = false;
    }
};

// -----------------------------------------------------------------------------
//  Ui 基础
// -----------------------------------------------------------------------------
Ui::Ui() : impl(new Impl()) {}
Ui::~Ui() { delete impl; impl = nullptr; }

StateScope::StateScope(StateScope&& other) noexcept
    : ui_(other.ui_), previous_(other.previous_) { other.ui_ = nullptr; }
StateScope::~StateScope() { if (ui_) ui_->impl->set_state_scope(previous_); }
StateScope Ui::state_scope(uint64_t id) {
    const uint64_t previous = impl->state_scope_id;
    impl->set_state_scope(id);
    return StateScope(this, previous);
}
void Ui::release_state_scope(uint64_t id) {
    if (!id || impl->measure_on) return;
    if (impl->frame_open) {
        auto& pending = impl->released_state_scopes;
        if (std::find(pending.begin(), pending.end(), id) == pending.end()) pending.push_back(id);
    } else impl->release_widget_state(id);
}

void Ui::begin_frame(float w, float h, float dt, float dpi) {
    impl->frame_open = true;
    impl->timed_repaint = false;
    impl->width = w;
    impl->height = h;
    impl->dt = (dt > 0.0f && dt < 1.0f) ? dt : (1.0f / 60.0f);
    impl->scale = dpi > 0.0f ? dpi : 1.0f;
    impl->clock += impl->dt;
    impl->caret_blink += impl->dt;
    if (impl->text_veto > 0.0f) impl->text_veto -= impl->dt;
    if (impl->input.any_ctrl_key() || impl->input.any_alt_key()) impl->text_veto = 0.15f;
    impl->ime_owner = false;
    // 浮层闸门：把上一帧登记的"可交互区"搬进本帧，再清空下一帧的槽位。
    // 没有浮层登记的那一帧就是"不挡"，所以关闭后自动释放。
    impl->gate_on = impl->gate_next;
    impl->gate_allow_n = impl->gate_next_allow_n;
    for (int i = 0; i < impl->gate_next_allow_n; ++i) {
        impl->gate_allow[i] = impl->gate_next_allow[i];
        impl->gate_allow_z[i] = impl->gate_next_allow_z[i];
        impl->gate_allow_scope[i] = impl->gate_next_allow_scope[i];
    }
    impl->gate_next = false;
    impl->gate_next_allow_n = 0;
    impl->seq = 0;          // 本帧的 z 序从头数
    impl->defer.clear();
    impl->hot = impl->next_hot;
    impl->hot_scope_id = impl->next_hot_scope_id;
    impl->next_hot = 0;
    impl->next_hot_scope_id = 0;
    impl->hot_rect = impl->next_hot_rect;
    impl->hot_rect_valid = (impl->hot != 0 || impl->next_region_hit) && impl->hot_rect.w > 0.0f;
    impl->next_hot_rect = Rect{};
    impl->next_region_hit = false;
    impl->hit_rects.swap(impl->hit_rects_next);
    impl->hit_rects_next.clear();
    // 滚轮归属用的滚动区快照：上一帧收集完的搬到 scroll_memo，本帧重新收集。
    impl->scroll_memo.swap(impl->scroll_memo_next);
    impl->scroll_memo_next.clear();
    impl->clips.clear();
    impl->clip_logic.clear();
    // 帧回调抛异常时滚动区不会走 end_scroll()。不清栈的话下一帧会在"上一个
    // 滚动区里"开始画，整页偏移一个滚动量 —— 症状很难查，代价只有一行。
    impl->scroll_stack.clear();
    impl->recompute_clip_logic();
    // 一次按下只能归一个控件。见 detail::interact() 里的用法。
    impl->press_taken = false;
    impl->recompute_clip();
    impl->cursor = Cursor::Arrow;
}

void Ui::end_frame() {
    // 帧末统一绘制弹出层，保证它盖在所有常规组件之上。
    // 序号同时抬到 Z_FRAME_END：面板里的控件从这一段开始数，和普通控件分开，
    // 闸门才分得清"弹层自己的内容"和"排在弹层后面画的普通控件"。
    impl->seq = Impl::Z_FRAME_END;
    for (size_t i = 0; i < impl->defer.size(); ++i) {
        // deque 追加不搬动已有节点；父回调的捕获须存活到所有嵌套回调结束。
        auto& call = impl->defer[i];
        auto state = state_scope(call.scope);
        if (call.fn) call.fn();
    }
    impl->defer.clear();

    // 只处理显式销毁请求，且等延迟回调及其捕获引用全部结束后再删除节点。
    for (uint64_t id : impl->released_state_scopes) impl->release_widget_state(id);
    impl->released_state_scopes.clear();
    if (impl->released_state_scopes.capacity() > 64)
        std::vector<uint64_t>().swap(impl->released_state_scopes);

    // 清掉"本帧一次性"的输入事件
    Input& in = impl->input;
    // 原控件可能因刷新、切页或布局变化消失；延迟控件消费完松手后统一释放。
    if (!in.down[0]) impl->active = 0;
    for (int i = 0; i < 3; ++i) { in.pressed[i] = false; in.released[i] = false; }
    in.wheel = 0.0f;
    in.double_click = false;
    in.click_count = 0;
    in.text_input.clear();
    for (int i = 0; i < KEY_COUNT; ++i) in.key_pressed[i] = false;
    in.mouse_delta = Vec2(0, 0);
    in.mouse_prev = in.mouse;
    if (impl->paste && impl->paste->id != impl->focus_id) impl->paste.reset();
    impl->frame_open = false;
}

// 测量模式下浮层不入队：测量跑的是同一份绘制代码，弹层会被登记两遍。
void Ui::deferred(std::function<void()> fn) {
    if (impl->measure_on) return;
    impl->defer.push_back({impl->state_scope_id, std::move(fn)});
}

const Input& Ui::input() const {
    if (pointer_available()) return impl->input;
    // 不修改后端事件：同一帧的浮层内容仍须收到原始按下和松手。
    Input& masked = impl->masked_input;
    masked = impl->input;
    for (int i = 0; i < 3; ++i)
        masked.down[i] = masked.pressed[i] = masked.released[i] = false;
    masked.wheel = 0.0f;
    masked.double_click = false;
    masked.click_count = 0;
    masked.mouse_delta = Vec2(0, 0);
    masked.dragging = false;
    return masked;
}
const Input& Ui::raw_input() const { return impl->input; }
bool Ui::pointer_available() const {
    return !impl->measure_on && !impl->gate_blocked_pointer() &&
           !impl->clip_excludes(impl->input.mouse.x, impl->input.mouse.y);
}
const Theme& Ui::theme() const { return impl->theme; }
void Ui::set_theme(const Theme& t) {
    const bool fonts_changed = impl->theme.font_family != t.font_family
        || impl->theme.font_family_mono != t.font_family_mono;
    impl->theme = t;
    if (fonts_changed) {
        if (impl->text) impl->text->reconfigure(impl->scale, t.font_family, t.font_family_mono);
        impl->adv_cache.clear();
        impl->wrap_cache.clear(); impl->wrap_lru.clear(); impl->wrap_bytes = 0;
    }
}
float Ui::dpi_scale() const { return impl->scale; }
float Ui::width() const { return impl->width; }
float Ui::height() const { return impl->height; }
float Ui::time() const { return impl->clock; }
float Ui::frame_dt() const { return impl->dt; }
bool Ui::request_next_frame() {
    if (impl->measure_on || impl->frame_open || !impl->request_repaint) return false;
    impl->request_repaint();
    return true;
}
void Ui::repaint() {
    if (!impl->measure_on && impl->request_repaint) impl->request_repaint();
}
bool Ui::request_animation_frame() {
    if (impl->measure_on || !impl->frame_open || !impl->request_repaint) return false;
    repaint();
    return true;
}
void Ui::request_frame_in(float seconds) {
    if (impl->measure_on || !impl->frame_open || !std::isfinite(seconds)) return;
    const auto delay = std::chrono::duration<double>(detail::clampf(seconds, 0.0f, 86400.0f));
    const auto at = std::chrono::steady_clock::now()
        + std::chrono::duration_cast<std::chrono::steady_clock::duration>(delay);
    if (!impl->timed_repaint || at < impl->repaint_at) impl->repaint_at = at;
    impl->timed_repaint = true;
}
void Ui::set_animating(bool on) {
    if (impl->measure_on) return;
    impl->animating = on;
    if (on) repaint();
}
bool  Ui::animating() const { return impl->animating; }
void  Ui::set_cursor(Cursor c) { if (impl->measure_on) return; impl->cursor = c; }
Cursor Ui::cursor() const { return impl->cursor; }
void* Ui::focused_id() const { return impl->focus; }
void  Ui::set_focus(void* id) {
    if (impl->measure_on) return;
    if (id != impl->focus) impl->paste.reset();
    impl->focus = id;
    impl->focus_id = uint64_t(reinterpret_cast<uintptr_t>(id));
    impl->focus_scope_id = id ? impl->state_scope_id : 0;
    if (id) impl->caret_blink = 0.0f;
}

void Ui::begin_measure() {
    if (impl->measure_on) return;
    impl->measure_input = std::move(impl->input);
    impl->input = Input{};
    impl->measure_on = true;
    impl->measure_clip_depth = impl->clip_logic.size();
    impl->each_widget_state([](auto& table) { table.set_measuring(true); });
    impl->measure_any = false;
    impl->measure_acc = Rect{};
    // 摘掉画布：万一有控件忘了判 measure_on，也不会把真实图形糊到屏幕左上角
    // （测量跑的是 box 原点的绘制代码，落了地就是一条从 (0,0) 开始的残影）。
    impl->measure_canvas = impl->canvas;
    impl->canvas = nullptr;
}
Rect Ui::end_measure() {
    if (!impl->measure_on) return {};
    impl->measure_on = false;
    impl->each_widget_state([](auto& table) { table.set_measuring(false); });
    impl->canvas = impl->measure_canvas;
    impl->measure_canvas = nullptr;
    impl->input = std::move(impl->measure_input);
    return impl->measure_any ? impl->measure_acc : Rect{};
}
bool Ui::measuring() const { return impl->measure_on; }

bool Ui::hovered(const Rect& box) {
    if (impl->measure_on) return false;
    impl->hover_region(box);
    return box.contains(impl->input.mouse.x, impl->input.mouse.y)
        && !impl->clip_excludes(impl->input.mouse.x, impl->input.mouse.y)
        && !impl->gate_blocked_pointer();
}

// -----------------------------------------------------------------------------
//  hover 等状态的帧间插值
// -----------------------------------------------------------------------------
namespace detail {
inline float animate_value(Ui::Impl* im, uint64_t id, float target, float speed = 14.0f) {
    // 测量不能创建动画状态；否则首次显示时会把选中控件从零重新播放。
    if (im->measure_on) {
        const float* found = im->anim.peek(id);
        return found ? *found : target;
    }
    float& cur = im->anim[id];
    float d = target - cur;
    if (std::fabs(d) < 0.0015f) {
        if (cur != target) { cur = target; if (im->request_repaint) im->request_repaint(); }
        return cur;
    }
    cur += d * clampf(im->dt * speed, 0.0f, 1.0f);
    if (im->request_repaint) im->request_repaint();   // 动画没结束，继续要帧
    return cur;
}

inline float animate_state(Ui::Impl* im, uint64_t id, float target, float speed = 14.0f) {
    // 首次出现直接显示真实值，仅对已存在控件的状态变化插值。
    if (!im->measure_on) im->anim.get_or_insert(id, target);
    return animate_value(im, id, target, speed);
}

struct Interact {
    bool hovered = false;
    bool pressed = false;    // 本帧刚按下
    bool held = false;       // 持续按住
    bool clicked = false;    // 本帧完成一次点击
    bool double_clicked = false;
};

inline Interact interact(Ui::Impl* im, uint64_t id, const Rect& r, bool enabled) {
    Interact res;
    // 测量模式只记账：不碰 hot/active/gate，也不产生 hover/click。
    // 否则每帧跑一遍测量就会把交互状态搅乱（按钮可能在测量阶段"被点"）。
    if (im->measure_on) { im->measure_add(r); return res; }
    const Input& in = im->input;
    const uint64_t z = ++im->seq;      // 先取序号：它同时是"我在第几层"
    // 浮层打开时，遮罩上的鼠标不属于任何下层控件；被浮层盖住的控件同样不算。
    // 少了这一句，点遮罩 / 点面板都会同时点到它下面的按钮 —— 最典型的穿透。
    if (im->gate_blocked(z, in.mouse.x, in.mouse.y)) {
        if (im->active == id) im->active = 0;   // 遮罩上松开不算点击
        return res;
    }
    bool inside = enabled && r.contains(in.mouse.x, in.mouse.y);
    // 裁剪区之外的东西**看得见才有资格被点**。滚动列表会把超出视口的行裁掉，
    // 那些行的矩形仍然落在坐标上（gallery 的页脚下方压着十几行），不判裁剪的话
    // 它们照样抢 hover / 点击。
    if (inside && im->clip_excludes(in.mouse.x, in.mouse.y)) inside = false;
    if (enabled && im->hit_rects_next.size() < 4096) {
        const Rect reg = im->clip_logic_on ? r.intersect(im->clip_logic_cur) : r;
        if (!reg.empty()) im->hit_rects_next.push_back(reg);
    }
    if (inside) {
        res.hovered = true;
        if (im->active == 0) {
            im->next_hot = id; im->next_hot_rect = r; im->next_hot_scope_id = im->state_scope_id;
        }
        // 一帧里可能有多个控件都包含指针（叠放的、或没被裁干净的），但"按下"
        // 只属于一个控件：谁先画谁先认领。少了这道闸，一次点击会被后面的控件
        // 重新认领一遍 —— 按下和松开落在同一帧时尤其明显（两个控件各点一次）。
        if (in.pressed[0] && im->active == 0 && !im->press_taken) {
            im->active = id;
            im->active_scope_id = im->state_scope_id;
            im->press_taken = true;
            res.pressed = true;
        }
    }
    if (im->active == id) {
        res.held = in.down[0];
        if (in.released[0]) {
            if (inside) { res.clicked = true; res.double_clicked = in.double_click; }
            im->active = 0;
        }
    }
    // 按下 / 点击都会改应用状态，而改出来的状态可能影响**这一帧早就画完**的控件：
    // gallery 就是这样 —— 导航行画在前面，点它时把页面切了，于是这一帧是"旧高亮 +
    // 新内容"的拼接。不补排一帧，用户得等下一次鼠标移动才看见高亮跟上，看起来像
    // "点了没反应/高亮卡住"。按下就要补，因为有些控件按下即改状态。
    if (res.pressed || res.clicked) {
        if (im->request_repaint) im->request_repaint();
    }
    return res;
}

// 把已解析的交互态换算成"该用哪个颜色画"
//
// 底色**全透明**时（Ghost / Outline / Link，base 就是 Color::none()）不能直接
// `base.mix(hover, t)`：none 是 (0,0,0,0)，`mix` 会把 RGB 和 alpha **一起**从黑色
// 拉上来 —— t=0.5 时拿到 (122,122,122, a=128)，光栅器 blend 完是 185，比背景(249)
// 和终态(244)都暗。于是每次悬停都先"闪一下深灰"再回到几乎看不见的 accent，
// 用户看到的就是"hover 莫名其妙地闪"，而且只有带 hover 淡入的控件（按钮 / 图标
// 按钮 / 分段选择器）有这毛病，没有淡入的静态表格没有。
//
// 这类底色要的是"淡入一个颜色"，不是"从黑色渐变到该颜色"：RGB 始终取目标色，
// 只推进 alpha，中间帧交给光栅器去 blend。真正该动 alpha 的地方只有这里。
inline Color state_fill(const Color& base, const Color& hover, const Color& press,
                        float t_hover, float t_press) {
    const float th = clampf(t_hover, 0.0f, 1.0f);
    const float tp = clampf(t_press, 0.0f, 1.0f);
    if (base.a == 0) {
        // 按下层压在 hover 层之上：颜色按 press 的权重走，但不透明度是
        // 两层叠加 1-(1-th)(1-tp)，**不能**让 alpha 跟着 mix 掉下去。
        Color c = tp > 0.0f ? hover.mix(press, tp) : hover;
        const float a = 1.0f - (1.0f - th) * (1.0f - tp);
        c.a = uint8_t(clampf(float(c.a) * a, 0.0f, 255.0f) + 0.5f);
        return c;
    }
    Color c = base.mix(hover, th);
    c = c.mix(press, tp);
    return c;
}

// 圆角矩形的多边形顶点（逻辑坐标 → 经当前变换映射到物理像素）。
// 只在旋转时用：无旋转走 raster_rrect 的专用路径，快而且边缘质量更好。
inline std::vector<Vec2>& poly_scratch() {
    static thread_local std::vector<Vec2> v;
    return v;
}

inline void rrect_poly(const Ui::Impl* im, const Rect& r, float radius, std::vector<Vec2>& out) {
    out.clear();
    const float rad = std::min(std::max(radius, 0.0f), std::min(r.w, r.h) * 0.5f);
    auto pt = [&](float x, float y) { out.push_back(im->xf_map(x, y)); };
    if (rad <= 0.35f) {
        pt(r.x, r.y); pt(r.right(), r.y); pt(r.right(), r.bottom()); pt(r.x, r.bottom());
        return;
    }
    const int seg = 5;                       // 每角 5 段：小控件上已经看不出折线
    const float hp = 1.57079632679f;
    const float cx[4] = { r.right() - rad, r.right() - rad, r.x + rad, r.x + rad };
    const float cy[4] = { r.y + rad,       r.bottom() - rad, r.bottom() - rad, r.y + rad };
    const float a0[4] = { -hp, 0.0f, hp, 2.0f * hp };
    for (int k = 0; k < 4; ++k)
        for (int i = 0; i <= seg; ++i) {
            const float ang = a0[k] + hp * float(i) / float(seg);
            pt(cx[k] + std::cos(ang) * rad, cy[k] + std::sin(ang) * rad);
        }
}

} // namespace detail

// =============================================================================
//  8.6  图元
// =============================================================================
void Ui::clear(const Color& c) {
    if (impl->measure_on) return;
    if (impl->canvas) impl->canvas->clear(c);
}

void Ui::fill_rect(const Rect& r, const Color& c, float radius) {
    detail::ProfTick _pt(&impl->pf_fill_us, &impl->pf_fill_n, impl->prof_on);
    if (impl->prof_on) impl->pf_fill_px += uint64_t(std::max(r.w, 0.0f) * std::max(r.h, 0.0f) * impl->scale * impl->scale);
    if (impl->measure_on) { if (c.a != 0 && !r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || c.a == 0 || r.empty()) return;
    // 有旋转 → 多边形（raster_rrect 只吃轴对齐矩形）
    if (impl->xf_rotated()) {
        detail::rrect_poly(impl, r, radius, detail::poly_scratch());
        detail::raster_poly(*impl->canvas, detail::poly_scratch(), c);
        return;
    }
    Rect p = impl->xf_phys(r);
    if (p.empty()) return;
    float rad = impl->xf_len(std::max(radius, 0.0f));
    // 半径大到覆盖整个矩形时退化成胶囊/圆
    rad = std::min(rad, std::min(p.w, p.h) * 0.5f);
    if (rad <= 0.35f) {
        // 无圆角：整行快填 + 上下边界按覆盖率补
        if (c.a == 255) {
            int y0 = std::max(0, int(std::floor(p.y)));
            int y1 = std::min(impl->canvas->h, int(std::ceil(p.bottom())));
            int x0 = int(std::floor(p.x)), x1 = int(std::ceil(p.right()));
            for (int y = y0; y < y1; ++y) {
                float cov_y = clampf(std::min(float(y + 1), p.bottom()) - std::max(float(y), p.y), 0.0f, 1.0f);
                if (cov_y >= 0.999f) {
                    impl->canvas->fill_row_fast(y, x0, x1, c);
                    // 只有首尾像素可能与矩形边界相交。
                    if (x0 < p.x || x0 + 1 > p.right()) {
                        float cov = clampf(std::min(float(x0 + 1), p.right()) - std::max(float(x0), p.x), 0.0f, 1.0f);
                        impl->canvas->blend(x0, y, c, cov);
                    }
                    const int last = x1 - 1;
                    if (last > x0 && (last < p.x || last + 1 > p.right())) {
                        float cov = clampf(std::min(float(last + 1), p.right()) - std::max(float(last), p.x), 0.0f, 1.0f);
                        impl->canvas->blend(last, y, c, cov);
                    }
                } else {
                    for (int x = x0; x < x1; ++x) {
                        float cov_x = clampf(std::min(float(x + 1), p.right()) - std::max(float(x), p.x), 0.0f, 1.0f);
                        if (cov_x > 0.0f) impl->canvas->blend(x, y, c, cov_x * cov_y);
                    }
                }
            }
        } else {
            detail::raster_rrect(*impl->canvas, p, c, 0.0f);
        }
        return;
    }
    detail::raster_rrect(*impl->canvas, p, c, rad, impl->theme.corner_smoothing);
}

void Ui::draw_image(const Image& image, const Rect& box) {
    if (!image.valid() || box.empty()) return;
    if (impl->measure_on) { impl->measure_add(box); return; }
    if (!impl->canvas) return;
    const auto& m = impl->xf;
    const float det = m.a * m.d - m.b * m.c;
    if (!std::isfinite(det) || std::fabs(det) < 1e-8f || impl->scale <= 0) return;
    const Vec2 p0 = impl->xf_map(box.x, box.y), p1 = impl->xf_map(box.right(), box.y);
    const Vec2 p2 = impl->xf_map(box.right(), box.bottom()), p3 = impl->xf_map(box.x, box.bottom());
    const float left = std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x));
    const float top = std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y));
    const float right = std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x));
    const float bottom = std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y));
    const Rect bounds(left, top, right - left, bottom - top);
    if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
        !std::isfinite(bounds.right()) || !std::isfinite(bounds.bottom())) return;
    Canvas& cv = *impl->canvas;
    const int x0 = std::max(cv.clipped ? cv.clip_x0 : 0,
        int(std::max(0.f, std::min(float(cv.w), std::floor(bounds.x)))));
    const int y0 = std::max(cv.clipped ? cv.clip_y0 : 0,
        int(std::max(0.f, std::min(float(cv.h), std::floor(bounds.y)))));
    const int x1 = std::min(cv.clipped ? cv.clip_x1 : cv.w,
        int(std::max(0.f, std::min(float(cv.w), std::ceil(bounds.right())))));
    const int y1 = std::min(cv.clipped ? cv.clip_y1 : cv.h,
        int(std::max(0.f, std::min(float(cv.h), std::ceil(bounds.bottom())))));
    for (int y = y0; y < y1; ++y) for (int x = x0; x < x1; ++x) {
        const float px = (float(x) + .5f) / impl->scale - m.e;
        const float py = (float(y) + .5f) / impl->scale - m.f;
        const float lx = (m.d * px - m.c * py) / det;
        const float ly = (m.a * py - m.b * px) / det;
        const float u = (lx - box.x) / box.w, v = (ly - box.y) / box.h;
        if (u < 0.f || u >= 1.f || v < 0.f || v >= 1.f) continue;
        const float fx = std::max(0.f, std::min(float(image.width - 1), u * image.width - .5f));
        const float fy = std::max(0.f, std::min(float(image.height - 1), v * image.height - .5f));
        const int sx = int(fx), sy = int(fy);
        const int sx1 = std::min(sx + 1, image.width - 1), sy1 = std::min(sy + 1, image.height - 1);
        const float dx = fx - sx, dy = fy - sy;
        const float weights[] = {(1 - dx) * (1 - dy), dx * (1 - dy), (1 - dx) * dy, dx * dy};
        const size_t indices[] = {
            (size_t(sy) * image.width + sx) * 4u, (size_t(sy) * image.width + sx1) * 4u,
            (size_t(sy1) * image.width + sx) * 4u, (size_t(sy1) * image.width + sx1) * 4u};
        float alpha = 0, red = 0, green = 0, blue = 0;
        for (int i = 0; i < 4; ++i) {
            const uint8_t* p = &image.pixels[indices[i]];
            const float a = weights[i] * p[3];
            alpha += a; red += a * p[0]; green += a * p[1]; blue += a * p[2];
        }
        if (alpha > 0) cv.blend(x, y, Color(uint8_t(red / alpha + .5f),
            uint8_t(green / alpha + .5f), uint8_t(blue / alpha + .5f),
            uint8_t(std::min(alpha + .5f, 255.f))), 1.f);
    }
}

void Ui::stroke_rect(const Rect& r, const Color& c, float radius, float width) {
    detail::ProfTick _pt(&impl->pf_stroke_us, &impl->pf_stroke_n, impl->prof_on);
    if (impl->prof_on) impl->pf_stroke_px += uint64_t(std::max(r.w, 0.0f) * std::max(r.h, 0.0f) * impl->scale * impl->scale);
    if (impl->measure_on) { if (c.a != 0 && !r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || c.a == 0 || r.empty()) return;
    const float w = std::max(impl->xf_len(width), 0.6f);
    if (impl->xf_rotated()) {
        // 描边 = 外轮廓 + 内轮廓，交给偶奇规则的扫描线填充（中间自然留空）
        std::vector<Vec2>& pts = detail::poly_scratch();
        detail::rrect_poly(impl, r, radius, pts);
        const float lw = w / std::max(impl->xf_scale() * impl->scale, 0.0001f);
        const Rect inner(r.x + lw, r.y + lw, std::max(r.w - lw * 2.0f, 0.0f), std::max(r.h - lw * 2.0f, 0.0f));
        if (inner.w > 0.0f && inner.h > 0.0f) {
            std::vector<Vec2> tmp;
            detail::rrect_poly(impl, inner, std::max(radius - lw, 0.0f), tmp);
            pts.insert(pts.end(), tmp.begin(), tmp.end());
        }
        detail::raster_poly(*impl->canvas, pts, c);
        return;
    }
    Rect p = impl->xf_phys(r);
    float rad = impl->xf_len(std::max(radius, 0.0f));
    rad = std::min(rad, std::min(p.w, p.h) * 0.5f);
    detail::raster_rrect_stroke(*impl->canvas, p, c, rad, w, impl->theme.corner_smoothing);
}

void Ui::fill_rect_s(const Rect& r, const Color& c, float radius, float smooth) {
    detail::ProfTick _pt(&impl->pf_fill_us, &impl->pf_fill_n, impl->prof_on);
    if (impl->prof_on) impl->pf_fill_px += uint64_t(std::max(r.w, 0.0f) * std::max(r.h, 0.0f) * impl->scale * impl->scale);
    if (impl->measure_on) { if (c.a != 0 && !r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || c.a == 0 || r.empty()) return;
    detail::raster_rrect(*impl->canvas, impl->xf_phys(r), c,
                         impl->xf_len(std::max(radius, 0.0f)),
                         smooth < 0.0f ? impl->theme.corner_smoothing : smooth);
}

void Ui::stroke_rect_s(const Rect& r, const Color& c, float radius, float width, float smooth) {
    if (impl->measure_on) { if (c.a != 0 && !r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || c.a == 0 || r.empty()) return;
    detail::raster_rrect_stroke(*impl->canvas, impl->xf_phys(r), c,
                                impl->xf_len(std::max(radius, 0.0f)),
                                std::max(impl->xf_len(width), 0.6f),
                                smooth < 0.0f ? impl->theme.corner_smoothing : smooth);
}

void Ui::inner_shadow(const Rect& r, const Color& c, float radius, float blur, float dy,
                             float smooth) {
    if (impl->measure_on) return;          // 内阴影不占布局
    if (!impl->canvas || c.a == 0 || r.empty()) return;
    (void)smooth;                          // 阴影带用纯圆弧求距离场，差值在亚像素级
    detail::raster_rrect_inner(*impl->canvas, impl->xf_phys(r), impl->xf_len(std::max(radius, 0.0f)),
                               impl->xf_len(std::max(blur, 0.01f)), c, impl->xf_len(dy));
}

void Ui::fill_rect_grad(const Rect& r, const Color& top, const Color& bottom,
                               float radius, float smooth) {
    if (impl->measure_on) { if (!r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || r.empty()) return;
    if (top.a == 0 && bottom.a == 0) return;
    const Rect p = impl->xf_phys(r);
    detail::RowPaint rp;
    rp.c0 = top; rp.c1 = bottom; rp.grad = true;
    rp.y0 = p.y; rp.y1 = p.bottom();
    detail::raster_rrect_paint(*impl->canvas, p, rp, impl->xf_len(std::max(radius, 0.0f)),
                               smooth < 0.0f ? impl->theme.corner_smoothing : smooth);
}

// 顶部高光：bezel 的一条 1px 亮边只出现在上半圈，整圈描边会变成"描边感"。
void Ui::stroke_rect_top(const Rect& r, const Color& c, float radius, float width, float height,
                                float smooth) {
    if (impl->measure_on) { if (c.a != 0 && !r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || c.a == 0 || r.empty()) return;
    Rect clip(r.x, r.y - width, r.w, height + width);
    Rect inter = clip.intersect(r);
    if (inter.empty()) return;
    push_clip(inter);
    detail::raster_rrect_stroke(*impl->canvas, impl->xf_phys(r), c,
                                impl->xf_len(std::max(radius, 0.0f)),
                                std::max(impl->xf_len(width), 0.6f),
                                smooth < 0.0f ? impl->theme.corner_smoothing : smooth);
    pop_clip();
}

void Ui::fill_circle(float cx, float cy, float radius, const Color& c) {
    detail::ProfTick _pt(&impl->pf_circle_us, &impl->pf_circle_n, impl->prof_on);
    if (impl->measure_on) {
        if (c.a != 0 && radius > 0.0f)
            impl->measure_add(Rect(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f));
        return;
    }
    if (!impl->canvas || c.a == 0) return;
    const Vec2 p = impl->xf_map(cx, cy);
    detail::raster_circle(*impl->canvas, p.x, p.y, impl->xf_len(radius), c);
}

void Ui::fill_crescent(float cx, float cy, float radius,
                              float cut_cx, float cut_cy, float cut_radius, const Color& c) {
    if (impl->measure_on) {
        if (c.a != 0 && radius > 0.0f) {
            const float x0 = std::min(cx - radius, cut_cx - cut_radius);
            const float y0 = std::min(cy - radius, cut_cy - cut_radius);
            const float x1 = std::max(cx + radius, cut_cx + cut_radius);
            const float y1 = std::max(cy + radius, cut_cy + cut_radius);
            impl->measure_add(Rect(x0, y0, x1 - x0, y1 - y0));
        }
        return;
    }
    if (!impl->canvas || c.a == 0) return;
    const Vec2 p = impl->xf_map(cx, cy);
    const Vec2 q = impl->xf_map(cut_cx, cut_cy);
    detail::raster_crescent(*impl->canvas, p.x, p.y, impl->xf_len(radius),
                            q.x, q.y, impl->xf_len(cut_radius), c);
}

void Ui::draw_line(float x0, float y0, float x1, float y1, const Color& c, float width) {
    if (impl->measure_on) {
        if (c.a != 0) {
            const float hw = std::max(width, 1.0f) * 0.5f;
            impl->measure_add(Rect(std::min(x0, x1) - hw, std::min(y0, y1) - hw,
                                   std::fabs(x1 - x0) + hw * 2.0f,
                                   std::fabs(y1 - y0) + hw * 2.0f));
        }
        return;
    }
    if (!impl->canvas || c.a == 0) return;
    const Vec2 a = impl->xf_map(x0, y0);
    const Vec2 b = impl->xf_map(x1, y1);
    detail::raster_line(*impl->canvas, a.x, a.y, b.x, b.y, c, impl->xf_len(width));
}

void Ui::drop_shadow(const Rect& r, float radius, float blur, const Color& c, float dy) {
    if (impl->measure_on) {
        if (c.a != 0) impl->measure_add(Rect(r.x - blur, r.y - blur + dy,
                                             r.w + blur * 2.0f, r.h + blur * 2.0f));
        return;
    }
    if (!impl->canvas || c.a == 0) return;
    // 阴影走的是距离场，只吃轴对齐矩形。旋转时用包围盒 —— 形状不再是"跟着转"，
    // 但至少不会凭空消失（这一点在文档里写明了）。
    detail::raster_shadow(*impl->canvas, impl->xf_phys(r), impl->xf_len(radius),
                          impl->xf_len(blur), c, impl->xf_len(dy));
}

// -----------------------------------------------------------------------------
//  背景模糊 / 玻璃面板
// -----------------------------------------------------------------------------
void Ui::backdrop_blur(const Rect& r, float radius, float blur) {
    if (impl->measure_on) { if (!r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || r.empty() || blur < 0.8f) return;
    // 旋转时不做：模糊要按"屏幕空间的一块矩形"采样，跟着转就不是那块像素了。
    if (impl->xf_rotated()) return;
    detail::blur_backdrop(*impl->canvas, impl->xf_phys(r), impl->xf_len(radius),
                          impl->xf_len(blur));
}

void Ui::glass(const Rect& r, float radius, const Style& st) {
    if (impl->measure_on) { if (!r.empty()) impl->measure_add(r); return; }
    if (!impl->canvas || r.empty()) return;
    const Theme& th = impl->theme;
    const bool dark = th.is_dark();
    const float rad  = st.radius >= 0.0f ? st.radius : radius;
    const float blur = st.blur_radius > 0.0f ? st.blur_radius : 12.0f;

    // 材质面板的投影。
    drop_shadow(r, rad, 18.0f, Color(0, 0, 0, dark ? 110 : 46), 6.0f);

    // 背景材质使用模糊近似。
    backdrop_blur(r, rad, blur);

    // 在模糊背景上叠加半透明色，深色主题降低白色填充强度。
    const Color tint = !st.bg.is_none()
                     ? st.bg
                     : (dark ? Color(255, 255, 255, 22) : Color(255, 255, 255, 46));
    fill_rect_s(r, tint, rad, th.corner_smoothing);

    // 4) 高光边：光角 0°（正上方）。顶部一圈亮线 + 内侧一圈极淡的白雾，
    //    这两笔就是"玻璃有厚度"的全部错觉来源。
    stroke_rect_top(r, Color(255, 255, 255, dark ? 96 : 150), rad, 1.0f, 1.4f,
                    th.corner_smoothing);
    inner_shadow(r, Color(255, 255, 255, dark ? 16 : 24), rad, 2.2f, 0.0f,
                 th.corner_smoothing);

    // 5) 外描边：只描一圈极淡的冷白，把玻璃从同色背景上分出来。
    stroke_rect_s(r, Color(255, 255, 255, dark ? 30 : 90), rad, 1.0f,
                  th.corner_smoothing);
}

Rect GlassPanel::draw(Ui& ui, const Rect& bounds, float radius, const Style& st) {
    const float min_x = -bounds.x;
    const float max_x = ui.width() - bounds.right();
    const float min_y = -bounds.y;
    const float max_y = ui.height() - bounds.bottom();
    if (max_x >= min_x) offset_x_ = std::max(min_x, std::min(offset_x_, max_x));
    else offset_x_ = (ui.width() - bounds.w) * 0.5f - bounds.x;
    if (max_y >= min_y) offset_y_ = std::max(min_y, std::min(offset_y_, max_y));
    else offset_y_ = (ui.height() - bounds.h) * 0.5f - bounds.y;

    Rect panel = bounds.offset(offset_x_, offset_y_);
    const Rect drag_area(panel.x, panel.y, panel.w, std::min(panel.h, drag_height_));
    const Input& input = ui.input();

    if (input.pressed[0] && drag_area.contains(input.mouse)) {
        dragging_ = true;
        drag_offset_x_ = input.mouse.x - panel.x;
        drag_offset_y_ = input.mouse.y - panel.y;
        ui.set_focus(nullptr);
    }
    if (!input.down[0]) dragging_ = false;
    if (dragging_ && input.down[0]) {
        offset_x_ = input.mouse.x - drag_offset_x_ - bounds.x;
        offset_y_ = input.mouse.y - drag_offset_y_ - bounds.y;
        panel = bounds.offset(offset_x_, offset_y_);
    }

    // Keep the title strip attached to the effective panel bounds on every frame.
    const Rect effective_drag_area(panel.x, panel.y, panel.w, std::min(panel.h, drag_height_));
    if (effective_drag_area.contains(input.mouse)) ui.set_cursor(Cursor::Hand);
    ui.glass(panel, radius, st);
    return panel;
}

// -----------------------------------------------------------------------------
//  动画
// -----------------------------------------------------------------------------
float Ui::tween(uint64_t id, float target, float duration, Easing e) {
    if (impl->measure_on) {
        const auto* tw = impl->tweens.peek(id);
        return tw && tw->init ? impl->tween_now(*tw) : target;
    }
    if (!e) e = easing::out_cubic;
    auto& tw = impl->tweens[id];
    if (!tw.init) {
        tw.init = true; tw.from = target; tw.to = target;
        tw.t = duration; tw.dur = duration; tw.e = e;
        return target;
    }
    if (target != tw.to) {
        // 从**当前显示值**重新起跑：目标中途改变不会跳变
        tw.from = impl->tween_now(tw);
        tw.to   = target;
        tw.t    = 0.0f;
        tw.dur  = std::max(duration, 0.0001f);
        tw.e    = e;
    }
    if (tw.t < tw.dur) {
        tw.t += impl->dt;
        if (tw.t >= tw.dur) tw.t = tw.dur;
        repaint();                          // 只有还在跑才要帧
    }
    return impl->tween_now(tw);
}

Color Ui::tween_color(uint64_t id, const Color& target, float duration, Easing e) {
    // 一个进度驱动四个通道。拆成 4 个独立 tween 会让各通道进度一致但**起点**
    // 被分别重置，颜色在中途会发灰发脏 —— 单状态才能保证插值路径是直线。
    // 测量模式：只读当前插值结果，不改状态（同 tween）
    if (impl->measure_on) {
        const auto* saved = impl->color_tweens.peek(id);
        if (!saved || !saved->init) return target;
        const auto& ct = *saved;
        const float k0 = ct.dur > 0.0001f ? clampf(ct.t / ct.dur, 0.0f, 1.0f) : 1.0f;
        const float e0 = (ct.e ? ct.e : easing::out_cubic)(k0);
        return Color(uint8_t(float(ct.from.r) + (float(ct.to.r) - float(ct.from.r)) * e0),
                     uint8_t(float(ct.from.g) + (float(ct.to.g) - float(ct.from.g)) * e0),
                     uint8_t(float(ct.from.b) + (float(ct.to.b) - float(ct.from.b)) * e0),
                     uint8_t(float(ct.from.a) + (float(ct.to.a) - float(ct.from.a)) * e0));
    }
    if (!e) e = easing::out_cubic;
    auto& ct = impl->color_tweens[id];
    if (!ct.init) {
        ct.init = true; ct.from = target; ct.to = target;
        ct.t = duration; ct.dur = duration; ct.e = e;
        return target;
    }
    if (target.r != ct.to.r || target.g != ct.to.g || target.b != ct.to.b || target.a != ct.to.a) {
        const float k = ct.dur > 0.0001f ? clampf(ct.t / ct.dur, 0.0f, 1.0f) : 1.0f;
        const float ek = (ct.e ? ct.e : easing::out_cubic)(k);
        ct.from = Color(uint8_t(float(ct.from.r) + (float(ct.to.r) - float(ct.from.r)) * ek),
                        uint8_t(float(ct.from.g) + (float(ct.to.g) - float(ct.from.g)) * ek),
                        uint8_t(float(ct.from.b) + (float(ct.to.b) - float(ct.from.b)) * ek),
                        uint8_t(float(ct.from.a) + (float(ct.to.a) - float(ct.from.a)) * ek));
        ct.to  = target;
        ct.t   = 0.0f;
        ct.dur = std::max(duration, 0.0001f);
        ct.e   = e;
    }
    if (ct.t < ct.dur) {
        ct.t += impl->dt;
        if (ct.t >= ct.dur) ct.t = ct.dur;
        repaint();
    }
    const float k  = ct.dur > 0.0001f ? clampf(ct.t / ct.dur, 0.0f, 1.0f) : 1.0f;
    const float ek = (ct.e ? ct.e : easing::out_cubic)(k);
    auto lerp_ch = [&](uint8_t a, uint8_t b) {
        return uint8_t(clampf(float(a) + (float(b) - float(a)) * ek, 0.0f, 255.0f));
    };
    return Color(lerp_ch(ct.from.r, ct.to.r), lerp_ch(ct.from.g, ct.to.g),
                 lerp_ch(ct.from.b, ct.to.b), lerp_ch(ct.from.a, ct.to.a));
}

float Ui::smooth(float current, float target, float rate) const {
    // 指数逼近，和帧率无关：dt=1/60 与 dt=1/30 落到同一个位置
    const float k = 1.0f - std::exp(-std::max(rate, 0.0f) * impl->dt);
    const float v = current + (target - current) * k;
    return std::fabs(target - v) < 0.001f ? target : v;
}

float Ui::fade(uint64_t id, bool visible, float duration, Easing e) {
    return tween(id, visible ? 1.0f : 0.0f, duration, e);
}

// -----------------------------------------------------------------------------
//  变换
// -----------------------------------------------------------------------------
void Ui::push_transform(const Transform& t) {
    impl->xf_stack.push_back(impl->xf);

    const float rad = t.rotate * 3.14159265358979f / 180.0f;
    const float cs = std::cos(rad), sn = std::sin(rad);
    // 局部矩阵 L：p' = T(translate) * R * S * T(-origin) * p
    const float s = t.scale;
    Impl::Affine L;
    L.a = cs * s;  L.b = sn * s;
    L.c = -sn * s; L.d = cs * s;
    L.e = t.translate.x + t.origin.x - (L.a * t.origin.x + L.c * t.origin.y);
    L.f = t.translate.y + t.origin.y - (L.b * t.origin.x + L.d * t.origin.y);

    // 与父矩阵复合：M = P * L
    const Impl::Affine& P = impl->xf_stack.back();
    Impl::Affine M;
    M.a = P.a * L.a + P.c * L.b;
    M.b = P.b * L.a + P.d * L.b;
    M.c = P.a * L.c + P.c * L.d;
    M.d = P.b * L.c + P.d * L.d;
    M.e = P.a * L.e + P.c * L.f + P.e;
    M.f = P.b * L.e + P.d * L.f + P.f;
    impl->xf = M;
}

void Ui::pop_transform() {
    if (impl->xf_stack.empty()) { impl->xf = Impl::Affine(); return; }
    impl->xf = impl->xf_stack.back();
    impl->xf_stack.pop_back();
}

bool Ui::transformed() const { return !impl->xf_identity(); }

Transform Ui::transform() const {
    Transform t;
    t.scale   = impl->xf_scale();
    t.rotate  = impl->xf_angle() * 57.2957795f;
    t.translate = Vec2(impl->xf.e, impl->xf.f);
    return t;
}

float Ui::text_width(const std::string& text, float font_size, int weight) const {
    if (!impl->text || text.empty()) return 0.0f;
    float fs = font_size > 0.0f ? font_size : impl->theme.font_size;
    return impl->text->measure(text, impl->px(fs), weight < 0 ? 400 : weight) / impl->scale;
}

float Ui::line_height(float font_size) const {
    float fs = font_size > 0.0f ? font_size : impl->theme.font_size;
    if (!impl->text) return fs * 1.42f;
    return impl->text->line_height(impl->px(fs), 400) / impl->scale;
}

// -----------------------------------------------------------------------------
//  等宽文本：度量与绘制
//
//  三者都走 TextEngine 的 *_mono 那一组虚函数（Windows 上是 Consolas 之类的
//  等宽字体，Linux/macOS 上由各自后端挑），所以"同字号下步进相同"这件事由
//  字体引擎保证，而不是在这里乘一个 0.6 的魔法系数。
//
//  绘制不做旋转、不做折行、不做居中：终端网格的内容全部由调用方按 (row, col)
//  算好坐标，越界部分交给裁剪区。样式越少，长滚动区里的行开销越可预测。
// -----------------------------------------------------------------------------
float Ui::mono_width(const std::string& text, float font_size, int weight) const {
    if (!impl->text || text.empty()) return 0.0f;
    const float fs = font_size > 0.0f ? font_size : impl->theme.font_size;
    return impl->text->measure_mono(text, impl->px(fs), weight) / impl->scale;
}

float Ui::mono_advance(float font_size, int weight) const {
    const float fs = font_size > 0.0f ? font_size : impl->theme.font_size;
    if (!impl->text) return fs * 0.6f;
    // 用 'M' 量：等宽字体里所有半角字符步进相同，'M' 不会退化成零宽。
    return impl->text->measure_mono("M", impl->px(fs), weight) / impl->scale;
}

float Ui::mono_ascent(float font_size, int weight) const {
    const float fs = font_size > 0.0f ? font_size : impl->theme.font_size;
    if (!impl->text) return fs * 1.1f;
    return impl->text->ascent_mono(impl->px(fs), weight) / impl->scale;
}

void Ui::draw_mono_text(const std::string& text, float x, float y_top, const Color& c,
                               float font_size, int weight) {
    const float fs = font_size > 0.0f ? font_size : impl->theme.font_size;
    // 参与布局测量，和 draw_text 一致。等宽文字的外接矩形只由字符串和字号决定，
    // 量的成本就是一次（已缓存的）字宽查询。早先这里直接 return —— 用等宽字自绘的
    // 组件在 begin_measure() 里量出来的高度不含它，示例只好自己补一份 mono_label。
    if (impl->measure_on) {
        if (text.empty() || c.a == 0) return;
        impl->measure_add(Rect(x, y_top, mono_width(text, fs, weight), line_height(fs)));
        return;
    }
    if (!impl->canvas || !impl->text || text.empty() || c.a == 0) return;
    const Rect clip = impl->effective_clip;     // w < 0 表示没裁
    impl->text->draw_mono(*impl->canvas, text, impl->px(x), impl->px(y_top), impl->px(fs),
                          weight, c, clip);
}

namespace detail {

// 第 i 个字节处那个字符占几个字节。非法字节按 1 前进 —— 宁可画错一个字，
// 也绝不允许死循环（这段代码吃的是用户输入）。
inline size_t utf8_step(const std::string& s, size_t i) {
    const unsigned char c = (unsigned char)s[i];
    size_t n = 1;
    if      ((c & 0xE0u) == 0xC0u) n = 2;
    else if ((c & 0xF0u) == 0xE0u) n = 3;
    else if ((c & 0xF8u) == 0xF0u) n = 4;
    if (i + n > s.size()) n = 1;
    return n;
}

inline bool is_break_space(unsigned char c) { return c == ' ' || c == '\t'; }

// 单个码点的宽度（带缓存）。折行时逐字量宽是热点：没有这层缓存，每个字
// 都要走一次 measure → 哈希 → 字符串比较（首次还要一次 GDI 往返）。
inline uint64_t glyph_key(float pfs, int weight, uint32_t cp) {
    const uint64_t q = uint64_t(uint32_t(int(pfs * 4.0f + 0.5f)));
    return (q << 40) ^ (uint64_t(uint32_t(weight & 0xFFFF)) << 24) ^ uint64_t(cp);
}

inline float glyph_adv(Ui::Impl* im, uint32_t cp, float pfs, int weight) {
    const uint64_t k = glyph_key(pfs, weight, cp);
    auto it = im->adv_cache.find(k);
    if (it != im->adv_cache.end()) return it->second;

    char buf[4];
    int n = 0;
    if (cp < 0x80) { buf[n++] = char(cp); }
    else if (cp < 0x800) {
        buf[n++] = char(0xC0 | (cp >> 6));
        buf[n++] = char(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        buf[n++] = char(0xE0 | (cp >> 12));
        buf[n++] = char(0x80 | ((cp >> 6) & 0x3F));
        buf[n++] = char(0x80 | (cp & 0x3F));
    } else {
        buf[n++] = char(0xF0 | (cp >> 18));
        buf[n++] = char(0x80 | ((cp >> 12) & 0x3F));
        buf[n++] = char(0x80 | ((cp >> 6) & 0x3F));
        buf[n++] = char(0x80 | (cp & 0x3F));
    }
    const float w = im->text->measure(std::string(buf, size_t(n)), pfs, weight);
    if (im->adv_cache.size() > 8192) {                        // 不可控输入，别无限长
        auto it = im->adv_cache.begin();                      // 同样只丢一半，保住热字形
        for (int i = 0; i < 4096 && it != im->adv_cache.end(); ++i) it = im->adv_cache.erase(it);
    }
    im->adv_cache.emplace(k, w);
    return w;
}

// 从 i 处解一个码点（不校验合法性，非法字节按单字节处理）
inline uint32_t utf8_decode(const std::string& s, size_t i) {
    const unsigned char c = (unsigned char)s[i];
    const size_t n = utf8_step(s, i);
    if (n == 1) return c;
    uint32_t cp = uint32_t(c) & (n == 2 ? 0x1F : (n == 3 ? 0x0F : 0x07));
    for (size_t k = 1; k < n; ++k) cp = (cp << 6) | (uint32_t((unsigned char)s[i + k]) & 0x3F);
    return cp;
}

// 切成"行"：先按 '\n' 分逻辑行；wrap 时再按 max_w 折行。
// 西文会退到本行最后一个空格处，不把单词劈两半（退回来的部分顺延下一行）；
// 一个字符本身就超宽时也必须吐出去，否则死循环。
inline void layout_wrapped_lines(TextRenderer* tr, const std::string& text,
                                 float max_w, float pfs, int weight, bool wrap,
                                 std::vector<std::string>& out, Ui::Impl* im = nullptr) {
    out.clear();
    if (!tr) return;
    const bool measure_width = (max_w > 1.0f);
    size_t begin = 0;
    for (;;) {
        const size_t nl  = text.find('\n', begin);
        const size_t end = (nl == std::string::npos) ? text.size() : nl;
        const std::string logical = text.substr(begin, end - begin);

        if (!wrap || !measure_width || logical.empty()) {
            out.push_back(logical);
        } else {
            std::string line;
            float line_w = 0.0f;                       // 用"逐字宽累加"代替整行 measure：
            size_t last_space = std::string::npos;     // 一次 O(n) 而不是每次重测整行
            size_t i = 0;
            while (i < logical.size()) {
                const size_t n = utf8_step(logical, i);
                const unsigned char first = (unsigned char)logical[i];
                const float cw = im ? glyph_adv(im, utf8_decode(logical, i), pfs, weight)
                                    : tr->measure(logical.substr(i, n), pfs, weight);
                if (!line.empty() && line_w + cw > max_w) {
                    if (is_break_space(first)) {
                        out.push_back(line);
                        line.clear(); line_w = 0.0f; last_space = std::string::npos;
                        i += n;                        // 行首的落单空格直接丢弃
                        continue;
                    }
                    if (last_space != std::string::npos && last_space > 0) {
                        out.push_back(line.substr(0, last_space));
                        line.erase(0, last_space + 1);
                        line_w = line.empty() ? 0.0f : tr->measure(line, pfs, weight);
                        last_space = std::string::npos;
                    } else {
                        out.push_back(line);
                        line.clear(); line_w = 0.0f;
                    }
                    continue;                          // i 不前进：这个字留给下一行
                }
                line.append(logical, i, n);
                line_w += cw;
                if (is_break_space(first)) last_space = line.size() - 1;
                i += n;
            }
            out.push_back(line);
        }
        if (nl == std::string::npos) break;
        begin = nl + 1;
    }
}

// 折行结果缓存。draw_text 和 text_height 都走这里 —— 两者本来就必须用同一套
// 规则，缓存顺带把"每帧重排整段"的成本压到一次查找。
//
// 内存有上限（默认 6 MB，环境变量 AINO_WRAP_BUDGET 可改，单位 KiB）：超了按
// 最久未用淘汰。一段 20 万字的中文大约 1.6 MB 行文本，够放 3~4 段，覆盖
// "正在看的文档 + 上一份文档"这种真实场景。
inline size_t memory_budget_kib(const char* variable, size_t fallback) {
    const char* value = std::getenv(variable);
    if (!value || *value < '1' || *value > '9') return fallback;
    char* end = nullptr;
    const auto n = std::strtoull(value, &end, 10);
    if (!end || *end || !n || n > std::numeric_limits<size_t>::max() / 1024u) return fallback;
    return size_t(n) * 1024u;
}

inline const std::vector<std::string>& wrap_layout(Ui::Impl* im, const std::string& text,
                                                   float max_w, float pfs, int weight, bool wrap) {
    static const std::vector<std::string> kEmpty;
    if (!im->text) return kEmpty;
    // 空串按直接折行的结果走（1 个空行），别在这里搞特殊 —— 否则
    // "缓存命中"和"直接折行"在边界上会给出不同的行数。
    if (text.empty()) {
        std::vector<std::string>().swap(im->wrap_scratch);
        static const std::vector<std::string> kOne(1);
        return kOne;
    }

    // 键里放地址而不是内容哈希：内容哈希要遍历整串（20 万字 ≈ 200µs/帧），
    // 地址在调用方的 string 不扩容时是稳定的。命中后用 memcmp 兜底校验，
    // 所以地址被复用（临时 string 落在同一个栈地址）也不会返回错的行。
    uint64_t k = (uint64_t(reinterpret_cast<uintptr_t>(text.data())) >> 3) * 0x9E3779B97F4A7C15ull;
    k ^= uint64_t(text.size()) * 0xC2B2AE3D27D4EB4Full;
    k ^= uint64_t(uint32_t(int(max_w * 4.0f + 0.5f))) * 1315423911u;
    k ^= (uint64_t(uint32_t(int(pfs * 4.0f + 0.5f))) << 32);
    k ^= (uint64_t(uint32_t(weight & 0xFFFF)) << 9);
    k ^= wrap ? 1u : 0u;

    auto it = im->wrap_cache.find(k);
    if (it != im->wrap_cache.end()) {
        Ui::Impl::WrapEntry& e = it->second;
        if (e.max_w == max_w && e.pfs == pfs && e.weight == weight && e.wrap == wrap
            && e.text.size() == text.size()
            && std::memcmp(e.text.data(), text.data(), text.size()) == 0) {
            im->wrap_lru.splice(im->wrap_lru.begin(), im->wrap_lru, e.recency);
            std::vector<std::string>().swap(im->wrap_scratch);
            return e.lines;
        }
        im->wrap_bytes -= e.bytes;            // 过期条目直接换掉
        im->wrap_lru.erase(e.recency);
        im->wrap_cache.erase(it);
    }

    Ui::Impl::WrapEntry e;
    layout_wrapped_lines(im->text, text, max_w, pfs, weight, wrap, e.lines, im);
    e.text   = text;
    e.max_w  = max_w;
    e.pfs    = pfs;
    e.weight = weight;
    e.wrap   = wrap;
    e.bytes = sizeof(e) + e.text.capacity() + 1 + e.lines.capacity() * sizeof(std::string);
    for (const std::string& l : e.lines) e.bytes += l.capacity() + 1;

    static const size_t kBudget = memory_budget_kib("AINO_WRAP_BUDGET", size_t(6u << 20));
    if (e.bytes > kBudget) {
        im->wrap_scratch = std::move(e.lines);
        return im->wrap_scratch; // 不驱逐热缓存，也不将超大条目塞进预算。
    }

    // 先腾地方再插入：保证"刚算出来的这段"不会一进去就把自己挤掉
    while (im->wrap_bytes > kBudget - e.bytes && !im->wrap_lru.empty()) {
        auto o = im->wrap_cache.find(im->wrap_lru.back());
        im->wrap_bytes -= o->second.bytes;
        im->wrap_cache.erase(o);
        im->wrap_lru.pop_back();
        ++im->wrap_evictions;
    }

    im->wrap_bytes += e.bytes;
    im->wrap_lru.push_front(k);
    e.recency = im->wrap_lru.begin();
    auto ins = im->wrap_cache.emplace(k, std::move(e));
    std::vector<std::string>().swap(im->wrap_scratch);
    return ins.first->second.lines;
}

} // namespace detail

void Ui::draw_text(const std::string& text, const Rect& box, const Color& c, const Style& st) {
    detail::ProfTick _pt(&impl->pf_text_us, &impl->pf_text_n, impl->prof_on);
    if (impl->prof_on) impl->pf_text_ch += uint32_t(text.size());
    if (impl->measure_on) { if (c.a != 0) impl->measure_add(box); return; }
    if (!impl->canvas || !impl->text || text.empty() || c.a == 0) return;
    const float tscale = impl->xf_scale();
    const float fs = st.font_size > 0.0f ? st.font_size : impl->theme.font_size;
    const int weight = st.font_weight > 0 ? st.font_weight : 400;
    const float pfs = impl->px(fs) * tscale;      // 字体随变换缩放（不是把位图拉大）
    const float lh = impl->text->line_height(pfs, weight);
    const Rect pb = impl->xf_phys(box);

    // 文字裁剪到自身布局区域，防止内容越过容器边界。
    Rect clip = impl->effective_clip;
    if (clip.w >= 0.0f) {
        const float x0 = std::max(clip.x, pb.x), y0 = std::max(clip.y, pb.y);
        const float x1 = std::min(clip.right(), pb.right()), y1 = std::min(clip.bottom(), pb.bottom());
        clip = Rect(x0, y0, std::max(x1 - x0, 0.0f), std::max(y1 - y0, 0.0f));
    } else {
        clip = pb;
    }
    if (clip.w <= 0.0f || clip.h <= 0.0f) return;

    // 逻辑单位换算（旋转路径里按逻辑坐标定位，再映射中心点）
    const float inv = 1.0f / std::max(impl->scale * tscale, 0.0001f);

    // ---- 旋转路径 ----
    // 每一行按自己的中心点定位，交给 GDI 用 lfEscapement 原生旋转。
    // 多行也能转：行的排列方向在局部坐标系里仍然是竖着往下，整块一起转。
    if (impl->xf_rotated()) {
        const float angle = impl->xf_angle() * 57.2957795f;
        const std::vector<std::string>& lines =
            detail::wrap_layout(impl, text, impl->px(box.w) * tscale, pfs, weight, st.wrap);
        if (lines.empty()) return;
        const int   n      = int(lines.size());
        const float lh_log = lh * inv;
        const float step   = lh_log + st.line_gap;
        const float total  = step * float(n - 1) + lh_log;
        float y0;
        if (st.valign == VAlign::Middle)   y0 = box.cy() - total * 0.5f;
        else if (st.valign == VAlign::Top) y0 = box.y;
        else                               y0 = box.bottom() - total;
        for (int i = 0; i < n; ++i) {
            const std::string& ln = lines[size_t(i)];
            if (ln.empty()) continue;
            const float tw_log = impl->text->measure(ln, pfs, weight) * inv;
            float x = box.x;
            if (st.align == Align::Center)     x = box.cx() - tw_log * 0.5f;
            else if (st.align == Align::Right) x = box.right() - tw_log;
            const Vec2 ctr = impl->xf_map(x + tw_log * 0.5f, y0 + step * float(i) + lh_log * 0.5f);
            impl->text->draw_rot(*impl->canvas, ln, ctr.x, ctr.y, pfs, weight, c, clip, angle);
        }
        return;
    }

    // 快路径：单行 + 不换行 —— 绝大多数调用走这里，行为与从前逐字节一致。
    if (!st.wrap && text.find('\n') == std::string::npos) {
        const float tw = impl->text->measure(text, pfs, weight);
        float x = pb.x;
        if (st.align == Align::Center)     x = pb.cx() - tw * 0.5f;
        else if (st.align == Align::Right) x = pb.right() - tw;
        float y_top;
        if (st.valign == VAlign::Middle)      y_top = pb.cy() - lh * 0.5f;
        else if (st.valign == VAlign::Top)    y_top = pb.y;
        else                                  y_top = pb.bottom() - lh;
        impl->text->draw(*impl->canvas, text, x, y_top, pfs, weight, c, clip);
        return;
    }

    // 多行：先按 '\n' 切逻辑行，再（可选）按可用宽度折行。结果进缓存 ——
    // 静态文本每帧只是命中一次，不再重排整段。
    const std::vector<std::string>& lines =
        detail::wrap_layout(impl, text, pb.w, pfs, weight, st.wrap);
    if (lines.empty()) return;
    const float step = lh + impl->px(st.line_gap) * tscale;
    const int   n    = int(lines.size());
    const float total = step * float(n - 1) + lh;

    float y0;
    if (st.valign == VAlign::Middle)   y0 = pb.cy() - total * 0.5f;
    else if (st.valign == VAlign::Top) y0 = pb.y;
    else                               y0 = pb.bottom() - total;

    // 只画落在裁剪区里的行。滚动区的长文档里一屏通常只有几十行可见，
    // 但 lines 可能上万行；全量交给 GDI 是纯浪费（每行一次 UTF-8→UTF-16 转换）。
    int i0 = 0, i1 = n - 1;
    if (step > 0.01f) {
        i0 = detail::clampi(int(std::floor((clip.y - y0) / step)) - 1, 0, n - 1);
        i1 = detail::clampi(int(std::ceil((clip.bottom() - y0) / step)) + 1, 0, n - 1);
    }
    for (int i = i0; i <= i1; ++i) {
        const std::string& ln = lines[size_t(i)];
        if (ln.empty()) continue;
        const float tw = impl->text->measure(ln, pfs, weight);
        float x = pb.x;
        if (st.align == Align::Center)     x = pb.cx() - tw * 0.5f;
        else if (st.align == Align::Right) x = pb.right() - tw;
        impl->text->draw(*impl->canvas, ln, x, y0 + step * float(i), pfs, weight, c, clip);
    }
}

// -----------------------------------------------------------------------------
//  多行文字的实际高度 —— 和 draw_text 用同一套折行规则，所以是"量出来的"，
//  不是"估出来的"。给组件留高度时：width 用 text_width()，height 用 text_height()。
// -----------------------------------------------------------------------------
float Ui::text_height(const std::string& text, float width, const Style& st) const {
    if (text.empty() || width <= 1.0f) return 0.0f;
    const float fs = st.font_size > 0.0f ? st.font_size : impl->theme.font_size;
    const int   wt = st.font_weight > 0 ? st.font_weight : 400;
    const std::vector<std::string>& lines =
        detail::wrap_layout(impl, text, impl->px(width), impl->px(fs), wt, st.wrap);
    const size_t n = lines.size();
    if (n == 0) return 0.0f;
    const float lh   = line_height(fs);
    const float step = lh + impl->px(st.line_gap);          // 与 draw_text 完全一致
    return (float(n) - 1.0f) * step + lh;
}

// 计算给定宽度下的 alert 高度；正文可折行，标题按单行裁剪。
float Ui::alert_height(const std::string& title, const std::string& description,
                              float width, const Style& st) const {
    const float fs  = st.font_size > 0.0f ? st.font_size : impl->theme.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : detail::kAlertPad;
    const float d   = fs + 3.0f;                             // 符号直径（见 alert 里的自绘）
    const float tw  = std::max(width - pad * 2.0f - d - 9.0f, 1.0f);
    const float lh  = line_height(fs);
    Style ds = st;
    ds.font_size = fs;
    ds.font_weight = 400;
    ds.wrap = true;
    const float desc_h = description.empty() ? 0.0f : text_height(description, tw, ds);
    const float title_h = title.empty() ? 0.0f : lh;
    return pad * 2.0f + title_h + (description.empty() ? 0.0f : 2.0f) + desc_h;
}

// =============================================================================
//  8.7  轻量布局（立即模式下的顺序布局）
// =============================================================================
// ratio = 宽 / 高。ratio <= 0 时原样返回（别把高度算成 inf）。
Rect Ui::aspect_rect(const Rect& box, float ratio) {
    Rect r = box;
    if (ratio > 0.0f) r.h = std::floor(box.w / ratio + 0.5f);
    return r;
}

// -----------------------------------------------------------------------------
//  Stack 实现
//  整个布局器只做加法和一次比较：不持有控件、不注册任何东西、不需要帧间
//  状态，所以嵌套、临时丢弃、值拷贝都是零代价的。
// -----------------------------------------------------------------------------
void Stack::advance(float d) {
    if (vertical_) { area.y += d; area.h = std::max(0.0f, area.h - d); }
    else           { area.x += d; area.w = std::max(0.0f, area.w - d); }
}

// 结算"待垫的 gap"后的区域。rest / remain / part 都基于它，这样"剩余空间"
// 始终指"下一个元素开始的位置"，而不是游标当前所在的（还差一个间距的）位置。
Rect Stack::rest() const {
    Rect a = area;
    if (pending_) {
        if (vertical_) { a.y += gap; a.h = std::max(0.0f, a.h - gap); }
        else           { a.x += gap; a.w = std::max(0.0f, a.w - gap); }
    }
    return a;
}

Rect Stack::next(float size) {
    if (pending_) { advance(gap); pending_ = false; }   // 间距只在元素之间垫
    float s = std::max(0.0f, size);
    Rect r;
    if (vertical_) {
        s = std::min(s, area.h);
        r = Rect(area.x, area.y, area.w, s);
    } else {
        s = std::min(s, area.w);
        r = Rect(area.x, area.y, s, area.h);
    }
    advance(s);
    pending_ = true;
    return r;
}

Rect Stack::next_frac(float fraction) {
    if (pending_) { advance(gap); pending_ = false; }
    const float span = vertical_ ? area.h : area.w;
    return next(std::max(0.0f, span * fraction));
}

Rect Stack::next_text(const std::string& s, float extra, float font_size, int weight) {
    float w = 0.0f;
    if (ui && !s.empty()) {
        const float fs = font_size > 0.0f ? font_size : ui->theme().font_size;
        w = ui->text_width(s, fs, weight);       // 已是逻辑单位
    }
    return next(w + extra);
}

Rect Stack::fill() {
    if (pending_) { advance(gap); pending_ = false; }
    Rect r = area;
    advance(vertical_ ? area.h : area.w);
    return r;
}

void Stack::skip(float amount) {
    if (pending_) { advance(gap); pending_ = false; }
    advance(std::max(0.0f, amount));
}

Rect Stack::part(int index, int count) const {
    if (count <= 0 || index < 0 || index >= count) return Rect();
    const Rect  a      = rest();
    const float span   = vertical_ ? a.h : a.w;
    const float usable = span - gap * float(count - 1);   // 份间的 gap 先扣掉
    if (usable <= 0.0f) return Rect();
    const float each = usable / float(count);
    const float off  = float(index) * (each + gap);
    return vertical_ ? Rect(a.x, a.y + off, a.w, each)
                     : Rect(a.x + off, a.y, each, a.h);
}

Stack Stack::vbox(const Rect& box, float g) const {
    return Stack(ui, box, true,  g >= 0.0f ? g : gap);
}
Stack Stack::hbox(const Rect& box, float g) const {
    return Stack(ui, box, false, g >= 0.0f ? g : gap);
}
Stack Stack::vbox(float size, float g) { return vbox(next(size), g); }
Stack Stack::hbox(float size, float g) { return hbox(next(size), g); }

Stack Ui::vbox(const Rect& box, float gap, float pad) {
    return Stack(this, pad > 0.0f ? box.inset(pad) : box, true, gap);
}
Stack Ui::hbox(const Rect& box, float gap, float pad) {
    return Stack(this, pad > 0.0f ? box.inset(pad) : box, false, gap);
}

void Ui::push_clip(const Rect& r) {
    // 裁剪区跟着变换走（旋转时用包围盒 —— 轴对齐的裁剪区表达不了旋转矩形，
    // 这一点在文档里写明；要精确裁旋转内容就自己用多边形，或者干脆别套裁剪）
    impl->clips.push_back(impl->xf_phys(r));
    impl->recompute_clip();
    // 命中测试用的那份不跟变换走：Transform 只影响绘制，不影响布局与命中。
    impl->clip_logic.push_back(r);
    impl->recompute_clip_logic();
}

void Ui::pop_clip() {
    if (!impl->clips.empty()) impl->clips.pop_back();
    impl->recompute_clip();
    if (!impl->clip_logic.empty()) impl->clip_logic.pop_back();
    impl->recompute_clip_logic();
}

// =============================================================================
//  8.8  组件通用工具
// =============================================================================
namespace detail {

// 一个组件"皮肤"的完整解析结果：base/hover/press 三态 + 前景 + 描边
struct Skin {
    Color base   = Color::none();
    Color hover  = Color::none();
    Color press  = Color::none();
    Color fg     = Color(0, 0, 0);
    Color border = Color::none();
    Color rim    = Color::none();      // 顶部 1px 高光（bezel 的上亮边）
    float border_w = 0.0f;
    float radius   = 8.0f;
    float font_size= 14.0f;
    int   weight   = 400;
};

inline Skin make_skin(const Theme& th, const Style& st, int default_variant, float default_radius) {
    const int v = st.variant >= 0 ? st.variant : default_variant;
    Skin sk;
    sk.radius    = st.radius >= 0.0f ? st.radius : default_radius;
    sk.font_size = st.font_size > 0.0f ? st.font_size : th.font_size;
    sk.weight    = st.font_weight > 0 ? st.font_weight : 500;   // 参考资料 kit：推按钮标签是 SF Pro Medium
    sk.fg        = th.label;

    // Default 使用强调色，Standard 为中性实心表面，Secondary 为半透明填充。
    switch (v) {
        case Style::Default:
            sk.base  = th.tint;  sk.fg = th.tint_foreground;
            sk.hover = th.tint;
            sk.press = th.tint.darken(0.12f);
            sk.rim   = Color(255, 255, 255, 40);
            break;
        case Style::Secondary:
            // kit `Buttons/Bordered` 的 BG 是 **8%**（Fills - Opaque/Secondary），
            // 不是 10% —— 10% 会让一整排按钮看着比工具栏重一档。
            sk.base = th.fill2;  sk.hover = th.fill2;  sk.press = th.fill_press;
            sk.fg   = th.label;
            break;
        case Style::Standard:
            sk.base = th.fill_hover.over(th.control_bg);
            sk.hover = sk.base;
            sk.press = th.fill_press.over(sk.base);
            break;
        case Style::Outline:
            // Outline 保留细描边，以区别于 Secondary 的半透明填充。
            sk.base = th.fill2; sk.hover = th.fill2; sk.press = th.fill_press;
            sk.fg   = th.label;
            sk.border = th.separator; sk.border_w = 1.0f;
            break;
        case Style::Ghost:
            sk.base = Color::none(); sk.hover = th.fill3; sk.press = th.fill2;
            sk.fg   = th.label;
            break;
        case Style::Destructive:
            // 危险操作使用中性填充与红色标签。
            sk.base  = th.fill2; sk.fg = th.red;
            sk.hover = th.fill2;
            sk.press = th.fill_press;
            break;
        case Style::Link:
            sk.base = Color::none(); sk.hover = Color::none(); sk.press = Color::none();
            sk.fg   = th.tint;
            break;
        default:
            sk.base = th.fill; sk.hover = th.fill; sk.press = th.fill_press;
            break;
    }

    // 显式覆盖优先
    if (!st.bg.is_none()) { sk.base = st.bg; sk.hover = st.bg; sk.press = press_color(st.bg); }
    if (!st.fg.is_none()) sk.fg = st.fg;
    if (st.muted) sk.fg = th.label2;
    if (!st.border_color.is_none()) { sk.border = st.border_color; if (sk.border_w <= 0.0f) sk.border_w = 1.0f; }
    if (st.border_width >= 0.0f) sk.border_w = st.border_width;
    return sk;
}

inline void paint_surface(Ui& ui, const Rect& r, const Skin& sk, float t_hover, float t_press,
                          bool disabled, float radius_override = -1.0f) {
    const float rad = radius_override >= 0.0f ? radius_override : sk.radius;
    Color bg = state_fill(sk.base, sk.hover, sk.press, t_hover, t_press);
    const float dim = disabled ? 0.55f : 1.0f;
    if (disabled && bg.a > 0 && bg.a < 255) bg.a = uint8_t(float(bg.a) * dim);
    if (disabled && bg.a == 255) bg = bg.with_alpha(uint8_t(255.0f * 0.55f));

    if (bg.a > 0) {
        ui.fill_rect(r, bg, rad);
    }
    // 顶部高光：只在上半圈画 1px 亮边。整圈描边会立刻变成"描边框"的 Web 味。
    if (!sk.rim.is_none() && bg.a > 0) {
        Color rc = sk.rim;
        if (disabled) rc.a = uint8_t(float(rc.a) * dim);
        ui.stroke_rect_top(r, rc, rad, 1.0f, std::max(r.h * 0.5f, 2.0f));
    }
    if (sk.border_w > 0.0f && !sk.border.is_none()) {
        Color bc = sk.border;
        if (disabled) bc.a = uint8_t(float(bc.a) * dim);
        ui.stroke_rect_s(r, bc, rad, sk.border_w, -1.0f);
    }
}

// 弹层使用紧轮廓与柔光两层投影，降低紧轮廓不透明度以避免深色描边。level 0 为菜单，1 为浮卡，2 为模态。
inline void pop_shadow(Ui& ui, const Rect& r, float rad, const Theme& th, int level = 1) {
    const bool dark = th.window_bg.luminance() < 0.5f;
    if (!dark) {
        ui.drop_shadow(r, rad, 5.0f, Color(0, 0, 0, 40), 0.5f);
        if (level >= 2)      ui.drop_shadow(r, rad, 90.0f, Color(0, 0, 0, 82), 30.0f);
        else if (level == 1) ui.drop_shadow(r, rad, 44.0f, Color(0, 0, 0, 56), 10.0f);
        else                 ui.drop_shadow(r, rad, 30.0f, Color(0, 0, 0, 52), 5.0f);
    } else {
        // `0 0 1 spread 1 #000`：外扩 1px 的实心黑。用 1px 描边画，不能拿
        // blur=1 的柔光冒充 —— 高斯之后边缘只剩半格，暗色下会"切"不出来。
        ui.stroke_rect(r.expand(0.5f), Color(0, 0, 0, 255), rad + 0.5f, 1.0f);
        if (level >= 2)      ui.drop_shadow(r, rad, 90.0f, Color(0, 0, 0, 150), 30.0f);
        else if (level == 1) ui.drop_shadow(r, rad, 44.0f, Color(0, 0, 0, 140), 10.0f);
        else                 ui.drop_shadow(r, rad, 30.0f, Color(0, 0, 0, 120), 5.0f);
    }
}

// NSPopUpButton 右侧的"上下双箭头"。这是 mac 下拉按钮最容易被忽略的一处特征：
// 一个向下箭头是 Web 的 <select>，一上一下两个才是 Pop-Up Button。
inline void paint_popup_chevrons(Ui& ui, const Rect& box, float pad, const Color& c) {
    const float cx = box.right() - pad - 3.0f, cy = box.cy();
    ui.system_icon(SystemIcon::ChevronUp, Rect::centered(cx, cy - 3.6f, 8, 8), c);
    ui.system_icon(SystemIcon::ChevronDown, Rect::centered(cx, cy + 3.6f, 8, 8), c);
}

// for_text = 文本类控件（点进去就有环，和系统一致）；其余控件只在键盘焦点时画。
inline void paint_focus_ring(Ui& ui, const Rect& r, float radius, const Color& ring,
                             bool for_text = false) {
    if (!for_text && ui.impl->focus_by_mouse) return;
    // 焦点环在控件外绘制，宽度 3pt，不占布局空间。
    ui.stroke_rect(r.expand(2.0f), ring.scaled_alpha(0.85f), radius + 2.0f, 3.0f);
}

// 叠加色（th.fill3 / th.fill 这类自带 alpha 的"面"）按 t 淡入。
// ⚠ 不能用 with_alpha(255*t)：那是**替换** alpha，会把 13% 的叠加色变成 100%
// 不透明 —— 深色主题下直接变成一块纯白实心矩形把文字盖死。必须缩现有 alpha。
inline Color on_color(const Color& bg);   // 反色文字（定义在后面）

// menu、menubar、context_menu、select、combobox 共用菜单绘制。所有行预留勾选列，避免文字位置变化。
inline void paint_menu(Ui& ui, const Rect& list,
                       const std::vector<std::string>& items,
                       const std::vector<Rect>& rects,
                       const std::vector<uint64_t>& ids,
                       const std::vector<uint8_t>& hov,
                       int selected, bool show_check, float fs) {
    const Theme& th = ui.theme();
    const float r = 12.0f;
    pop_shadow(ui, list, r, th, 0);          // 菜单 = Quaternary 投影
    ui.fill_rect(list, th.popover_bg, r);
    // 不描边：mac 的菜单/气泡只有投影，一圈 1px 线是 Web 的下拉面板

    ui.push_clip(list);
    // 菜单行使用中性色悬停高亮。
    const Color hover_bg = th.is_dark() ? Color::hex(0x141414) : Color::hex(0xE6E6E6);
    for (size_t i = 0; i < rects.size() && i < items.size(); ++i) {
        const float t_h = animate_value(ui.impl, ids[i] * 83u + 1u, hov[i] ? 1.0f : 0.0f, 20.0f);
        if (t_h > 0.02f) ui.fill_rect_s(rects[i], hover_bg.scaled_alpha(t_h), 8.0f, 0.35f);
        const Color fg = th.label;
        const float left = rects[i].x + (show_check ? 24.0f : 10.0f);
        if (show_check && selected == int(i)) {
            const float ccx = rects[i].x + 11.0f, ccy = rects[i].cy(), sc = 3.1f;
            ui.draw_line(ccx - sc, ccy, ccx - sc * 0.15f, ccy + sc * 0.8f, fg, 1.6f);
            ui.draw_line(ccx - sc * 0.15f, ccy + sc * 0.8f, ccx + sc * 1.1f, ccy - sc * 0.85f, fg, 1.6f);
        }
        Style its;
        its.font_size = fs;
        its.font_weight = 500;                 // kit：选中与未选中都是 Medium 13
        its.align = Align::Left;
        its.valign = VAlign::Middle;
        ui.draw_text(items[i], Rect(left, rects[i].y, std::max(rects[i].right() - left - 8.0f, 1.0f),
                                    rects[i].h), fg, its);
    }
    ui.pop_clip();
}

inline Color fade(const Color& c, float t) {
    return c.scaled_alpha(t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t));
}

// 实心强调色上的文字。亮强调色（黄/绿/橙）上用黑字，暗的用白字 ——
// 参考资料 kit 里黄色 Selected 段就是黑字，一律白字在浅色强调色上会糊。
inline Color on_color(const Color& bg) {
    return bg.luminance() > 0.55f ? Color(0, 0, 0, 217) : Color(255, 255, 255);
}

// 把 Style 中与文字相关的部分转成 draw_text 用的 Style
inline Style text_style_of(const Style& st, const Skin& sk, int default_weight) {
    Style ts;
    ts.font_size   = sk.font_size;
    ts.font_weight = sk.weight > 0 ? sk.weight : default_weight;
    ts.align       = st.align;
    ts.valign      = st.valign;
    ts.align_set   = st.align_set;
    ts.valign_set  = st.valign_set;
    ts.italic      = st.italic;
    return ts;
}

inline bool is_focused(Ui::Impl* im, uint64_t id) {
    return !im->measure_on && im->focus == reinterpret_cast<void*>(static_cast<uintptr_t>(id));
}
inline void grab_focus(Ui::Impl* im, uint64_t id) {
    if (im->focus_id != id || im->focus_scope_id != im->state_scope_id) im->paste.reset();
    im->focus = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
    im->focus_id = id;
    im->focus_scope_id = im->state_scope_id;
    im->caret_blink = 0.0f;
    im->focus_by_mouse = im->input.pressed[0] || im->input.pressed[1];
}
inline void drop_focus(Ui::Impl* im) {
    im->focus = nullptr; im->focus_id = 0; im->focus_scope_id = 0; im->paste.reset();
}

inline bool editor_focused(Ui::Impl* im, uint64_t id, bool enabled) {
    if (!is_focused(im, id)) return false;
    if (!enabled) { drop_focus(im); return false; }
    return true;
}

// -----------------------------------------------------------------------------
//  文本编辑小工具（input 组件用）
// -----------------------------------------------------------------------------
// 由横坐标反查光标应落在第几个码点
inline int caret_from_x(const Ui& ui, const std::string& s, float local_x, float font_size, int weight) {
    if (!ui.impl->text) return 0;
    std::vector<float> positions;
    ui.impl->text->caret_positions(s, font_size * ui.dpi_scale(), weight, positions);
    int best = 0;
    float best_d = 1e9f;
    for (size_t i = 0; i < positions.size(); ++i) {
        float d = std::fabs(positions[i] / ui.dpi_scale() - local_x);
        if (d < best_d) { best_d = d; best = int(i); }
    }
    return best;
}

// input 与 textarea 共用按词跳转和撤销栈。词类型：0 为空白或标点，1 为西文词，2 为 CJK 字符。
inline int cp_class(uint32_t c) {
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return 0;
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') return 1;
    if (c >= 0x2E80) return 2;
    return 0;
}

inline int word_left(const std::vector<uint32_t>& cps, int i) {
    int n = int(cps.size());
    if (i > n) i = n;
    if (i <= 0) return 0;
    while (i > 0 && cp_class(cps[size_t(i - 1)]) == 0) --i;
    if (i == 0) return 0;
    const int cls = cp_class(cps[size_t(i - 1)]);
    if (cls == 2) return i - 1;                       // 中文一个字就是一步
    while (i > 0 && cp_class(cps[size_t(i - 1)]) == cls) --i;
    return i;
}

inline int word_right(const std::vector<uint32_t>& cps, int i) {
    const int n = int(cps.size());
    if (i < 0) i = 0;
    if (i >= n) return n;
    const int cls = cp_class(cps[size_t(i)]);
    if (cls == 0) {
        while (i < n && cp_class(cps[size_t(i)]) == 0) ++i;
        return i;
    }
    if (cls == 2) return i + 1;
    while (i < n && cp_class(cps[size_t(i)]) == cls) ++i;
    return i;
}

// 双击选词：返回 [a, b)；点在空白处时选中那一段空白
inline void word_span(const std::vector<uint32_t>& cps, int at, int* a, int* b) {
    const int n = int(cps.size());
    if (n == 0) { *a = 0; *b = 0; return; }
    if (at >= n) at = n - 1;
    const int cls = cp_class(cps[size_t(at)]);
    int lo = at, hi = at + 1;
    if (cls == 2) { *a = at; *b = at + 1; return; }
    while (lo > 0 && cp_class(cps[size_t(lo - 1)]) == cls) --lo;
    while (hi < n && cp_class(cps[size_t(hi)]) == cls) ++hi;
    *a = lo;
    *b = hi;
}

enum : uint64_t { EDIT_TYPE = 1, EDIT_DELETE = 2, EDIT_OTHER = 3 };

// 压一份"改动前"的快照。连续同类输入（0.7s 内）合并成一步 —— 否则打 20 个字
// 要按 20 次 Ctrl+Z 才能退掉一句话，这是编辑器最让人烦的细节之一。
inline size_t edit_history_bytes(const Ui::Impl::EditStack& st) {
    size_t bytes = 0;
    for (const auto& s : st.undo) bytes += sizeof(s) + s.text.capacity() + 1;
    for (const auto& s : st.redo) bytes += sizeof(s) + s.text.capacity() + 1;
    return bytes;
}
inline size_t edit_history_budget() {
    static const size_t budget = memory_budget_kib("AINO_UNDO_BUDGET", size_t(4u << 20));
    return budget;
}
inline void trim_edit_history(Ui::Impl::EditStack& st) {
    const size_t budget = edit_history_budget();
    size_t bytes = edit_history_bytes(st);
    while (st.undo.size() + st.redo.size() > 200 || bytes > budget) {
        // 两端只删最远历史，留下的撤销/重做链仍连续；空栈不参与选择。
        auto& stack = st.undo.size() > st.redo.size() ? st.undo : st.redo;
        if (stack.empty()) break;
        bytes -= sizeof(stack.front()) + stack.front().text.capacity() + 1;
        stack.pop_front();
    }
}

inline void edit_snapshot(Ui::Impl* im, uint64_t id, const std::string& before,
                          int caret0, int anchor0, uint64_t kind) {
    auto& st = im->edit_stack[id];
    const double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (before.size() > edit_history_budget() - sizeof(Ui::Impl::EditSnap) - 1) {
        st.undo.clear();
        st.redo.clear();
        st.kind = 0;
        st.last = now;
        return;
    }
    if (kind != EDIT_OTHER && st.kind == kind && !st.undo.empty() && (now - st.last) < 0.7f) {
        st.last = now;
        st.redo.clear();
        return;
    }
    st.undo.push_back(Ui::Impl::EditSnap{before, caret0, anchor0});
    st.redo.clear();
    trim_edit_history(st);
    st.last = now;
    st.kind = kind;
}

inline bool edit_undo(Ui::Impl* im, uint64_t id, std::string* text, int* caret, int* anchor) {
    auto* saved = im->edit_stack.get_if(id);
    if (!saved || saved->undo.empty()) return false;
    auto& st = *saved;
    st.redo.push_back(Ui::Impl::EditSnap{*text, *caret, *anchor});
    Ui::Impl::EditSnap s = std::move(st.undo.back());
    st.undo.pop_back();
    *text = std::move(s.text);
    *caret = s.caret;
    *anchor = s.anchor;
    trim_edit_history(st);
    st.kind = 0;
    st.last = -1e9f;                       // 撤销之后不再和后续输入合并
    if (im->request_repaint) im->request_repaint();
    return true;
}

inline bool edit_redo(Ui::Impl* im, uint64_t id, std::string* text, int* caret, int* anchor) {
    auto* saved = im->edit_stack.get_if(id);
    if (!saved || saved->redo.empty()) return false;
    auto& st = *saved;
    st.undo.push_back(Ui::Impl::EditSnap{*text, *caret, *anchor});
    Ui::Impl::EditSnap s = std::move(st.redo.back());
    st.redo.pop_back();
    *text = std::move(s.text);
    *caret = s.caret;
    *anchor = s.anchor;
    trim_edit_history(st);
    st.kind = 0;
    st.last = -1e9f;
    if (im->request_repaint) im->request_repaint();
    return true;
}

} // namespace detail

// =============================================================================
//  8.9  文本类组件
// =============================================================================
void Ui::label(const std::string& text, const Rect& box, const Style& st) {
    Style s = st;
    if (s.font_size <= 0.0f) s.font_size = impl->theme.font_size;
    if (s.font_weight <= 0) s.font_weight = 400;
    const Color c = !s.fg.is_none() ? s.fg
                  : (s.muted ? impl->theme.label2 : impl->theme.label);
    draw_text(text, box, c, s);
}

void Ui::heading(const std::string& text, const Rect& box, const Style& st) {
    Style s = st;
    // 默认标题使用 22pt Bold。
    if (s.font_size <= 0.0f) s.font_size = 22.0f;
    if (s.font_weight <= 0) s.font_weight = 700;
    const Color c = !s.fg.is_none() ? s.fg
                  : (s.muted ? impl->theme.label2 : impl->theme.label);
    draw_text(text, box, c, s);
}

void Ui::subheading(const std::string& text, const Rect& box, const Style& st) {
    Style s = st;
    // Title 3 = 15/20 Semibold
    if (s.font_size <= 0.0f) s.font_size = 15.0f;
    if (s.font_weight <= 0) s.font_weight = 600;
    const Color c = !s.fg.is_none() ? s.fg
                  : (s.muted ? impl->theme.label2 : impl->theme.label);
    draw_text(text, box, c, s);
}

void Ui::muted_text(const std::string& text, const Rect& box, const Style& st) {
    Style s = st;
    if (s.font_size <= 0.0f) s.font_size = impl->theme.font_size;
    if (s.font_weight <= 0) s.font_weight = 400;
    const Color c = !s.fg.is_none() ? s.fg : impl->theme.label2;
    draw_text(text, box, c, s);
}

void Ui::code_text(const std::string& text, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size - 1.0f;
    const Color bg  = !st.bg.is_none() ? st.bg : th.fill3;
    const float pad = 6.0f;
    fill_rect(box, bg, rad);
    if (st.border_width > 0.0f && !st.border_color.is_none())
        stroke_rect(box, st.border_color, rad, st.border_width);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 400;
    ts.align = st.align_set ? st.align : Align::Left;
    ts.valign = VAlign::Middle;
    const Color c = !st.fg.is_none() ? st.fg : th.label;
    draw_text(text, box.inset(pad, 0.0f), c, ts);
}

// =============================================================================
//  8.9.1  代码块（只读）
//
//  画廊、文档站这类程序要展示源码，所以这里带一个最小词法器：拆行 → 每行切段
//  → 逐段着色。切段不只是为了颜色，同色相邻字符合并后，一行十几次 GDI 调用
//  能压到三五次。
//
//  不做完整语法解析：代码块是给人看的，颜色只用来帮眼睛分块，认错一个标识符
//  的类别不会有什么后果。字符串/注释/数字这三类是必须正确的，其余尽力。
// =============================================================================
namespace detail {

enum class TokKind { Text, Keyword, Type, String, Comment, Number };

struct CodeSeg {
    TokKind kind = TokKind::Text;
    std::string text;
};

struct CodeColors {
    Color plain, keyword, type, str, comment, number, gutter;
};

// 两套色板按背景亮度选。跟着主题走而不是跟着 style 走 —— 调用方通常只想说
// "用当前主题的代码块"，不想每次自己配七个颜色。
inline const CodeColors& code_colors(const Theme& th) {
    static const CodeColors kLight = {
        Color(0x24, 0x28, 0x2e),   // plain
        Color(0x82, 0x50, 0xdf),   // keyword
        Color(0x0b, 0x63, 0xa8),   // type
        Color(0x0a, 0x7d, 0x32),   // string
        Color(0x8a, 0x91, 0x9c),   // comment
        Color(0xb4, 0x53, 0x09),   // number
        Color(0x9a, 0xa1, 0xac),   // gutter
    };
    static const CodeColors kDark = {
        Color(0xe6, 0xed, 0xf3),
        Color(0xd2, 0xa8, 0xff),
        Color(0x79, 0xc0, 0xff),
        Color(0x7e, 0xe7, 0x87),
        Color(0x8b, 0x94, 0x9e),
        Color(0xff, 0xab, 0x70),
        Color(0x6a, 0x73, 0x7d),
    };
    const int luma = (th.window_bg.r * 299 + th.window_bg.g * 587 + th.window_bg.b * 114) / 1000;
    return luma > 128 ? kLight : kDark;
}

inline bool tok_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
inline bool tok_ident(char c) {
    return tok_ident_start(c) || (c >= '0' && c <= '9');
}
inline bool tok_digit(char c) { return c >= '0' && c <= '9'; }

inline bool tok_in(const char* const* tab, size_t n, const std::string& w) {
    for (size_t i = 0; i < n; ++i) if (w == tab[i]) return true;
    return false;
}

inline bool tok_is_keyword(const std::string& w) {
    static const char* const kTab[] = {
        "bool", "int", "float", "double", "char", "void", "auto", "const", "constexpr",
        "static", "inline", "virtual", "override", "struct", "class", "enum", "namespace",
        "using", "return", "if", "else", "for", "while", "switch", "case", "default",
        "break", "continue", "true", "false", "nullptr", "new", "delete", "public",
        "private", "protected", "template", "typename", "sizeof", "explicit", "operator",
        "this", "extern", "union", "friend",
    };
    return tok_in(kTab, sizeof(kTab) / sizeof(kTab[0]), w);
}

inline bool tok_is_type(const std::string& w) {
    static const char* const kTab[] = {
        "std", "string", "vector", "function", "array", "pair", "map", "size_t",
        "uint32_t", "uint64_t", "int32_t", "aino",
        "Ui", "Style", "Rect", "Stack", "Color", "Vec2", "Transform", "Window",
        "Input", "Theme", "Canvas", "Easing", "Align", "VAlign", "Cursor", "MenuPick",
    };
    return tok_in(kTab, sizeof(kTab) / sizeof(kTab[0]), w);
}

// 把一行切成同色段。相邻且同类会并进上一段（push 里做）。
inline void code_lex(const std::string& s, std::vector<CodeSeg>& out) {
    out.clear();
    const size_t n = s.size();
    size_t i = 0;

    auto push = [&](TokKind k, size_t a, size_t b) {
        if (b <= a) return;
        if (!out.empty() && out.back().kind == k) {
            out.back().text.append(s, a, b - a);
        } else {
            out.push_back(CodeSeg{k, s.substr(a, b - a)});
        }
    };

    while (i < n) {
        const char c = s[i];

        // 行注释：整行剩余部分一个颜色
        if (c == '/' && i + 1 < n && s[i + 1] == '/') {
            push(TokKind::Comment, i, n);
            break;
        }

        // 字符串 / 字符字面量：只认转义，不认跨行（示例代码里没有裸换行串）
        if (c == '"' || c == '\'') {
            const char q = c;
            size_t j = i + 1;
            while (j < n) {
                if (s[j] == '\\' && j + 1 < n) { j += 2; continue; }
                if (s[j] == q) { ++j; break; }
                ++j;
            }
            push(TokKind::String, i, j);
            i = j;
            continue;
        }

        // 数字：0xFF / 3.14f / 1e7 一起吃掉（后缀字母靠 tok_ident 一并吞下）
        if (tok_digit(c)) {
            size_t j = i;
            while (j < n && (tok_ident(s[j]) || s[j] == '.')) ++j;
            push(TokKind::Number, i, j);
            i = j;
            continue;
        }

        if (tok_ident_start(c)) {
            size_t j = i;
            while (j < n && tok_ident(s[j])) ++j;
            const std::string w = s.substr(i, j - i);
            push(tok_is_keyword(w) ? TokKind::Keyword
                 : tok_is_type(w) ? TokKind::Type
                 : TokKind::Text, i, j);
            i = j;
            continue;
        }

        // 标点与空白：连续吃掉，避免每个括号都单独成段
        size_t j = i + 1;
        while (j < n && !tok_ident_start(s[j]) && !tok_digit(s[j])
               && s[j] != '"' && s[j] != '\''
               && !(s[j] == '/' && j + 1 < n && s[j + 1] == '/')) ++j;
        push(TokKind::Text, i, j);
        i = j;
    }
}

inline int code_line_count(const std::string& code) {
    if (code.empty()) return 0;
    int n = 1;
    for (char c : code) if (c == '\n') ++n;
    // 末尾带换行时最后那个空行不占位
    if (code.back() == '\n') --n;
    return n;
}

inline std::string code_line_at(const std::string& code, int index) {
    int cur = 0;
    size_t a = 0;
    for (size_t i = 0; i <= code.size(); ++i) {
        if (i == code.size() || code[i] == '\n') {
            if (cur == index) {
                size_t b = i;
                if (b > a && code[b - 1] == '\r') --b;
                return code.substr(a, b - a);
            }
            ++cur;
            a = i + 1;
            if (i == code.size()) break;
        }
    }
    return std::string();
}

} // namespace detail

float Ui::code_block_height(const std::string& code, const Style& st) const {
    const float fs = st.font_size > 0.0f ? st.font_size : 12.5f;
    const float lh = std::max(fs * 1.62f, 15.0f);
    return 10.0f + lh * float(detail::code_line_count(code)) + 10.0f;
}

void Ui::code_block(const std::string& code, const Rect& box, const Style& st) {
    // 代码块自己画文字（走 TextRenderer 的 mono 通道，不经过 draw_text），
    // 所以测量模式要在这里单独拦一道，否则测量阶段会真把字画到屏幕上。
    if (impl->measure_on) { impl->measure_add(box); return; }
    if (!impl->canvas || !impl->text) return;
    const Theme& th = impl->theme;

    const float fs   = st.font_size > 0.0f ? st.font_size : 12.5f;
    const float lh   = std::max(fs * 1.62f, 15.0f);
    const float rad  = st.radius >= 0.0f ? st.radius : 8.0f;
    // macOS 的源码/文本块是"凹陷面"：比背景**暗**一档、没有描边。一圈 1px 边框
    // 是 Web 卡片语言，macOS 用明度差分层。
    // 注意暗色下也要压暗（叠黑），不是叠白 —— 叠白会让它比背景更亮，看起来像
    // 又一张卡片浮在 Group Box 上，整页就成了"卡片套卡片"。
    // 暗色的窗口底已经是 #1E1E1E，留给"更暗"的空间只有几级：#1E 上叠 10% 黑只差
    // 3 级，等于没画。所以暗色压到 25%（#1E → #16，差 8 级），浅色仍是 3%。
    const Color bg   = !st.bg.is_none() ? st.bg
                     : (th.window_bg.luminance() < 0.5f ? Color(0, 0, 0, 64) : Color(0, 0, 0, 8));
    const detail::CodeColors& cc = detail::code_colors(th);

    fill_rect(box, bg, rad);

    const int total = detail::code_line_count(code);
    if (total <= 0) return;

    // 行号列宽按最大行号量，右侧留 10px 间隙
    const float pfs   = impl->px(fs);
    const float pfs_s = impl->px(fs - 1.0f);
    const std::string max_num = std::to_string(total);
    const float num_w = impl->text->measure_mono(max_num, pfs_s, 400) / impl->scale + 10.0f;
    const float gpad  = 12.0f;
    const float text_x0 = box.x + gpad + num_w;

    // 行号只靠颜色区分，不画竖线 —— 竖线是文档站的分栏语汇，macOS 编辑器没有

    // 文本基线：行距减掉字高后居中留白，避免所有行都贴着格子顶边
    const float dy = (lh - fs * 1.22f) * 0.5f;
    const int cap = int((box.h - 12.0f) / lh);

    const Rect clip = impl->effective_clip;
    std::vector<detail::CodeSeg> segs;

    for (int i = 0; i < total && i < cap; ++i) {
        const float ly = box.y + 6.0f + lh * float(i) + dy;

        const std::string num = std::to_string(i + 1);
        const float nw = impl->text->measure_mono(num, pfs_s, 400) / impl->scale;
        impl->text->draw_mono(*impl->canvas, num,
                              impl->px(box.x + gpad + num_w - 10.0f - nw), impl->px(ly),
                              pfs_s, 400, cc.gutter, clip);

        detail::code_lex(detail::code_line_at(code, i), segs);
        float x = text_x0;
        for (const detail::CodeSeg& sg : segs) {
            Color c = cc.plain;
            switch (sg.kind) {
                case detail::TokKind::Keyword: c = cc.keyword; break;
                case detail::TokKind::Type:    c = cc.type;    break;
                case detail::TokKind::String:  c = cc.str;     break;
                case detail::TokKind::Comment: c = cc.comment; break;
                case detail::TokKind::Number:  c = cc.number;  break;
                default: break;
            }
            if (x >= box.right() - 4.0f) break;      // 长行右侧直接断，不折行
            impl->text->draw_mono(*impl->canvas, sg.text, impl->px(x), impl->px(ly),
                                  pfs, 400, c, clip);
            x += impl->text->measure_mono(sg.text, pfs, 400) / impl->scale;
        }
    }
}

// =============================================================================
//  8.10  容器类组件
// =============================================================================
void Ui::panel(const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const Color bg = !st.bg.is_none() ? st.bg : th.card_bg;
    // macOS 的 Group Box 是**一层填充，没有描边**（Figma `Group Box`：r12、
    // #00000008 再乘 50% 不透明度，图层里没有任何 stroke）。一圈 1px 线是
    // Web 卡片语汇 —— 系统的分组靠"面比底深一档"分层，不靠画框。
    // 要框得显式给 border_color，这里不再默认描边。
    const Color bd = st.border_color;
    const float bw = st.border_width >= 0.0f ? st.border_width : 1.0f;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_xl;
    // 填充取 Fills - Opaque/Tertiary（5%）：kit 的 Group Box 本身是 3%@50%
    // ≈1.5%，在白底上等于看不见；而示例卡片是"分组容器"，对应系统里的
    // grouped background（#F2F2F2 = 5%）。两者取同一个档位，深浅都成立。
    if (!st.bg.is_none())      fill_rect(box, bg, rad);
    else if (st.muted)         fill_rect(box, th.fill4, rad);
    else                       fill_rect(box, th.fill3, rad);
    if (bw > 0.0f && bd.a > 0) stroke_rect(box, bd, rad, bw);
}


void Ui::separator(const Rect& box, const Style& st) {
    const Color c = !st.bg.is_none() ? st.bg : impl->theme.separator;
    if (box.w >= box.h)
        fill_rect(Rect(box.x, box.cy() - 0.5f, box.w, 1.0f), c, 0.0f);
    else
        fill_rect(Rect(box.cx() - 0.5f, box.y, 1.0f, box.h), c, 0.0f);
}

void Ui::badge(const std::string& text, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    // 参考资料标记是小圆角矩形（r4.5 = Global/Radius 的 3/4），不是胶囊；
    // 字号 Caption 1 = 10/13。
    const float rad = st.radius >= 0.0f ? st.radius : 4.5f;
    const float fs  = st.font_size > 0.0f ? st.font_size : 10.0f;
    // Badge 的背景、前景与字重由 make_skin 统一计算。
    detail::Skin sk = detail::make_skin(th, st, Style::Default, rad);
    if (sk.weight == 400) sk.weight = 500;
    // macOS 的标记（badge / tag）是**强调色 15% 底 + 强调色字**，不是实心色块 + 白字。
    // 实心小色块是 iOS / Web 的写法，在一个 mac 界面里会变成最抢眼的东西。
    const int bv = st.variant >= 0 ? st.variant : Style::Default;
    if (st.bg.is_none() && (bv == Style::Default || bv == Style::Destructive)) {
        const Color tone = (bv == Style::Destructive) ? th.red : th.tint;
        sk.base = tone; sk.base.a = 38;
        sk.fg = tone;
    }
    if (sk.base.a > 0) fill_rect(box, sk.base, rad);
    if (sk.border_w > 0.0f && !sk.border.is_none())
        stroke_rect(box, sk.border, rad, sk.border_w);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = sk.weight;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;
    draw_text(text, box.inset(6.0f, 0.0f), sk.fg, ts);
}

void Ui::progress(float value, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const bool dark = th.window_bg.luminance() < 0.5f;
    // 参考资料 Determinate/Medium：100×10，圆角 100（胶囊），cornerSmoothing 0.6。
    // 轨道 = Fills 5% 半透明；填充 = 强调色胶囊。
    const float h = st.radius >= 0.0f ? box.h : std::min(box.h, 10.0f);
    Rect bar(box.x, box.cy() - h * 0.5f, box.w, h);
    const float rad = h * 0.5f;
    const Color track = !st.border_color.is_none() ? st.border_color
                      : (dark ? Color(255, 255, 255, 26) : Color(0, 0, 0, 13));
    const Color fill  = !st.bg.is_none() ? st.bg : th.tint;
    fill_rect_s(bar, track, rad, 0.0f);
    const float t = detail::clampf(value, 0.0f, 1.0f);
    if (t > 0.0f) {
        // 胶囊填充有个最小宽度：参考资料 10% 时填充是 19px 而不是 10px ——
        // 小于两端圆角的宽度会被圆角吃掉，看起来像"没有头"。
        const float w = std::max(bar.w * t, h);
        fill_rect_s(Rect(bar.x, bar.y, w, bar.h), fill, rad, 0.0f);
    }
}

void Ui::progress_ring(float value, const Rect& box, const Style& st) {
    if (box.empty()) return;
    if (impl->measure_on) { impl->measure_add(box); return; }
    if (!impl->canvas) return;
    const float size = std::min(box.w, box.h);
    const float stroke = st.border_width > 0 ? std::min(st.border_width, size * .4f)
                                             : std::max(2.f, size * .075f);
    const float radius = std::max(0.f, (size - stroke) * .5f);
    const Color track = !st.bg.is_none() ? st.bg : impl->theme.track;
    const Color color = !st.fg.is_none() ? st.fg : impl->theme.tint;
    value = clampf(value, 0.f, 1.f);
    const Vec2 center = impl->xf_map(box.cx(), box.cy());
    auto arc = [&](float fraction, Color c) {
        if (fraction <= 0) return;
        if (st.disabled) c = c.scaled_alpha(.45f);
        // 整段圆弧只混合一次，避免半透明轨道在短线接头处重复叠色。
        detail::raster_arc(*impl->canvas, center.x, center.y, impl->xf_len(radius),
                           impl->xf_len(stroke), c, -1.57079633f + impl->xf_angle(),
                           fraction >= 1.f ? 0.f : 6.2831853f * fraction, 1.f);
    };
    arc(1.f, track);
    arc(value, color);
}

void Ui::skeleton(const Rect& box, const Style& st) {
    if (impl->measure_on) { impl->measure_add(box); return; }
    const Theme& th = impl->theme;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    Color c = !st.bg.is_none() ? st.bg : th.fill3;
    // Skeleton 默认静止；启用 animate 后也仅在悬停时请求动画帧，以控制空闲重绘开销。
    if (st.animated) {
        // 被浮层遮挡时不触发悬停动画。
        const bool hovered = !st.disabled && this->hovered(box);
        float& ph = impl->phase[impl->widget_id("skeleton_phase", box)];
        if (hovered) {
            ph += impl->dt * 3.2f;
            repaint();
        }
        c.a = uint8_t(255.0f * (0.55f + 0.25f * std::sin(ph)));
    }
    fill_rect(box, c, rad);
}

// =============================================================================
//  8.11  交互类组件
// =============================================================================
bool Ui::button(const std::string& text, const Rect& box, const Style& st) {
    uint64_t id = impl->widget_id("button", box, detail::hash_string(text.c_str()), st.identity);
    Style s = st;
    if (!s.align_set) s.align = Align::Center;
    if (!s.valign_set) s.valign = VAlign::Middle;

    const bool enabled = !s.disabled;
    // 链接使用手形指针，普通按钮保留箭头指针。
    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered && s.variant == Style::Link) set_cursor(Cursor::Hand);

    const float t_hover = detail::animate_value(impl, id * 31u + 1u, in.hovered ? 1.0f : 0.0f, 18.0f);
    const float t_press = detail::animate_value(impl, id * 31u + 2u, (in.held && in.hovered) ? 1.0f : 0.0f, 34.0f);

    detail::Skin sk = detail::make_skin(impl->theme, s, Style::Default, impl->theme.radius);

    Rect r = box;
    if (s.press_scale > 0.0f && s.press_scale < 1.0f && t_press > 0.01f) {
        float k = 1.0f - (1.0f - s.press_scale) * t_press;
        r = Rect::centered(box.cx(), box.cy(), box.w * k, box.h * k);
    }

    detail::paint_surface(*this, r, sk, t_hover, t_press, s.disabled);

    Style ts = detail::text_style_of(s, sk, 500);
    Color fg = sk.fg;
    if (s.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    // 显式内边距作用于文字区域，背景与命中区域仍覆盖整个按钮。
    const float padx = s.pad_x >= 0.f ? std::min(s.pad_x, r.w * .5f) : 0.f;
    const float pady = s.pad_y >= 0.f ? std::min(s.pad_y, r.h * .5f) : 0.f;
    draw_text(text, r.inset(padx, pady), fg, ts);

    if (detail::is_focused(impl, id))
        detail::paint_focus_ring(*this, r, sk.radius, impl->theme.focus_ring, true);

    if (in.pressed) detail::grab_focus(impl, id);
    return in.clicked && enabled;
}

bool Ui::icon_button(const std::string& glyph, const Rect& box, const Style& st) {
    uint64_t id = impl->widget_id("iconbtn", box, detail::hash_string(glyph.c_str()), st.identity);
    Style s = st;
    if (s.variant < 0) s.variant = Style::Ghost;
    if (!s.align_set) s.align = Align::Center;
    if (!s.valign_set) s.valign = VAlign::Middle;

    const bool enabled = !s.disabled;
    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    const float t_hover = detail::animate_value(impl, id * 31u + 1u, in.hovered ? 1.0f : 0.0f, 18.0f);
    const float t_press = detail::animate_value(impl, id * 31u + 2u, (in.held && in.hovered) ? 1.0f : 0.0f, 34.0f);

    float rad = s.radius >= 0.0f ? s.radius : impl->theme.radius_sm;
    detail::Skin sk = detail::make_skin(impl->theme, s, Style::Ghost, rad);
    detail::paint_surface(*this, box, sk, t_hover, t_press, s.disabled);

    float fs = s.font_size > 0.0f ? s.font_size : std::min(box.w, box.h) * 0.55f;
    Style ts;
    ts.font_size = fs;
    ts.font_weight = s.font_weight > 0 ? s.font_weight : 400;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;
    Color fg = sk.fg;
    if (s.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    draw_text(glyph, box, fg, ts);
    if (in.pressed) detail::grab_focus(impl, id);
    if (detail::is_focused(impl, id))
        detail::paint_focus_ring(*this, box, rad, impl->theme.focus_ring);
    return in.clicked && enabled;
}

// -----------------------------------------------------------------------------
//  nav_item —— 侧边栏 / 导航列表里的一行
//  和 button 的区别全在"它是导航，不是动作"：文字左对齐（button 居中）、
//  选中态是持续的（button 只有 hover/press），并且右侧可以留白给计数或快捷键。
// -----------------------------------------------------------------------------
bool Ui::nav_item(const std::string& label, bool active, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("navitem", box, detail::hash_string(label.c_str()), st.identity);
    const bool enabled = !st.disabled;

    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    const float t_hover  = detail::animate_value(impl, id * 31u + 1u, (in.hovered && !active) ? 1.0f : 0.0f, 20.0f);
    const float t_active = detail::animate_state(impl, id * 31u + 2u, active ? 1.0f : 0.0f, 24.0f);

    // 选中底是 **r8**（kit `Item/Small/Selected` 的选中 Frame 是 178×24 r8，
    // 而整行自己的圆角只有 5）—— 用按钮的 6 会显得比系统"方"半档。
    const float rad = st.radius >= 0.0f ? st.radius : 8.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : 8.0f;

    // 侧栏选中项使用强调色半透明填充，文字保留标签色。
    if (t_active > 0.01f) {
        Color bg = !st.bg.is_none() ? st.bg : th.tint;
        const uint8_t a = bg.a == 255 ? 59 : bg.a;   // 23% ≈ 59/255
        bg.a = uint8_t(clampf(float(a) * t_active, 0.0f, 255.0f));
        fill_rect(box, bg, rad);
    }
    if (t_hover > 0.01f) {
        Color bg = th.fill3;
        bg.a = uint8_t(clampf(float(bg.a) * t_hover * 0.8f, 0.0f, 255.0f));
        fill_rect(box, bg, rad);
    }

    // 字重固定 500：选中时换字重会让文字宽度跳一下，看着像整个侧边栏在抖。
    // 选中只叠加底色，文字颜色不变（参考资料 Selected 的 Label 仍是 85% 黑）。
    Color fg = !st.fg.is_none() ? st.fg : th.label;
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = st.font_weight > 0 ? st.font_weight : 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    draw_text(label, Rect(box.x + pad, box.y, std::max(box.w - pad - 8.0f, 1.0f), box.h), fg, ts);

    if (in.pressed) detail::grab_focus(impl, id);
    return in.clicked && enabled;
}

// -----------------------------------------------------------------------------
//  input —— 带光标、选区、编辑、剪贴板的单行文本框
// -----------------------------------------------------------------------------
bool Ui::input(std::string* value, const Rect& box, const Style& st) {
    if (!value) return false;
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("input", box, 0,
        st.identity ? st.identity : uint64_t(reinterpret_cast<uintptr_t>(value)));
    const bool enabled = !st.disabled;

    detail::Interact in = detail::interact(impl, id, box, enabled);

    bool focused = detail::editor_focused(impl, id, enabled);
    // 被浮层盖住就交回键盘焦点。块本身已经收不到点击，但焦点还留着，
    // 用户打的字会照样进这个框 —— 和"能点到后面"是同一类穿透，
    // 只是走的是键盘而不是鼠标。
    if (focused && impl->gate_blocked_here(box)) {
        detail::drop_focus(impl);
        focused = false;
    }
    if (in.hovered || focused) set_cursor(Cursor::IBeam);
    bool changed = false;

    // 点击别处 → 失焦
    if (focused && impl->input.pressed[0] && !in.hovered) {
        detail::drop_focus(impl);
        focused = false;
    }
    // 点进来 → 取焦 + 定位光标
    if (in.pressed) {
        detail::grab_focus(impl, id);
        focused = true;
    }

    const float rad  = st.radius >= 0.0f ? st.radius : th.radius;      // macOS 27 Medium 圆角 6
    const float pad  = st.pad_x >= 0.0f ? st.pad_x : 8.0f;             // Fields/Inset - Left = 8
    const float fs   = st.font_size > 0.0f ? st.font_size : th.font_size;
    auto display = [&](const std::string& text) {
        if (!st.password) return text;
        std::string masked;
        const int count = utf8_count(text);
        for (int i = 0; i < count; ++i) masked += "•";
        return masked;
    };
    // 非密码框直接引用调用方的串：这个函数每帧都跑，按值接一份等于每帧复制
    // 整段文本（长输入 / 大段粘贴时很可观）。密码框才需要一份打码副本。
    const std::string masked = st.password ? display(*value) : std::string();
    const std::string& hit_text = st.password ? masked : *value;
    const float avail = std::max(box.w - pad * 2.0f, 1.0f);

    if (focused) {
        // 光标定位（单击 / 拖拽 / 双击选词 / 三击全选 / Shift 扩选）
        if (impl->input.pressed[0] && in.hovered) {
            const float lx = impl->input.mouse.x - (box.x + pad);
            impl->caret = detail::caret_from_x(*this, hit_text, lx, fs, 400);
            const std::vector<uint32_t> cps = utf8_codepoints(*value);
            if (impl->input.click_count >= 3) {
                impl->sel_anchor = 0;
                impl->caret = int(cps.size());
            } else if (impl->input.click_count == 2) {
                int a = 0, b = 0;
                detail::word_span(cps, impl->caret, &a, &b);
                impl->sel_anchor = a;
                impl->caret = b;
            } else if (!impl->input.shift()) {
                impl->sel_anchor = -1;
            }
            impl->flags[id * 31u + 7u] = float(impl->caret);   // 拖选锚点
        }
        if (impl->input.down[0] && in.hovered && impl->active == id) {
            const float lx = impl->input.mouse.x - (box.x + pad);
            const int before = impl->caret;
            impl->caret = detail::caret_from_x(*this, hit_text, lx, fs, 400);
            if (impl->caret != before && impl->sel_anchor < 0)
                impl->sel_anchor = int(impl->flags[id * 31u + 7u]);
        }

        const Input& kin = impl->input;
        int n_cp = utf8_count(*value);
        if (impl->caret > n_cp) impl->caret = n_cp;
        if (impl->caret < 0) impl->caret = 0;

        // ---- 撤销 / 重做 ----
        // Ctrl+Z 撤销，Ctrl+Shift+Z / Ctrl+Y 重做。和输入合并成一步的规则在
        // edit_snapshot 里（连续打字 0.7s 内算一次）。
        bool undone = false;
        if (kin.ctrl_of('Z') || kin.ctrl_of('Y')) {
            const bool redo = kin.key_pressed['Y'] || (kin.key_pressed['Z'] && kin.shift_of('Z'));
            if (redo ? detail::edit_redo(impl, id, value, &impl->caret, &impl->sel_anchor)
                     : detail::edit_undo(impl, id, value, &impl->caret, &impl->sel_anchor)) {
                n_cp = utf8_count(*value);
                impl->caret = detail::clampi(impl->caret, 0, n_cp);
                impl->caret_blink = 0.0f;
                changed = true;
                undone = true;
            }
        }

        const bool edit_event = !kin.text_input.empty() || kin.key_hit(KEY_BACKSPACE) || kin.key_hit(KEY_DELETE)
            || kin.key_hit(KEY_ENTER) || kin.key_hit(KEY_TAB) || kin.ctrl_of('X') || kin.ctrl_of('V')
            || (impl->paste && impl->paste->id == id && impl->paste->ready);
        const std::string before_text = edit_event && !undone ? *value : std::string();
        const int caret0 = impl->caret, anchor0 = impl->sel_anchor;
        uint64_t edit_kind = detail::EDIT_OTHER;

        int sel_from = impl->sel_anchor >= 0 ? std::min(impl->caret, impl->sel_anchor) : impl->caret;
        int sel_to   = impl->sel_anchor >= 0 ? std::max(impl->caret, impl->sel_anchor) : impl->caret;
        bool has_sel = sel_to > sel_from;

        auto delete_selection = [&]() {
            if (!has_sel) return false;
            std::vector<uint32_t> cps = utf8_codepoints(*value);
            std::vector<uint32_t> keep;
            keep.reserve(cps.size());
            for (int i = 0; i < int(cps.size()); ++i)
                if (i < sel_from || i >= sel_to) keep.push_back(cps[i]);
            *value = utf8_from_codepoints(keep, 0, keep.size());
            impl->caret = sel_from;
            impl->sel_anchor = -1;
            changed = true;
            return true;
        };

        if (!undone && kin.key_hit(KEY_BACKSPACE)) {
            edit_kind = detail::EDIT_DELETE;
            if (kin.ctrl_of(KEY_BACKSPACE) || kin.alt_of(KEY_BACKSPACE)) {
                // Ctrl+Backspace：删到上一个词边界
                const std::vector<uint32_t> cps = utf8_codepoints(*value);
                const int to = impl->caret;
                const int from = detail::word_left(cps, impl->caret);
                if (has_sel || from < to) {
                    impl->sel_anchor = from;
                    impl->caret = has_sel ? sel_to : to;
                    has_sel = true;
                    sel_from = from;
                    sel_to = impl->caret;
                    delete_selection();
                }
            } else if (!delete_selection() && impl->caret > 0) {
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                cps.erase(cps.begin() + (impl->caret - 1));
                *value = utf8_from_codepoints(cps, 0, cps.size());
                impl->caret -= 1;
                changed = true;
            }
        }
        if (!undone && kin.key_hit(KEY_DELETE)) {
            edit_kind = detail::EDIT_DELETE;
            if (!delete_selection()) {
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                if (impl->caret < int(cps.size())) {
                    cps.erase(cps.begin() + impl->caret);
                    *value = utf8_from_codepoints(cps, 0, cps.size());
                    changed = true;
                }
            }
        }
        // ←/→：Ctrl 按词跳，Shift 扩选
        if (!undone && kin.key_hit(KEY_LEFT)) {
            if (!kin.shift()) {
                impl->caret = has_sel ? sel_from
                                      : (kin.ctrl_of(KEY_LEFT) ? detail::word_left(utf8_codepoints(*value), impl->caret)
                                                               : std::max(0, impl->caret - 1));
                impl->sel_anchor = -1;
            } else {
                if (impl->sel_anchor < 0) impl->sel_anchor = has_sel ? sel_to : impl->caret;
                impl->caret = kin.ctrl_of(KEY_LEFT) ? detail::word_left(utf8_codepoints(*value), impl->caret)
                                                    : std::max(0, impl->caret - 1);
            }
            impl->caret_blink = 0.0f;
        }
        if (!undone && kin.key_hit(KEY_RIGHT)) {
            if (!kin.shift()) {
                impl->caret = has_sel ? sel_to
                                      : (kin.ctrl_of(KEY_RIGHT) ? detail::word_right(utf8_codepoints(*value), impl->caret)
                                                                : std::min(n_cp, impl->caret + 1));
                impl->sel_anchor = -1;
            } else {
                if (impl->sel_anchor < 0) impl->sel_anchor = has_sel ? sel_from : impl->caret;
                impl->caret = kin.ctrl_of(KEY_RIGHT) ? detail::word_right(utf8_codepoints(*value), impl->caret)
                                                     : std::min(n_cp, impl->caret + 1);
            }
            impl->caret_blink = 0.0f;
        }
        if (!undone && kin.key_hit(KEY_HOME)) {
            if (kin.shift()) { if (impl->sel_anchor < 0) impl->sel_anchor = impl->caret; }
            else impl->sel_anchor = -1;
            impl->caret = 0;
        }
        if (!undone && kin.key_hit(KEY_END)) {
            if (kin.shift()) { if (impl->sel_anchor < 0) impl->sel_anchor = impl->caret; }
            else impl->sel_anchor = -1;
            impl->caret = n_cp;
        }

        // 剪贴板。三条纪律：
        //  · impl->set_clipboard 可能为空（Ui 脱离 Window 使用时没人绑），
        //    直接调会抛 bad_function_call → terminate，所以每个调用点都判空；
        //  · 没有选区时**不碰**系统剪贴板，否则按一下 Ctrl+C 就把用户刚复制的
        //    东西清空了；
        //  · 用 ctrl_of() 而非 ctrl()：Ctrl 的 KEYUP 可能排在字母 KEYDOWN 之后。
        if (!st.password && !undone && kin.ctrl_of('C')) {
            if (has_sel && impl->try_set_clipboard) {
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                impl->copy_to_clipboard(utf8_from_codepoints(cps, size_t(sel_from), size_t(sel_to - sel_from)));
            }
        }
        // 剪切必须"进了剪贴板才删"。回调不可用却照样删，等于把用户选中的东西
        // 凭空丢掉了 —— 比不响应更糟。
        if (!st.password && !undone && kin.ctrl_of('X') && has_sel && impl->try_set_clipboard) {
            edit_kind = detail::EDIT_OTHER;
            std::vector<uint32_t> cps = utf8_codepoints(*value);
            if (impl->copy_to_clipboard(utf8_from_codepoints(cps, size_t(sel_from), size_t(sel_to - sel_from))))
                delete_selection();
        }
        if (!undone && (kin.ctrl_of('V') || (impl->paste && impl->paste->id == id && impl->paste->ready))) {
            std::string paste = impl->paste_text(id, *value, kin.ctrl_of('V'));
            // 单行输入不接收 Tab、换行及其它控制字符，保留 UTF-8 字节。
            std::string filtered;
            for (unsigned char ch : paste) if (ch >= 32 && ch != 127) filtered.push_back(char(ch));
            // 先确认有东西可粘再删选区 —— 剪贴板为空（或只有换行）时先删后插
            // 会把选区直接抹掉，什么都没换来。
            if (!filtered.empty()) {
                edit_kind = detail::EDIT_OTHER;
                delete_selection();
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                std::vector<uint32_t> ins = utf8_codepoints(filtered);
                cps.insert(cps.begin() + impl->caret, ins.begin(), ins.end());
                *value = utf8_from_codepoints(cps, 0, cps.size());
                impl->caret += int(ins.size());
                changed = true;
            }
        }
        if (!undone && kin.ctrl_of('A')) {
            impl->sel_anchor = 0; impl->caret = n_cp;
        }
        if (!undone && kin.key_pressed[KEY_ESC]) { detail::drop_focus(impl); focused = false; }

        // 组合键期间不提交文本，避免输入法将快捷键字符写入输入框。
        if (!undone && !kin.ctrl() && impl->text_veto <= 0.0f && !kin.text_input.empty()) {
            std::string filtered;
            for (unsigned char ch : kin.text_input) if (ch >= 32 && ch != 127) filtered.push_back(char(ch));
            if (!filtered.empty()) {
                edit_kind = detail::EDIT_TYPE;
                delete_selection();
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                std::vector<uint32_t> ins = utf8_codepoints(filtered);
                cps.insert(cps.begin() + impl->caret, ins.begin(), ins.end());
                *value = utf8_from_codepoints(cps, 0, cps.size());
                impl->caret += int(ins.size());
                impl->caret_blink = 0.0f;
                changed = true;
            }
        }

        // 有实际文本改动才进撤销栈（光标移动、选区变化不算）
        if (changed && !undone && *value != before_text)
            detail::edit_snapshot(impl, id, before_text, caret0, anchor0, edit_kind);
    }

    // ---------------- 绘制 ----------------
    const float t_hover = detail::animate_value(impl, id * 31u + 1u, (in.hovered && !focused) ? 1.0f : 0.0f, 18.0f);
    // border_width 为 0 且未指定背景时，由调用方绘制外壳，供 input_group 组合使用；普通输入框使用 th.input_border。
    Color bg = !st.bg.is_none() ? st.bg
             : (th.is_dark() ? Color(255, 255, 255, 20) : th.window_bg);
    const bool bare = st.border_width == 0.0f && st.bg.is_none();
    if (st.disabled && bg.a > 0) bg.a = uint8_t(float(bg.a) * 0.45f);
    if (bg.a > 0 && !bare) fill_rect(box, bg, rad);
    // 凹陷：参考资料 Text Field 的填色是纯白，靠**顶部内阴影 + 极淡描边**"陷"进背景。
    // 只描一圈 1px 边框那是 Web 的 input，平的。
    if (!bare) {
        const Color ish = th.window_bg.luminance() < 0.5f ? Color(0, 0, 0, 70) : Color(0, 0, 0, focused ? 10 : 18);
        inner_shadow(box, ish, rad, 1.8f, 1.0f);
    }
    // 描边就是分隔线色（10%），悬停抬到 25% —— 参考资料输入框只有这两级，没有"变粗"
    Color bd = !st.border_color.is_none() ? st.border_color : th.separator;
    if (t_hover > 0.01f && !focused) bd = bd.mix(th.label3, t_hover);
    float bw = st.border_width >= 0.0f ? st.border_width : 1.0f;
    if (bw > 0.0f) stroke_rect(box, bd, rad, bw);
    // 焦点环：macOS 是**外面**一圈 3px 的强调色光晕，不是把边框染蓝变粗。
    if (focused && !bare) {
        // 焦点环使用低透明度强调色。
        stroke_rect(box.outset(3.0f), th.focus_ring.scaled_alpha(0.5f), rad + 3.0f, 3.0f);
    }

    // IME 预编辑串（拼音）。它不是 value 的一部分，所以单独量宽、单独画：
    // 画在 caret 处、带下划线，caret 顺延到它后面，看上去才是"字还没定"。
    // 不画的话框里就是空的 —— 用户只能盯着框下面那个系统组合窗念拼音。
    //
    // 组字期间**必须**让 placeholder 让位：空框 + hint 的组合太常见，条件里带
    // !show_placeholder 就等于"空输入框永远不显示拼音"，正是用户截图那一幕。
    const bool composing = focused && !impl->ime_comp.empty();
    const bool show_placeholder = value->empty() && !st.placeholder.empty() && !composing;
    const std::string displayed = display(*value);
    const std::string composition = display(impl->ime_comp);
    const float tw = text_width(displayed, fs, 400);
    const std::string& shown = show_placeholder ? st.placeholder : displayed;

    const float comp_w = composing ? text_width(composition, fs, 400) : 0.0f;
    const float shown_w = (show_placeholder ? text_width(st.placeholder, fs, 400) : tw) + comp_w;

    // 水平滚动：保证光标可见（组字串也算进宽度，否则打长拼音时会被框边切掉）
    float scroll = 0.0f;
    if (shown_w > avail) {
        float caret_x = text_width(utf8_from_codepoints(utf8_codepoints(displayed), 0, size_t(impl->caret)), fs, 400) + comp_w;
        if (caret_x - scroll > avail) scroll = caret_x - avail;
        if (caret_x - scroll < 0.0f) scroll = caret_x;
        scroll = detail::clampf(scroll, 0.0f, std::max(0.0f, shown_w - avail));
    }

    Rect text_box(box.x + pad - scroll, box.y, std::max(box.w - pad * 2.0f, 1.0f), box.h);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 400;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    push_clip(box);   // 文本与选区都不得溢出输入框

    if (focused && impl->sel_anchor >= 0) {
        int sf = std::min(impl->caret, impl->sel_anchor), stt = std::max(impl->caret, impl->sel_anchor);
        std::vector<uint32_t> cps = utf8_codepoints(displayed);
        float x0 = text_width(utf8_from_codepoints(cps, 0, size_t(sf)), fs, 400);
        float x1 = text_width(utf8_from_codepoints(cps, 0, size_t(stt)), fs, 400);
        fill_rect(Rect(box.x + pad + x0 - scroll, box.cy() - fs * 0.72f, x1 - x0, fs * 1.44f),
                  th.tint.with_alpha(52), 2.0f);
    }

    // 占位符是 **25%**（kit `Text Field/Rounded` 的 Placeholder 是 #000000 @0.25），
    // 不是 secondary label 的 50% —— 后者会让空输入框看着像"已经有内容"。
    Color fg = !st.fg.is_none() ? st.fg : (show_placeholder ? th.label3 : th.label);
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    if (composing) {
        // 分三段画：正文前半 / 预编辑串（主色 + 下划线）/ 正文后半。
        // 只在组字期间分段，平时还是一次 draw_text —— 不拖慢常规输入。
        const std::vector<uint32_t> cps = utf8_codepoints(displayed);
        const size_t cut = size_t(detail::clampi(impl->caret, 0, int(cps.size())));
        const std::string before = utf8_from_codepoints(cps, 0, cut);
        const std::string after  = utf8_from_codepoints(cps, cut);
        const float bx = box.x + pad - scroll;
        const float bw = text_width(before, fs, 400);
        draw_text(before, Rect(bx, box.y, std::max(bw, 1.0f), box.h), fg, ts);
        draw_text(composition, Rect(bx + bw, box.y, comp_w + 2.0f, box.h), th.tint, ts);
        fill_rect(Rect(bx + bw, box.cy() + fs * 0.60f, comp_w, 1.0f),
                  th.tint.with_alpha(170), 0.0f);
        draw_text(after, Rect(bx + bw + comp_w, box.y, std::max(box.w, 1.0f), box.h), fg, ts);
    } else {
        draw_text(shown, text_box, fg, ts);
    }

    // 光标位置（不管闪不闪都要算：IME 的组合窗/候选窗跟着它走）
    float caret_x = box.x + pad;
    {
        std::vector<uint32_t> cps = utf8_codepoints(displayed);
        float cx = text_width(utf8_from_codepoints(cps, 0, size_t(impl->caret)), fs, 400);
        caret_x = detail::clampf(box.x + pad + cx - scroll + comp_w, box.x + pad - 1.0f, box.right() - pad);
    }
    if (focused && std::fmod(impl->caret_blink, 1.06f) < 0.62f) {
        float ch = fs * 1.15f;
        fill_rect(Rect(caret_x, box.cy() - ch * 0.5f, 1.4f, ch), th.label, 0.7f);
    }
    if (focused) impl->schedule_caret();
    // 输入法：把拼音串钉在输入框下沿、候选框再往下一点。
    // 不做这一步，系统会把它们放在客户区左上角，和输入框完全对不上。
    if (focused && impl->ime_pos) {
        if (impl->ime_content_type) impl->ime_content_type(st.password, false);
        const Rect pb = impl->phys(Rect(caret_x, box.y, 1.0f, box.h));
        impl->ime_pos(pb.x, pb.y, pb.bottom());
    }
    pop_clip();

    if (detail::is_focused(impl, id) && focused)
        detail::paint_focus_ring(*this, box, rad, th.focus_ring, true);

    return changed;
}

// -----------------------------------------------------------------------------
//  checkbox
// -----------------------------------------------------------------------------
bool Ui::checkbox(bool* value, const std::string& label_text, const Rect& box, const Style& st) {
    if (!value) return false;
    const Theme& th = impl->theme;
    const bool dark = th.window_bg.luminance() < 0.5f;
    uint64_t id = impl->widget_id("checkbox", box, detail::hash_string(label_text.c_str()), st.identity);
    const bool enabled = !st.disabled;

    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    bool changed = false;
    if (in.clicked) { *value = !*value; changed = true; }

    const float t = detail::animate_state(impl, id * 37u + 1u, *value ? 1.0f : 0.0f, 20.0f);

    // ---- Checkbox：14×14，圆角 3.5，cornerSmoothing 0.6 ----
    //   fill   #F6F6F6 @84%
    //   stroke #000000 10% / 0.5px INSIDE
    //   fx     内阴影 (dy 0,  blur 2.0, 8%)  +  内阴影 (dy 1.5, blur 1.6, 10%)
    // 那两道内阴影就是"凹陷"的全部来源 —— 少掉它们，复选框就只是一个带边框的方块。
    const float bs  = 14.0f;
    const float rad = st.radius >= 0.0f ? st.radius : 3.5f;
    const float sm  = 0.6f;
    Rect box_r(box.x, box.cy() - bs * 0.5f, bs, bs);

    if (t < 0.995f) {
        const float k = 1.0f - t;                      // 未选中层的可见度
        Color base = dark ? Color(60, 60, 60, 214) : Color(246, 246, 246, 214);
        base.a = uint8_t(float(base.a) * k * (st.disabled ? 0.55f : 1.0f));
        fill_rect_s(box_r, base, rad, sm);
        if (k > 0.02f) {
            inner_shadow(box_r, Color(0, 0, 0, uint8_t(20 * k)), rad, 2.0f, 0.0f);
            inner_shadow(box_r, Color(0, 0, 0, uint8_t(26 * k)), rad, 1.6f, 1.5f);
            stroke_rect_s(box_r, Color(0, 0, 0, uint8_t(26 * k)), rad, 0.5f, sm);
        }
    }
    if (t > 0.005f) {
        Color acc = th.tint;
        if (st.disabled) acc = acc.with_alpha(uint8_t(float(acc.a) * 0.5f));
        if (t < 0.995f) acc.a = uint8_t(float(acc.a) * t);
        fill_rect_s(box_r, acc, rad, sm);
        // 强调色面上同样压一道顶部内阴影，避免"平涂色块"
        inner_shadow(box_r, Color(0, 0, 0, uint8_t(28.0f * t)), rad, 1.8f, 0.0f);
    }

    if (t > 0.30f) {
        // 对勾自绘；浅强调色上沿用主题的深色前景。
        Color fg = th.tint_foreground;
        fg.a = uint8_t(float(fg.a) * detail::clampf((t - 0.30f) / 0.45f, 0.0f, 1.0f)
                       * (st.disabled ? 0.45f : 1.0f));
        const float cx = box_r.cx(), cy = box_r.cy();
        const float s = bs * 0.205f;
        const float lw = std::max(1.5f, bs * 0.115f);
        draw_line(cx - s * 1.25f, cy + s * 0.02f, cx - s * 0.18f, cy + s * 0.86f, fg, lw);
        draw_line(cx - s * 0.18f, cy + s * 0.86f, cx + s * 1.28f, cy - s * 0.80f, fg, lw);
    }

    if (!label_text.empty()) {
        Rect lr(box.x + bs + 6.0f, box.y, std::max(box.w - bs - 6.0f, 1.0f), box.h);
        Style ts;
        ts.font_size = st.font_size > 0.0f ? st.font_size : th.font_size;
        ts.font_weight = 400;
        ts.align = Align::Left;
        ts.valign = VAlign::Middle;
        Color c = !st.fg.is_none() ? st.fg : th.label;
        if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
        draw_text(label_text, lr, c, ts);
    }

    if (in.pressed) detail::grab_focus(impl, id);
    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box_r, rad, th.focus_ring);
    return changed;
}

// -----------------------------------------------------------------------------
//  toggle —— NSSwitch（54×24 轨道 + 32×20 白胶囊滑块）
// -----------------------------------------------------------------------------
bool Ui::toggle(bool* value, const Rect& box, const Style& st) {
    if (!value || box.empty()) return false;
    if (impl->measure_on) { impl->measure_add(box); return false; }
    const Theme& th = impl->theme;
    const bool dark = th.window_bg.luminance() < 0.5f;
    uint64_t id = impl->widget_id("toggle", box, 0, st.identity);
    const bool enabled = !st.disabled;

    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    bool changed = false;
    if (in.clicked) { *value = !*value; changed = true; }

    const float t       = detail::animate_state(impl, id * 41u + 1u, *value ? 1.0f : 0.0f, 22.0f);
    const float t_press = detail::animate_value(impl, id * 41u + 2u, (in.held && in.hovered) ? 1.0f : 0.0f, 30.0f);

    // Medium 轨道为 54×24 胶囊，滑块为 32×20，四周内缩 2。
    // 五档宽高比并不相等。宽、高都必须装进 box，不能只按高度选档。
    const float widths[] = {36.0f, 44.0f, 54.0f, 64.0f, 80.0f};
    const float knob_widths[] = {21.0f, 26.0f, 32.0f, 38.0f, 47.0f};
    const float knob_heights[] = {13.0f, 16.0f, 20.0f, 24.0f, 30.0f};
    const float insets[] = {1.5f, 2.0f, 2.0f, 2.0f, 3.0f};
    int size = 0;
    for (int i = 1; i < 5; ++i)
        if (widths[i] <= box.w && th.size_h[i] <= box.h) size = i;
    const float fit = std::min(1.0f, std::min(box.w / widths[size], box.h / th.size_h[size]));
    const float track_h = th.size_h[size] * fit;
    const float track_w = widths[size] * fit;
    Rect track = Rect::centered(box.cx(), box.cy(), track_w, track_h);
    const float rad = track_h * 0.5f;

    // 关闭态 = Fills - Opaque 5%（参考资料 Idle/False 的 Black 层 op0.05）
    // 打开态 = 强调色；按下再叠 13%（参考资料 Idle/Clicked 的 Black 层 op0.13）
    const Color off_c = dark ? Color(255, 255, 255, 26) : Color(0, 0, 0, 13);
    Color track_c = off_c.mix(th.tint, t);
    if (*value && t_press > 0.01f) track_c = track_c.mix(Color(0, 0, 0, 33), t_press * 0.5f);
    if (st.disabled) track_c = track_c.with_alpha(uint8_t(float(track_c.a) * 0.55f));
    fill_rect_s(track, track_c, rad, 0.0f);   // 胶囊：半径已到短边一半，不用再压 smoothing

    // kit：滑块 32×20 装在 54×24 的轨道里，四周内缩 2（比例 0.593 / h-4）
    const float inset = insets[size] * fit;
    const float knob_w = knob_widths[size] * fit, knob_h = knob_heights[size] * fit;
    const float kx = detail::lerpf(track.x + inset, track.right() - inset - knob_w, t);
    Rect knob_r(kx, track.cy() - knob_h * 0.5f, knob_w, knob_h);
    // 参考资料 Knob 阴影：drop(y3 blur36 #0000000D) —— 36 是 Figma 的模糊半径，落到像素上取 7
    drop_shadow(knob_r, knob_h * 0.5f, 7.0f, Color(0, 0, 0, 26), 1.5f);
    Color knob = Color(255, 255, 255);
    if (st.disabled) knob = knob.with_alpha(230);
    fill_rect_s(knob_r, knob, knob_h * 0.5f, 0.0f);
    // 极淡描边把白片从浅色轨道上"托"起来（否则关闭态白片会糊进 5% 灰里）
    stroke_rect_s(knob_r, Color(0, 0, 0, 18), knob_h * 0.5f, 1.0f, 0.0f);

    if (in.pressed) detail::grab_focus(impl, id);
    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, track, rad, th.focus_ring);
    return changed;
}

// -----------------------------------------------------------------------------
//  slider
// -----------------------------------------------------------------------------
bool Ui::slider(float* value, float min_value, float max_value, const Rect& box, const Style& st) {
    if (!value) return false;
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("slider", box, 0, st.identity);
    const bool enabled = !st.disabled;

    Rect hit(box.x, box.y, box.w, box.h);
    detail::Interact in = detail::interact(impl, id, hit, enabled);
    if (in.hovered || impl->active == id) set_cursor(Cursor::ResizeH);

    bool changed = false;
    float t = (max_value > min_value) ? detail::clampf((*value - min_value) / (max_value - min_value), 0.0f, 1.0f) : 0.0f;

    if (impl->active == id && impl->input.down[0]) {
        // 输入与绘制共用两端把手中心之间的轨道，点到可见端点就应得到 min/max。
        const float half = std::min(10.0f, box.w * 0.5f);
        float nt = detail::clampf((impl->input.mouse.x - box.x - half)
                                 / std::max(box.w - 2.0f * half, 1.0f), 0.0f, 1.0f);
        float nv = min_value + (max_value - min_value) * nt;
        if (std::fabs(nv - *value) > 1e-6f) { *value = nv; changed = true; }
        t = nt;
    }

    // 滑杆轨道高 4px，把手为 20×16 胶囊；两端预留把手半宽。
    const float track_h = 4.0f;
    const float knob_w = 20.0f, knob_h = 16.0f;
    const float half = knob_w * 0.5f;
    Rect track(box.x + half, box.cy() - track_h * 0.5f,
               std::max(box.w - knob_w, 1.0f), track_h);
    const float rad = track_h * 0.5f;

    Color track_c = !st.border_color.is_none() ? st.border_color : th.track;
    fill_rect_s(track, track_c, rad, 0.0f);
    Color fill_c = !st.bg.is_none() ? st.bg : th.tint;
    if (st.disabled) fill_c.a = uint8_t(float(fill_c.a) * 0.45f);
    if (t > 0.0f) fill_rect_s(Rect(track.x, track.y, std::max(track.w * t, track_h), track.h), fill_c, rad, 0.0f);

    // 参考资料只有 Non-ticked 20×16 与 Clicked 25×20 两态：悬停不变形，按下才拉长
    const float t_press = detail::animate_value(impl, id * 43u + 2u,
                                                (impl->active == id) ? 1.0f : 0.0f, 24.0f);
    const float kw = knob_w + (25.0f - 20.0f) * t_press;
    const float kh = knob_h + (20.0f - 16.0f) * t_press;
    const float kx = track.x + track.w * t;
    Color knob = Color(255, 255, 255);
    const Rect knob_r_r = Rect::centered(kx, track.cy(), kw, kh);
    drop_shadow(knob_r_r, kh * 0.5f, 3.5f, Color(0, 0, 0, 70), 0.5f);
    fill_rect_s(knob_r_r, knob, kh * 0.5f, 0.0f);
    stroke_rect_s(knob_r_r, Color(0, 0, 0, 30), kh * 0.5f, 1.0f, 0.0f);

    if (in.pressed) detail::grab_focus(impl, id);
    return changed;
}

// Tabs 按文字宽度排列，Segmented 等分并共用凹槽背景；两者均返回选中索引。
bool Ui::tabs(const std::vector<std::string>& labels, int* index, const Rect& box, const Style& st) {
    return tabs(labels, index, box, TabOptions{}, st);
}

bool Ui::tabs(const std::vector<std::string>& labels, int* index, const Rect& box,
                     const TabOptions& options, const Style& st) {
    return tabs_impl(labels, index, box, options, false, st);
}

bool Ui::tabs_impl(const std::vector<std::string>& labels, int* index, const Rect& box,
                   const TabOptions& options, bool segmented_control, const Style& st) {
    if (options.close_index) *options.close_index = -1;
    if (!index || labels.empty() || box.w <= 0.0f || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    uint64_t base_id = impl->widget_id("tabs", box, 0, st.identity);
    const bool pill = (st.variant == Style::Secondary || st.variant == Style::Outline);
    const float rad = st.radius >= 0.0f ? st.radius : (pill ? box.h * 0.5f : 6.0f);
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    const bool closable = !pill && options.close_index;
    auto indicator = [&](size_t i) {
        return !pill && options.indicators && i < options.indicators->size()
                   ? (*options.indicators)[i] : Color::none();
    };

    bool changed = false;
    Rect container(box.x, box.y, box.w, box.h);
    // Segmented Control 的选中段与轨道等高；普通胶囊页签仍保留 2px 内缩。
    const Rect inner = pill && !segmented_control ? container.inset(2.0f, 2.0f) : container;
    const float equal_w = inner.w / float(labels.size());

    // 胶囊分段等宽，普通标签栏按文字宽度计算位置。
    std::vector<float> xs(labels.size(), 0.0f), ws(labels.size(), equal_w);
    {
        float x = inner.x;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (!pill) {
                ws[i] = text_width(labels[i], fs, 500) + 36.0f + (closable ? 22.0f : 0.0f)
                        + (indicator(i).is_none() ? 0.0f : 12.0f);
                if (options.max_width > 0.0f)
                    ws[i] = std::min(ws[i], std::max(options.max_width, 70.0f));
            }
            xs[i] = x;
            x += ws[i];
        }
    }

    Rect viewport = inner;
    float offset = 0.0f;
    const float total_w = xs.back() + ws.back() - inner.x;
    const bool overflow = !pill && (closable || options.indicators || options.max_width > 0.0f)
                          && total_w > inner.w;
    if (overflow) {
        const float side = std::min(24.0f, std::min(inner.h, inner.w * 0.25f));
        viewport.w = std::max(0.0f, inner.w - side * 2.0f - 4.0f);
        const uint64_t offset_id = base_id * 47u + 3u, selection_id = base_id * 47u + 4u;
        const float max_offset = std::max(0.0f, total_w - viewport.w);
        offset = std::max(0.0f, std::min(impl->flags[offset_id], max_offset));
        const float selection = float(*index + 2);
        if (impl->flags[selection_id] != selection && *index >= 0 && *index < int(labels.size())) {
            const float start = xs[size_t(*index)] - inner.x;
            const float end = start + ws[size_t(*index)];
            offset = std::min(start, std::max(offset, end - viewport.w));
        }
        impl->flags[selection_id] = selection;
        for (int direction : {-1, 1}) {
            const Rect arrow(inner.right() - side * (direction < 0 ? 2.0f : 1.0f),
                             inner.cy() - side * 0.5f, side, side);
            Style button_style = Style().ghost();
            button_style.disabled = st.disabled || (direction < 0 ? offset <= 0 : offset >= max_offset);
            if (icon_button("", arrow, button_style))
                offset = std::max(0.0f, std::min(max_offset, offset + float(direction) * std::max(70.0f, viewport.w * 0.75f)));
            system_icon(direction < 0 ? SystemIcon::Prev : SystemIcon::Next,
                        arrow.inset(side * 0.3f), th.label2.scaled_alpha(button_style.disabled ? 0.45f : 1.0f));
        }
        impl->flags[offset_id] = offset;
    } else if (!pill) {
        impl->flags.erase(base_id * 47u + 3u);
        impl->flags.erase(base_id * 47u + 4u);
    }

    if (pill) {
        // 参考资料 Segmented Control 的轨道是一层**凹槽**（8%），没有描边；和
        // toggle_group 用的是同一个画法 —— 它们在 AppKit 里本来就是同一个控件。
        Color container_bg = !st.bg.is_none() ? st.bg : th.track_well();
        if (st.disabled) container_bg.a = uint8_t(float(container_bg.a) * 0.45f);
        fill_rect_s(container, container_bg, rad, 0.35f);
    }

    push_clip(viewport);
    for (size_t i = 0; i < labels.size(); ++i) {
        Rect item(xs[i] - offset, inner.y, ws[i], inner.h);
        const float close_side = std::min(18.0f, std::max(0.0f, item.h - 8.0f));
        const Rect close(item.right() - 6.0f - close_side, item.cy() - close_side * 0.5f, close_side, close_side);
        Rect hit = item;
        if (closable) hit.w = std::max(0.0f, close.x - item.x - 2.0f);
        uint64_t id = impl->widget_id("tab", item, base_id + i, st.identity ? base_id + i : 0);
        detail::Interact in = detail::interact(impl, id, hit, !st.disabled);
        if (in.hovered) set_cursor(Cursor::Arrow);
        if (in.clicked && *index != int(i)) { *index = int(i); changed = true; }

        const float t_sel = detail::animate_state(impl, id * 47u + 1u, (*index == int(i)) ? 1.0f : 0.0f, 22.0f);
        const float t_hov = detail::animate_value(impl, id * 47u + 2u, in.hovered ? 1.0f : 0.0f, 18.0f);

        if (pill) {
            // Segmented 的活动段填满所属分段；普通胶囊页签仍上下内缩 2px。
            const Rect seg = segmented_control
                                 ? item
                                 : item.inset(2.0f, 2.0f);
            const float sr = segmented_control ? std::min(seg.h * 0.5f, rad)
                                               : std::max(0.0f, seg.h * 0.5f);
            if (t_sel > 0.02f) {
                Color sel_bg = !st.border_color.is_none() ? st.border_color : th.tint;
                sel_bg.a = uint8_t(float(sel_bg.a) * t_sel + 0.5f);
                fill_rect_s(seg, sel_bg, sr, 0.35f);
            } else if (t_hov > 0.02f) {
                // 悬停只浮一层 3%（参考资料 Idle 没有 hover 态，这层是给指针的反馈下限）
                fill_rect_s(seg, th.fill4.scaled_alpha(t_hov), sr, 0.35f);
            }
        } else {
            // 标签栏：选中 = 3% 圆角底（参考资料 44×29 的 Item 就是 r6 + #00000008），
            // 悬停 = 同款更淡的一层。没有下划线、没有滑块。
            const Rect seg = item.inset(2.0f, 2.0f);
            if (t_sel > 0.02f) fill_rect_s(seg, th.fill4.scaled_alpha(t_sel), 6.0f, 0.35f);
            else if (t_hov > 0.02f) fill_rect_s(seg, th.fill5.scaled_alpha(t_hov), 6.0f, 0.35f);
        }

        Style ts;
        ts.font_size = fs;
        ts.font_weight = 500;                 // 参考资料：选中/未选中都是 SF Pro Medium
        ts.align = Align::Center;
        ts.valign = VAlign::Middle;
        // 分段控件与标签栏分别使用各自的选中及未选中标签色。
        Color c;
        if (pill) c = (*index == int(i)) ? detail::on_color(th.tint) : th.label;
        else      c = (*index == int(i)) ? th.label : th.label2;
        if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
        Rect text_box = item;
        const Color dot = indicator(i);
        if (!dot.is_none()) {
            fill_circle(item.x + 12.0f, item.cy(), 2.5f, dot.scaled_alpha(st.disabled ? 0.45f : 1.0f));
            text_box.x += 12.0f;
            text_box.w -= 12.0f;
        }
        if (closable) text_box.w = std::max(0.0f, text_box.w - 22.0f);
        draw_text(labels[i], text_box, c, ts);
        if (closable) {
            Style button_style = Style().ghost();
            button_style.disabled = st.disabled;
            if (icon_button("", close, button_style)) *options.close_index = int(i);
            system_icon(SystemIcon::Close, close.inset(close_side * 0.28f),
                        th.label2.scaled_alpha(st.disabled ? 0.45f : 1.0f));
        }
    }
    pop_clip();
    return changed;
}

// -----------------------------------------------------------------------------
//  segmented —— 等宽胶囊分段选择
// -----------------------------------------------------------------------------
bool Ui::segmented(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st) {
    // 调用方没点明变体时使用分段控件样式；显式传了别的则沿用 tabs 对应变体。
    Style ps = st;
    if (ps.variant < 0) ps.variant = Style::Secondary;
    const bool segmented_control = ps.variant == Style::Secondary || ps.variant == Style::Outline;
    return tabs_impl(items, index, box, TabOptions{}, segmented_control, ps);
}

// -----------------------------------------------------------------------------
//  select —— 下拉选择。弹层用 deferred 画到帧末，保证盖在其它组件之上
// -----------------------------------------------------------------------------
bool Ui::select(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st) {
    if (!index || items.empty()) return false;
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("select", box, 0, st.identity);
    // 本组件第一个控件的层级号。弹层下一帧用它判定"谁在下面"。
    const uint64_t z_first = impl->seq + 1;
    // 下拉触发器与普通按钮共用默认圆角。
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    const bool enabled = !st.disabled;

    bool open = impl->flags[id] > 0.5f;
    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered) set_cursor(Cursor::Arrow);

    bool changed = false;
    if (in.clicked) { open = !open; impl->flags[id] = open ? 1.0f : 0.0f; }
    // Esc 关闭列表。网页端的 Select 支持，桌面端更必须有 —— 否则打开之后
    // 只能靠点外部来收，键盘用户没有出路。
    if (open && impl->input.key_pressed[KEY_ESC]) { open = false; impl->flags[id] = 0.0f; }

    const float t_hover = detail::animate_value(impl, id * 53u + 1u, (in.hovered || open) ? 1.0f : 0.0f, 18.0f);

    // 弹出选择器使用半透明按钮面，无输入框描边。
    Color bg = !st.bg.is_none() ? st.bg : th.fill2;
    if (t_hover > 0.01f) bg = bg.mix(th.label.with_alpha(38), t_hover * 0.55f);
    if (bg.a > 0) fill_rect_s(box, bg, rad, 0.35f);
    Color bd = !st.border_color.is_none() ? st.border_color : Color(0, 0, 0, 0);
    if (bd.a > 0) stroke_rect(box, bd, rad, st.border_width >= 0.0f ? st.border_width : 1.0f);

    const float pad = 12.0f;
    const std::string cur = (*index >= 0 && *index < int(items.size())) ? items[size_t(*index)] : std::string();
    Rect label_box(box.x + pad, box.y, std::max(box.w - pad * 2.0f - 18.0f, 1.0f), box.h);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    Color fg = !st.fg.is_none() ? st.fg : (cur.empty() ? th.label2 : th.label);
    draw_text(cur.empty() && !st.placeholder.empty() ? st.placeholder : cur, label_box, fg, ts);

    // 右侧是**上下两个小三角**（NSPopUpButton 的经典标记）。单个向下箭头是
    // Web 的 <select>；mac 的下拉按钮从上世纪到现在都是"上下"两个箭头。
    detail::paint_popup_chevrons(*this, box, pad, th.label2);

    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box, rad, th.focus_ring);

    // --- 弹层 ---
    if (open) {
        const float item_h = std::max(box.h, 24.0f);   // kit Menu/Height Medium = 24
        const float list_h = item_h * float(items.size()) + 8.0f;
        float ly = box.bottom() + 4.0f;
        if (ly + list_h > height() - 4.0f) ly = std::max(4.0f, box.y - 4.0f - list_h);
        Rect list(box.x, ly, box.w, list_h);

        // ---- 逐项交互：必须在**帧内**做完 ----
        // 早先把交互放在 deferred 里，写回的是调用方栈上的 int*（帧末早就出栈了），
        // 而且 changed 永远是 false —— 表现就是"点了没反应"。menu 也是这么处理的：
        // 交互帧内、绘制帧末。
        const float pad = 4.0f;
        std::vector<Rect>    rects(items.size());
        std::vector<uint64_t> ids(items.size());
        std::vector<uint8_t>  hov(items.size(), 0);
        int clicked_index = -1;
        {
            Impl::SeqFrameEnd guard(impl);   // 视觉属于帧末层，序号也算帧末层
            for (size_t i = 0; i < items.size(); ++i) {
                rects[i] = Rect(list.x + pad, list.y + pad + item_h * float(i),
                                list.w - pad * 2.0f, item_h);
                ids[i] = impl->widget_id("select_item", rects[i], 0, st.identity ? id + i : 0);
                detail::Interact ii = detail::interact(impl, ids[i], rects[i], true);
                if (ii.hovered) { set_cursor(Cursor::Arrow); hov[i] = 1; }
                if (ii.clicked) clicked_index = int(i);
            }
        }

        const Input& kin = impl->input;
        const bool click_outside = (kin.pressed[0] || kin.pressed[1])
                                && !list.contains(kin.mouse.x, kin.mouse.y)
                                && !box.contains(kin.mouse.x, kin.mouse.y);

        if (clicked_index >= 0) {
            *index = clicked_index;
            impl->flags[id] = 0.0f;
            open = false;
            changed = true;
            repaint();
        } else if (click_outside) {
            impl->flags[id] = 0.0f;
            open = false;
            repaint();
        }

        // ---- 绘制：帧末，只画不判 ----
        // 选中/点外部的那一帧不再画：状态已经变了，再画一遍旧弹层会多闪一帧。
        // 同上：展开期间只有主体和下拉列表能接收点击。
        // 触发器在普通层（它自己已经 interact 过），列表是帧末画的，层级取帧末基准。
        if (clicked_index < 0 && !click_outside) {
            impl->gate_claim(box, z_first);
            impl->gate_claim(list, Impl::Z_FRAME_END);
            const int sel_now = *index;
            const std::vector<std::string> items_copy = items;
            deferred([this, list, items_copy, rects, ids, hov, sel_now, fs]() {
                detail::paint_menu(*this, list, items_copy, rects, ids, hov, sel_now, true, fs);
            });
        }
    }

    return changed;
}

// =============================================================================
//  8.12  表单补充
// =============================================================================

// -----------------------------------------------------------------------------
//  radio_group —— 单选组
//  纵向等分 box.h，每行"圆环 + 标签"。选中态：圆环外圈加粗 + 内点
//  并转成 primary，中心长出一个实心点（靠一个动画量 t 同时驱动两者）。
// -----------------------------------------------------------------------------
bool Ui::radio_group(const std::vector<std::string>& items, int* index, const Rect& box, const Style& st) {
    if (!index || items.empty() || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    uint64_t base = impl->widget_id("radiogroup", box, 0, st.identity);
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float row_h = box.h / float(items.size());
    const float dot = std::max(16.0f, fs + 3.0f);

    bool changed = false;
    for (size_t i = 0; i < items.size(); ++i) {
        Rect row(box.x, box.y + row_h * float(i), box.w, row_h);
        uint64_t id = impl->widget_id("radio", row, base + i, st.identity ? base + i : 0);
        detail::Interact in = detail::interact(impl, id, row, !st.disabled);
        if (in.hovered) set_cursor(Cursor::Arrow);
        if (in.pressed) detail::grab_focus(impl, id);
        if (in.clicked && *index != int(i)) { *index = int(i); changed = true; }

        const float t     = detail::animate_state(impl, id * 59u + 1u, (*index == int(i)) ? 1.0f : 0.0f, 22.0f);

        const Rect dot_r = Rect::centered(row.x + dot * 0.5f, row.cy(), dot, dot);
        // 和复选框同一套"凹陷"配方：浅底 + 极淡描边 + 两道内阴影，
        // 选中时底下压一层强调色（圆点浮在上面）。
        const bool dk = th.window_bg.luminance() < 0.5f;
        if (t < 0.995f) {
            Color base = dk ? Color(60, 60, 60, 214) : Color(246, 246, 246, 214);
            base.a = uint8_t(float(base.a) * (1.0f - t) * (st.disabled ? 0.55f : 1.0f));
            fill_rect_s(dot_r, base, dot * 0.5f, 0.0f);
            inner_shadow(dot_r, Color(0, 0, 0, uint8_t(20 * (1.0f - t))), dot * 0.5f, 2.0f, 0.5f);
            stroke_rect_s(dot_r, Color(0, 0, 0, 32), dot * 0.5f, 1.0f, 0.0f);
        }
        if (t > 0.005f) {
            Color acc = th.tint;
            if (st.disabled) acc = acc.with_alpha(uint8_t(float(acc.a) * 0.5f));
            if (t < 0.995f) acc.a = uint8_t(float(acc.a) * t);
            fill_rect_s(dot_r, acc, dot * 0.5f, 0.0f);
        }

        if (t > 0.02f) {
            // 中心点用 t*(2-t) 做一次 ease-out，弹出感比线性更像"长出来"。
            // 沿用原有的 6px 中心点，只修复它与选中底色相同的问题。
            const float ir = 3.0f * (t * (2.0f - t));
            if (ir > 0.3f) {
                Color fg = th.tint_foreground;
                if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
                fill_circle(dot_r.cx(), dot_r.cy(), ir, fg);
            }
        }

        Style ts;
        ts.font_size = fs;
        // 参考资料 `Type=Radio Buttons`：标签 13 Medium，**选中不换字重**（换字重会让
        // 整行文字宽度跳一下，一排单选看着在抖）。
        ts.font_weight = 500;
        ts.align = Align::Left;
        ts.valign = VAlign::Middle;
        Color c = !st.fg.is_none() ? st.fg : th.label;
        if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
        draw_text(items[i], Rect(row.x + dot + 9.0f, row.y, std::max(row.w - dot - 9.0f, 1.0f), row.h), c, ts);

        if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, dot_r, dot * 0.5f, th.focus_ring);
    }
    return changed;
}

// -----------------------------------------------------------------------------
//  stepper —— 数值与右侧上下增减按钮
// -----------------------------------------------------------------------------
bool Ui::stepper(float* value, float step, float min_value, float max_value, const Rect& box, const Style& st) {
    if (!value || box.w <= 12.0f) return false;
    const Theme& th = impl->theme;
    uint64_t base = impl->widget_id("stepper", box, 0, st.identity);
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;

    // Stepper 位于数值框右侧，上下两个按钮竖向排列。
    const float btn_w = std::min(std::max(box.h * 0.83f, 16.0f), 20.0f);
    // 上按钮增加数值，下按钮减少数值。
    const Rect plus_r (box.right() - btn_w, box.y, btn_w, box.h * 0.5f);
    const Rect minus_r(box.right() - btn_w, box.y + box.h * 0.5f, btn_w, box.h - box.h * 0.5f);
    const Rect value_r(box.x, box.y, std::max(box.w - btn_w, 1.0f), box.h);
    const float br = std::min(rad, box.h * 0.5f);

    // 数值框 + 步进条：两者拼成一个整体，只有最外侧四角是圆的（同心）
    Color bg = !st.bg.is_none() ? st.bg
             : (th.is_dark() ? Color(255, 255, 255, 20) : th.window_bg);
    if (st.disabled && bg.a > 0) bg.a = uint8_t(float(bg.a) * 0.45f);
    if (bg.a > 0) fill_rect_s(box, bg, br, 0.35f);
    // 竖条底色：中性可点面（8%），比数值框亮/暗各一档
    fill_rect_s(Rect(plus_r.x, box.y, btn_w, box.h), th.fill2, br, 0.35f);
    Color bd = !st.border_color.is_none() ? st.border_color : th.separator;
    if (st.disabled) bd.a = uint8_t(float(bd.a) * 0.45f);
    stroke_rect_s(box, bd, br, st.border_width >= 0.0f ? st.border_width : 1.0f, 0.35f);
    // 中缝：kit 是 14×1、r10、12% 黑，左右各留 3 —— 通栏 1px 是分隔线，
    // 不是"两个按钮之间的凹槽"。
    fill_rect(Rect(plus_r.x + (btn_w - 14.0f) * 0.5f, box.cy() - 0.5f, 14.0f, 1.0f),
              th.is_dark() ? Color(255, 255, 255, 31) : Color(0, 0, 0, 31), 0.5f);

    auto draw_btn = [&](const Rect& r, uint64_t id, bool up) -> bool {
        detail::Interact in = detail::interact(impl, id, r, !st.disabled);
        if (in.hovered) set_cursor(Cursor::Arrow);
        if (in.pressed) detail::grab_focus(impl, id);
        const float t_p = detail::animate_value(impl, id * 61u + 2u, (in.held && in.hovered) ? 1.0f : 0.0f, 30.0f);
        if (t_p > 0.02f)
            fill_rect_s(r.inset(0.5f), th.fill_press.scaled_alpha(t_p), std::max(0.0f, br - 1.0f), 0.35f);
        // 指示符是 **chevron**（V 形描边），不是实心三角。
        // kit `Stepper/No Field`：每半高里一个 20×12 的 SF Symbol 􀆇/􀆈，
        // 12px Medium、85% 标签色 —— 实心三角是旧版 AppKit 的三角形按钮。
        Color c = !st.fg.is_none() ? st.fg : th.label;
        if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
        system_icon(up ? SystemIcon::ChevronUp : SystemIcon::ChevronDown,
                    Rect::centered(r.cx(), r.cy(), 10, 8), c);
        if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, r, br, th.focus_ring);
        return in.clicked;
    };

    bool changed = false;
    if (draw_btn(minus_r, base + 1u, false)) {
        const float nv = detail::clampf(*value - step, min_value, max_value);
        if (nv != *value) { *value = nv; changed = true; }
    }
    if (draw_btn(plus_r, base + 2u, true)) {
        const float nv = detail::clampf(*value + step, min_value, max_value);
        if (nv != *value) { *value = nv; changed = true; }
    }

    char buf[64];
    if (std::fabs(*value - std::floor(*value + 0.5f)) < 0.0005f)
        std::snprintf(buf, sizeof(buf), "%.0f", double(*value));
    else
        std::snprintf(buf, sizeof(buf), "%.2f", double(*value));

    Style ts;
    ts.font_size = fs;
    ts.font_weight = 400;
    ts.align = Align::Right;      // macOS 的数值框数字靠右
    ts.valign = VAlign::Middle;
    Color fc = !st.fg.is_none() ? st.fg : th.label;
    if (st.disabled) fc.a = uint8_t(float(fc.a) * 0.45f);
    push_clip(value_r);
    draw_text(buf, value_r.inset(8.0f, 0.0f), fc, ts);
    pop_clip();
    return changed;
}

// -----------------------------------------------------------------------------
//  search_field —— NSSearchField（胶囊 + 放大镜 + 清除钮）
//  kit `Search/Medium`：r = 100（胶囊），放大镜在框内左端，文字从 26 起排；
//  有内容时右端出现一个 14 的清除徽标。其余行为（光标/选区/IME/剪贴板）全部
//  复用 input —— 搜索框不是另一个控件，只是 input 的一层皮。
// -----------------------------------------------------------------------------
namespace detail {
// 搜索符号由系统提供，与工具栏和图标按钮保持一致。
inline void paint_magnifier(Ui& ui, float cx, float cy, float r, const Color& c) {
    ui.system_icon(SystemIcon::Search, Rect::centered(cx, cy, r * 3, r * 3), c);
}
// 清除徽标：实心灰圆 + 白色叉
inline void paint_clear_badge(Ui& ui, float cx, float cy, float r,
                              const Color& fill, const Color& x) {
    ui.fill_circle(cx, cy, r, fill);
    ui.system_icon(SystemIcon::Close, Rect::centered(cx, cy, r, r), x);
}
}  // namespace detail

bool Ui::search_field(std::string* value, const Rect& box, const Style& st) {
    if (!value) return false;
    const Theme& th = impl->theme;
    Style s = st;
    if (s.radius < 0.0f) s.radius = box.h * 0.5f;         // 胶囊
    if (s.pad_x < 0.0f)  s.pad_x  = 26.0f;                // 给放大镜让位
    const bool has_clear = !value->empty();
    const Rect cr(box.right() - box.h, box.y, box.h, box.h);
    detail::Interact ci;
    // 清除键与输入框重叠，必须先认领按下事件，避免被整个输入框的热区吞掉。
    if (has_clear)
        ci = detail::interact(impl, impl->widget_id("searchclear", box, 0, st.identity ? st.identity : uint64_t(reinterpret_cast<uintptr_t>(value))), cr, !st.disabled);
    bool changed = input(value, box, s);

    // 禁用时放大镜跟着文字一起淡到 45%：只淡文字、图标还是满色的话，看着像
    // "图标可点、文字是灰的"。
    Color mag = th.label2;
    if (st.disabled) mag.a = uint8_t(float(mag.a) * 0.45f);
    detail::paint_magnifier(*this, box.x + 13.0f, box.cy(), 4.2f, mag);

    // 清除按钮限制在输入框内部，右侧预留边距。
    if (has_clear) {
        if (ci.hovered) set_cursor(Cursor::Arrow);
        // 禁用时叉照画、只是淡下去（kit 的 `State=Disabled` 变体里 Clear 还在，
        // 直接不画反而对不上）。
        const float dim = st.disabled ? 0.45f : 1.0f;
        Color badge = th.label3.is_none() ? th.label2 : th.label3;
        Color cross = Color(255, 255, 255, 235);
        if (dim < 1.0f) {
            badge.a = uint8_t(float(badge.a) * dim);
            cross.a = uint8_t(float(cross.a) * dim);
        }
        detail::paint_clear_badge(*this, cr.cx(), cr.cy(), 7.0f, badge, cross);
        if (ci.clicked) {
            value->clear();
            impl->caret = 0;
            impl->sel_anchor = 0;
            changed = true;
            repaint();
        }
    }
    return changed;
}

// -----------------------------------------------------------------------------
//  color_well —— NSColorWell:48×24 的实心圆角色块
//  白色色块在浅色窗口上会"消失"，所以它必须有三层：色块本身 + 内侧 1px 白边
//  （把颜色从背景上托起来）+ 外侧 1px 分隔线（白块也能看见轮廓）。
// -----------------------------------------------------------------------------
bool Ui::color_well(const Color& c, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("colorwell", box, 0, st.identity);
    detail::Interact in = detail::interact(impl, id, box, !st.disabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;

    Color cc = c;
    if (st.disabled) cc = cc.mix(th.window_bg, 0.5f);
    fill_rect_s(box, cc, rad, th.corner_smoothing);
    stroke_rect_s(box, Color(255, 255, 255, 130), rad, 1.0f, th.corner_smoothing);
    stroke_rect_s(box, th.separator, rad, 1.0f, th.corner_smoothing);

    if (in.pressed) detail::grab_focus(impl, id);
    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box, rad, th.focus_ring);
    return in.clicked && !st.disabled;
}

// -----------------------------------------------------------------------------
//  image_well —— 图像池的空位（NSImageView 的拖放占位）
//  5% 凹面 + 1px 分隔线 + 图片符号 + 一行说明。它是"槽"，不是按钮，所以不画
//  悬停底色；点进去由调用方接（本库不做文件拖放）。
// -----------------------------------------------------------------------------
void Ui::image_well(const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_lg;
    fill_rect_s(box, th.fill3, rad, th.corner_smoothing);
    stroke_rect_s(box, th.separator, rad, 1.0f, th.corner_smoothing);

    const float ix = box.cx(), iy = box.cy() - 7.0f, s = 15.0f;
    system_icon(SystemIcon::Image, Rect::centered(ix, iy, s * 2.f, s * 1.4f), th.label3);

    Style ts;
    ts.font_size = 11.0f;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;
    draw_text(st.placeholder.empty() ? "拖入图片" : st.placeholder,
              Rect(box.x, iy + s * 1.05f, box.w, 16.0f), th.label2, ts);
}

// -----------------------------------------------------------------------------
//  textarea —— 多行文本框
//  与 input 共享同一套编辑状态（impl->caret / sel_anchor），差别只在"坐标 <-> 码点"
//  的换算要过一层行号。'\n' 硬换行，不做软换行；超出视口时内部纵向滚动。
// -----------------------------------------------------------------------------
bool Ui::textarea(std::string* value, const Rect& box, const Style& st) {
    if (!value) return false;
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("textarea", box, 0,
        st.identity ? st.identity : uint64_t(reinterpret_cast<uintptr_t>(value)));
    const bool enabled = !st.disabled;
    // 和 input() 同一档圆角（6）与同一套皮 —— 多行框不是"另一种控件"，
    // 它是同一个 Text Field 长高了。
    const float rad  = st.radius >= 0.0f ? st.radius : th.radius;
    const float padx = st.pad_x >= 0.0f ? st.pad_x : 10.0f;
    const float pady = st.pad_y >= 0.0f ? st.pad_y : 8.0f;
    const float fs   = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float lh   = std::max(line_height(fs), fs * 1.15f);

    detail::Interact in = detail::interact(impl, id, box, enabled);
    bool focused = detail::editor_focused(impl, id, enabled);
    if (focused && impl->gate_blocked_here(box)) {   // 同上：焦点也要被挡住
        detail::drop_focus(impl);
        focused = false;
    }
    if (in.hovered || focused) set_cursor(Cursor::IBeam);

    bool changed = false;
    if (focused && impl->input.pressed[0] && !in.hovered) { detail::drop_focus(impl); focused = false; }
    if (in.pressed) { detail::grab_focus(impl, id); focused = true; }

    auto split_lines = [](const std::string& s) {
        std::vector<std::string> out;
        std::string cur;
        for (char ch : s) {
            if (ch == '\n') { out.push_back(cur); cur.clear(); }
            else cur.push_back(ch);
        }
        out.push_back(cur);            // 末尾的 '\n' 也算一个空行
        return out;
    };

    // ---- 切行 + 每行码点数 + 行首 caret 偏移 ----
    // 三者都只由全文决定，所以一起缓存：光标闪烁期间每帧都要重画，而大文档
    // 静止时内容一个字节都没变 —— 切一遍是 O(全文) 加每行一次分配，白付。
    //
    // 命中判据是**全文 memcmp**，不是长度或指针相等：程序改文本而长度不变
    // （替换、撤销到同长度）也能立刻发现，不会画出上一版的行。
    // 64 KiB 以下不缓存：重建本身只要几十微秒，留一份副本不划算。
    std::vector<std::string> lines_local;
    std::vector<int> cps_local, base_local;
    const std::vector<std::string>* lines_p = &lines_local;
    const std::vector<int>* cps_p = &cps_local;
    const std::vector<int>* base_p = &base_local;
    Impl::TextLines* tc = nullptr;
    bool discard_text_cache = false;
    // 本帧布局持有缓存内 vector 的引用；超预算条目只能在使用结束后释放。
    detail::ScopeExit trim_text_cache([&] { if (discard_text_cache) impl->text_lines.erase(id); });
    static const size_t text_budget = detail::memory_budget_kib("AINO_TEXT_BUDGET", size_t(16u << 20));
    if (value->size() >= size_t(64u << 10) && value->size() <= text_budget / 2) {
        tc = &impl->text_lines[id]; tc->used = ++impl->text_lines_serial;
    } else impl->text_lines.erase(id);    // 缩回小文档后不再留着整篇副本

    // 重建（或重建后写回缓存）。原地赋值：下面所有 lambda 都按引用捕获
    // lines / cps_n / cp_base，重建后引用仍然有效。
    auto rebuild_lines = [&]() {
        std::vector<std::string> nl = split_lines(*value);
        std::vector<int> nc(nl.size()), nb(nl.size());
        int acc = 0;
        for (size_t i = 0; i < nl.size(); ++i) {
            nc[i] = utf8_count(nl[i]);
            nb[i] = acc;
            acc += nc[i] + 1;           // +1 吃掉换行符
        }
        if (tc) {
            tc->src = *value;
            tc->lines = std::move(nl);
            tc->cps_n = std::move(nc);
            tc->cp_base = std::move(nb);
            if (tc->bytes() > text_budget) {
                discard_text_cache = true;
            } else impl->text_lines.trim_bytes(text_budget, id,
                [](const Impl::TextLines& entry) { return entry.bytes(); },
                [](const Impl::TextLines& entry) { return entry.used; });
            // 数据搬进缓存后局部向量是空的，指针必须跟着改过去。
            lines_p = &tc->lines; cps_p = &tc->cps_n; base_p = &tc->cp_base;
        } else {
            lines_local = std::move(nl);
            cps_local = std::move(nc);
            base_local = std::move(nb);
        }
    };

    const bool lines_cached = tc && tc->src.size() == value->size()
        && (value->empty() || std::memcmp(tc->src.data(), value->data(), value->size()) == 0);
    if (lines_cached) { lines_p = &tc->lines; cps_p = &tc->cps_n; base_p = &tc->cp_base; }
    else              { rebuild_lines(); }
    const std::vector<std::string>& lines = *lines_p;
    const std::vector<int>& cps_n = *cps_p;
    const std::vector<int>& cp_base = *base_p;
    auto to_linecol = [&](int cp) -> std::pair<int, int> {
        int remain = cp < 0 ? 0 : cp;
        // cp_base 单调递增，二分定位行号（原来是从第一行线性扫）。
        const auto it = std::upper_bound(cp_base.begin(), cp_base.end(), remain);
        size_t i = it == cp_base.begin() ? 0 : size_t(it - cp_base.begin()) - 1;
        if (i >= lines.size()) i = lines.size() - 1;
        remain -= cp_base[i];
        if (remain > cps_n[i]) remain = cps_n[i];    // 越过行尾时夹到行尾
        return std::make_pair(int(i), remain);
    };
    auto to_caret = [&](int ln, int col) {
        if (ln < 0) return col < 0 ? 0 : col;
        if (ln >= int(lines.size())) ln = int(lines.size()) - 1;
        return cp_base[size_t(ln)] + (col < 0 ? 0 : col);
    };
    auto line_x = [&](int ln, int col) {
        if (ln < 0 || ln >= int(lines.size())) return 0.0f;
        const int n = cps_n[size_t(ln)];
        if (col > n) col = n;
        const std::vector<uint32_t> cps = utf8_codepoints(lines[size_t(ln)]);
        return text_width(utf8_from_codepoints(cps, 0, size_t(col)), fs, 400);
    };

    float& tscroll = impl->flags[id * 89u + 1u];
    const float view_h   = std::max(box.h - pady * 2.0f, 1.0f);
    const float need_h   = lh * float(lines.size());
    const float max_off  = std::max(0.0f, need_h - view_h);
    int   n_cp     = cp_base.back() + cps_n.back();

    if (max_off > 0.0f && in.hovered && std::fabs(impl->input.wheel) > 0.001f) {
        tscroll = detail::clampf(tscroll - impl->input.wheel * 40.0f, 0.0f, max_off);
        repaint();
    }

    if (focused) {
        if (impl->caret > n_cp) impl->caret = n_cp;
        if (impl->caret < 0) impl->caret = 0;

        // 鼠标 → 光标：单击定位 / 拖选 / 双击选词 / 三击全选 / Shift 扩选
        auto caret_at_mouse = [&]() -> int {
            const float ly = impl->input.mouse.y - (box.y + pady) + tscroll;
            int ln = int(std::floor(ly / lh));
            if (ln < 0) ln = 0;
            if (ln > int(lines.size()) - 1) ln = int(lines.size()) - 1;
            const int col = detail::caret_from_x(*this, lines[size_t(ln)],
                                                 impl->input.mouse.x - (box.x + padx), fs, 400);
            return to_caret(ln, col);
        };
        if (impl->input.pressed[0] && in.hovered) {
            impl->caret = caret_at_mouse();
            if (impl->input.click_count >= 3) {
                impl->sel_anchor = 0;
                impl->caret = n_cp;
            } else if (impl->input.click_count == 2) {
                const std::vector<uint32_t> cps = utf8_codepoints(*value);
                int a = 0, b = 0;
                detail::word_span(cps, impl->caret, &a, &b);
                impl->sel_anchor = a;
                impl->caret = b;
            } else if (!impl->input.shift()) {
                impl->sel_anchor = -1;
            }
            impl->flags[id * 89u + 3u] = float(impl->caret);      // 拖选锚点
        }
        if (impl->input.down[0] && impl->active == id && in.hovered) {
            const int prev = impl->caret;
            impl->caret = caret_at_mouse();
            if (impl->caret != prev && impl->sel_anchor < 0)
                impl->sel_anchor = int(impl->flags[id * 89u + 3u]);
        }

        const Input& kin = impl->input;

        // ---- 撤销 / 重做 ----
        bool undone = false;
        if (kin.ctrl_of('Z') || kin.ctrl_of('Y')) {
            const bool redo = kin.key_pressed['Y'] || (kin.key_pressed['Z'] && kin.shift_of('Z'));
            if (redo ? detail::edit_redo(impl, id, value, &impl->caret, &impl->sel_anchor)
                     : detail::edit_undo(impl, id, value, &impl->caret, &impl->sel_anchor)) {
                n_cp = utf8_count(*value);
                impl->caret = detail::clampi(impl->caret, 0, n_cp);
                impl->caret_blink = 0.0f;
                changed = true;
                undone = true;
            }
        }

        const bool edit_event = !kin.text_input.empty() || kin.key_hit(KEY_BACKSPACE) || kin.key_hit(KEY_DELETE)
            || kin.key_hit(KEY_ENTER) || kin.key_hit(KEY_TAB) || kin.ctrl_of('X') || kin.ctrl_of('V')
            || (impl->paste && impl->paste->id == id && impl->paste->ready);
        const std::string before_text = edit_event && !undone ? *value : std::string();
        const int caret0 = impl->caret, anchor0 = impl->sel_anchor;
        uint64_t edit_kind = detail::EDIT_OTHER;

        int sel_from = impl->sel_anchor >= 0 ? std::min(impl->caret, impl->sel_anchor) : impl->caret;
        int sel_to   = impl->sel_anchor >= 0 ? std::max(impl->caret, impl->sel_anchor) : impl->caret;
        bool has_sel = sel_to > sel_from;

        auto delete_sel = [&]() -> bool {
            if (!has_sel) return false;
            const std::vector<uint32_t> cps = utf8_codepoints(*value);
            std::vector<uint32_t> keep;
            keep.reserve(cps.size());
            for (int i = 0; i < int(cps.size()); ++i)
                if (i < sel_from || i >= sel_to) keep.push_back(cps[i]);
            *value = utf8_from_codepoints(keep, 0, keep.size());
            impl->caret = sel_from;
            impl->sel_anchor = -1;
            changed = true;
            return true;
        };
        auto insert_text = [&](const std::string& ins) {
            delete_sel();
            std::vector<uint32_t> cps = utf8_codepoints(*value);
            const std::vector<uint32_t> add = utf8_codepoints(ins);
            cps.insert(cps.begin() + impl->caret, add.begin(), add.end());
            *value = utf8_from_codepoints(cps, 0, cps.size());
            impl->caret += int(add.size());
            impl->caret_blink = 0.0f;
            changed = true;
        };
        // 选中 [from, to) 后删掉（按词删用的）
        auto delete_span = [&](int from, int to) {
            if (to <= from) return;
            impl->sel_anchor = from;
            impl->caret = to;
            has_sel = true;
            sel_from = from;
            sel_to = to;
            delete_sel();
        };

        if (!undone && kin.key_hit(KEY_BACKSPACE)) {
            edit_kind = detail::EDIT_DELETE;
            if ((kin.ctrl_of(KEY_BACKSPACE) || kin.alt_of(KEY_BACKSPACE)) && !has_sel) {
                delete_span(detail::word_left(utf8_codepoints(*value), impl->caret), impl->caret);
            } else if (!delete_sel() && impl->caret > 0) {
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                cps.erase(cps.begin() + (impl->caret - 1));
                *value = utf8_from_codepoints(cps, 0, cps.size());
                impl->caret -= 1;
                changed = true;
            }
        }
        if (!undone && kin.key_hit(KEY_DELETE)) {
            edit_kind = detail::EDIT_DELETE;
            if ((kin.ctrl_of(KEY_DELETE) || kin.alt_of(KEY_DELETE)) && !has_sel) {
                delete_span(impl->caret, detail::word_right(utf8_codepoints(*value), impl->caret));
            } else if (!delete_sel()) {
                std::vector<uint32_t> cps = utf8_codepoints(*value);
                if (impl->caret < int(cps.size())) {
                    cps.erase(cps.begin() + impl->caret);
                    *value = utf8_from_codepoints(cps, 0, cps.size());
                    changed = true;
                }
            }
        }
        if (!undone && kin.key_hit(KEY_ENTER)) {
            edit_kind = detail::EDIT_TYPE;
            insert_text(kin.ctrl() || kin.shift() ? "\n\n" : "\n");
        }

        // ←/→：Ctrl 按词跳；Home/End：Ctrl 到文档首尾；Shift 一律扩选
        if (!undone && kin.key_hit(KEY_LEFT)) {
            if (!kin.shift()) {
                impl->caret = has_sel ? sel_from
                                      : (kin.ctrl() ? detail::word_left(utf8_codepoints(*value), impl->caret)
                                                    : std::max(0, impl->caret - 1));
                impl->sel_anchor = -1;
            } else {
                if (impl->sel_anchor < 0) impl->sel_anchor = has_sel ? sel_to : impl->caret;
                impl->caret = kin.ctrl() ? detail::word_left(utf8_codepoints(*value), impl->caret)
                                         : std::max(0, impl->caret - 1);
            }
            impl->caret_blink = 0.0f;
        }
        if (!undone && kin.key_hit(KEY_RIGHT)) {
            if (!kin.shift()) {
                impl->caret = has_sel ? sel_to
                                      : (kin.ctrl() ? detail::word_right(utf8_codepoints(*value), impl->caret)
                                                    : std::min(n_cp, impl->caret + 1));
                impl->sel_anchor = -1;
            } else {
                if (impl->sel_anchor < 0) impl->sel_anchor = has_sel ? sel_from : impl->caret;
                impl->caret = kin.ctrl() ? detail::word_right(utf8_codepoints(*value), impl->caret)
                                         : std::min(n_cp, impl->caret + 1);
            }
            impl->caret_blink = 0.0f;
        }
        if (!undone && (kin.key_hit(KEY_UP) || kin.key_hit(KEY_DOWN))) {
            const std::pair<int, int> lc = to_linecol(impl->caret);
            const int target = lc.first + (kin.key_pressed[KEY_UP] ? -1 : 1);
            if (target >= 0 && target < int(lines.size())) {
                // 上下移动保持"视觉列"，所以先量出当前行的像素横坐标再用它反查列
                const float x = line_x(lc.first, lc.second);
                const int col = detail::caret_from_x(*this, lines[size_t(target)], x, fs, 400);
                impl->caret = to_caret(target, col);
                impl->caret_blink = 0.0f;
            }
            if (kin.shift()) { if (impl->sel_anchor < 0) impl->sel_anchor = has_sel ? sel_from : impl->caret; }
            else impl->sel_anchor = -1;
        }
        if (!undone && kin.key_hit(KEY_HOME)) {
            if (kin.shift()) { if (impl->sel_anchor < 0) impl->sel_anchor = impl->caret; }
            else impl->sel_anchor = -1;
            if (kin.ctrl_of(KEY_HOME)) impl->caret = 0;
            else { const std::pair<int, int> lc = to_linecol(impl->caret); impl->caret = to_caret(lc.first, 0); }
            impl->caret_blink = 0.0f;
        }
        if (!undone && kin.key_hit(KEY_END)) {
            if (kin.shift()) { if (impl->sel_anchor < 0) impl->sel_anchor = impl->caret; }
            else impl->sel_anchor = -1;
            if (kin.ctrl_of(KEY_END)) impl->caret = n_cp;
            else {
                const std::pair<int, int> lc = to_linecol(impl->caret);
                impl->caret = to_caret(lc.first, cps_n[size_t(lc.first)]);
            }
            impl->caret_blink = 0.0f;
        }
        // 与单行 input 同一条纪律：可能没绑回调 → 必须判空；无选区不动剪贴板。
        if (!undone && kin.ctrl_of('C')) {
            if (has_sel && impl->try_set_clipboard) {
                const std::vector<uint32_t> cps = utf8_codepoints(*value);
                impl->copy_to_clipboard(utf8_from_codepoints(cps, size_t(sel_from), size_t(sel_to - sel_from)));
            }
        }
        if (!undone && kin.ctrl_of('X') && has_sel && impl->try_set_clipboard) {
            edit_kind = detail::EDIT_OTHER;
            const std::vector<uint32_t> cps = utf8_codepoints(*value);
            if (impl->copy_to_clipboard(utf8_from_codepoints(cps, size_t(sel_from), size_t(sel_to - sel_from))))
                delete_sel();
        }
        if (!undone && (kin.ctrl_of('V') || (impl->paste && impl->paste->id == id && impl->paste->ready))) {
            const std::string paste = impl->paste_text(id, *value, kin.ctrl_of('V'));
            if (!paste.empty()) { edit_kind = detail::EDIT_OTHER; insert_text(paste); }
        }
        if (!undone && kin.ctrl_of('A')) { impl->sel_anchor = 0; impl->caret = n_cp; }
        if (!undone && kin.key_pressed[KEY_ESC]) { detail::drop_focus(impl); focused = false; }
        // 字符输入（Enter 走上面的 KEY_ENTER，所以这里排除掉 ctrl 组合键）
        if (!undone && !kin.ctrl() && impl->text_veto <= 0.0f && !kin.text_input.empty()) {
            edit_kind = detail::EDIT_TYPE;
            std::string ins;
            for (char ch : kin.text_input) if (ch != '\r') ins.push_back(ch);
            if (!ins.empty()) insert_text(ins);
        }

        if (changed && !undone && *value != before_text)
            detail::edit_snapshot(impl, id, before_text, caret0, anchor0, edit_kind);

        // 编辑完重新分行，否则下面的绘制还在用旧的 lines。
        // 行首偏移表也要一起重建 —— 光标定位、选区、视口跟随都读它。
        if (changed) rebuild_lines();
    }

    // 光标行保持在视口内（先按光标位置调整，再统一夹到合法区间）
    {
        const std::pair<int, int> lc = to_linecol(impl->caret);
        const float cy0 = lh * float(lc.first);
        const float cy1 = cy0 + lh;
        if (cy0 - tscroll < 0.0f) tscroll = cy0;
        if (cy1 - tscroll > view_h) tscroll = cy1 - view_h;
    }
    tscroll = detail::clampf(tscroll, 0.0f, max_off);

    // ---------------- 绘制 ----------------
    const float t_hover = detail::animate_value(impl, id * 89u + 2u, (in.hovered && !focused) ? 1.0f : 0.0f, 18.0f);
    // 纯白底 + 顶部内阴影 + 极淡描边（同 input），焦点走 3px 光环而不是"边框染粗"
    Color bg = !st.bg.is_none() ? st.bg : th.window_bg;
    if (st.disabled && bg.a > 0) bg.a = uint8_t(float(bg.a) * 0.45f);
    if (bg.a > 0) fill_rect(box, bg, rad);
    inner_shadow(box, th.is_dark() ? Color(0, 0, 0, 70) : Color(0, 0, 0, focused ? 10 : 18),
                 rad, 1.8f, 1.0f);
    Color bd = !st.border_color.is_none() ? st.border_color : th.separator;
    if (t_hover > 0.01f && !focused) bd = bd.mix(th.label3, t_hover);
    const float bw = st.border_width >= 0.0f ? st.border_width : 1.0f;
    if (bw > 0.0f) stroke_rect(box, bd, rad, bw);

    // 组字串优先于 placeholder（空框 + hint 很常见，不能让 hint 把拼音顶掉）
    const bool composing = focused && !impl->ime_comp.empty();
    const bool show_ph = value->empty() && !st.placeholder.empty() && !composing;
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 400;
    ts.align = Align::Left;
    ts.valign = VAlign::Top;

    push_clip(box);

    // 选区：按行画，跨行时每行一段，视觉上连续
    if (focused && impl->sel_anchor >= 0) {
        const int sf  = std::min(impl->caret, impl->sel_anchor);
        const int stt = std::max(impl->caret, impl->sel_anchor);
        // 从可见区第一行开始：整篇被选中时，屏幕外的行不需要量宽度（每行要
        // 两次 utf8_codepoints + 一次字体度量）。前缀表让起点可以直接算出来。
        size_t i0 = 0;
        if (lh > 0.01f) {
            const float first_y = box.y + pady - tscroll;
            if (first_y < box.y) i0 = size_t(std::max(0.0f, std::floor((box.y - first_y) / lh)));
            if (i0 >= lines.size()) i0 = lines.size() - 1;
        }
        for (size_t i = i0; i < lines.size(); ++i) {
            const int base = cp_base[i];
            const int n = cps_n[i];
            const float y = box.y + pady + lh * float(i) - tscroll;
            if (y > box.bottom()) break;
            const int a = std::max(sf, base) - base;
            const int b = std::min(stt, base + n) - base;
            if (b > a) {
                const float x0 = line_x(int(i), a);
                const float x1 = line_x(int(i), b);
                fill_rect(Rect(box.x + padx + x0, y, x1 - x0, lh), th.tint.with_alpha(52), 2.0f);
            }
            // 行尾的换行符也被选中时补一小段，不然"选中换行"看不出反馈
            if (stt > base + n && sf <= base + n)
                fill_rect(Rect(box.x + padx + line_x(int(i), n), y, 6.0f, lh), th.tint.with_alpha(52), 2.0f);
        }
    }

    // 占位符 25%（同 Text Field）
    const Color fg = show_ph ? th.label3 : (!st.fg.is_none() ? st.fg : th.label);

    // IME 预编辑串只挂在一行上（caret 所在行），在这里算一次就够
    std::pair<int, int> comp_lc(-1, -1);
    float comp_w = 0.0f;
    if (composing) { comp_lc = to_linecol(impl->caret); comp_w = text_width(impl->ime_comp, fs, 400); }

    size_t draw_from = 0;
    if (lh > 0.01f && tscroll > 0.0f)
        draw_from = size_t(std::max(0.0f, std::floor(tscroll / lh)));
    for (size_t i = draw_from; i < lines.size(); ++i) {
        const float y = box.y + pady + lh * float(i) - tscroll;
        if (y > box.bottom()) break;
        if (y + lh < box.y) continue;
        const std::string text_line = (show_ph && i == 0) ? st.placeholder : lines[i];
        if (composing && int(i) == comp_lc.first) {
            // 与 input 同理：正文前半 / 拼音（主色+下划线）/ 正文后半
            const std::vector<uint32_t> cps = utf8_codepoints(text_line);
            const size_t cut = size_t(detail::clampi(comp_lc.second, 0, int(cps.size())));
            const std::string before = utf8_from_codepoints(cps, 0, cut);
            const std::string after  = utf8_from_codepoints(cps, cut);
            const float bx = box.x + padx;
            const float bw = text_width(before, fs, 400);
            draw_text(before, Rect(bx, y, std::max(bw, 1.0f), lh), fg, ts);
            draw_text(impl->ime_comp, Rect(bx + bw, y, comp_w + 2.0f, lh), th.tint, ts);
            fill_rect(Rect(bx + bw, y + lh * 0.78f, comp_w, 1.0f),
                      th.tint.with_alpha(170), 0.0f);
            draw_text(after, Rect(bx + bw + comp_w, y, std::max(box.w, 1.0f), lh), fg, ts);
        } else {
            draw_text(text_line, Rect(box.x + padx, y, std::max(box.w - padx * 2.0f, 1.0f), lh), fg, ts);
        }
    }

    // 光标（IME 的组合窗/候选窗也跟着它走，所以位置先算出来）
    if (focused) {
        const std::pair<int, int> lc = to_linecol(impl->caret);
        const float cx = box.x + padx + line_x(lc.first, lc.second)
                       + (composing && lc.first == comp_lc.first ? comp_w : 0.0f);
        float cy = box.y + pady + lh * float(lc.first) - tscroll;
        if (std::fmod(impl->caret_blink, 1.06f) < 0.62f) {
            if (cy + lh > box.y && cy < box.bottom())
                fill_rect(Rect(cx, cy + lh * 0.06f, 1.4f, lh * 0.88f), th.label, 0.7f);
        }
        impl->schedule_caret();
        if (impl->ime_pos) {
            if (impl->ime_content_type) impl->ime_content_type(false, true);
            // 光标滚出可视区时把 IME 钉在最近的那一行，别让它飞到框外
            const float iy = detail::clampf(cy, box.y, std::max(box.y, box.bottom() - lh));
            const Rect pb = impl->phys(Rect(cx, iy, 1.0f, lh));
            impl->ime_pos(pb.x, pb.y, pb.bottom());
        }
    }

    pop_clip();

    if (focused) detail::paint_focus_ring(*this, box, rad, th.focus_ring, true);
    return changed;
}

// -----------------------------------------------------------------------------
//  rating —— 星级评分
//  星形是凹多边形，普通圆角矩形光栅化器画不了，所以这里用 raster_poly
//  （奇偶规则扫描线填充）。点第 i 颗星 → *value = i+1；再点当前值 → 清零。
// -----------------------------------------------------------------------------
bool Ui::rating(int* value, int max_stars, const Rect& box, const Style& st) {
    if (!value || max_stars <= 0 || max_stars > 32) return false;
    // 星形是直接 raster_poly 到画布的，不经过图元函数 —— 测量模式必须自己拦一道，
    // 否则"先量高度"那趟会在屏幕左上角画出一排真星星。
    if (impl->measure_on) { impl->measure_add(box); return false; }
    const Theme& th = impl->theme;
    uint64_t base = impl->widget_id("rating", box, 0, st.identity);
    const float gap = 4.0f;
    const float size = std::min((box.w - gap * float(max_stars - 1)) / float(max_stars), box.h);
    if (size <= 3.0f) return false;
    const float top = box.y + (box.h - size) * 0.5f;
    const float r_out = size * 0.5f;
    const float r_in  = r_out * 0.44f;

    // 空星：先压平再判断"是不是浅到看不见" —— th.fill3 本身是半透明的，量它自己的
    // 亮度永远是 0，这个判断从来不会生效，空星就一直停在 5% 黑上。
    Color empty_c = th.fill3.over(th.window_bg);
    if (empty_c.luminance() > 0.82f) empty_c = th.separator;     // 浅色主题下 muted 太淡，用 border
    const Color full_c = !st.bg.is_none() ? st.bg : th.tint;

    bool changed = false;
    for (int i = 0; i < max_stars; ++i) {
        const Rect sr(box.x + float(i) * (size + gap), top, size, size);
        uint64_t id = impl->widget_id("star", sr, base + uint64_t(i), st.identity ? base + uint64_t(i) : 0);
        detail::Interact in = detail::interact(impl, id, sr, !st.disabled);
        if (in.hovered) set_cursor(Cursor::Arrow);
        if (in.pressed) detail::grab_focus(impl, id);
        if (in.clicked) {
            const int nv = (*value == i + 1) ? 0 : (i + 1);
            if (nv != *value) { *value = nv; changed = true; }
        }
        const float t   = detail::animate_state(impl, id * 67u + 1u, (i < *value) ? 1.0f : 0.0f, 24.0f);
        const float t_h = detail::animate_value(impl, id * 67u + 2u, in.hovered ? 1.0f : 0.0f, 18.0f);

        if (!impl->canvas) continue;
        Color c = empty_c.mix(full_c, t);
        if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
        const std::vector<Vec2> pts = detail::star_points(impl->px(sr.cx()), impl->px(sr.cy()),
                                                         5, impl->px(r_out), impl->px(r_in));
        detail::raster_poly(*impl->canvas, pts, c);

        // 悬停时勾一圈细边，让"可点"这件事看得出来
        if (t_h > 0.01f && t < 0.9f) {
            const Color hc = th.label.with_alpha(uint8_t(80.0f * t_h));
            for (size_t k = 0; k < pts.size(); ++k) {
                const Vec2& p0 = pts[k];
                const Vec2& p1 = pts[(k + 1) % pts.size()];
                detail::raster_line(*impl->canvas, p0.x, p0.y, p1.x, p1.y, hc, impl->px(1.0f));
            }
        }
    }
    return changed;
}

// =============================================================================
//  8.13  展示补充
// =============================================================================

// 头像：圆形底 + 首个码点。传 "张三" 画"张"，传 "AB" 画"A"。
void Ui::avatar(const std::string& text, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const float d = std::min(box.w, box.h);
    if (d <= 2.0f) return;
    const Rect c = Rect::centered(box.cx(), box.cy(), d, d);

    // 默认底 = 10% 的填充（`th.fill`）。用 `th.fill3`（5%）在浅色主题下几乎看不见，
    // 头像就只剩一个描边圈；10% 既能当"没照片的占位"，又不至于抢照片位。
    Color bg = !st.bg.is_none() ? st.bg : th.fill;
    if (st.disabled) bg.a = uint8_t(float(bg.a) * 0.45f);
    fill_rect(c, bg, d * 0.5f);
    // 默认 1px 描边保证头像在浅色容器内保持可辨识轮廓。
    const float bw = st.border_width >= 0.0f ? st.border_width : 1.0f;
    if (bw > 0.0f) {
        const Color bc = !st.border_color.is_none() ? st.border_color : th.separator;
        stroke_rect(c, bc, d * 0.5f, bw);
    }

    const std::vector<uint32_t> cps = utf8_codepoints(text);
    std::string initial = cps.empty() ? std::string("?") : utf8_from_codepoints(cps, 0, 1);

    Style ts;
    ts.font_size = st.font_size > 0.0f ? st.font_size : d * 0.42f;
    ts.font_weight = 600;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;
    // 前景色按**压平后**的底色判断。默认底是 th.fill3 = rgba(0,0,0,13)：量它自己的
    // 亮度是 0（纯黑），于是"浅色主题给白字" —— 白字落在 5% 黑的圆上，等于隐形。
    const Color eff = bg.over(th.window_bg);
    Color fg = !st.fg.is_none() ? st.fg
             : (eff.luminance() > 0.55f ? th.label : Color(255, 255, 255));
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    draw_text(initial, c, fg, ts);
}

// 加载指示器：底环 + 一段绕圈旋转的弧。
//
// 两者都是**一次成形的环/弧**（raster_arc），不是拼线段 —— 拼线段会在接缝处
// 叠色，看起来一圈一节一节的"碎片"。也更快：整圈一次扫描 vs 84 次。
//
// 弧身**没有端点**：整圈画满，用沿角度连续变化的 alpha 表示方向（拖尾）。
// 带端点的弧（圆头 / 平头 / 渐隐）都会在转到起点时"断"一下 —— 尾端重新长出来，
// 用户看到的就是接缝。COS 型 alpha 是周期的，首尾数值相等，绕多少圈都无缝。
void Ui::spinner(const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const float d = std::min(box.w, box.h);
    if (d <= 6.0f) return;
    // 同 rating：底环与拖尾环直接 raster_arc / raster_ring_tail 到画布
    if (impl->measure_on) { impl->measure_add(box); return; }

    Color track = !st.border_color.is_none() ? st.border_color : th.fill3;
    Color arc   = !st.bg.is_none() ? st.bg : th.tint;
    if (st.disabled) { track.a = uint8_t(float(track.a) * 0.45f); arc.a = uint8_t(float(arc.a) * 0.45f); }

    const bool hovered = this->hovered(box);
    const bool visible = !impl->clip_logic_on || !box.intersect(impl->clip_logic_cur).empty();
    const bool spin = !st.disabled && visible && !impl->gate_blocked_pointer()
                   && (st.animated || hovered);

    const float cx = impl->px(box.cx());
    const float cy = impl->px(box.cy());
    const float rad = impl->px(std::max(d * 0.5f - 1.5f, 1.0f));
    const float lw  = impl->px(std::max(1.5f, d * 0.12f));

    // 底环：整圈
    detail::raster_arc(*impl->canvas, cx, cy, rad, lw, track, 0.0f, 0.0f, 1.0f);

    float& ang = impl->phase[impl->widget_id("spinner_phase", box)];
    if (spin) {
        ang += impl->dt * 4.6f;                    // 角速度：约 0.73 圈/秒
        if (ang > 6.2831853f) ang = std::fmod(ang, 6.2831853f);
        repaint();                                 // 只有"该转"的时候才要帧
    }

    // 拖尾环：头端满色，对面最暗但不是 0（留 14% 底，环始终连成完整一圈）。
    detail::raster_ring_tail(*impl->canvas, cx, cy, rad, lw, arc, ang, 0.14f);
}

// Alert 默认显示符号、标题和描述；显式背景或 Secondary 变体可绘制底面。标题使用标签色。
void Ui::alert(const std::string& title, const std::string& description, const Rect& box, const Style& st) {
    // 不铺底时整块区域没有 fill_rect 可登记，测量阶段会只剩文字的包围盒 —— 两条
    // 提示条叠出来的高度会比实际矮一大截，第二个的描述被裁掉。所以自己登记。
    if (impl->measure_on) { impl->measure_add(box); return; }

    const Theme& th = impl->theme;
    const float rad = st.radius >= 0.0f ? st.radius : 10.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : detail::kAlertPad;

    const bool danger = (st.variant == Style::Destructive);
    const bool quiet  = (st.variant == Style::Secondary);

    Color bg = st.bg;
    if (bg.is_none() && quiet) bg = th.fill4;
    Color head_c = !st.fg.is_none() ? st.fg : th.label;
    Color icon_c = danger ? th.red : th.label2;
    if (st.disabled) {
        bg.a     = uint8_t(float(bg.a) * 0.45f);
        head_c.a = uint8_t(float(head_c.a) * 0.45f);
        icon_c.a = uint8_t(float(icon_c.a) * 0.45f);
    }

    if (bg.a > 0) fill_rect(box, bg, rad);

    // 提示符号使用系统的 info / warning，与其他图标采用同一来源。
    const float d = std::min(fs + 3.0f, std::max(12.0f, box.h - pad * 2.0f));
    const float lh = line_height(fs);
    const float icy = box.y + pad + lh * 0.5f;           // 与标题首行中心对齐
    const float icx = box.x + pad + d * 0.5f;
    system_icon(danger ? SystemIcon::Warning : SystemIcon::Info,
                Rect::centered(icx, icy, d, d), icon_c);

    const float tx = icx + d * 0.5f + 9.0f;
    const float tw = std::max(box.right() - pad - tx, 1.0f);

    if (description.empty()) {
        Style ts;
        ts.font_size = fs;
        ts.font_weight = 600;
        ts.align = Align::Left;
        ts.valign = VAlign::Middle;
        draw_text(title, Rect(tx, box.y, tw, box.h), head_c, ts);
    } else {
        Style hs;
        hs.font_size = fs;
        hs.font_weight = 600;
        hs.align = Align::Left;
        hs.valign = VAlign::Top;
        draw_text(title, Rect(tx, box.y + pad, tw, lh), head_c, hs);
        Style ds;
        ds.font_size = fs;
        ds.font_weight = 400;
        ds.align = Align::Left;
        ds.valign = VAlign::Top;
        ds.wrap = true;
        draw_text(description, Rect(tx, box.y + pad + lh + 2.0f, tw,
                                    std::max(box.h - pad * 2.0f - lh - 2.0f, 1.0f)),
                  th.label2, ds);
    }
}

// 面包屑：a › b › c，最后一段高亮
void Ui::breadcrumb(const std::vector<std::string>& items, const Rect& box, const Style& st) {
    if (items.empty()) return;
    const Theme& th = impl->theme;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    push_clip(box);
    float x = box.x;
    for (size_t i = 0; i < items.size(); ++i) {
        const bool last = (i + 1 == items.size());
        const float w = text_width(items[i], fs, last ? 500 : 400);
        Style ts;
        ts.font_size = fs;
        ts.font_weight = last ? 500 : 400;
        ts.align = Align::Left;
        ts.valign = VAlign::Middle;
        Color c = last ? (!st.fg.is_none() ? st.fg : th.label) : th.label2;
        if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
        draw_text(items[i], Rect(x, box.y, w + 1.0f, box.h), c, ts);
        x += w;
        if (!last) {
            const float mx = x + 7.0f, my = box.cy();
            const Color ac = th.label2.scaled_alpha(0.85f);
            system_icon(SystemIcon::Next, Rect::centered(mx, my, fs * .7f, fs * .7f), ac);
            x += 14.0f;
        }
    }
    pop_clip();
}

// 按键提示键帽
void Ui::kbd(const std::string& text, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    // macOS 的快捷键提示就是一行 11px 的灰字（⌘K），没有键帽。
    // 这里保留一个极轻的 5% 底 + r4 让它在正文里可辨识，但去掉"机械键帽"那圈
    // 立体下沿 —— 那是网页键盘图的画法。
    const float rad = st.radius >= 0.0f ? st.radius : 4.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : 11.0f;
    Color bg = !st.bg.is_none() ? st.bg : th.fill3;
    Color fg = !st.fg.is_none() ? st.fg : th.label2;
    if (st.disabled) { bg.a = uint8_t(float(bg.a) * 0.45f); fg.a = uint8_t(float(fg.a) * 0.45f); }
    if (bg.a > 0) fill_rect(box, bg, rad);
    if (!st.border_color.is_none())
        stroke_rect(box, st.border_color, rad, st.border_width >= 0.0f ? st.border_width : 1.0f);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;
    draw_text(text, Rect(box.x, box.y - 1.0f, box.w, box.h), fg, ts);
}

// 折叠面板：只有标题行可点，内容由调用方在 *open 为真时自己画。
// 返回 true 表示本帧切换了展开状态。
bool Ui::accordion(const std::string& title, bool* open, const Rect& box, const Style& st) {
    if (!open || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("accordion", box, detail::hash_string(title.c_str()), st.identity);
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    // 折叠行是列表行：24 高（= Global/Height Medium），不是 Web 手风琴的 38
    const float head_h = std::min(box.h, std::max(fs + 8.0f, 24.0f));
    const Rect head(box.x, box.y, box.w, head_h);
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;

    detail::Interact in = detail::interact(impl, id, head, !st.disabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    if (in.pressed) detail::grab_focus(impl, id);
    bool changed = false;
    if (in.clicked) { *open = !*open; changed = true; }

    const float t_h = detail::animate_value(impl, id * 71u + 1u, in.hovered ? 1.0f : 0.0f, 18.0f);
    if (t_h > 0.02f) fill_rect(head, th.fill3.scaled_alpha(t_h), rad);

    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    Color c = !st.fg.is_none() ? st.fg : th.label;
    if (st.disabled) c.a = uint8_t(float(c.a) * 0.45f);
    // 三角在**行首**（NSDisclosureGroup / 系统设置的披露行都这样），标题跟在它
    // 后面缩进 22。把三角放到行尾是 Web 手风琴的写法（右侧 chevron），在 mac
    // 界面里会让"这一行能不能展开"变得很不显眼。
    draw_text(title, Rect(head.x + 22.0f, head.y, std::max(head.w - 34.0f, 1.0f), head.h), c, ts);

    system_icon(*open ? SystemIcon::ChevronDown : SystemIcon::Next,
                Rect::centered(head.x + 10, head.cy(), 12, 12), th.label2);

    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, head, rad, th.focus_ring);
    return changed;
}

// 表格使用等宽列；pad_x 只控制单元格内边距。
void Ui::table(const std::vector<std::string>& headers,
                      const std::vector<std::vector<std::string>>& rows,
                      const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    size_t cols = headers.size();
    if (cols == 0) for (const auto& r : rows) cols = std::max(cols, r.size());
    if (cols == 0 || box.w <= 4.0f || box.h <= 4.0f) return;

    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float head_h = std::max(fs + 10.0f, 24.0f);   // kit: 表头 24，不是 Web 的 36
    // macOS 的表格行是紧的：正文 13pt + 7 → 20pt 行高，不是 Web 表格那种 34
    const float row_h  = std::max(fs + 7.0f, 20.0f);
    const float col_w  = box.w / float(cols);
    const float cell_pad = st.pad_x >= 0.0f ? st.pad_x : 12.0f;

    push_clip(box);

    // 每列是不是数字列：看该列第一个非空单元格。表头要跟着数据一起右对齐，
    // 否则"耗时"贴在左边、"12 ms"贴在右边，看着像两列。
    std::vector<bool> num_col(cols, false);
    for (size_t c = 0; c < cols; ++c) {
        for (const auto& row : rows) {
            if (c < row.size() && !row[c].empty()) { num_col[c] = detail::looks_numeric(row[c]); break; }
        }
    }

    const Rect hr(box.x, box.y, box.w, head_h);
    // 表头不铺底色（macOS 列表的表头就是一行灰字 + 一条分隔线），传了 bg 才铺
    if (!st.bg.is_none()) fill_rect(hr, st.bg, rad);
    for (size_t c = 0; c < cols; ++c) {
        const Rect cr(hr.x + col_w * float(c), hr.y, col_w, hr.h);
        // 列之间不画竖线：macOS 的列表靠留白和行分隔线分列，竖线是 Excel/Web 的语汇
        if (c >= headers.size()) continue;
        Style ts;
        ts.font_size = 11.0f;               // Subheadline 11/14
        ts.font_weight = 600;
        ts.align = num_col[c] ? Align::Right : Align::Left;
        ts.valign = VAlign::Middle;
        Color cc = th.label2;
        if (st.disabled) cc.a = uint8_t(float(cc.a) * 0.45f);
        draw_text(headers[c], cr.inset(cell_pad, 0.0f), cc, ts);
    }
    fill_rect(Rect(box.x, hr.bottom() - 1.0f, box.w, 1.0f), th.separator, 0.0f);

    for (size_t r = 0; r < rows.size(); ++r) {
        const Rect rr(box.x, hr.bottom() + row_h * float(r), box.w, row_h);
        if (rr.y >= box.bottom()) break;
        // macOS 表格默认不画斑马纹。要隔行底色得显式传 Style().bg(...)。
        // 系统自己只在「交替行」打开时才画（Miscellaneous/Tables/Alternating Row Color）。
        if (r % 2 == 1 && !st.bg.is_none()) fill_rect(rr, th.row_alt, 0.0f);
        for (size_t c = 0; c < cols && c < rows[r].size(); ++c) {
            Style ts;
            ts.font_size = fs;
            ts.font_weight = 400;
            // macOS 表格的数字列一律右对齐（小数点对齐是 Finder 的规矩）。
            // 用上面预算好的 num_col：逐格再判一次，等于把每行的字符串又扫一遍。
            ts.align = num_col[c] ? Align::Right : Align::Left;
            ts.valign = VAlign::Middle;
            Color cc = !st.fg.is_none() ? st.fg : th.label;
            if (st.disabled) cc.a = uint8_t(float(cc.a) * 0.45f);
            draw_text(rows[r][c], Rect(rr.x + col_w * float(c), rr.y, col_w, rr.h).inset(cell_pad, 0.0f), cc, ts);
        }
        if (r + 1 < rows.size())
            fill_rect(Rect(rr.x, rr.bottom() - 1.0f, rr.w, 1.0f), th.separator, 0.0f);
    }

    pop_clip();
    // 外侧不描边：macOS 的列表/表格没有一圈边框，靠分隔线和留白分块
}

// 迷你折线图。默认在折线下方垫一层低透明度填充（经典 sparkline 观感），
// 只想要裸折线就传 Style::ghost()。
void Ui::sparkline(const std::vector<float>& values, const Rect& box, const Style& st) {
    if (values.size() < 2 || box.w < 4.0f || box.h < 4.0f) return;
    float lo = values[0], hi = values[0];
    for (float v : values) { lo = std::min(lo, v); hi = std::max(hi, v); }
    if (hi - lo < 1e-6f) { hi = lo + 1.0f; lo -= 1.0f; }

    const Theme& th = impl->theme;
    const float pad = 2.0f;
    const float iw  = std::max(box.w - pad * 2.0f, 1.0f);
    const float ih  = std::max(box.h - pad * 2.0f, 1.0f);
    const float step = iw / float(values.size() - 1);
    const Color line_c = !st.bg.is_none() ? st.bg : th.tint;
    const float lw = st.border_width > 0.0f ? st.border_width : 1.8f;

    std::vector<Vec2> pts;
    pts.reserve(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        const float x = box.x + pad + step * float(i);
        const float y = box.y + pad + ih * (1.0f - (values[i] - lo) / (hi - lo));
        pts.push_back(Vec2(impl->px(x), impl->px(y)));
    }

    if (impl->canvas && st.variant != Style::Ghost) {
        std::vector<Vec2> area = pts;
        area.push_back(Vec2(impl->px(box.right() - pad), impl->px(box.bottom() - pad)));
        area.push_back(Vec2(impl->px(box.x + pad), impl->px(box.bottom() - pad)));
        Color fc = line_c;
        fc.a = 30;
        if (st.disabled) fc.a = 14;
        detail::raster_poly(*impl->canvas, area, fc);
    }
    for (size_t i = 1; i < pts.size(); ++i)
        draw_line(pts[i - 1].x / impl->scale, pts[i - 1].y / impl->scale,
                  pts[i].x / impl->scale, pts[i].y / impl->scale, line_c, lw);
}

// 迷你柱状图，负值使用 muted 色。
void Ui::bars(const std::vector<float>& values, const Rect& box, const Style& st) {
    if (values.empty() || box.w < 4.0f || box.h < 4.0f) return;
    const Theme& th = impl->theme;
    float hi = 0.0f;
    for (float v : values) hi = std::max(hi, std::fabs(v));
    if (hi < 1e-6f) return;

    const float gap = values.size() > 1 ? std::max(2.0f, box.w * 0.02f) : 0.0f;
    const float bw  = std::max(1.0f, (box.w - gap * float(values.size() - 1)) / float(values.size()));
    const float rad = st.radius >= 0.0f ? st.radius : std::min(3.0f, bw * 0.5f);
    const Color pos_c = !st.bg.is_none() ? st.bg : th.tint;

    for (size_t i = 0; i < values.size(); ++i) {
        const float t = detail::clampf(std::fabs(values[i]) / hi, 0.0f, 1.0f);
        const float h = std::max(1.0f, box.h * t);
        Color cc = values[i] < 0.0f ? th.fill3 : pos_c;
        if (st.disabled) cc.a = uint8_t(float(cc.a) * 0.45f);
        fill_rect(Rect(box.x + (bw + gap) * float(i), box.bottom() - h, bw, h), cc, rad);
    }
}

// =============================================================================
//  8.14  浮层类
// =============================================================================

// 悬停提示。点击态与普通控件一致，但显示条件多一条：指针停留满 0.4 秒。
// 只在这段计时窗口内请求重绘，弹出之后不再要帧 —— 否则指针压着不动也会
// 常驻刷新，把空闲 CPU 从 0% 抬起来。
void Ui::tooltip(const Rect& anchor, const std::string& text, const Style& st) {
    if (text.empty() || st.disabled) return;
    // 锚点不产生 interact，得自己登记成悬停区域，否则指针移上来不会重绘，
    // 计时器根本没机会开始跑。
    impl->hover_region(anchor);

    // 悬停 0.4 秒后显示，避免划过相邻控件时连续闪现。
    const uint64_t tid = impl->widget_id("tip", anchor, uint64_t(text.size() * 131u + 7u));
    float& since = impl->flags[tid * 7u + 5u];
    const Input& kin = impl->input;
    if (!anchor.contains(kin.mouse.x, kin.mouse.y) || impl->gate_blocked_pointer()) {
        since = 0.0f;                 // 离开即重新计时，回来要再等一次
        return;
    }
    if (since <= 0.0f) since = impl->clock;
    if (impl->clock - since < 0.40f) {
        repaint();                    // 计时靠帧推进；显示之后就不再请求
        return;
    }

    const Theme& th = impl->theme;
    // Tooltip 默认 11pt Medium，水平与垂直内边距分别为 6px 和 3px。
    const float fs = st.font_size > 0.0f ? st.font_size : 11.0f;
    const float padx = 6.0f, pady = 3.0f;
    const float w = text_width(text, fs, 500) + padx * 2.0f;
    const float h = std::max(line_height(fs), fs * 1.2f) + pady * 2.0f;
    float x = anchor.cx() - w * 0.5f;
    float y = anchor.y - h - 7.0f;
    if (y < 4.0f) y = anchor.bottom() + 7.0f;          // 上方放不下就翻到下方
    x = detail::clampf(x, 4.0f, std::max(4.0f, width() - w - 4.0f));

    const Style st_copy = st;
    deferred([this, x, y, w, h, text, fs, th, st_copy]() mutable {
        const Rect box(x, y, w, h);
        detail::pop_shadow(*this, box, 5.0f, th, 0);
        fill_rect(box, !st_copy.bg.is_none() ? st_copy.bg : th.tooltip_bg, 5.0f);
        if (!th.tooltip_stroke.is_none() && th.tooltip_stroke.a > 0)
            stroke_rect(box, th.tooltip_stroke, 5.0f, 0.6f);
        Style ts;
        ts.font_size = fs;
        ts.font_weight = 500;
        ts.align = Align::Center;
        ts.valign = VAlign::Middle;
        draw_text(text, box, !st_copy.fg.is_none() ? st_copy.fg : th.label, ts);
    });
}

// 下拉菜单。
// 关键点：**交互在帧内做，绘制推迟到帧末**。这样既保证了弹层盖住其它组件，
// 又能让返回值在点击当帧就生效（不用等到下一帧）。
bool Ui::menu(const std::string& label, const std::vector<std::string>& items,
                     int* clicked_index, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    uint64_t id = impl->widget_id("menu", box, 0, st.identity);
    // 同 select：触发器在 claim 之前就 interact 过了，层级号要取入口处。
    const uint64_t z_first = impl->seq + 1;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const bool enabled = !st.disabled;

    bool open = impl->flags[id] > 0.5f;
    detail::Interact in = detail::interact(impl, id, box, enabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    const float t_hover = detail::animate_value(impl, id * 73u + 1u, (in.hovered || open) ? 1.0f : 0.0f, 18.0f);
    const float t_press = detail::animate_value(impl, id * 73u + 2u, (in.held && in.hovered) ? 1.0f : 0.0f, 34.0f);

    detail::Skin sk = detail::make_skin(th, st, Style::Outline, rad);
    if (st.font_weight <= 0) sk.weight = 500;
    detail::paint_surface(*this, box, sk, t_hover, t_press, st.disabled);

    const float pad = st.pad_x >= 0.0f ? st.pad_x : 12.0f;
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    Color fg = sk.fg;
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    draw_text(label, Rect(box.x + pad, box.y, std::max(box.w - pad * 2.0f - 18.0f, 1.0f), box.h), fg, ts);
    system_icon(SystemIcon::ChevronDown,
                Rect::centered(box.right() - pad - 5, box.cy(), 12, 12), fg);
    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box, rad, th.focus_ring);
    if (in.pressed) detail::grab_focus(impl, id);
    if (in.clicked) {
        open = !open;
        impl->flags[id] = open ? 1.0f : 0.0f;
        repaint();
    }

    int chosen = -1;
    bool closing = false;
    if (open && !items.empty()) {
        const float item_h = std::max(box.h, 24.0f);   // kit Menu/Height Medium = 24
        float list_w = box.w;
        for (const auto& it : items) list_w = std::max(list_w, text_width(it, fs, 500) + 34.0f);
        const float list_h = item_h * float(items.size()) + 8.0f;
        float ly = box.bottom() + 4.0f;
        if (ly + list_h > height() - 4.0f) ly = std::max(4.0f, box.y - 4.0f - list_h);
        const Rect list(box.x, ly, list_w, list_h);

        std::vector<Rect>    rects;
        std::vector<uint64_t> ids;
        std::vector<uint8_t> hov;
        rects.reserve(items.size());
        ids.reserve(items.size());
        hov.reserve(items.size());
        for (size_t i = 0; i < items.size(); ++i) {
            const Rect r(list.x + 4.0f, list.y + 4.0f + item_h * float(i), list.w - 8.0f, item_h);
            rects.push_back(r);
            ids.push_back(impl->widget_id("menu_item", r, id + i, st.identity ? id + i : 0));
            hov.push_back(0);
        }
        {
        Impl::SeqFrameEnd guard(impl);   // 项在帧内交互、帧末绘制，序号算帧末层
        for (size_t i = 0; i < items.size(); ++i) {
            detail::Interact ii = detail::interact(impl, ids[i], rects[i], true);
            if (ii.hovered) { set_cursor(Cursor::Arrow); hov[i] = 1; }
            if (ii.clicked) chosen = int(i);
        }
        }
        const bool inside_list = list.contains(impl->input.mouse.x, impl->input.mouse.y);
        closing = impl->input.key_pressed[KEY_ESC] ||
                  ((impl->input.pressed[0] || impl->input.pressed[1]) && !inside_list
                   && !box.contains(impl->input.mouse.x, impl->input.mouse.y));

        // 打开时把"触发器 + 下拉列表"登记为可交互区，其余地方全被闸门挡住。
        // 触发器在普通层；列表帧末才画，层级用帧末基准（否则同帧后面画的控件会穿透）。
        if (chosen < 0 && !closing) {
            impl->gate_claim(box, z_first);
            impl->gate_claim(list, Impl::Z_FRAME_END);
            std::vector<std::string> items_copy = items;
            deferred([this, list, rects, ids, hov, items_copy, fs]() mutable {
                detail::paint_menu(*this, list, items_copy, rects, ids, hov, -1, false, fs);
            });
        }
    }

    if (chosen >= 0) {
        if (clicked_index) *clicked_index = chosen;
        impl->flags[id] = 0.0f;
        repaint();
        return true;
    }
    if (closing) {
        impl->flags[id] = 0.0f;
        repaint();
    }
    return false;
}

// 菜单栏。和 menu() 的差别只有一条，但这条恰恰是桌面菜单栏的灵魂：
// **点开一个之后，鼠标划到隔壁会自动切过去**（Web 上一堆库都做不到）。
//
// 实现上靠的顺序很要紧。gate_claim() 登记的是**下一帧**的白名单，所以：
//   1) 本帧先 interact 所有顶级项（此时闸门还是上一帧的状态）；
//   2) 再处理点击 / 划动切换，把最终的 open_idx 算出来；
//   3) 这时才把**整条菜单栏** claim 进去 —— 只有这样下一帧隔壁那几项
//      才收得到 hover，联动才成立。只 claim 触发器本身的话，鼠标一离开
//      当前项就被闸门挡住，看起来像"菜单卡住了"。
MenuPick Ui::menubar(const std::vector<std::string>& labels,
                            const std::vector<std::vector<std::string>>& items,
                            const Rect& box, const Style& st) {
    MenuPick pick;
    const Theme& th = impl->theme;
    const uint64_t bar_id = impl->widget_id("menubar", box, 0, st.identity);
    // 顶级项在 claim 之前就 interact 过了，层级号必须在**入口处**取。
    const uint64_t z_first = impl->seq + 1;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;
    const bool enabled = !st.disabled;
    const float padx = st.pad_x >= 0.0f ? st.pad_x : 12.0f;

    // flags 里存「打开的是第几个 + 1」，0 表示全关（-1 也要能表示，所以不直接存下标）。
    int open_idx = int(impl->flags[bar_id]) - 1;

    // --- 底板：默认 Ghost（透明），只留一条底边线，正好当菜单栏用 ---
    detail::Skin sk = detail::make_skin(th, st, Style::Ghost, rad);
    if (st.font_weight <= 0) sk.weight = 500;
    detail::paint_surface(*this, box, sk, 0.0f, 0.0f, st.disabled);
    draw_line(box.x, box.bottom() - 0.5f, box.right(), box.bottom() - 0.5f, th.separator, 1.0f);

    // --- 顶级项排布：文字宽 + 左右各 padx，从左边依次排 ---
    std::vector<Rect> trg(labels.size());
    {
        float x = box.x + 4.0f;
        for (size_t i = 0; i < labels.size(); ++i) {
            const float w = text_width(labels[i], fs, 500) + padx * 2.0f;
            trg[i] = Rect(x, box.y + 2.0f, w, std::max(box.h - 5.0f, 1.0f));
            x += w;
        }
    }

    // --- 1) 顶级项交互 ---
    std::vector<uint8_t> hov(labels.size(), 0);
    for (size_t i = 0; i < labels.size(); ++i) {
        const uint64_t tid = impl->widget_id("menubar_item", trg[i], bar_id + i, st.identity ? bar_id + i : 0);
        detail::Interact in = detail::interact(impl, tid, trg[i], enabled);
        if (in.hovered) { set_cursor(Cursor::Arrow); hov[i] = 1; }
        if (in.clicked) {
            open_idx = (open_idx == int(i)) ? -1 : int(i);   // 点自己 = 收起
            repaint();
        }
    }

    // --- 2) 划到隔壁自动切（已经有打开的才切，全关时划过去不动） ---
    if (open_idx >= 0) {
        for (size_t i = 0; i < labels.size(); ++i) {
            if (hov[i] && int(i) != open_idx) {
                open_idx = int(i);
                repaint();
                break;
            }
        }
    }
    impl->flags[bar_id] = open_idx < 0 ? 0.0f : float(open_idx) + 1.0f;

    // --- 顶级项视觉 ---
    for (size_t i = 0; i < labels.size(); ++i) {
        const bool act = (int(i) == open_idx);
        const float t = detail::animate_value(impl, bar_id * 97u + i, (hov[i] || act) ? 1.0f : 0.0f, 20.0f);
        if (t > 0.02f)
            fill_rect(trg[i], th.fill3.mix(th.label.with_alpha(10), t * 0.3f),
                      std::max(0.0f, rad - 4.0f));
        Style ts;
        ts.font_size = fs;
        ts.font_weight = 500;
        ts.align = Align::Center;
        ts.valign = VAlign::Middle;
        Color fg = sk.fg;
        if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
        draw_text(labels[i], trg[i], fg, ts);
    }

    // --- 3) 整条菜单栏一起 claim：给下一帧的联动放行（见函数头注释） ---
    if (open_idx >= 0 && enabled) impl->gate_claim(box, z_first);

    // --- 下拉 ---
    if (open_idx >= 0 && open_idx < int(labels.size()) && open_idx < int(items.size())
        && !items[size_t(open_idx)].empty()) {
        const std::vector<std::string>& src = items[size_t(open_idx)];
        const float item_h = std::max(box.h, 24.0f);   // kit Menu/Height Medium = 24
        float list_w = std::max(trg[size_t(open_idx)].w, 140.0f);
        for (const auto& it : src) list_w = std::max(list_w, text_width(it, fs, 500) + 34.0f);
        const float list_h = item_h * float(src.size()) + 8.0f;
        float lx = trg[size_t(open_idx)].x;
        if (lx + list_w > width() - 4.0f) lx = std::max(4.0f, width() - 4.0f - list_w);
        const Rect list(lx, box.bottom() + 3.0f, list_w, list_h);

        impl->gate_claim(list, Impl::Z_FRAME_END);
        impl->gate_claim(trg[size_t(open_idx)], z_first);

        std::vector<Rect>     rects;
        std::vector<uint64_t> ids;
        std::vector<uint8_t>  lhov;
        rects.reserve(src.size());
        ids.reserve(src.size());
        lhov.reserve(src.size());
        for (size_t i = 0; i < src.size(); ++i) {
            const Rect r(list.x + 4.0f, list.y + 4.0f + item_h * float(i), list.w - 8.0f, item_h);
            rects.push_back(r);
            ids.push_back(impl->widget_id("menubar_li", r, bar_id + i, st.identity ? bar_id + i : 0));
            lhov.push_back(0);
        }
        int chosen = -1;
        {
        Impl::SeqFrameEnd guard(impl);  // 项在帧内交互、帧末绘制，序号算帧末层
        for (size_t i = 0; i < rects.size(); ++i) {
            detail::Interact ii = detail::interact(impl, ids[i], rects[i], true);
            if (ii.hovered) { set_cursor(Cursor::Arrow); lhov[i] = 1; }
            if (ii.clicked) chosen = int(i);
        }
        }
        // 点菜单栏自己不算"点外面" —— 那是切换/收起，由上面的逻辑管。
        const bool in_list = list.contains(impl->input.mouse.x, impl->input.mouse.y);
        const bool in_bar  = box.contains(impl->input.mouse.x, impl->input.mouse.y);
        const bool closing = (impl->input.pressed[0] || impl->input.pressed[1])
                          && !in_list && !in_bar;

        std::vector<std::string> litems = src;
        deferred([this, list, rects, ids, lhov, litems, fs]() mutable {
            detail::paint_menu(*this, list, litems, rects, ids, lhov, -1, false, fs);
        });

        if (chosen >= 0) {
            pick.menu = open_idx;
            pick.item = chosen;
            impl->flags[bar_id] = 0.0f;
            repaint();
        } else if (closing) {
            impl->flags[bar_id] = 0.0f;
            repaint();
        }
    }
    return pick;
}

// 模态框。约定：在帧回调的**末尾**调用；返回 true 表示本帧处于打开态，
// 想画自己的按钮就紧接着 dialog() 之后画，这样天然落在遮罩之上。
bool Ui::dialog(const std::string& title, const std::string& body, bool* open,
                       const Rect& box, const Style& st) {
    if (!open || !*open) return false;
    const Theme& th = impl->theme;
    const uint64_t z_first = impl->seq + 1;
    // 参考资料 `Alert/3 Options + Cancel`：面板 r10、无描边（层次全靠 Primary 投影）、
    // 内边距 16/20、标题 13 Bold、正文 **11 Regular 85%**（不是 50% 的次要灰）。
    const float rad = st.radius >= 0.0f ? st.radius : 10.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : 20.0f;
    const Input& kin = impl->input;

    // 面板外点击或 Esc 请求关闭。
    bool dismiss = false;
    if (!box.contains(kin.mouse.x, kin.mouse.y) && (kin.pressed[0] || kin.pressed[1])) dismiss = true;
    if (kin.key_pressed[KEY_ESC]) dismiss = true;

    // 绘制：面板 + 投影（**不铺整屏黑幕** —— macOS 的模态只是窗口失去焦点，
    // 不会被压暗；整屏半透明黑是 Web Dialog 的 overlay）
    detail::pop_shadow(*this, box, rad, th, 2);
    fill_rect(box, !st.bg.is_none() ? st.bg : th.popover_bg, rad);
    // 面板不描边：macOS 靠投影与底色分层，一圈 1px 会让它看起来像 Web 的卡片

    // 文字必须裁在面板里 —— 正文一长就会横着爬出去（溢出到遮罩上的字最难发现，
    // 因为遮罩本身是半透明黑，看起来只是"有点脏"）。
    push_clip(box);
    const float title_lh = line_height(fs);
    Style hs;
    hs.font_size = fs;          // Headline 13 Bold
    hs.font_weight = 700;
    hs.align = Align::Left;
    hs.valign = VAlign::Top;
    hs.wrap = true;
    draw_text(title, Rect(box.x + pad, box.y + pad, std::max(box.w - pad * 2.0f - 26.0f, 1.0f), title_lh),
              !st.fg.is_none() ? st.fg : th.label, hs);
    if (!body.empty()) {
        Style bs;
        bs.font_size = 11.0f;               // Footnote 11（kit 的 Alert Description）
        bs.font_weight = 400;
        bs.align = Align::Left;
        bs.valign = VAlign::Top;
        bs.wrap = true;
        bs.line_gap = 3.0f;                 // 正文比标题松一点，长段落才好读
        draw_text(body, Rect(box.x + pad, box.y + pad + title_lh + 10.0f, box.w - pad * 2.0f,
                             std::max(box.h - pad * 2.0f - title_lh - 10.0f, 1.0f)),
                  th.label, bs);
    }
    pop_clip();

    // 关闭时仍登记闸门，多阻挡一帧，避免关闭点击穿透到后方控件。
    impl->gate_claim(box, z_first);
    if (dismiss) { *open = false; repaint(); }
    return true;      // 本帧仍然按"打开"画完了，下一帧才真的消失（不会闪一下）
}

// -----------------------------------------------------------------------------
//  sheet —— 贴在标题栏下方的工作表（上平下圆）
// -----------------------------------------------------------------------------
// macOS 的 sheet 不是"浮在窗口中间的对话框"：它从标题栏往下滑出来，上边缘和
// 标题栏齐平（所以上面两个角是直角），只有下面两个角是圆的；窗口其余部分
// **不压黑**，只是失去焦点。
//
// 图元只有"四角同半径"，所以这里把面板往上多画 12px 再裁掉 —— 上方两角被裁平，
// 比为了一个直角去加一个"逐角半径"参数划算得多。
bool Ui::sheet(const std::string& title, const std::string& body, bool* open,
                      const Rect& box, const Style& st) {
    if (!open || !*open || box.w <= 0.0f || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t z_first = impl->seq + 1;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : 20.0f;
    const Input& kin = impl->input;

    bool dismiss = false;
    if (!box.contains(kin.mouse.x, kin.mouse.y) && (kin.pressed[0] || kin.pressed[1])) dismiss = true;
    if (kin.key_pressed[KEY_ESC]) dismiss = true;

    // 投影要裁在 sheet 的上边缘以下：不裁的话，圆角面板的投影会浮在标题栏
    // 和 sheet 之间，看着像"面板从天上掉下来停在半空"。
    const Rect draw_r(box.x, box.y - 12.0f, box.w, box.h + 12.0f);
    push_clip(Rect(0.0f, box.y, width(), std::max(height() - box.y, 1.0f)));
    detail::pop_shadow(*this, draw_r, 12.0f, th, 2);
    pop_clip();
    push_clip(box);
    fill_rect(draw_r, !st.bg.is_none() ? st.bg : th.popover_bg, 12.0f);

    const float title_lh = line_height(fs);
    Style hs;
    hs.font_size = fs;
    hs.font_weight = 600;
    hs.align = Align::Left;
    hs.valign = VAlign::Top;
    hs.wrap = true;
    draw_text(title, Rect(box.x + pad, box.y + pad, box.w - pad * 2.0f, title_lh),
              !st.fg.is_none() ? st.fg : th.label, hs);
    if (!body.empty()) {
        Style bs;
        bs.font_size = 11.0f;
        bs.font_weight = 400;
        bs.align = Align::Left;
        bs.valign = VAlign::Top;
        bs.wrap = true;
        bs.line_gap = 3.0f;
        draw_text(body, Rect(box.x + pad, box.y + pad + title_lh + 8.0f,
                             box.w - pad * 2.0f, std::max(box.h - pad * 2.0f - title_lh - 8.0f, 1.0f)),
                  th.label, bs);
    }
    pop_clip();

    impl->gate_claim(box, z_first);
    if (dismiss) { *open = false; repaint(); }
    return true;
}

// 轻提示条
void Ui::toast(const std::string& text, const Rect& box, const Style& st) {
    const Theme& th = impl->theme;
    const float rad = st.radius >= 0.0f ? st.radius : 10.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const bool danger = (st.variant == Style::Destructive);

    Color bg = !st.bg.is_none() ? st.bg : th.popover_bg;
    Color fg = !st.fg.is_none() ? st.fg : th.label;
    if (st.disabled) { bg.a = uint8_t(float(bg.a) * 0.6f); fg.a = uint8_t(float(fg.a) * 0.6f); }
    if (danger) fg = th.red;

    // 浮起的通知条：r10 + Secondary 投影，**不描边**（描一圈线是 Web 的 toast）。
    // Web 那种"左边一条色带"在 macOS 里也不存在，告警用**文字颜色**表达。
    detail::pop_shadow(*this, box, rad, th, 1);
    fill_rect(box, bg, rad);
    const float text_x = box.x + 14.0f;
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    push_clip(box);
    draw_text(text, Rect(text_x, box.y, std::max(box.right() - 14.0f - text_x, 1.0f), box.h), fg, ts);
    pop_clip();
}

// =============================================================================
//  8.15  对齐 macOS 的基础控件（含 kit 里没有对应物的推导）
//
//  本节控件的输入与绘制约定：
//    1) 交互一律走 detail::interact()，动画一律走 detail::animate_value()；
//    2) 浮层用 deferred() 画到帧末，并在帧内 gate_claim() 登记下一帧的输入白名单；
//    3) **不做无条件 repaint()** —— 空闲 0% CPU 是这个库的头号承诺，
//       只有"状态真的变了"才 repaint()；
//    4) 尺寸全部由传入的 box 推出来，不写死像素。
// =============================================================================

namespace detail {

// ---- 公历日期（不依赖 <ctime>）----
// 为什么自己算：<ctime> 的 mktime/localtime 走本地时区与夏令时，同一份代码在
// UTC 的 CI runner 和东八区的开发机上会差一天。月历要的是"日历上第几格"，
// 跟时区无关，所以纯整数运算最稳，也最适合三平台断言。
inline bool leap_year(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

inline int days_in_month(int y, int m) {
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1) m = 1;
    if (m > 12) m = 12;
    return (m == 2 && leap_year(y)) ? 29 : kDays[m - 1];
}

// 星期几：0 = 周日。Sakamoto 算法。
inline int weekday_of(int y, int m, int d) {
    static const int kOff[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    const int w = (y + y / 4 - y / 100 + y / 400 + kOff[(m - 1) % 12] + d) % 7;
    return w < 0 ? w + 7 : w;
}

// 不区分大小写的子串匹配（combobox 过滤用）。只折叠 ASCII：
// 中文没有大小写，UTF-8 里的非 ASCII 字节原样比较即可。
inline bool contains_ci(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    if (needle.size() > hay.size()) return false;
    auto low = [](char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; };
    for (size_t i = 0; i + needle.size() <= hay.size(); ++i) {
        size_t j = 0;
        for (; j < needle.size(); ++j)
            if (low(hay[i + j]) != low(needle[j])) break;
        if (j == needle.size()) return true;
    }
    return false;
}

// 分组控件里"一格"的公共部分：Toggle Group / Button Group 三处共用。
// 返回 true = 本帧被点击。把视觉收在一处，是为了让三种用法的外观永远一致 ——
// 复制三份最容易出的问题就是改了一处忘另外两处。
inline bool group_cell(Ui& ui, Ui::Impl* im, uint64_t id, const Rect& r, const std::string& text,
                       bool on, float fs, float outer_rad, const Style& st,
                       bool selectable, bool divider) {
    const Theme& th = im->theme;
    Interact in = interact(im, id, r, !st.disabled);
    if (in.hovered) ui.set_cursor(Cursor::Arrow);
    if (in.pressed) grab_focus(im, id);

    const float t_on = animate_state(im, id * 101u + 1u, on ? 1.0f : 0.0f, 22.0f);
    const float t_h  = animate_value(im, id * 101u + 2u, in.hovered ? 1.0f : 0.0f, 18.0f);

    // 分隔线先画，免得压在选中底上（只在相邻两项之间）。
    // 参考资料 Segmented Control：分隔线是 1×14 的 25% 标签色，不是 1px 边框色整条通高 ——
    // 通高会像表格竖线，缩到 14 才是 macOS 的"段间细缝"。
    if (divider) {
        const float dh = std::min(14.0f, std::max(r.h - 10.0f, 1.0f));
        ui.fill_rect(Rect(r.x, r.y + (r.h - dh) * 0.5f, 1.0f, dh), th.label3, 0.0f);
    }

    // 分组按钮的选中面在浅色下加深、深色下提亮，以区别于轨道。
    const Rect inner = r.inset(2.0f, 2.0f);
    const float ir = inner.h * 0.5f;
    const Color pill = th.is_dark() ? Color::hex(0x3A3A3A) : Color::hex(0xE6E6E6);
    if (t_on > 0.01f) {
        ui.fill_rect_s(inner, pill.scaled_alpha(t_on), ir, 0.35f);
    } else if (t_h > 0.01f) {
        ui.fill_rect(inner, th.fill4.scaled_alpha(t_h * 0.7f), ir);
    }

    // 有选中语义的组：选中项 85%，其余 50%（参考资料 Segmented 就是这样分级的）；
    // 一次性动作的按钮组没有"选中"，所有标签都是主标签色。
    Color fg = !selectable ? th.label : (on ? th.label : th.label2);
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = st.font_weight > 0 ? st.font_weight : 500;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;
    ui.draw_text(text, r, fg, ts);

    if (is_focused(im, id)) paint_focus_ring(ui, r, outer_rad, th.focus_ring);
    return in.clicked;
}

} // namespace detail

// -----------------------------------------------------------------------------
//  Toggle —— 按下态按钮
//
//  toggle() 是开关；toggle_button() 保持按钮的按下状态。
// -----------------------------------------------------------------------------
bool Ui::toggle_button(const std::string& label, bool* value, const Rect& box, const Style& st) {
    if (!value || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("toggle_btn", box, detail::hash_string(label.c_str()), st.identity);
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;

    detail::Interact in = detail::interact(impl, id, box, !st.disabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    if (in.pressed) detail::grab_focus(impl, id);

    const float t_on = detail::animate_state(impl, id * 97u + 1u, *value ? 1.0f : 0.0f, 22.0f);
    const float t_h  = detail::animate_value(impl, id * 97u + 2u, in.hovered ? 1.0f : 0.0f, 18.0f);

    bool changed = false;
    if (in.clicked) { *value = !*value; changed = true; repaint(); }

    Color bg = st.bg;
    if (t_on > 0.01f) {
        bg = th.tint;
        bg.a = uint8_t(56.0f * t_on + 0.5f);       // ~22%
    } else if (t_h > 0.01f) {
        bg = detail::fade(th.fill, t_h);
    }
    if (bg.a > 0) fill_rect_s(box, bg, rad, 0.35f);
    if (st.border_width > 0.0f) stroke_rect(box, th.separator, rad, st.border_width);

    Color fg = !st.fg.is_none() ? st.fg : th.label;
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = st.font_weight > 0 ? st.font_weight : 500;   // kit 两态都是 Medium
    ts.align = st.align_set ? st.align : Align::Center;
    ts.valign = VAlign::Middle;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : 10.0f;
    draw_text(label, Rect(box.x + pad, box.y, std::max(box.w - pad * 2.0f, 1.0f), box.h), fg, ts);

    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box, rad, th.focus_ring);
    return changed;
}

// 单选 Toggle Group 允许再次点击选中项以取消选择。
bool Ui::toggle_group(const std::vector<std::string>& items, int* index,
                             const Rect& box, const Style& st) {
    if (!index || items.empty() || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const int n = int(items.size());
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    // 胶囊轨道：这三个组控件在 AppKit 里就是 NSSegmentedControl，半径 = h/2
    const float rad = st.radius >= 0.0f ? st.radius : box.h * 0.5f;
    const float iw  = box.w / float(n);
    const uint64_t base = impl->widget_id("toggle_group", box, 0, st.identity);

    // 分段控件使用半透明黑色凹槽背景，无外框。
    fill_rect(box, !st.bg.is_none() ? st.bg : th.track_well(), rad);

    bool changed = false;
    for (int i = 0; i < n; ++i) {
        const Rect r(box.x + iw * float(i), box.y, iw, box.h);
        const uint64_t id = impl->widget_id("tg_item", r, base + uint64_t(i), st.identity ? base + uint64_t(i) : 0);
        const bool on = (*index == i);
        // 分隔线只在两侧都没选中时画：参考资料 kit 的 Separator 会被 Selected 段整块盖住
        const bool div = i > 0 && !on && !(*index == i - 1);
        if (detail::group_cell(*this, impl, id, r, items[size_t(i)], on,
                               fs, rad, st, true, div)) {
            *index = (*index == i) ? -1 : i;
            changed = true;
            repaint();
        }
    }
    return changed;
}

// Toggle Group（多选）：mask 的第 i 位对应第 i 项
bool Ui::toggle_group_multi(const std::vector<std::string>& items, std::uint32_t* mask,
                                   const Rect& box, const Style& st) {
    if (!mask || items.empty() || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const int n = std::min(int(items.size()), 32);
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float rad = st.radius >= 0.0f ? st.radius : box.h * 0.5f;
    const float iw  = box.w / float(n);
    const uint64_t base = impl->widget_id("toggle_group_m", box, 0, st.identity);

    fill_rect(box, !st.bg.is_none() ? st.bg : th.track_well(), rad);

    bool changed = false;
    for (int i = 0; i < n; ++i) {
        const Rect r(box.x + iw * float(i), box.y, iw, box.h);
        const uint64_t id = impl->widget_id("tgm_item", r, base + uint64_t(i), st.identity ? base + uint64_t(i) : 0);
        const bool on = ((*mask >> i) & 1u) != 0u;
        const bool prev_on = i > 0 && (((*mask >> (i - 1)) & 1u) != 0u);
        if (detail::group_cell(*this, impl, id, r, items[size_t(i)], on,
                               fs, rad, st, true, i > 0 && !on && !prev_on)) {
            *mask ^= (1u << i);
            changed = true;
            repaint();
        }
    }
    return changed;
}

// Button Group：相邻按钮连成一片。和 Toggle Group 的区别是**没有持久选中态** ——
// 每个按钮都是一次性动作，点了就返回索引。
bool Ui::button_group(const std::vector<std::string>& items, int* clicked_index,
                             const Rect& box, const Style& st) {
    if (items.empty() || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const int n = int(items.size());
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float rad = st.radius >= 0.0f ? st.radius : box.h * 0.5f;
    const float iw  = box.w / float(n);
    const uint64_t base = impl->widget_id("button_group", box, 0, st.identity);

    // 按钮组与分段控件共用凹槽背景，无外框。
    fill_rect_s(box, !st.bg.is_none() ? st.bg : th.track_well(), rad, 0.35f);

    bool clicked = false;
    for (int i = 0; i < n; ++i) {
        const Rect r(box.x + iw * float(i), box.y, iw, box.h);
        const uint64_t id = impl->widget_id("bg_item", r, base + uint64_t(i), st.identity ? base + uint64_t(i) : 0);
        if (detail::group_cell(*this, impl, id, r, items[size_t(i)], false,
                               fs, rad, st, false, i > 0)) {
            if (clicked_index) *clicked_index = i;
            clicked = true;
            repaint();
        }
    }
    return clicked;
}

// -----------------------------------------------------------------------------
//  Input Group —— 输入框左右挂前缀/后缀
//
//  做法：自己画外框，中间的 input 传"透明底 + 0 边框"，这样它只负责编辑和
//  光标，不再画第二层框。input() 里 `bg.a > 0` 与 `border_width > 0` 两个
//  判断正好让这个组合成立，不需要改 input 的实现。
// -----------------------------------------------------------------------------
bool Ui::input_group(std::string* value, const std::string& prefix, const std::string& suffix,
                            const Rect& box, const Style& st) {
    if (!value || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = 12.0f;

    // +10 而不是 +4：前缀文字和输入内容之间必须有可见的间隔，
    // 否则 "¥" 和 "128" 会挤成 "¥128"，看起来像一个数
    const float pre_w = prefix.empty() ? 0.0f : text_width(prefix, fs, 400) + 10.0f;
    const float suf_w = suffix.empty() ? 0.0f : text_width(suffix, fs, 400) + 10.0f;

    // 组合输入框与 input 共用 th.input_border 背景、顶部内阴影和细描边。
    fill_rect(box, !st.bg.is_none() ? st.bg
                                   : (th.is_dark() ? th.control_bg : th.window_bg), rad);
    inner_shadow(box, th.is_dark() ? Color(0, 0, 0, 70) : Color(0, 0, 0, 18), rad, 1.8f, 1.0f);
    stroke_rect(box, !st.border_color.is_none() ? st.border_color : th.separator, rad, 1.0f);

    Style ts;
    ts.font_size = fs;
    ts.font_weight = 400;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    if (!prefix.empty())
        draw_text(prefix, Rect(box.x + pad, box.y, pre_w, box.h), th.label2, ts);
    if (!suffix.empty()) {
        ts.align = Align::Right;
        draw_text(suffix, Rect(box.right() - pad - suf_w, box.y, suf_w, box.h), th.label2, ts);
    }

    const float inner_x = box.x + pad + pre_w;
    const float inner_r = box.right() - pad - suf_w;
    const Rect inner(inner_x, box.y, std::max(inner_r - inner_x, 1.0f), box.h);

    Style is;
    is.background(Color(0, 0, 0, 0));   // alpha=0 → input 内部跳过 fill_rect
    is.stroke(0.0f, Color::none());     // width=0 → 跳过 stroke_rect
    is.pad_x = 4.0f;
    is.font_size = fs;
    is.disabled = st.disabled;
    is.identity = st.identity;

    const Input& kin = impl->input;
    const bool outside_press = kin.pressed[0]
                            && box.contains(kin.mouse.x, kin.mouse.y)
                            && !inner.contains(kin.mouse.x, kin.mouse.y);

    const bool changed = input(value, inner, is);

    // 前后缀区域也得点得进输入框：整块看起来是一个框，点在"元"上却什么都不发生
    // 是最容易让人以为"控件坏了"的那种细节。
    // 必须等 input() 跑完再抢焦点 —— 它内部遇到"按下了但不在自己框内"会主动
    // 丢掉焦点，先抢会被它丢回去。晚一帧生效，肉眼无感。
    if (outside_press) {
        detail::grab_focus(impl, impl->widget_id("input", inner, 0,
            st.identity ? st.identity : uint64_t(reinterpret_cast<uintptr_t>(value))));
        repaint();
    }

    // 组合输入框负责外层焦点边框，必须在内部 input 绘制后补画，避免被其背景覆盖。
    if (detail::is_focused(impl, impl->widget_id("input", inner, 0,
            st.identity ? st.identity : uint64_t(reinterpret_cast<uintptr_t>(value))))) {
        stroke_rect(box, th.focus_ring, rad, 1.4f);
        detail::paint_focus_ring(*this, box, rad, th.focus_ring, true);
    }
    return changed;
}

// -----------------------------------------------------------------------------
//  Input OTP —— 分格验证码
//
//  不复用 input()：那个输入框带着选区、拖选、双击选词、IME、撤销栈一整套，
//  塞进一个 40×40 的格子既不合适也画不进去。这里只要"数字 + 退格 + 左右键"。
// -----------------------------------------------------------------------------
bool Ui::input_otp(std::string* value, int length, const Rect& box, const Style& st) {
    if (!value || length <= 0 || box.w <= 0.0f) return false;
    if (length > 12) length = 12;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("otp", box, 0, st.identity);
    // 验证码各格使用 24px 高、r5、13pt 的文本框尺寸。
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float rad = st.radius >= 0.0f ? st.radius : 5.0f;
    const float gap = 6.0f;
    const float cell = std::max(1.0f, std::min(std::min(box.h, 24.0f),
                                               (box.w - gap * float(length - 1)) / float(length)));
    const float total = cell * float(length) + gap * float(length - 1);
    const float x0 = box.x + (box.w - total) * 0.5f;
    const float cy = box.cy() - cell * 0.5f;

    // 当前格存在 flags 里：必须夹住，length 改小以后旧值会越界。
    float& cur_f = impl->flags[id * 103u + 1u];
    int cur = detail::clampi(int(cur_f + 0.5f), 0, int(value->size()));

    detail::Interact in = detail::interact(impl, id, box, !st.disabled);
    bool focused = detail::editor_focused(impl, id, !st.disabled);
    if (focused && impl->gate_blocked_here(box)) { detail::drop_focus(impl); focused = false; }

    bool changed = false;
    if (impl->input.pressed[0] && in.hovered) {
        detail::grab_focus(impl, id);
        focused = true;
    }
    if (in.hovered) set_cursor(Cursor::IBeam);

    if (focused) {
        const Input& kin = impl->input;
        // 只收数字：粘贴 "12a3" 也只吃 1、2、3
        for (char ch : kin.text_input) {
            if (ch < '0' || ch > '9') continue;
            if (cur < int(value->size())) (*value)[size_t(cur)] = ch;
            else if (cur < length)        value->push_back(ch);
            else break;
            ++cur;
            changed = true;
        }
        if (kin.key_hit(KEY_BACKSPACE) && cur > 0) {
            --cur;
            if (cur < int(value->size())) value->erase(size_t(cur), 1);
            changed = true;
        }
        if (kin.key_hit(KEY_LEFT) && cur > 0) --cur;
        if (kin.key_hit(KEY_RIGHT) && cur < int(value->size())) ++cur;
        if (changed) { repaint(); impl->caret_blink = 0.0f; }
    }
    if (cur > int(value->size())) cur = int(value->size());
    cur_f = float(cur);

    for (int i = 0; i < length; ++i) {
        const Rect r(x0 + (cell + gap) * float(i), cy, cell, cell);
        const bool active = focused && i == cur;
        fill_rect(r, !st.bg.is_none() ? st.bg : th.window_bg, rad);
        stroke_rect(r, active ? th.focus_ring : th.input_border, rad, active ? 1.6f : 1.0f);

        Color fg = !st.fg.is_none() ? st.fg : th.label;
        if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
        Style ts;
        ts.font_size = fs;
        ts.font_weight = 600;
        ts.align = Align::Center;
        ts.valign = VAlign::Middle;
        if (i < int(value->size())) {
            draw_text(value->substr(size_t(i), 1), r, fg, ts);
        } else if (active) {
            // 空格子里的插入点：一闪一闪太费帧，做成静止的竖线
            fill_rect(Rect(r.cx() - 0.75f, r.y + r.h * 0.28f, 1.5f, r.h * 0.44f), fg, 0.0f);
        }
    }
    return changed;
}

// -----------------------------------------------------------------------------
//  Field —— 表单一行：标签 + 控件 + 说明/错误
//
//  高度必须能"先算后排"：field_height() 与 field() 用的是同一组数字，
//  改一处就必须改另一处（放得近就是为了这个）。
// -----------------------------------------------------------------------------
float Ui::field_height(const std::string& label, const std::string& hint,
                              const std::string& error, float control_h, const Style& st) const {
    const Theme& th = impl->theme;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    float h = control_h;
    if (!label.empty()) h += line_height(fs + 1.0f) + 6.0f;
    if (!hint.empty() || !error.empty()) h += line_height(fs - 1.0f) + 6.0f;
    return h;
}

void Ui::field(const std::string& label, const std::string& hint, const std::string& error,
                      const Rect& box, const std::function<void(const Rect&)>& control,
                      const Style& st) {
    if (box.w <= 0.0f || box.h <= 0.0f) return;
    const Theme& th = impl->theme;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float lh_label = line_height(fs + 1.0f);
    const float lh_msg   = line_height(fs - 1.0f);
    const bool has_msg   = !error.empty() || !hint.empty();

    float y = box.y;
    if (!label.empty()) {
        Style ls;
        ls.font_size = fs + 1.0f;
        ls.font_weight = 500;
        ls.align = Align::Left;
        ls.valign = VAlign::Top;
        Color lc = !st.fg.is_none() ? st.fg : th.label;
        draw_text(label, Rect(box.x, y, box.w, lh_label), lc, ls);
        y += lh_label + 6.0f;
    }

    const float ctrl_h = std::max(box.bottom() - y - (has_msg ? lh_msg + 6.0f : 0.0f), 1.0f);
    if (control) control(Rect(box.x, y, box.w, ctrl_h));
    y += ctrl_h + 6.0f;

    if (!error.empty()) {
        Style ms;
        ms.font_size = fs - 1.0f;
        ms.font_weight = 400;
        ms.align = Align::Left;
        ms.valign = VAlign::Top;
        draw_text(error, Rect(box.x, y, box.w, lh_msg), th.red, ms);
    } else if (!hint.empty()) {
        Style ms;
        ms.font_size = fs - 1.0f;
        ms.font_weight = 400;
        ms.align = Align::Left;
        ms.valign = VAlign::Top;
        draw_text(hint, Rect(box.x, y, box.w, lh_msg), th.label2, ms);
    }
}

// -----------------------------------------------------------------------------
//  Item —— 一行条目（标题 + 描述 + 尾部文字），整行可点
// -----------------------------------------------------------------------------
bool Ui::item(const std::string& title, const std::string& description,
                     const std::string& trailing, const Rect& box, const Style& st) {
    if (box.w <= 0.0f || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("item", box, detail::hash_string(title.c_str()), st.identity);
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float rad = st.radius >= 0.0f ? st.radius : 5.0f;

    detail::Interact in = detail::interact(impl, id, box, !st.disabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    if (in.pressed) detail::grab_focus(impl, id);
    const float t_h = detail::animate_value(impl, id * 107u + 1u, in.hovered ? 1.0f : 0.0f, 18.0f);

    // 列表行悬停只浮 5% 一层（参考资料 List Item 的 hover），选中才是 23% 强调色
    if (t_h > 0.01f)
        fill_rect(box, th.fill3.scaled_alpha(t_h), rad);

    const float pad = st.pad_x >= 0.0f ? st.pad_x : 10.0f;
    const float trail_w = trailing.empty() ? 0.0f : text_width(trailing, fs - 1.0f, 400) + 12.0f;
    const Rect text_box(box.x + pad, box.y, std::max(box.w - pad * 2.0f - trail_w, 1.0f), box.h);

    if (description.empty()) {
        Style ts;
        ts.font_size = fs;
        ts.font_weight = st.font_weight > 0 ? st.font_weight : 500;
        ts.align = Align::Left;
        ts.valign = VAlign::Middle;
        draw_text(title, text_box, !st.fg.is_none() ? st.fg : th.label, ts);
    } else {
        const float lh1 = line_height(fs);
        const float lh2 = line_height(11.0f);
        const float total = lh1 + 2.0f + lh2;
        const float y0 = box.cy() - total * 0.5f;
        Style ts;
        ts.font_size = fs;
        ts.font_weight = st.font_weight > 0 ? st.font_weight : 500;
        ts.align = Align::Left;
        ts.valign = VAlign::Top;
        draw_text(title, Rect(text_box.x, y0, text_box.w, lh1), !st.fg.is_none() ? st.fg : th.label, ts);

        Style ds;
        ds.font_size = 11.0f;      // Subheadline：列表副标题
        ds.font_weight = 400;
        ds.align = Align::Left;
        ds.valign = VAlign::Top;
        draw_text(description, Rect(text_box.x, y0 + lh1 + 2.0f, text_box.w, lh2),
                  th.label2, ds);
    }

    if (!trailing.empty()) {
        Style tr;
        tr.font_size = fs - 1.0f;
        tr.font_weight = 400;
        tr.align = Align::Right;
        tr.valign = VAlign::Middle;
        draw_text(trailing, Rect(box.right() - pad - trail_w, box.y, trail_w, box.h),
                  th.label2, tr);
    }

    // 行底线内缩到文字起点，和 macOS 列表一致（通栏的线会显得像分隔条）
    if (st.row_sep && box.h >= 8.0f)
        fill_rect(Rect(box.x + pad, box.bottom() - 1.0f, std::max(box.w - pad, 1.0f), 1.0f),
                  th.separator, 0.0f);

    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box, rad, th.focus_ring);
    return in.clicked;
}

// -----------------------------------------------------------------------------
//  Empty —— 居中的空状态标题与描述
// -----------------------------------------------------------------------------
void Ui::empty_state(const std::string& title, const std::string& description,
                            const Rect& box, const Style& st) {
    if (box.w <= 0.0f || box.h <= 0.0f) return;
    const Theme& th = impl->theme;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;

    const float lh1 = line_height(fs + 1.0f);
    const float lh2 = description.empty() ? 0.0f : line_height(fs - 1.0f) + 4.0f;
    const float total = lh1 + lh2;
    const float y0 = box.cy() - total * 0.5f;

    Style ts;
    ts.font_size = fs + 1.0f;
    ts.font_weight = 500;
    ts.align = Align::Center;
    ts.valign = VAlign::Top;
    draw_text(title, Rect(box.x + 16.0f, y0, std::max(box.w - 32.0f, 1.0f), lh1),
              !st.fg.is_none() ? st.fg : th.label, ts);

    if (!description.empty()) {
        Style ds;
        ds.font_size = fs - 1.0f;
        ds.font_weight = 400;
        ds.align = Align::Center;
        ds.valign = VAlign::Top;
        draw_text(description, Rect(box.x + 16.0f, y0 + lh1, std::max(box.w - 32.0f, 1.0f), lh2),
                  th.label2, ds);
    }
}

// -----------------------------------------------------------------------------
//  Pagination —— 页码器
//
//  页数较多时保留首尾、当前页邻域与省略号；省略号不参与交互。
// -----------------------------------------------------------------------------
bool Ui::pagination(int* page, int total_pages, const Rect& box, const Style& st) {
    if (!page || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    int total = std::max(total_pages, 1);
    int cur = detail::clampi(*page, 0, total - 1);
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const uint64_t base = impl->widget_id("pagination", box, 0, st.identity);

    // 槽位编码：-1=上一页 -2=省略号 -3=下一页，>=0 是页码（0 基）。
    //
    // **槽位数恒定**（页数 > 7 时固定 7 个）。这一点很关键：如果按"当前页附近
    // 三个都列出来"的写法，翻页时槽位数会从 8 变 6，整排按钮跟着左右跳 ——
    // 用户点第二下就会点偏。固定下来以后，翻页只有内容变、位置不动。
    std::vector<int> slots;
    slots.push_back(-1);
    if (total <= 7) {
        for (int i = 0; i < total; ++i) slots.push_back(i);
    } else if (cur <= 1) {
        // 1 2 3 … 末
        slots.push_back(0); slots.push_back(1); slots.push_back(2);
        slots.push_back(-2); slots.push_back(total - 1);
    } else if (cur >= total - 2) {
        // 首 … 倒数三 倒数二 末
        slots.push_back(0); slots.push_back(-2);
        slots.push_back(total - 3); slots.push_back(total - 2); slots.push_back(total - 1);
    } else {
        // 首 … 当前 … 末
        slots.push_back(0); slots.push_back(-2);
        slots.push_back(cur); slots.push_back(-2); slots.push_back(total - 1);
    }
    slots.push_back(-3);

    const int n = int(slots.size());
    const float sw = std::min(box.h, box.w / float(n));
    const float x0 = box.x + (box.w - sw * float(n)) * 0.5f;

    bool changed = false;
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Center;
    ts.valign = VAlign::Middle;

    for (int i = 0; i < n; ++i) {
        const Rect r(x0 + sw * float(i), box.y, sw, box.h);
        const int s = slots[size_t(i)];

        if (s == -2) {                           // 省略号
            draw_text("...", r, th.label2, ts);
            continue;
        }

        const uint64_t id = impl->widget_id("pg", r, base + uint64_t(i), st.identity ? base + uint64_t(i) : 0);
        detail::Interact in = detail::interact(impl, id, r, !st.disabled);
        if (in.hovered) set_cursor(Cursor::Arrow);
        if (in.pressed) detail::grab_focus(impl, id);

        const bool is_cur = (s >= 0 && s == cur);
        const float t_h = detail::animate_value(impl, id * 103u + 1u, (in.hovered && !is_cur) ? 1.0f : 0.0f, 20.0f);
        // 当前页 = **强调色 23% 的胶囊**，不是实心蓝圆点。实心蓝是 iOS 的页码点，
        // 也会跟"主操作按钮"抢视觉权重；mac 的选中/当前位置一律是 23% 的淡底
        // （见 Item/Small/Selected）。
        const Rect inner = r.inset(2.0f, 2.0f);
        const float pill_r = inner.h * 0.5f;
        if (is_cur) {
            Color cur_bg = th.tint;
            cur_bg.a = 59;                      // ≈23%
            fill_rect_s(inner, cur_bg, pill_r, 0.35f);
        } else if (t_h > 0.01f) {
            fill_rect_s(inner, th.fill.scaled_alpha(t_h), pill_r, 0.35f);
        }

        if (s < 0) {
            // 上一页 / 下一页使用系统箭头。
            const bool left = (s == -1);
            const bool enabled = left ? cur > 0 : cur < total - 1;
            const Color ac = enabled ? th.label : th.label2.with_alpha(110);
            system_icon(left ? SystemIcon::Prev : SystemIcon::Next,
                        Rect::centered(r.cx(), r.cy(), 12, 12), ac);
            if (in.clicked && enabled) { *page = cur + (left ? -1 : 1); changed = true; repaint(); }
        } else {
            char buf[16];
            std::snprintf(buf, sizeof buf, "%d", s + 1);
            // 选中页使用浅色底与主标签色。
            draw_text(buf, r, th.label, ts);
            if (in.clicked && s != cur) { *page = s; changed = true; repaint(); }
        }

        if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, inner, std::max(0.0f, rad - 3.0f), th.focus_ring);
    }
    return changed;
}

// -----------------------------------------------------------------------------
//  Calendar —— 月历
//
//  点 < > 翻月（*year / *month 会被改），点日期写 *day，两者都算"变了"。
//  日期运算全部在 detail:: 里做，不碰 <ctime>：时区/夏令时会让 CI 与本机
//  差一天，而那是最难查的一类"只在我的机器上错"。
// -----------------------------------------------------------------------------
bool Ui::calendar(int* year, int* month, int* day, const Rect& box, const Style& st) {
    if (!year || !month || !day || box.w <= 0.0f || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius_sm;

    int y = *year, m = detail::clampi(*month, 1, 12);
    const int dim = detail::days_in_month(y, m);
    int sel = detail::clampi(*day, 1, dim);

    const float head_h = std::max(fs + 26.0f, 42.0f);
    const float wk_h   = std::max(fs + 12.0f, 26.0f);
    const Rect head(box.x, box.y, box.w, head_h);
    const Rect wkh(box.x, head.bottom(), box.w, wk_h);
    const float grid_h = std::max(box.bottom() - wkh.bottom(), 1.0f);
    const float cw = box.w / 7.0f;
    const float ch = grid_h / 6.0f;
    const uint64_t base = impl->widget_id("calendar", box, 0, st.identity);

    bool changed = false;

    // ---- 头部：< 2026 年 10 月 > ----
    const Rect prev_r(head.x + 2.0f, head.y + (head_h - head_h * 0.7f) * 0.5f, head_h * 0.7f, head_h * 0.7f);
    const Rect next_r(head.right() - head_h * 0.7f - 2.0f, prev_r.y, prev_r.w, prev_r.h);
    for (int k = 0; k < 2; ++k) {
        const Rect& r = (k == 0) ? prev_r : next_r;
        const uint64_t id = impl->widget_id("cal_nav", r, base + uint64_t(k), st.identity ? base + uint64_t(k) : 0);
        detail::Interact in = detail::interact(impl, id, r, true);
        if (in.hovered) set_cursor(Cursor::Arrow);
        const float t_h = detail::animate_value(impl, id * 109u + 1u, in.hovered ? 1.0f : 0.0f, 20.0f);
        if (t_h > 0.01f) fill_rect(r, th.fill3.scaled_alpha(t_h), rad);
        system_icon(k == 0 ? SystemIcon::Prev : SystemIcon::Next,
                    Rect::centered(r.cx(), r.cy(), 12, 12), th.label2);
        if (in.clicked) {
            if (k == 0) { if (--m < 1) { m = 12; --y; } }
            else        { if (++m > 12) { m = 1; ++y; } }
            sel = std::min(sel, detail::days_in_month(y, m));
            changed = true;
            repaint();
        }
    }

    char title[64];
    std::snprintf(title, sizeof title, "%d 年 %d 月", y, m);
    Style hs;
    hs.font_size = fs + 1.0f;
    hs.font_weight = 600;
    hs.align = Align::Center;
    hs.valign = VAlign::Middle;
    draw_text(title, head, th.label, hs);

    // ---- 星期表头 ----
    static const char* kWeek[7] = {"日", "一", "二", "三", "四", "五", "六"};
    Style ws;
    ws.font_size = fs - 2.0f;
    ws.font_weight = 500;
    ws.align = Align::Center;
    ws.valign = VAlign::Middle;
    for (int c = 0; c < 7; ++c) {
        draw_text(kWeek[c], Rect(box.x + cw * float(c), wkh.y, cw, wk_h), th.label2, ws);
    }

    // ---- 日期网格 ----
    const int wd0 = detail::weekday_of(y, m, 1);
    const int dim_now = detail::days_in_month(y, m);
    int d = 1 - wd0;
    for (int row = 0; row < 6 && d <= dim_now; ++row) {
        for (int col = 0; col < 7; ++col, ++d) {
            if (d < 1) continue;
            if (d > dim_now) break;
            const Rect r(box.x + cw * float(col), wkh.bottom() + ch * float(row), cw, ch);
            const Rect inner = r.inset(3.0f, 2.0f);
            const uint64_t id = impl->widget_id("cal_day", r, base + uint64_t(d), st.identity ? base + uint64_t(d) : 0);
            detail::Interact in = detail::interact(impl, id, inner, true);
            if (in.hovered) set_cursor(Cursor::Arrow);
            const bool is_sel = (d == sel);
            const float t_h = detail::animate_value(impl, id * 113u + 1u, (in.hovered && !is_sel) ? 1.0f : 0.0f, 20.0f);
            // 参考资料日期格是**正圆**，不是圆角方块：直径取格子的短边减 6，最小 22。
            // 选中 = 强调色实心圆 + 白字；悬停 = 5% 标签色圆。
            const float dia = std::max(22.0f, std::min(std::min(cw, ch) - 6.0f, 30.0f));
            const Rect dot = Rect::centered(r.cx(), r.cy(), dia, dia);
            if (is_sel) fill_rect_s(dot, th.tint, dia * 0.5f, 0.0f);
            else if (t_h > 0.01f) fill_rect_s(dot, th.label5.scaled_alpha(t_h), dia * 0.5f, 0.0f);

            char buf[8];
            std::snprintf(buf, sizeof buf, "%d", d);
            Style ds;
            ds.font_size = fs - 1.0f;
            ds.font_weight = is_sel ? 600 : 400;
            ds.align = Align::Center;
            ds.valign = VAlign::Middle;
            draw_text(buf, r, is_sel ? detail::on_color(th.tint) : th.label, ds);

            if (in.clicked) {
                sel = d;
                changed = true;
                repaint();
            }
            if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, inner, std::max(0.0f, rad - 2.0f), th.focus_ring);
        }
    }

    if (changed) { *year = y; *month = m; *day = sel; }
    return changed;
}

// -----------------------------------------------------------------------------
//  Popover —— 点触发器弹任意内容
//
//  draw 在**帧末**被调用（和弹层同一时刻），所以可以在里面直接调 ui.button()、
//  ui.input() 之类 —— 它们画出来的东西天然盖在所有普通组件之上。
//  唯一要注意的是：这些控件在帧末才 interact，seq 已经到最大值，所以闸门
//  一定会放行（面板区域本来就 claim 过，而它们的 z 更大，判定为"在上面"）。
// -----------------------------------------------------------------------------
bool Ui::popover(const std::string& label, bool* open, const Rect& box,
                        Vec2 content, const std::function<void(const Rect&)>& draw,
                        const Style& st) {
    if (!open || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("popover", box, 0, st.identity);
    // 层级号取入口处的 seq：触发器和面板都从它往后数。
    const uint64_t z_first = impl->seq + 1;
    // Popover 触发器使用按钮圆角，面板圆角为 12px。
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    const float panel_rad = 12.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;

    detail::Interact in = detail::interact(impl, id, box, !st.disabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    if (in.pressed) detail::grab_focus(impl, id);
    const float t_h = detail::animate_value(impl, id * 109u + 1u,
                                            (in.hovered || *open) ? 1.0f : 0.0f, 18.0f);

    // 触发器外观和 menu() 一致 —— 用户不该为"弹列表"和"弹内容"学两套样子。
    detail::Skin sk = detail::make_skin(th, st, Style::Outline, rad);
    if (st.font_weight <= 0) sk.weight = 500;
    detail::paint_surface(*this, box, sk, t_h, 0.0f, st.disabled);

    const float pad = st.pad_x >= 0.0f ? st.pad_x : 12.0f;
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    Color fg = sk.fg;
    if (st.disabled) fg.a = uint8_t(float(fg.a) * 0.45f);
    draw_text(label, Rect(box.x + pad, box.y, std::max(box.w - pad * 2.0f - 18.0f, 1.0f), box.h), fg, ts);
    system_icon(SystemIcon::ChevronDown,
                Rect::centered(box.right() - pad - 5, box.cy(), 12, 12), fg);
    if (detail::is_focused(impl, id)) detail::paint_focus_ring(*this, box, rad, th.focus_ring);

    if (in.clicked) { *open = !*open; repaint(); }
    if (!*open) return false;

    // 面板：内容尺寸 + 内边距；下方放不下就翻到上方。内边距 20 ——
    // kit `Popover` 的 Content Area 就是 pad 20/20（气泡里的内容离边缘很远）。
    const float inner_pad = 20.0f;
    const float pw = std::max(content.x, 1.0f) + inner_pad * 2.0f;
    const float ph = std::max(content.y, 1.0f) + inner_pad * 2.0f;
    float px = box.x;
    float py = box.bottom() + 6.0f;
    if (py + ph > height() - 6.0f) py = std::max(6.0f, box.y - 6.0f - ph);
    px = detail::clampf(px, 6.0f, std::max(6.0f, width() - pw - 6.0f));
    const Rect panel(px, py, pw, ph);
    const Rect inner = panel.inset(inner_pad, inner_pad);

    // 点面板和触发器以外的地方、或按 Esc → 关闭
    bool dismiss = false;
    const Input& kin = impl->input;
    if (kin.pressed[0] || kin.pressed[1]) {
        if (!panel.contains(kin.mouse.x, kin.mouse.y) && !box.contains(kin.mouse.x, kin.mouse.y))
            dismiss = true;
    }
    if (kin.key_pressed[KEY_ESC]) dismiss = true;

    if (dismiss) { *open = false; repaint(); return false; }

    deferred([this, open, box, z_first, panel, inner, panel_rad, draw]() {
        if (!*open) return;
        const Theme& t2 = impl->theme;
        // 气泡只有投影，没有描边（一圈 1px 线是 Web 的 Popover）
        detail::pop_shadow(*this, panel, panel_rad, t2, 1);
        fill_rect(panel, t2.popover_bg, panel_rad);
        push_clip(panel);
        if (draw) draw(inner);          // 内容自己画，可以继续调 ui.* 组件
        pop_clip();
        // 回调可能关闭面板；只给仍打开的面板登记下一帧闸门。
        if (*open) {
            impl->gate_claim(box, z_first);
            impl->gate_claim(panel, Impl::Z_FRAME_END);
        } else repaint();
    });

    return true;
}

void Ui::color_picker(Color* value, bool* open, const Rect& box,
                      const std::function<void(Color)>& on_change, const Style& st) {
    if (!value || !open || box.empty()) return;
    const uint64_t id = impl->widget_id("colorpicker", box,
        uint64_t(reinterpret_cast<uintptr_t>(value)), st.identity);
    Style trigger_style = st;
    if (trigger_style.variant < 0) trigger_style.secondary();
    const Vec2 content(std::min(288.0f, std::max(1.0f, width() - 52.0f)),
                       std::min(282.0f, std::max(1.0f, height() - 52.0f)));
    popover("", open, box, content, [this, value, open, on_change, st, id](const Rect& area) {
        auto scope = state_scope(id);
        auto sc = scroll(area, 282.0f);
        const Rect r(area.x, area.y - sc.offset, area.w, 282.0f);
        const Theme& th = theme();
        Style controls;
        controls.disabled = st.disabled;
        draw_text("颜色", Rect(r.x, r.y, r.w - 72, 26), th.label,
                  Style().font(16).semi_bold());
        if (button("完成", Rect(r.right() - 56, r.y, 56, 26),
                   Style(controls).primary().font(12).corner(13)))
            *open = false;

        bool changed = false;
        const char* names[] = {"R", "G", "B"};
        const Color fills[] = {th.red, th.green, th.blue};
        for (int i = 0; i < 3; ++i) {
            const float y = r.y + 112 + i * 36.0f;
            draw_text(names[i], Rect(r.x, y, 20, 32), th.label, Style().font(12).semi_bold());
            float channel = float(i == 0 ? value->r : i == 1 ? value->g : value->b);
            if (slider(&channel, 0, 255, Rect(r.x + 24, y, r.w - 68, 32),
                       Style(controls).background(fills[i]).id(uint64_t(i + 1)))) {
                const auto next = uint8_t(std::lround(channel));
                uint8_t& target = i == 0 ? value->r : i == 1 ? value->g : value->b;
                changed |= target != next;
                target = next;
            }
            draw_text(std::to_string(int(std::lround(channel))), Rect(r.right() - 36, y, 36, 32),
                      th.label2, Style().font(12).right());
        }

        draw_text("预设颜色", Rect(r.x, r.y + 228, r.w, 18), th.label2, Style().font(11));
        const Color colors[] = {th.blue, th.indigo, th.purple, th.pink, th.red, th.orange, th.green, th.teal};
        const char* color_names[] = {"Blue", "Indigo", "Purple", "Pink", "Red", "Orange", "Green", "Teal"};
        for (int i = 0; i < 8; ++i) {
            const Rect cell(r.x + 3 + i * (r.w - 30) / 7, r.y + 254, 24, 24);
            if (color_well(colors[i], cell, Style(controls).corner(12))) {
                *value = colors[i];
                *open = false;
                changed = true;
            }
            if (value->rgb24() == colors[i].rgb24() && value->a == colors[i].a)
                stroke_rect(cell.outset(3), colors[i], 15, 1.5f);
            tooltip(cell, color_names[i]);
        }

        const Rect preview(r.x, r.y + 40, r.w, 56);
        fill_rect(preview, *value, 8);
        char hex[10];
        if (value->a == 255)
            std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", value->r, value->g, value->b);
        else
            std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", value->r, value->g, value->b, value->a);
        const Color foreground = value->over(th.popover_bg).luminance() > .55f
                                     ? Color(0, 0, 0, 217) : Color::white();
        draw_text(hex, preview.inset(14, 0), foreground, Style().font(18).semi_bold().right());
        if (changed) {
            if (on_change) on_change(*value);
            request_animation_frame();
        }
    }, trigger_style);
    fill_circle(box.x + 15, box.cy(), 6, st.disabled ? value->scaled_alpha(.45f) : *value);
    char hex[10];
    if (value->a == 255)
        std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", value->r, value->g, value->b);
    else
        std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", value->r, value->g, value->b, value->a);
    const Color foreground = st.fg.is_none() ? theme().label : st.fg;
    draw_text(hex, Rect(box.x + 30, box.y, std::max(0.0f, box.w - 60), box.h),
              st.disabled ? foreground.scaled_alpha(.45f) : foreground,
              Style().font(st.font_size > 0 ? st.font_size : theme().font_size));
}

// Hover Card —— 悬停弹卡片。和 tooltip 一样不 claim 输入：
// 它只是"看"的，抢了点击反而会挡住下面的控件。
void Ui::hover_card(const Rect& anchor, Vec2 content,
                           const std::function<void(const Rect&)>& draw, const Style& st) {
    if (!draw || content.x <= 0.0f || content.y <= 0.0f) return;
    const Input& kin = impl->input;
    impl->hover_region(anchor);       // 同上：锚点不是 interact 控件
    if (!anchor.contains(kin.mouse.x, kin.mouse.y)) return;
    if (impl->gate_blocked_pointer()) return;

    // 和 popover 同一套面板：r12、只有投影（悬停卡片是"轻气泡"，不是卡片）
    const float rad = st.radius >= 0.0f ? st.radius : 12.0f;
    const float pad = 12.0f;
    const float pw = content.x + pad * 2.0f;
    const float ph = content.y + pad * 2.0f;

    float px = anchor.right() + 8.0f;
    if (px + pw > width() - 6.0f) px = anchor.x - 8.0f - pw;
    px = detail::clampf(px, 6.0f, std::max(6.0f, width() - pw - 6.0f));
    float py = anchor.y;
    if (py + ph > height() - 6.0f) py = std::max(6.0f, height() - ph - 6.0f);

    const Rect panel(px, py, pw, ph);
    const Rect inner = panel.inset(pad, pad);
    deferred([this, panel, inner, rad, draw]() {
        const Theme& t2 = impl->theme;
        detail::pop_shadow(*this, panel, rad, t2, 1);
        fill_rect(panel, t2.popover_bg, rad);
        push_clip(panel);
        draw(inner);
        pop_clip();
    });
}

// -----------------------------------------------------------------------------
//  Alert Dialog —— 带确认/取消的模态框
//
//  和 dialog() 的差别不只是多了两个按钮：**返回值语义**不同。
//  dialog() 返回"本帧是否打开"（因为按钮由调用方自己画），
//  这里返回"本帧是否点了确认" —— 危险操作要的就是这一个布尔。
// -----------------------------------------------------------------------------
bool Ui::alert_dialog(const std::string& title, const std::string& body,
                             const std::string& confirm, const std::string& cancel,
                             bool* open, const Rect& box, const Style& st) {
    if (!open || !*open) return false;
    const Theme& th = impl->theme;
    const uint64_t z_first = impl->seq + 1;
    // Alert 面板圆角 12px，标题 13pt Bold，正文 13pt，内边距 20px。
    const float rad = st.radius >= 0.0f ? st.radius : 12.0f;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : 20.0f;
    const bool danger = (st.variant == Style::Destructive);

    const Input& kin = impl->input;
    bool dismiss = false;
    if (!box.contains(kin.mouse.x, kin.mouse.y) && (kin.pressed[0] || kin.pressed[1])) dismiss = true;
    if (kin.key_pressed[KEY_ESC]) dismiss = true;

    // 操作按钮以通宽行纵向排列，取消位于底部；交互在遮罩绘制前处理。
    const float pad_v = 14.0f;
    const float btn_h = 28.0f;
    const float action_gap = 6.0f;
    const float inner_w = std::max(box.w - pad * 2.0f, 1.0f);
    const Rect cancel_r(box.x + pad, box.bottom() - pad_v - btn_h, inner_w, btn_h);
    const Rect ok_r(box.x + pad, cancel_r.y - action_gap - btn_h, inner_w, btn_h);

    const uint64_t ok_id = impl->widget_id("alert_ok", box, 0, st.identity);
    const uint64_t no_id = impl->widget_id("alert_no", box, 1u, st.identity);
    detail::Interact ok_in = detail::interact(impl, ok_id, ok_r, true);
    detail::Interact no_in = detail::interact(impl, no_id, cancel_r, true);
    if (ok_in.hovered || no_in.hovered) set_cursor(Cursor::Arrow);

    bool confirmed = false;
    if (ok_in.clicked) confirmed = true;
    if (no_in.clicked) dismiss = true;

    const float ok_t = detail::animate_value(impl, ok_id * 127u + 1u, ok_in.hovered ? 1.0f : 0.0f, 20.0f);
    const float no_t = detail::animate_value(impl, no_id * 127u + 1u, no_in.hovered ? 1.0f : 0.0f, 20.0f);

    // ---- 画 ----
    // 不铺整屏黑幕：macOS 的 alert 是"浮起来的一块面板 + 强投影"，窗口本身
    // 只是失去焦点，不会被压暗。整屏 35% 黑是 Web Dialog 的 overlay。
    const Color panel_bg = !st.bg.is_none() ? st.bg : th.popover_bg;
    detail::pop_shadow(*this, box, rad, th, 2);
    fill_rect(box, panel_bg, rad);
    // 面板不描边：macOS 靠投影与底色分层，一圈 1px 会让它看起来像 Web 的卡片

    push_clip(box);
    const float title_lh = line_height(fs);
    Style hs;
    hs.font_size = fs;          // Headline 13 Bold
    hs.font_weight = 700;
    hs.align = Align::Left;
    hs.valign = VAlign::Top;
    hs.wrap = true;
    draw_text(title, Rect(box.x + pad, box.y + pad_v, std::max(box.w - pad * 2.0f, 1.0f), title_lh),
              !st.fg.is_none() ? st.fg : th.label, hs);

    if (!body.empty()) {
        Style bs;
        bs.font_size = 11.0f;              // 说明文字是 Footnote 11，不是 13
        bs.font_weight = 400;
        bs.align = Align::Left;
        bs.valign = VAlign::Top;
        bs.wrap = true;
        bs.line_gap = 3.0f;
        draw_text(body, Rect(box.x + pad, box.y + pad_v + title_lh + 10.0f, box.w - pad * 2.0f,
                             std::max(ok_r.y - 12.0f - (box.y + pad_v + title_lh + 10.0f), 1.0f)),
                  th.label, bs);
    }
    pop_clip();

    // 两个操作复用普通按钮的表面与圆角，避免确认行在悬停时出现方形色块。
    const float cr = cancel_r.h * 0.5f;
    Style ok_style, cancel_style;
    ok_style.corner(cr);
    if (danger) {
        ok_style.destructive();
        ok_style.background(th.red.mix(panel_bg, 0.78f));
    } else {
        ok_style.primary();
    }
    cancel_style.secondary().corner(cr);
    const auto ok_skin = detail::make_skin(th, ok_style, Style::Default, cr);
    const auto cancel_skin = detail::make_skin(th, cancel_style, Style::Default, cr);
    detail::paint_surface(*this, ok_r, ok_skin, ok_t, ok_in.held ? 1.f : 0.f, false);
    detail::paint_surface(*this, cancel_r, cancel_skin, no_t, no_in.held ? 1.f : 0.f, false);

    Style bs2;
    bs2.font_size = fs;
    bs2.align = Align::Center;
    bs2.valign = VAlign::Middle;
    bs2.font_weight = 500;
    draw_text(cancel, cancel_r, cancel_skin.fg, bs2);
    draw_text(confirm, ok_r, ok_skin.fg, bs2);

    impl->gate_claim(box, z_first);

    if (confirmed)        { *open = false; repaint(); }
    else if (dismiss)     { *open = false; repaint(); }
    return confirmed;
}

// -----------------------------------------------------------------------------
//  Drawer / Sheet —— 从某一边滑出的面板
//
//  面板位置由 box 指定，内容回调在裁剪区内绘制，不做隐式位移动画。
// -----------------------------------------------------------------------------
bool Ui::drawer(const std::string& title, bool* open, const Rect& box,
                       const std::function<void(const Rect&)>& draw, const Style& st) {
    if (!open || box.w <= 0.0f || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t z_first = impl->seq + 1;
    const float fs = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = st.pad_x >= 0.0f ? st.pad_x : 20.0f;
    const float head_h = std::max(fs + 32.0f, 54.0f);

    if (!*open) return false;

    const Input& kin = impl->input;
    bool dismiss = false;
    if (kin.key_pressed[KEY_ESC]) dismiss = true;
    if ((kin.pressed[0] || kin.pressed[1]) && !box.contains(kin.mouse.x, kin.mouse.y)) dismiss = true;

    // 面板依靠圆角和投影分层，不绘制全窗口遮罩。
    const Color bg = !st.bg.is_none() ? st.bg : th.window_bg;

    // 面板：r10 + 二级投影，不描边（描边会把它读成"网页侧边栏"）
    detail::pop_shadow(*this, box, 10.0f, th, 2);
    fill_rect(box, bg, 10.0f);

    Style hs;
    hs.font_size = fs + 2.0f;
    hs.font_weight = 600;
    hs.align = Align::Left;
    hs.valign = VAlign::Middle;
    draw_text(title, Rect(box.x + pad, box.y, std::max(box.w - pad * 2.0f - 30.0f, 1.0f), head_h),
              !st.fg.is_none() ? st.fg : th.label, hs);

    fill_rect(Rect(box.x, box.y + head_h - 1.0f, box.w, 1.0f), th.separator, 0.0f);

    const Rect content(box.x + pad, box.y + head_h + pad * 0.5f,
                       std::max(box.w - pad * 2.0f, 1.0f),
                       std::max(box.h - head_h - pad * 0.5f - pad, 1.0f));
    if (draw) { push_clip(content); draw(content); pop_clip(); }

    impl->gate_claim(box, z_first);
    if (dismiss) { *open = false; repaint(); }
    return true;
}

// -----------------------------------------------------------------------------
//  Context Menu —— 在 area 内右键弹出
//
//  位置记在 flags 里（打开那一刻的鼠标坐标）。为什么不每帧读鼠标：
//  弹出来以后手一动菜单就会跟着跑，那没法用。
// -----------------------------------------------------------------------------
bool Ui::context_menu(const Rect& area, const std::vector<std::string>& items,
                             int* clicked_index, const Style& st) {
    if (items.empty()) return false;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("ctxmenu", area, uint64_t(items.size()), st.identity);
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float item_h = std::max(fs + 10.0f, 24.0f);   // kit Menu/Height Medium = 24
    const Input& kin = impl->input;

    bool open = impl->flags[id] > 0.5f;

    // 右键：必须真的落在 area 里，而且没被浮层压住
    if (!open && kin.pressed[1] && area.contains(kin.mouse.x, kin.mouse.y)
        && !impl->gate_blocked(impl->seq, kin.mouse.x, kin.mouse.y)) {
        impl->flags[id * 127u + 1u] = kin.mouse.x;
        impl->flags[id * 127u + 2u] = kin.mouse.y;
        impl->flags[id] = 1.0f;
        open = true;
        repaint();
    }
    if (!open) return false;

    float lw = 120.0f;
    for (const auto& it : items) lw = std::max(lw, text_width(it, fs, 500) + 36.0f);
    const float lh = item_h * float(items.size()) + 8.0f;
    float lx = impl->flags[id * 127u + 1u];
    float ly = impl->flags[id * 127u + 2u];
    lx = detail::clampf(lx, 4.0f, std::max(4.0f, width() - lw - 4.0f));
    ly = detail::clampf(ly, 4.0f, std::max(4.0f, height() - lh - 4.0f));
    const Rect list(lx, ly, lw, lh);

    std::vector<Rect> rects;
    std::vector<uint64_t> ids;
    std::vector<uint8_t> hov;
    rects.reserve(items.size());
    ids.reserve(items.size());
    hov.reserve(items.size());
    for (size_t i = 0; i < items.size(); ++i) {
        const Rect r(list.x + 4.0f, list.y + 4.0f + item_h * float(i), list.w - 8.0f, item_h);
        rects.push_back(r);
        ids.push_back(impl->widget_id("ctx_item", r, id + uint64_t(i), st.identity ? id + uint64_t(i) : 0));
        hov.push_back(0);
    }

    int chosen = -1;
    {
    Impl::SeqFrameEnd guard(impl);      // 项在帧内交互、帧末绘制，序号算帧末层
    for (size_t i = 0; i < items.size(); ++i) {
        detail::Interact ii = detail::interact(impl, ids[i], rects[i], true);
        if (ii.hovered) { set_cursor(Cursor::Arrow); hov[i] = 1; }
        if (ii.clicked) chosen = int(i);
    }
    }

    bool close = false;
    if (chosen >= 0) close = true;
    if ((kin.pressed[0] || kin.pressed[1]) && !list.contains(kin.mouse.x, kin.mouse.y)) close = true;
    if (kin.key_pressed[KEY_ESC]) close = true;

    if (!close) {
        impl->gate_claim(list, Impl::Z_FRAME_END);
        std::vector<std::string> items_copy = items;
        deferred([this, list, rects, ids, hov, items_copy, fs]() mutable {
            detail::paint_menu(*this, list, items_copy, rects, ids, hov, -1, false, fs);
        });
    }

    if (close) {
        impl->flags[id] = 0.0f;
        repaint();
    }
    if (chosen >= 0) {
        if (clicked_index) *clicked_index = chosen;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
//  Combobox —— 可搜索的下拉选择
//
//  和 select() 的差别只有一条：能打字过滤。但就是这一条决定了它不能直接
//  套用 select —— 展开期间要持续接收字符，所以必须真正拿住焦点。
//
//  绘制全部丢进 deferred()：列表是浮层，帧内画会被后面调用的组件压住。
//  交互仍在帧内做完，把"这一帧该长什么样"用值捕获带进 lambda。
// -----------------------------------------------------------------------------
bool Ui::combobox(const std::vector<std::string>& items, int* index,
                         const Rect& box, const Style& st) {
    if (!index || items.empty() || box.w <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("combobox", box, 0, st.identity);
    const uint64_t z_first = impl->seq + 1;
    const float rad = st.radius >= 0.0f ? st.radius : th.radius;
    const float fs  = st.font_size > 0.0f ? st.font_size : th.font_size;
    const float pad = 12.0f;
    // kit: Menu Item/Menu 高 **24**（不是 32）。行太高会立刻变成 Web 的列表。
    const float item_h = std::max(fs + 10.0f, 24.0f);

    bool open = impl->flags[id] > 0.5f;
    std::string& query = impl->strings[id];

    detail::Interact in = detail::interact(impl, id, box, !st.disabled);
    if (in.hovered) set_cursor(Cursor::Arrow);
    if (in.pressed) detail::grab_focus(impl, id);
    bool focused = detail::is_focused(impl, id);
    if (focused && impl->gate_blocked_here(box)) { detail::drop_focus(impl); focused = false; }

    if (in.clicked) {
        open = !open;
        impl->flags[id] = open ? 1.0f : 0.0f;
        if (open) {
            query.clear();
            impl->flags[id * 131u + 1u] = float(std::max(*index, 0));
        }
        repaint();
    }
    const float t_h = detail::animate_value(impl, id * 137u + 1u, (in.hovered || open) ? 1.0f : 0.0f, 18.0f);

    // ---- 触发器 ----
    // 复用主题输入表面、顶部内阴影和细描边，右侧绘制上下箭头。
    fill_rect(box, !st.bg.is_none() ? st.bg
                                   : (th.is_dark() ? th.control_bg : th.window_bg), rad);
    inner_shadow(box, th.is_dark() ? Color(0, 0, 0, 70) : Color(0, 0, 0, focused ? 10 : 18),
                 rad, 1.8f, 1.0f);
    Color bd = !st.border_color.is_none() ? st.border_color : th.separator;
    if (t_h > 0.01f && !focused) bd = bd.mix(th.label3, t_h);
    stroke_rect(box, bd, rad, 1.0f);

    const bool has_sel = (*index >= 0 && *index < int(items.size()));
    // 占位符 25%（同 Text Field），已选值 85%
    Color fg = !st.fg.is_none() ? st.fg : (has_sel ? th.label : th.label3);
    Style ts;
    ts.font_size = fs;
    ts.font_weight = 500;
    ts.align = Align::Left;
    ts.valign = VAlign::Middle;
    draw_text(has_sel ? items[size_t(*index)]
                      : (st.placeholder.empty() ? std::string("选择...") : st.placeholder),
              Rect(box.x + pad, box.y, std::max(box.w - pad * 2.0f - 18.0f, 1.0f), box.h), fg, ts);
    detail::paint_popup_chevrons(*this, box, pad, th.label2);
    // 焦点走 3px 光环（同 input），不要"把边框染粗"—— 那是 Web 的 :focus 写法
    if (focused) detail::paint_focus_ring(*this, box, rad, th.focus_ring, true);

    if (!open) return false;

    // ---- 键盘：搜索词 / 上下移动 / 回车 / Esc ----
    if (focused) {
        const Input& kin = impl->input;
        if (!kin.text_input.empty()) { query += kin.text_input; repaint(); }
        if (kin.key_hit(KEY_BACKSPACE) && !query.empty()) {
            // 删最后一个 UTF-8 码点（不能直接 pop_back：一个汉字三字节）
            size_t n = query.size();
            while (n > 0 && (uint8_t(query[n - 1]) & 0xC0) == 0x80) --n;
            if (n > 0) --n;
            query.erase(n);
            repaint();
        }
    }

    // ---- 过滤 ----
    std::vector<int> vis;
    vis.reserve(items.size());
    for (int i = 0; i < int(items.size()); ++i)
        if (detail::contains_ci(items[size_t(i)], query)) vis.push_back(i);

    const int max_rows = 7;
    const int rows = std::min(int(vis.size()), max_rows);
    const float search_h = std::max(fs + 8.0f, 22.0f);

    float& hi_f = impl->flags[id * 131u + 1u];
    int hi = detail::clampi(int(hi_f + 0.5f), 0, std::max(0, int(vis.size()) - 1));

    bool picked = false, close = false;
    if (focused) {
        const Input& kin = impl->input;
        if (kin.key_hit(KEY_DOWN) && hi + 1 < int(vis.size())) { ++hi; repaint(); }
        if (kin.key_hit(KEY_UP) && hi > 0) { --hi; repaint(); }
        if (kin.key_pressed[KEY_ENTER] && !vis.empty()) {
            *index = vis[size_t(hi)];
            picked = true;
            close = true;
        }
        if (kin.key_pressed[KEY_ESC]) close = true;
    }

    // ---- 列表几何 ----
    const float list_h = search_h + 4.0f + item_h * float(std::max(rows, 1)) + 8.0f;
    float ly = box.bottom() + 4.0f;
    if (ly + list_h > height() - 4.0f) ly = std::max(4.0f, box.y - 4.0f - list_h);
    const Rect list(box.x, ly, box.w, list_h);
    const Rect sr(list.x + 4.0f, list.y + 4.0f, list.w - 8.0f, search_h);

    impl->gate_claim(box, z_first);
    impl->gate_claim(list, Impl::Z_FRAME_END);

    // ---- 逐项交互（帧内），绘制另算 ----
    // 交互确实发生在帧内，但这些项视觉上属于帧末弹层。临时把序号抬到帧末区：
    // 闸门才会把它们当"弹层内容"放行，同时挡住同帧排在后面的普通控件。
    int chosen_now = -1;
    {
    Impl::SeqFrameEnd guard(impl);
    for (int r = 0; r < rows; ++r) {
        const Rect rr(list.x + 4.0f, sr.bottom() + 4.0f + item_h * float(r), list.w - 8.0f, item_h);
        const uint64_t iid = impl->widget_id("cb_item", rr, id + uint64_t(vis[size_t(r)]), st.identity ? id + uint64_t(vis[size_t(r)]) : 0);
        detail::Interact ii = detail::interact(impl, iid, rr, true);
        if (ii.hovered) { set_cursor(Cursor::Arrow); hi = r; }
        if (ii.clicked) chosen_now = vis[size_t(r)];
    }
    }
    hi_f = float(hi);

    if ((impl->input.pressed[0] || impl->input.pressed[1])
        && !list.contains(impl->input.mouse.x, impl->input.mouse.y)
        && !box.contains(impl->input.mouse.x, impl->input.mouse.y)) {
        close = true;
    }

    // ---- 帧末绘制 ----
    const Theme th_c = th;
    std::vector<std::string> items_c = items;
    std::vector<int> vis_c = vis;
    const std::string query_c = query;
    const int hi_c = hi;
    const int sel_c = *index;
    deferred([this, list, sr, rad, item_h, fs, th_c, items_c, vis_c, query_c, hi_c, sel_c, rows]() {
        // 容器和 menu 是同一套：r10、Quaternary 投影、1px 分隔线
        const float lrad = 10.0f;
        detail::pop_shadow(*this, list, lrad, th_c, 0);
        fill_rect(list, th_c.popover_bg, lrad);
        stroke_rect(list, th_c.separator, lrad, 1.0f);
        push_clip(list);

        // 搜索框采用文本框样式，行首绘制系统搜索图标。
        const float search_r = 5.0f;
        fill_rect_s(sr, th_c.is_dark() ? Color(255, 255, 255, 18) : Color(0, 0, 0, 13), search_r, 0.35f);
        {
            system_icon(SystemIcon::Search, Rect::centered(sr.x + 10.f, sr.cy(), 12, 12), th_c.label2);
        }
        Style ss;
        ss.font_size = fs;
        ss.font_weight = 400;
        ss.align = Align::Left;
        ss.valign = VAlign::Middle;
        draw_text(query_c.empty() ? std::string("搜索") : query_c, sr.inset(20.0f, 0.0f),
                  query_c.empty() ? th_c.label3 : th_c.label, ss);

        for (int r = 0; r < rows; ++r) {
            const int ii = vis_c[size_t(r)];
            const Rect rr(list.x + 4.0f, sr.bottom() + 4.0f + item_h * float(r), list.w - 8.0f, item_h);
            const bool on  = (sel_c == ii);
            const bool hot = (r == hi_c);
            // 参考资料弹出菜单：高亮行是**强调色实心 + 白字**，不是 13% 的一层灰。
            // 当前值不靠底色，靠行首的对勾（所以所有行都留出 16px 的勾位）。
            if (hot) fill_rect_s(rr, th_c.tint, 5.0f, 0.35f);
            const Color fg = hot ? detail::on_color(th_c.tint) : th_c.label;
            if (on) {
                system_icon(SystemIcon::Check, Rect::centered(rr.x + 10.f, rr.cy(), 12, 12), fg);
            }
            Style its;
            its.font_size = fs;
            its.font_weight = on ? 600 : 500;
            its.align = Align::Left;
            its.valign = VAlign::Middle;
            draw_text(items_c[size_t(ii)], Rect(rr.x + 24.0f, rr.y, rr.w - 30.0f, rr.h), fg, its);
        }
        if (rows == 0) {
            Style es;
            es.font_size = fs;
            es.font_weight = 400;
            es.align = Align::Left;
            es.valign = VAlign::Middle;
            draw_text("无匹配项",
                      Rect(list.x + 14.0f, sr.bottom() + 4.0f, list.w - 20.0f, item_h),
                      th_c.label2, es);
        }
        pop_clip();
    });

    if (chosen_now >= 0) { *index = chosen_now; picked = true; close = true; }
    if (close) {
        impl->flags[id] = 0.0f;
        query.clear();
        repaint();
    }
    return picked;
}

// -----------------------------------------------------------------------------
//  Split Pane —— 可拖拽的分割
//
//  拖拽状态放 flags：不能靠 "按下 + 在热区" 判断，因为鼠标一动就离开热区了。
//  vertical=true 表示分隔条是竖的（左右两块），和参数名的方位一致。
// -----------------------------------------------------------------------------
bool Ui::split_pane(float* ratio, const Rect& box, Rect* first, Rect* second,
                           bool vertical, float min_ratio, const Style& st) {
    if (!ratio || box.w <= 0.0f || box.h <= 0.0f) return false;
    const Theme& th = impl->theme;
    const uint64_t id = impl->widget_id("split_pane", box, vertical ? 1u : 2u, st.identity);
    const float bar = 7.0f;                    // 热区宽度（视觉只有 1px）
    float r = detail::clampf(*ratio, min_ratio, 1.0f - min_ratio);

    Rect handle;
    if (vertical) handle = Rect(box.x + box.w * r - bar * 0.5f, box.y, bar, box.h);
    else          handle = Rect(box.x, box.y + box.h * r - bar * 0.5f, box.w, bar);

    const bool hot = handle.contains(impl->input.mouse.x, impl->input.mouse.y);
    impl->hover_region(handle);       // 分隔条靠光标形状反馈，得能被"唤醒"
    if (hot) set_cursor(vertical ? Cursor::ResizeH : Cursor::ResizeV);

    bool dragging = impl->flags[id] > 0.5f;
    if (hot && impl->input.pressed[0]) { impl->flags[id] = 1.0f; dragging = true; }
    if (dragging && !impl->input.down[0]) { impl->flags[id] = 0.0f; dragging = false; }

    bool changed = false;
    if (dragging) {
        const float raw = vertical
            ? (impl->input.mouse.x - box.x) / std::max(box.w, 1.0f)
            : (impl->input.mouse.y - box.y) / std::max(box.h, 1.0f);
        const float nr = detail::clampf(raw, min_ratio, 1.0f - min_ratio);
        if (std::fabs(nr - *ratio) > 0.0005f) { *ratio = nr; r = nr; changed = true; repaint(); }
    }

    // 视觉只有 1px，热区是 7px —— 和滚动条同一个取舍：看得见但不抢眼。
    const Color bc = dragging ? th.focus_ring : (hot ? th.label.with_alpha(70) : th.separator);
    if (vertical) fill_rect(Rect(box.x + box.w * r - 0.5f, box.y, 1.0f, box.h), bc, 0.0f);
    else          fill_rect(Rect(box.x, box.y + box.h * r - 0.5f, box.w, 1.0f), bc, 0.0f);

    if (first) {
        *first = vertical ? Rect(box.x, box.y, std::max(box.w * r - bar * 0.5f, 0.0f), box.h)
                          : Rect(box.x, box.y, box.w, std::max(box.h * r - bar * 0.5f, 0.0f));
    }
    if (second) {
        *second = vertical
            ? Rect(box.x + box.w * r + bar * 0.5f, box.y, std::max(box.w * (1.0f - r) - bar * 0.5f, 0.0f), box.h)
            : Rect(box.x, box.y + box.h * r + bar * 0.5f, box.w, std::max(box.h * (1.0f - r) - bar * 0.5f, 0.0f));
    }
    return changed;
}

// =============================================================================
//  8.16  滚动
//  作用域保存偏移并压入裁剪区，Stack 构建时应用该偏移。
//
//  通用滚动与虚拟滚动走的是**同一个函数**：虚拟模式只是把内容高度算成
//  行数 × 行高，然后按可见区间挑着调用回调。所以偏移只存一份（按 box 记忆），
//  滚动条、裁剪、滚轮都只有一套 —— 不会出现"两个滚动区各自记位置"的怪事。
// =============================================================================
namespace detail {
// 滚动条几何。视觉上只有 7px 宽（macOS 的 overlay 滑块），热区给到 15px ——
// 7px 是画给人看的，不是点得中的尺寸。
struct ScrollbarGeom {
    Rect track;     // 命中区（右侧 15px 竖条）
    Rect thumb;     // 滑块（7px 宽，距右边缘 2.5px）
    float travel;   // 滑块可移动的行程 = box.h - thumb.h
};

inline ScrollbarGeom scrollbar_geom(const Rect& box, float offset, float max_offset) {
    ScrollbarGeom g;
    g.track = Rect(box.right() - 15.0f, box.y, 15.0f, box.h);
    const float mo = std::max(max_offset, 0.0f);
    const float thumb_h = std::max(28.0f, box.h * (box.h / (box.h + mo)));
    const float t = mo > 0.0f ? clampf(offset / mo, 0.0f, 1.0f) : 0.0f;
    g.thumb = Rect(box.right() - 9.5f, box.y + (box.h - thumb_h) * t, 7.0f, thumb_h);
    g.travel = std::max(box.h - thumb_h, 0.0f);
    return g;
}

// macOS 的滚动条是 **overlay**：只在滚动 / 悬停时出现，停手约 0.9 秒后淡出。
// 常驻的细条是 Windows 与浏览器的习惯 —— 一条永远挂在那里的 3px 竖线，是
// "这是个网页控件"最直白的提示。activity 记在 flags 里（按滚动区 id 区分）。
//
// 四个状态槽（偏移 / 激活时刻 / 上次偏移 / 拖动抓点）全部从**滚动区自己的 id**
// 派生。早先 alpha / prev / grab 走 scrollbar_id()（它会在"栈顶 box 相同"时
// 悄悄换成滚动区 id），而 end_scroll() 又自己拼一套 hash_combine —— 两套键只是
// 碰巧相等，改任一处的键名就会漂。
inline uint64_t scrollbar_slot(uint64_t id, const char* kind) {
    return hash_combine(id, hash_string(kind));
}

inline float scrollbar_alpha(Ui::Impl* ui, uint64_t id, bool hot, bool moved) {
    float& act = ui->flags[scrollbar_slot(id, "scrollact")];
    if (hot || moved) act = ui->clock;
    if (act <= 0.0f) return 0.0f;                 // 还没滚过：一帧都不画
    const float age = ui->clock - act;
    if (hot) return 0.80f;
    if (age >= 0.9f) return 0.0f;
    return 0.62f * (1.0f - clampf(age / 0.9f, 0.0f, 1.0f));
}

// 滚动条交互：按下滑块 → 拖动；按下轨道 → 滑块中心跳到点击处（Windows 传统行为，
// 比"翻一页"更好预测，也少一次点击）。拖动时鼠标可以离开视口 —— 后端已经
// SetCapture，down[0] 仍然成立。返回鼠标是否在热区上（用来加深颜色）。
inline bool scrollbar_interact(Ui::Impl* ui, uint64_t id, const Rect& box, float max_offset, float& offset) {
    const ScrollbarGeom g = scrollbar_geom(box, offset, max_offset);
    const Input& kin = ui->input;
    const bool on_track = g.track.contains(kin.mouse.x, kin.mouse.y);
    ui->hover_region(g.track);        // 悬停要变亮，时钟靠帧推进
    if (ui->gate_blocked_pointer()) return on_track;   // 浮层打开时不许动下层滚动条

    const uint64_t sid = scrollbar_slot(id, "scrollgrab");
    float& grab = ui->flags[sid];       // 按下时鼠标在滑块内的相对位置
    const float travel = std::max(g.travel, 1.0f);

    if (kin.pressed[0] && on_track) {
        ui->active = sid;
        ui->active_scope_id = ui->state_scope_id;
        if (g.thumb.contains(kin.mouse.x, kin.mouse.y)) {
            grab = kin.mouse.y - g.thumb.y;
        } else {
            grab = g.thumb.h * 0.5f;
            offset = clampf((kin.mouse.y - box.y - grab) / travel, 0.0f, 1.0f) * max_offset;
            if (ui->request_repaint) ui->request_repaint();
        }
    }
    if (ui->active == sid && kin.down[0]) {
        const float nv = clampf((kin.mouse.y - box.y - grab) / travel, 0.0f, 1.0f) * max_offset;
        if (std::fabs(nv - offset) > 0.01f) {
            offset = nv;
            if (ui->request_repaint) ui->request_repaint();
        }
    }
    if (kin.released[0] && ui->active == sid) ui->active = 0;
    return on_track;
}

// 画一次滚动条：夹偏移 → 吃交互 → 判"刚动过" → 淡出。
// end_scroll() 和 scroll_bar() 走同一份实现（早先两处各写一遍，连 prev 的键都不同）。
struct ScrollbarDraw {
    float offset = 0.0f;
    bool  need_frame = false;   // 淡出中途，需要下一帧继续推进
};

inline ScrollbarDraw scrollbar_draw(Ui& ui, uint64_t id, const Rect& box, float max_offset) {
    Ui::Impl* im = ui.impl;
    ScrollbarDraw out;
    float& off = im->flags[id];
    off = clampf(off, 0.0f, max_offset);
    const bool hot = scrollbar_interact(im, id, box, max_offset, off);
    off = clampf(off, 0.0f, max_offset);
    out.offset = off;

    // "刚才滚过没有"：和上一帧的偏移比。滚轮、拖动、程序调用都算。
    float& prev = im->flags[scrollbar_slot(id, "scrollprev")];
    const bool moved = std::fabs(off - prev) > 0.01f;
    prev = off;

    const float a = scrollbar_alpha(im, id, hot, moved);
    if (a <= 0.01f) return out;
    const ScrollbarGeom g = scrollbar_geom(box, off, max_offset);
    ui.fill_rect(g.thumb, ui.theme().scrollbar.scaled_alpha(a), 3.5f);
    out.need_frame = (a < 0.79f);
    return out;
}
} // namespace detail

ScrollScope Ui::scroll(const Rect& box, float content_h) {
    // 测量模式：滚动区按**视口**占高度（内容多高跟布局无关，那是滚动条的事）
    if (impl->measure_on) impl->measure_add(box);
    ScrollScope sc;
    sc.ui         = this;
    sc.id         = impl->widget_id("scroll", box);
    sc.state_scope_id = impl->state_scope_id;
    sc.box        = box;
    sc.content_h  = content_h;
    sc.max_offset = std::max(0.0f, content_h - box.h);

    float& off = impl->flags[sc.id];
    const Input& kin = impl->input;
    const int depth = int(impl->scroll_stack.size());
    if (!impl->measure_on) impl->scroll_memo_next.push_back(Impl::ScrollRectMemo{box, depth});
    // 测量模式不吃滚轮：同一份绘制代码要跑两遍，吃两次就等于把滚动距离翻倍。
    //
    // 嵌套时只让最内层吃：外层滚动区一定也 contains(鼠标)，只按位置判定就会
    // 出现"滚内层列表，内容区跟着一起滚"。判据用上一帧的滚动区快照（更深且含
    // 指针 → 本层让路），第一帧没有快照，退化成老行为。
    if (!impl->measure_on && sc.max_offset > 0.0f && box.contains(kin.mouse.x, kin.mouse.y)
        && !impl->gate_blocked_pointer()
        && !impl->deeper_scroll_has_pointer(depth, kin.mouse.x, kin.mouse.y)
        && std::fabs(kin.wheel) > 0.001f) {
        off = detail::clampf(off - kin.wheel * 44.0f, 0.0f, sc.max_offset);
        repaint();
    }
    off = detail::clampf(off, 0.0f, sc.max_offset);
    sc.offset = off;

    Ui::Impl::ScrollCtx ctx;
    ctx.id = sc.id;
    ctx.state_scope_id = sc.state_scope_id;
    ctx.box = box;
    ctx.offset = off;
    ctx.max_offset = sc.max_offset;

    impl->scroll_stack.push_back(ctx);
    impl->clips.push_back(impl->phys(box));
    impl->clip_logic.push_back(box);
    impl->recompute_clip_logic();
    impl->recompute_clip();
    sc.active = true;
    return sc;
}

// 收尾：画滚动条（放在收尾而不是开头，才能盖在内容之上）、弹裁剪与偏移。
// 幂等收尾，允许在作用域析构前显式 close()。
void ScrollScope::close() {
    if (!active || !ui) return;
    active = false;
    if (ui->impl && !ui->impl->scroll_stack.empty() &&
        ui->impl->scroll_stack.back().id == id)
        ui->end_scroll();
}

ScrollScope::~ScrollScope() { close(); }

void ScrollScope::content(float h) {
    content_h  = h;
    max_offset = std::max(0.0f, h - box.h);
    if (!ui || !ui->impl) return;
    auto state = ui->state_scope(state_scope_id);
    float& off = ui->impl->flags[id];
    off = detail::clampf(off, 0.0f, max_offset);
    offset = off;
    if (!ui->impl->scroll_stack.empty() && ui->impl->scroll_stack.back().id == id) {
        ui->impl->scroll_stack.back().max_offset = max_offset;
        ui->impl->scroll_stack.back().offset = off;
    }
}

void ScrollScope::to(float v) {
    if (!ui || !ui->impl) return;
    auto state = ui->state_scope(state_scope_id);
    float& off = ui->impl->flags[id];
    const float nv = detail::clampf(v, 0.0f, max_offset);
    if (nv != off) { off = nv; ui->repaint(); }
    offset = nv;
    if (!ui->impl->scroll_stack.empty() && ui->impl->scroll_stack.back().id == id) {
        ui->impl->scroll_stack.back().offset = nv;
    }
}


// 仅供 ScrollScope 收尾，先绘制滚动条，再恢复外层裁剪。
void Ui::end_scroll() {
    if (impl->scroll_stack.empty()) return;
    const Ui::Impl::ScrollCtx ctx = impl->scroll_stack.back();
    auto state = state_scope(ctx.state_scope_id);

    // 滚动条必须在内容绘制后叠加。
    // 先吃交互再画：同一帧里拖动要立刻反映到滑块位置上，晚一帧会明显发黏。
    if (ctx.max_offset > 0.0f) {
        const detail::ScrollbarDraw sb =
            detail::scrollbar_draw(*this, ctx.id, ctx.box, ctx.max_offset);

        if (sb.need_frame) repaint();      // 淡出要帧推进，否则停在半透明上
    }

    impl->scroll_stack.pop_back();
    if (!impl->clips.empty()) impl->clips.pop_back();
    if (!impl->clip_logic.empty()) impl->clip_logic.pop_back();
    impl->recompute_clip_logic();
    impl->recompute_clip();
}

// -----------------------------------------------------------------------------
//  虚拟滚动
//  只算可见区间，其余项**不构建、不绘制**。这是"一万行列表"和"一万行布局 +
//  一万次绘制"的区别：后者的成本随数据量线性增长，前者与数据量无关。
// -----------------------------------------------------------------------------
float Ui::scroll_visible(const Rect& box, int count, float item_h, int* first, int* last) {
    if (first) *first = 0;
    if (last)  *last  = -1;
    if (count < 0) count = 0;

    const uint64_t id = impl->widget_id("scroll", box);
    const float content_h = float(count) * item_h;
    const float max_off   = std::max(0.0f, content_h - box.h);

    float& off = impl->flags[id];
    const Input& kin = impl->input;
    // 同上：测量模式不吃滚轮（否则测一遍 + 画一遍 = 滚两格）。
    if (!impl->measure_on && max_off > 0.0f && box.contains(kin.mouse.x, kin.mouse.y)
        && !impl->gate_blocked_pointer()
        && std::fabs(kin.wheel) > 0.001f) {
        off = detail::clampf(off - kin.wheel * 44.0f, 0.0f, max_off);
        repaint();
    }
    off = detail::clampf(off, 0.0f, max_off);
    if (count == 0 || item_h <= 0.01f) return off;

    int i0 = int(std::floor(off / item_h));
    int i1 = int(std::ceil((off + box.h) / item_h)) - 1;
    i0 = detail::clampi(i0, 0, count - 1);
    i1 = detail::clampi(i1, 0, count - 1);
    if (first) *first = i0;
    if (last)  *last  = i1;
    return off;
}

void Ui::scroll_bar(const Rect& box, float content_h) {
    const float max_off = std::max(0.0f, content_h - box.h);
    if (max_off <= 0.0f) return;
    const detail::ScrollbarDraw sb =
        detail::scrollbar_draw(*this, impl->widget_id("scroll", box), box, max_off);
    if (sb.need_frame) repaint();
}

float Ui::scroll_offset(const Rect& box) const {
    const float* off = impl->flags.get_if(impl->widget_id("scroll", box));
    return off ? *off : 0.0f;
}

void Ui::scroll_to_index(const Rect& box, int index, float item_h, int count) {
    const float max_off = std::max(0.0f, float(std::max(count, 0)) * item_h - box.h);
    const float target  = detail::clampf(float(std::max(index, 0)) * item_h, 0.0f, max_off);
    float& off = impl->flags[impl->widget_id("scroll", box)];
    if (off != target) { off = target; repaint(); }
}

// =============================================================================
//  8.17  Window 内部状态
// =============================================================================
namespace detail {
// 后台通知只接触事件/管道，不调用窗口系统。pending 保留 drain 期间到达的请求。
class WakeSignal {
public:
    WakeSignal() {
#ifdef AINO_PLATFORM_WINDOWS
        event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
#else
        if (::pipe(pipe_) == 0) {
            for (int fd : pipe_) {
                const int flags = fcntl(fd, F_GETFL);
                const int descriptor = fcntl(fd, F_GETFD);
                if (flags < 0 || descriptor < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ||
                    fcntl(fd, F_SETFD, descriptor | FD_CLOEXEC) < 0) {
                    ::close(pipe_[0]); ::close(pipe_[1]);
                    pipe_[0] = pipe_[1] = -1;
                    break;
                }
            }
        }
#endif
    }
    ~WakeSignal() {
#ifdef AINO_PLATFORM_WINDOWS
        if (event_) CloseHandle(event_);
#else
        for (int fd : pipe_) if (fd >= 0) ::close(fd);
#endif
    }
    WakeSignal(const WakeSignal&) = delete;
    WakeSignal& operator=(const WakeSignal&) = delete;
    bool valid() const {
#ifdef AINO_PLATFORM_WINDOWS
        return event_ != nullptr;
#else
        return pipe_[0] >= 0;
#endif
    }
    bool pending() const { return pending_.load(); }
    void notify() {
        if (pending_.exchange(true)) return;
#ifdef AINO_PLATFORM_WINDOWS
        if (event_) SetEvent(event_);
#else
        if (pipe_[1] >= 0) {
            const char byte = 1;
            while (::write(pipe_[1], &byte, 1) < 0 && errno == EINTR) {}
        }
#endif
    }
    bool consume() {
        const bool requested = pending_.exchange(false);
#ifdef AINO_PLATFORM_WINDOWS
        if (event_) WaitForSingleObject(event_, 0);
#else
        char bytes[128];
        if (pipe_[0] >= 0) {
            while (true) {
                const auto n = ::read(pipe_[0], bytes, sizeof bytes);
                if (n > 0 || (n < 0 && errno == EINTR)) continue;
                break;
            }
        }
#endif
        return requested;
    }
    void wait(int timeout_ms = -1, int native_fd = -1) {
        if (pending_.load()) return;
#ifdef AINO_PLATFORM_WINDOWS
        (void)native_fd;
        const DWORD timeout = timeout_ms < 0 ? INFINITE : DWORD(timeout_ms);
        MsgWaitForMultipleObjects(event_ ? 1 : 0, event_ ? &event_ : nullptr,
                                  FALSE, timeout, QS_ALLINPUT);
#else
        pollfd fds[2] = {{pipe_[0], POLLIN, 0}, {native_fd, POLLIN, 0}};
        ::poll(fds, 2, timeout_ms);
#endif
    }
#if defined(AINO_PLATFORM_WINDOWS)
    HANDLE wait_handle() const { return event_; }
#else
    int read_descriptor() const { return pipe_[0]; }
#endif
private:
    std::atomic<bool> pending_{false};
#ifdef AINO_PLATFORM_WINDOWS
    HANDLE event_ = nullptr;
#else
    int pipe_[2] = {-1, -1};
#endif
};
} // namespace detail

namespace detail {
inline std::vector<Window*>& scheduled_windows() {
    thread_local std::vector<Window*> windows;
    return windows;
}
} // namespace detail

struct Window::Impl {
    Ui ui;
    Theme theme = Theme::light();
    // 创建参数。后端（WM_GETMINMAXINFO / XSizeHints / setMinSize:）要回头读它。
    WindowConfig cfg;
    // 最小 / 最大尺寸的**物理像素**版本，由后端在创建时算好（含边框）。
    // WM_GETMINMAXINFO 里只有它、没有逻辑坐标换算的余量可算。
    int min_pw = 0, min_ph = 0, max_pw = 0, max_ph = 0;
    std::atomic<bool> running{true};
    detail::WakeSignal wake_signal;
    bool need_frame = true;
    // 上一帧的时刻（毫秒）。每窗口独立 —— 见 render() 里的说明。
    uint64_t last_frame_ms = 0;
    std::chrono::steady_clock::time_point next_frame_at{};
    // 上一帧的"再要一帧"是组件自己提的（spinner / 光标闪烁 / hover 插值），
    // 而不是用户输入触发的。主循环靠它决定要不要按 60fps 节流：
    // 区分不出来的话，只要有一个连续动画在跑，need_frame 就恒为真 →
    // 主循环退化成满速重绘（实测空闲 CPU 98%）。
    bool self_repaint = false;
    // 连续动画的重绘预算（毫秒/帧）。默认 16ms ≈ 60fps；调大可显著降低
    // spinner / 光标闪烁这类常驻动画的 CPU（代价是动画帧率下降）。
    uint32_t anim_budget_ms = 16;
    // 上一帧是不是"只因为指针移动才要重画"的。这类帧单独按 move_budget_ms
    // 合并；拖动（active != 0）、动画、点击、键盘、重绘请求都不走这个预算。
    //
    // 现在指针移动本身已经按"悬停目标变没变"过滤过了（见 Impl::hover_would_change），
    // 能被排出来的帧必然真的会改变画面，所以默认不再额外降频（16ms 与动画同一档）。
    // 留这个字段是给"极低功耗"场景用的：调到 33/50 就是 30/20fps 的悬停反馈。
    bool move_only = false;
    uint32_t move_budget_ms = 16;
    // 关掉"同一控件内移动不重绘"的优化。只有一种应用需要：
    // 帧回调里画了跟着指针跑的东西（自定义准星、拖影），又不经过任何控件。
    bool hover_skip = true;
    // AINO_FRAME_LOG=1 时把每帧渲染耗时打到 stderr，用来定位"谁在吃 CPU"。
    // 值是文件路径时写文件 —— WIN32 子系统的程序没有控制台，stderr 是无效句柄，
    // 重定向到文件也拿不到任何输出（实测：0 字节），而示例程序全是 WIN32 子系统。
    bool frame_log = false;
    struct FileCloser { void operator()(std::FILE* file) const { std::fclose(file); } };
    std::unique_ptr<std::FILE, FileCloser> frame_log_out;
    uint32_t frame_log_n = 0;
    uint64_t frame_log_sum = 0;     // µs
    uint32_t frame_log_worst = 0;   // µs
    uint64_t t_feed = 0, t_cb = 0, t_end = 0, t_present = 0;   // 分段累计 µs
    float scale = 1.0f;
    int client_w = 0, client_h = 0;     // 物理像素
    Canvas canvas;
    TextRenderer* text = nullptr;
    Cursor cursor = Cursor::Arrow;
    void* native = nullptr;
    std::function<void()> frame_cb_ptr;   // run() 期间指向用户回调，render() 用
    bool close_reported = false;         // 子窗口关闭时只通知其它窗口一次。
    std::function<void(std::string)> clipboard_complete;
    uint64_t clipboard_serial = 0;
    std::chrono::steady_clock::time_point clipboard_deadline;
    int clipboard_fd = -1;
    std::string clipboard_data;
    void finish_clipboard(std::string text) {
#ifndef AINO_PLATFORM_WINDOWS
        if (clipboard_fd >= 0) ::close(clipboard_fd);
#endif
        clipboard_fd = -1; clipboard_data.clear();
        auto callback = std::move(clipboard_complete); clipboard_complete = {};
        need_frame = true;
        if (callback) callback(std::move(text));
    }
    // run() 期间指向 Window::render()。窗口过程里要"同步补一帧"时用它 ——
    // 拖边框缩放时主循环被 user32 的模态循环卡住，根本轮不到 run() 出帧。
    std::function<void()> render_fn;

    ~Impl() {
#ifndef AINO_PLATFORM_WINDOWS
        if (clipboard_fd >= 0) ::close(clipboard_fd);
#endif
        if (text) { text->release(); delete text; text = nullptr; }
    }
};

namespace detail { inline void service_clipboard(Window::Impl*); }

#ifdef AINO_PLATFORM_WINDOWS
namespace detail {
struct Win32State;   // 定义在下面（win_proc 之前）—— 构造函数只用它的指针，声明就够
inline void win_set_ime_pos(Win32State* s, float caret_x, float top, float bottom);
} // namespace detail
#elif defined(AINO_PLATFORM_X11)
namespace detail {
struct X11State;
inline void x11_set_ime_pos(X11State* s, float caret_x, float top, float bottom);
} // namespace detail
#elif defined(AINO_PLATFORM_WAYLAND)
namespace detail {
struct WaylandState;
inline void wayland_set_ime_pos(WaylandState* s, float x, float top, float bottom);
} // namespace detail
#endif

Window::Window(const WindowConfig& cfg) : impl(new Impl()) {
    if (!impl->wake_signal.valid()) return;
    impl->cfg = cfg;
    impl->theme = Theme::light();
    // 字体家族名先进主题，再交给后端建文本渲染器 —— 渲染器和主题必须用同一个
    // 名字，否则"量出来的宽度"和"画出来的字形"会来自两套字体。
    if (!cfg.font_family.empty())      impl->theme.font_family = cfg.font_family;
    if (!cfg.font_family_mono.empty()) impl->theme.font_family_mono = cfg.font_family_mono;
    // 给了字体文件就以文件为准：从 `name` 表里读出家族名再排版。
    // 读不出来（文件不是字体 / 打不开）就算了，保留 font_family。
    if (!cfg.font_path.empty()) {
        const std::string fam = detail::font_family_from_file(cfg.font_path);
        if (!fam.empty()) impl->theme.font_family = fam;
    }
    impl->ui.set_theme(impl->theme);
    // 让 Ui 的 repaint() 能叫醒主循环（同时标记"这是组件自请求的帧"）
    impl->ui.impl->request_repaint = [this]() {
        impl->need_frame = true;
        impl->self_repaint = true;
    };
#ifdef AINO_PLATFORM_WINDOWS
    // 输入框有焦点时把 IME 的组合窗 / 候选窗钉到光标处。
    // 不绑的话系统会把它们丢在客户区左上角 —— 中文输入时候选框飘得老远。
    impl->ui.impl->ime_pos = [this](float x, float top, float bottom) {
        impl->ui.impl->ime_owner = true;   // 本帧有输入框接管 IME（组字串自己画）
        detail::win_set_ime_pos(reinterpret_cast<detail::Win32State*>(impl->native), x, top, bottom);
    };
#elif defined(AINO_PLATFORM_X11)
    impl->ui.impl->ime_pos = [this](float x, float top, float bottom) {
        impl->ui.impl->ime_owner = true;
        detail::x11_set_ime_pos(static_cast<detail::X11State*>(impl->native), x, top, bottom);
    };
#elif defined(AINO_PLATFORM_WAYLAND)
    impl->ui.impl->ime_pos = [this](float x, float top, float bottom) {
        impl->ui.impl->ime_owner = true;
        detail::wayland_set_ime_pos(static_cast<detail::WaylandState*>(impl->native), x, top, bottom);
    };
#endif
    // 剪贴板：input / textarea 的 Ctrl+C / X / V 从这里读写系统剪贴板。
    // 这两个回调原先**忘了绑** —— 结果是按 Ctrl+C 时调用一个空的 std::function，
    // 抛 std::bad_function_call、没人接、std::terminate，进程当场没了（用户看到的
    // 就是"复制一下程序自己关了"）。Ctrl+V 因为调用点写了判空所以不崩，只是没反应。
    // 现在两头都补：这里绑上真实实现，调用点也一律判空（Ui 可以脱离 Window 用）。
    impl->ui.impl->try_set_clipboard = [this](const std::string& t) { return Window::try_set_clipboard(t); };
    impl->ui.impl->get_clipboard_async = [this](auto callback) { request_clipboard(std::move(callback)); };
    // 诊断开关：AINO_FRAME_LOG=1 时每 120 帧打印一次渲染耗时。
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4996)   // getenv 在 MSVC 下被标记为不安全，这里只读环境变量
#endif
    {   const char* fl = std::getenv("AINO_FRAME_LOG");
        if (fl && fl[0] && fl[0] != '0') {
            impl->frame_log = true;
            if (!(fl[0] == '1' && fl[1] == 0))          // 不是 "1" 就当输出路径
                impl->frame_log_out.reset(std::fopen(fl, "w"));
        } }
    {   const char* pf = std::getenv("AINO_PROF");
        if (pf && pf[0] && pf[0] != '0') {
            impl->ui.impl->prof_on = true;              // 分项计时，帧日志里多打一行
            impl->frame_log = true;
            if (!(pf[0] == '1' && pf[1] == 0)) impl->frame_log_out.reset(std::fopen(pf, "w"));
        } }
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#ifdef AINO_PLATFORM_WINDOWS
    detail::w32_create(this, impl->cfg);
#elif defined(AINO_PLATFORM_X11)
    detail::x11_create(this, impl->cfg);
#elif defined(AINO_PLATFORM_WAYLAND)
    detail::wayland_create(this, impl->cfg);
#elif defined(AINO_PLATFORM_MACOS)
    detail::cocoa_create(this, impl->cfg);
#endif
}

Window::~Window() {
    auto& windows = detail::scheduled_windows();
    windows.erase(std::remove(windows.begin(), windows.end(), this), windows.end());
#ifdef AINO_PLATFORM_WINDOWS
    detail::w32_destroy(this);
#elif defined(AINO_PLATFORM_X11)
    detail::x11_destroy(this);
#elif defined(AINO_PLATFORM_WAYLAND)
    detail::wayland_destroy(this);
#elif defined(AINO_PLATFORM_MACOS)
    detail::cocoa_destroy(this);
#endif
    delete impl;
    impl = nullptr;
}

bool  Window::ok() const { return impl && impl->native != nullptr; }
void Window::render(const std::function<void(Ui&)>& frame) {
    if (!ok() || impl->ui.impl->frame_open) return;
    // 回调只借用到本次同步绘制结束；异常退出也恢复原主循环回调。
    struct Restore {
        std::function<void()>& target;
        std::function<void()> previous;
        ~Restore() { target = std::move(previous); }
    } restore{impl->frame_cb_ptr, std::move(impl->frame_cb_ptr)};
    impl->frame_cb_ptr = [&] { frame(impl->ui); };
    render();
}
Ui&   Window::ui() { return impl->ui; }
void  Window::set_theme(const Theme& t) {
    // 与"渲染实际在用的那份主题"比较（不是 Window 自己的副本，否则用户直接改
    // ui.set_theme() 之后这里会误判成"没变"）。
    // 少了这一步，在帧回调里每帧调用 set_theme 会让 need_frame 永远为真，
    // 主循环退化成忙等（实测 ~960fps、空闲吃满一个核）。
    if (impl->ui.impl->theme.same_as(t)) { impl->theme = t; return; }
    impl->theme = t;
    impl->ui.set_theme(t);
    impl->need_frame = true;
}
#ifndef AINO_PLATFORM_WAYLAND
int   Window::width() const { return int(float(impl->client_w) / impl->scale + 0.5f); }
int   Window::height() const { return int(float(impl->client_h) / impl->scale + 0.5f); }
#endif
float Window::dpi_scale() const { return impl->scale; }
bool  Window::should_close() const { return !impl->running; }
void  Window::request_repaint() {
    if (!impl->ui.impl->frame_open) impl->need_frame = true;
}

void  Window::set_anim_fps(float fps) {
    if (fps < 1.0f) fps = 1.0f;
    if (fps > 240.0f) fps = 240.0f;
    impl->anim_budget_ms = uint32_t(1000.0f / fps + 0.5f);
    if (impl->anim_budget_ms < 1) impl->anim_budget_ms = 1;
    impl->move_budget_ms = impl->anim_budget_ms;
}

void  Window::set_move_fps(float fps) {
    if (fps < 1.0f) fps = 1.0f;
    if (fps > 240.0f) fps = 240.0f;
    impl->move_budget_ms = uint32_t(1000.0f / fps + 0.5f);
    if (impl->move_budget_ms < 1) impl->move_budget_ms = 1;
}

void  Window::set_hover_skip(bool on) { impl->hover_skip = on; }

// 调用方须保证窗口存活；后台线程必须在 Window 析构前结束。
void Window::wake() { if (impl) impl->wake_signal.notify(); }
void Window::close() {
    if (!impl) return;
    impl->running.store(false);
    impl->wake_signal.notify();
}

#ifdef AINO_PLATFORM_WINDOWS
// =============================================================================
//  8.18  Windows 后端
// =============================================================================
namespace detail {

// 从 DLL 取函数指针。直接 reinterpret_cast<Fn>(GetProcAddress(...)) 在
// -Wextra 下会报 -Wcast-function-type（FARPROC 的签名与目标类型不兼容），
// 而那是动态加载的常规写法。经 void* 中转一次，语义不变、告警消灭。
// 两条 static_assert 保证平台上的指针尺寸一致，不会在奇怪的 ABI 上悄悄出错。
template <class Fn>
inline Fn dll_proc(HMODULE module, const char* name) {
    static_assert(sizeof(Fn) == sizeof(FARPROC), "function pointer size mismatch");
    FARPROC raw = GetProcAddress(module, name);
    Fn fn = nullptr;
    std::memcpy(&fn, &raw, sizeof(fn));
    return fn;
}

// 显式用宽字符版资源 ID —— IDC_* 会随 UNICODE 宏在 A/W 之间切换，
// 而我们要的是一个不依赖调用方编译开关的库
#ifndef AINO_IDC_ARROW
#  define AINO_IDC_ARROW   MAKEINTRESOURCEW(32512)
#  define AINO_IDC_IBEAM   MAKEINTRESOURCEW(32513)
#  define AINO_IDC_HAND    MAKEINTRESOURCEW(32649)
#  define AINO_IDC_SIZEWE  MAKEINTRESOURCEW(32644)
#  define AINO_IDC_SIZENS  MAKEINTRESOURCEW(32645)
#endif

struct Win32State {
    Window::Impl* owner = nullptr;
    HWND hwnd = nullptr;
    HICON icon_big = nullptr, icon_small = nullptr;
    std::wstring icon_path;
    HDC mem_dc = nullptr;
    HBITMAP hbmp = nullptr;
    void* bits = nullptr;
    int pw = 0, ph = 0;
    bool tracking = false;
    bool has_backbuffer = false;
    int caption_theme = -1;
    // 后备缓冲里装的是**当前尺寸下渲染过的**画面吗？
    // 重建缓冲（resize）后立刻变 false，渲染一帧后变 true。
    // 铁律：bb_valid 为 false 时**绝不允许 BitBlt 上屏** —— 那块新建的 DIB 全是 0，
    // 贴上去就是一片黑（这就是拖边框时的黑闪）。
    bool bb_valid = false;
    // 组字期间的低频计时器（见 WM_TIMER）。0 = 没开。
    UINT_PTR ime_timer = 0;
    // 系统那个浮动组合窗，以及它的高度（物理像素）。
    // 为什么要高度：候选窗是**挂在组合窗底下**的 —— 输入法自己算，我们的
    // ImmSetCandidateWindow 说了不算。所以想让候选窗贴着输入框，就得把组合窗
    // 整体上移它自己的高度，让它的**底边**落在框底（反正它已经被我们涂透明了）。
    HWND ime_ghost = nullptr;
    int  ime_ghost_h = 0;

    // 待消费的输入
    float mx = 0.0f, my = 0.0f;
    bool  inside = false;
    bool  btn_down[3]    = {false, false, false};
    bool  btn_pressed[3] = {false, false, false};
    bool  btn_released[3]= {false, false, false};
    float wheel = 0.0f;
    int   clicks = 0;
    std::string text_in;
    bool  key_down[KEY_COUNT]    = {};
    bool  key_pressed[KEY_COUNT] = {};
    unsigned char key_repeat[KEY_COUNT] = {};
    bool  key_ctrl[KEY_COUNT]    = {};
    bool  key_shift[KEY_COUNT]   = {};
    bool  key_alt[KEY_COUNT]     = {};
    uint32_t high_surrogate = 0;
};

inline void win_sync_caption_theme(Win32State* state, bool dark) {
    if (state->caption_theme == int(dark)) return;
    // 系统标题栏不在客户区画布中；主题变化时单独通知 DWM，避免深色窗口仍有浅色顶栏。
    using SetAttribute = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
    static SetAttribute set_attribute = []() -> SetAttribute {
        // 函数指针在进程存续期间复用，因此系统模块也保留至进程退出。
        HMODULE module = LoadLibraryW(L"dwmapi.dll");
        return module ? dll_proc<SetAttribute>(module, "DwmSetWindowAttribute") : nullptr;
    }();
    if (set_attribute) {
        const BOOL enabled = dark ? TRUE : FALSE;
        if (FAILED(set_attribute(state->hwnd, 20, &enabled, sizeof(enabled))))
            set_attribute(state->hwnd, 19, &enabled, sizeof(enabled));
    }
    state->caption_theme = int(dark);
}

inline UINT win_dpi_of(HWND hwnd) {
    typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND);
    static PFN_GetDpiForWindow fn = nullptr;
    static bool tried = false;
    if (!tried) {
        tried = true;
        HMODULE u = GetModuleHandleW(L"user32.dll");
        if (u) fn = dll_proc<PFN_GetDpiForWindow>(u, "GetDpiForWindow");
    }
    if (fn && hwnd) { UINT d = fn(hwnd); if (d) return d; }
    HDC dc = GetDC(nullptr);
    UINT d = UINT(GetDeviceCaps(dc, LOGPIXELSX));
    ReleaseDC(nullptr, dc);
    return d ? d : 96;
}

inline void win_update_icons(Win32State* state, UINT dpi) {
    using MetricsForDpi = int (WINAPI*)(int, UINT);
    static const auto metrics = dll_proc<MetricsForDpi>(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi");
    const auto size = [&](int index) {
        return metrics ? metrics(index, dpi)
                       : MulDiv(GetSystemMetrics(index), int(dpi), int(win_dpi_of(nullptr)));
    };
    const bool from_file = !state->icon_path.empty();
    const auto instance = from_file ? nullptr : GetModuleHandleW(nullptr);
    const auto source = from_file ? state->icon_path.c_str() : MAKEINTRESOURCEW(1);
    const UINT flags = from_file ? LR_LOADFROMFILE : 0;
    // 每个窗口拥有匹配自身 DPI 的图标；不使用共享类图标的尺寸与所有权。
    HICON big_icon = reinterpret_cast<HICON>(LoadImageW(instance, source, IMAGE_ICON,
        size(SM_CXICON), size(SM_CYICON), flags));
    HICON small_icon = reinterpret_cast<HICON>(LoadImageW(instance, source, IMAGE_ICON,
        size(SM_CXSMICON), size(SM_CYSMICON), flags));
    if (big_icon) {
        SendMessageW(state->hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big_icon));
        if (state->icon_big) DestroyIcon(state->icon_big);
        state->icon_big = big_icon;
    }
    if (small_icon) {
        SendMessageW(state->hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small_icon));
        if (state->icon_small) DestroyIcon(state->icon_small);
        state->icon_small = small_icon;
    }
    if (from_file && (!big_icon || !small_icon))
        std::fprintf(stderr, "[aino] Cannot load icon at %u DPI\n", dpi);
}

inline void win_enable_dpi_awareness() {
    typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(void*);
    static bool done = false;
    if (done) return;
    done = true;
    HMODULE u = GetModuleHandleW(L"user32.dll");
    if (u) {
        auto fn = dll_proc<PFN_SetProcessDpiAwarenessContext>(u, "SetProcessDpiAwarenessContext");
        if (fn && fn(reinterpret_cast<void*>(static_cast<intptr_t>(-4)))) return;   // PER_MONITOR_AWARE_V2
    }
    SetProcessDPIAware();
}

inline int win_map_vk(WPARAM vk) {
    if (vk >= 'A' && vk <= 'Z') return int(vk);
    if (vk >= '0' && vk <= '9') return int(vk);
    switch (vk) {
        case VK_BACK:   return KEY_BACKSPACE;
        case VK_TAB:    return KEY_TAB;
        case VK_RETURN: return KEY_ENTER;
        case VK_ESCAPE: return KEY_ESC;
        case VK_SPACE:  return KEY_SPACE;
        case VK_DELETE: return KEY_DELETE;
        case VK_LEFT:   return KEY_LEFT;
        case VK_RIGHT:  return KEY_RIGHT;
        case VK_UP:     return KEY_UP;
        case VK_DOWN:   return KEY_DOWN;
        case VK_HOME:   return KEY_HOME;
        case VK_END:    return KEY_END;
        case VK_PRIOR:  return KEY_PAGEUP;
        case VK_NEXT:   return KEY_PAGEDOWN;
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: return KEY_SHIFT;
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return KEY_CTRL;
        case VK_MENU: case VK_LMENU: case VK_RMENU: return KEY_ALT;
        case VK_LWIN: case VK_RWIN: return KEY_SUPER;
        case VK_F1: return KEY_F1; case VK_F2: return KEY_F2; case VK_F3: return KEY_F3;
        case VK_F4: return KEY_F4; case VK_F5: return KEY_F5; case VK_F6: return KEY_F6;
        case VK_F7: return KEY_F7; case VK_F8: return KEY_F8; case VK_F9: return KEY_F9;
        case VK_F10: return KEY_F10; case VK_F11: return KEY_F11; case VK_F12: return KEY_F12;
        default: return 0;
    }
}

// pending_high 挂在窗口状态上，不是函数级 static：后者是**进程级**共享的，
// 两个窗口交替收字符时，A 窗口留下的半个代理对会被 B 窗口的下一个字符拼走。
inline void win_append_utf16(std::string& out, uint32_t& pending_high, uint32_t unit) {
    // 处理代理对，否则 emoji / 扩展汉字会被切成两个非法码点
    if (unit >= 0xD800 && unit <= 0xDBFF) { pending_high = unit; return; }
    uint32_t cp = unit;
    if (unit >= 0xDC00 && unit <= 0xDFFF && pending_high) {
        cp = 0x10000 + ((pending_high - 0xD800) << 10) + (unit - 0xDC00);
        pending_high = 0;
    }
    char buf[8];
    int n = utf8_encode(cp, buf);
    out.append(buf, size_t(n));
}

inline void win_create_backbuffer(Win32State* s, int w, int h) {
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384 || uint64_t(w) * uint64_t(h) > 16777216u) return;
    if (s->pw == w && s->ph == h && s->has_backbuffer) return;

    HDC screen = GetDC(nullptr);
    if (!screen) return;
    if (!s->mem_dc) s->mem_dc = CreateCompatibleDC(screen);
    if (!s->mem_dc) { ReleaseDC(nullptr, screen); return; }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;          // top-down，跟我们的行序一致
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP hb = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (!hb || !bits) { if (hb) DeleteObject(hb); return; }

    // 换上新的，旧的被换出后才允许删除
    HBITMAP old = reinterpret_cast<HBITMAP>(SelectObject(s->mem_dc, hb));
    if (!old || old == HGDI_ERROR) { DeleteObject(hb); return; }
    if (old && old == s->hbmp) DeleteObject(old);
    s->hbmp = hb;
    SetBkMode(s->mem_dc, TRANSPARENT);

    s->bits = bits;
    s->pw = w; s->ph = h;
    s->has_backbuffer = true;
    s->bb_valid = false;      // 新缓冲是零填充的，没渲染过之前不许上屏

    Window::Impl* o = s->owner;
    o->canvas.px     = reinterpret_cast<uint8_t*>(bits);
    o->canvas.w      = w;
    o->canvas.h      = h;
    o->canvas.stride = w * 4;
    o->canvas.platform_dc = s->mem_dc;
    o->canvas.clear_clip();
    o->client_w = w;
    o->client_h = h;

    // run() 之前（窗口刚建出来、还没绑定 render_fn）没有任何人会马上渲染它，
    // 这时先把缓冲铺成主题底色，免得首帧出现前露一块黑。
    if (!o->render_fn) {
        const Color& bg = o->theme.window_bg;
        uint8_t* p = reinterpret_cast<uint8_t*>(bits);
        const size_t n = size_t(w) * size_t(h);
        for (size_t i = 0; i < n; ++i, p += 4) {
            p[detail::CH_R] = bg.r; p[detail::CH_G] = bg.g;
            p[detail::CH_B] = bg.b; p[detail::CH_A] = 255;
        }
        s->bb_valid = true;   // 至少是"对的颜色"，可以贴
    }
}

inline void win_present(Win32State* s) {
    if (!s->has_backbuffer || !s->hwnd) return;
    HDC dc = GetDC(s->hwnd);
    BitBlt(dc, 0, 0, s->pw, s->ph, s->mem_dc, 0, 0, SRCCOPY);
    ReleaseDC(s->hwnd, dc);
}

// 交互缩放进入 user32 模态循环时，主循环暂不执行。
// 在消息处理路径同步渲染，确保 WM_SIZE 重建的后备缓冲在 WM_PAINT 前已有内容。
inline void win_render_sync(Win32State* s) {
    if (!s || !s->has_backbuffer) return;
    if (!s->owner || !s->owner->render_fn) return;   // run() 之前没绑，跳过
    s->owner->render_fn();                           // 内部会 win_present
}

// -----------------------------------------------------------------------------
//  输入法（IME）定位
// -----------------------------------------------------------------------------
// 窗口如果什么都不说，系统就把拼音串和候选框丢在客户区左上角（或者跟着鼠标）
// —— 于是中文输入的候选窗和那个正在编辑的输入框完全对不上。必须显式告诉 IME
// 该画在哪：ImmSetCompositionWindow 定组合串的原点，ImmSetCandidateWindow 定
// 候选框的左上角。
//
// 这里**不静态链接 imm32**，而是 LoadLibrary + GetProcAddress 取那四个入口，
// 为的是守住"只依赖 user32 + gdi32"这条（README 里写死的卖点）。imm32.dll 在
// 每个 Windows 上都有，取不到就当没有输入法支持，不影响其它功能。
//
// 结构体自己定义而不用 <imm.h>：COMPOSITIONFORM / CANDIDATEFORM 的布局是 ABI
// 固定的，抄一份比多引一个头文件更可控。
struct AinoCompositionFormW {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
};
struct AinoCandidateFormW {
    DWORD dwIndex;
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
};
constexpr DWORD AINO_CFS_POINT        = 0x0002;   // 组合窗左上角 = ptCurrentPos
constexpr DWORD AINO_CFS_CANDIDATEPOS = 0x0040;   // 候选窗左上角 = ptCurrentPos
// WM_IME_COMPOSITION 的 lParam 位
constexpr DWORD AINO_GCS_COMPSTR      = 0x0008;   // 还没上屏的组字串（拼音）
constexpr DWORD AINO_GCS_RESULTSTR    = 0x0800;   // 已经上屏的结果串
// 组字期间压输入法浮动组合窗用的线程计时器 id（见 Window::poll）。
// 只是个"想要的" id —— 线程计时器最终由系统分配，实际值存在 Win32State::ime_timer。
constexpr UINT_PTR AINO_IME_TIMER_ID = 0xA1;

// WM_IME_SETCONTEXT 的 lParam 位：让系统画那个悬空的组合窗
constexpr LPARAM AINO_ISC_SHOWUICOMPOSITIONWINDOW = LPARAM(0x80000000);

struct ImmApi {
    HIMC (WINAPI* get_ctx)   (HWND) = nullptr;
    BOOL (WINAPI* rel_ctx)   (HWND, HIMC) = nullptr;
    BOOL (WINAPI* set_comp)  (HIMC, const AinoCompositionFormW*) = nullptr;
    BOOL (WINAPI* set_cand)  (HIMC, const AinoCandidateFormW*) = nullptr;
    LONG (WINAPI* get_str)   (HIMC, DWORD, void*, DWORD) = nullptr;
    bool ok = false;
    ImmApi() {
        HMODULE h = LoadLibraryW(L"imm32.dll");
        if (!h) return;
        get_ctx  = dll_proc<HIMC (WINAPI*)(HWND)>(h, "ImmGetContext");
        rel_ctx  = dll_proc<BOOL (WINAPI*)(HWND, HIMC)>(h, "ImmReleaseContext");
        set_comp = dll_proc<BOOL (WINAPI*)(HIMC, const AinoCompositionFormW*)>(
                       h, "ImmSetCompositionWindow");
        set_cand = dll_proc<BOOL (WINAPI*)(HIMC, const AinoCandidateFormW*)>(
                       h, "ImmSetCandidateWindow");
        get_str  = dll_proc<LONG (WINAPI*)(HIMC, DWORD, void*, DWORD)>(h, "ImmGetCompositionStringW");
        ok = get_ctx && rel_ctx && set_comp && set_cand && get_str;
    }
};

// caret_x / top / bottom 都是**物理客户区**像素。
// 组合串贴着输入框下沿开始（不遮挡正在编辑的内容），候选框再往下一点。
inline void win_set_ime_pos(Win32State* s, float caret_x, float top, float bottom) {
    (void)top;
    if (!s || !s->hwnd) return;
    static ImmApi api;
    if (!api.ok) return;
    HIMC himc = api.get_ctx(s->hwnd);
    if (!himc) return;

    // 组合窗整体上移它自己的高度：让它的**底边**落在输入框底边上。
    // 候选窗是挂在组合窗底下由输入法自己摆的（ImmSetCandidateWindow 对它无效，
    // 实测报 (caret, bottom+4) 完全没人理），所以只报 bottom 的话，候选窗会被
    // 组合窗那 22px 顶下去 —— 用户看到的就是"候选窗离输入框那么远"。
    // 组合窗本身已经被涂成全透明了，挪上去不会有任何视觉副作用。
    const int ghost_h = s->ime_ghost_h;
    AinoCompositionFormW cf{};
    cf.dwStyle = AINO_CFS_POINT;
    cf.ptCurrentPos.x = LONG(caret_x);
    cf.ptCurrentPos.y = LONG(bottom - float(ghost_h));
    api.set_comp(himc, &cf);

    AinoCandidateFormW cd{};
    cd.dwIndex = 0;
    cd.dwStyle = AINO_CFS_CANDIDATEPOS;
    cd.ptCurrentPos.x = LONG(caret_x);
    cd.ptCurrentPos.y = LONG(bottom + 2.0f);
    api.set_cand(himc, &cd);

    api.rel_ctx(s->hwnd, himc);
}

// 微软拼音这类 TSF 输入法经 CUAS 转发时，会**自己在进程里**开一个
// "MSCTFIME Composition" 顶层窗口画组字串。我们已经把同一串画进输入框了，
// 两个一起出现就是重影 —— 用户截图里"框里框外各一份"就是这个。
// 这个窗口属于本进程（不一定在 UI 线程），按 pid + 类名找出来藏掉；
// 候选窗是另一个类（"MSCTFIME UI"），绝不能动 —— 那个我们画不了。
struct AinoImeHideCtx { DWORD pid; void* st; };

inline BOOL CALLBACK aino_hide_ime_comp_cb(HWND w, LPARAM lp) {
    DWORD p = 0;
    GetWindowThreadProcessId(w, &p);
    if (p != reinterpret_cast<AinoImeHideCtx*>(lp)->pid) return TRUE;
    wchar_t cls[64];
    if (!GetClassNameW(w, cls, 64)) return TRUE;
    if (lstrcmpW(cls, L"MSCTFIME Composition") != 0) return TRUE;

    // ① 先把它变成"全透明的分层窗"。这一步才是防闪烁的关键。
    //    SW_HIDE 是**状态**操作，可输入法在每次组字更新时会再 ShowWindow 一次 ——
    //    我们 20 ms 后才压回去，于是"亮一下、藏起来、再亮一下"。
    //    实测：空闲时它是干净的（240 帧零变化），**每个按键**都会露出一次
    //    （框下 11x15 的小白块），连打就是用户说的"一直闪烁"。
    //    分层 + alpha=0 之后窗口**照旧是可见的**（输入法不会再来抢显示），
    //    只是画不出任何像素；WS_EX_TRANSPARENT 让点击穿过去，别挡住输入框。
    //    对不可见的窗口也照做：提前上好，等它下次亮出来时已经是透明的。
    LONG_PTR ex = GetWindowLongPtrW(w, GWL_EXSTYLE);
    const LONG_PTR want = ex | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    if (want != ex) SetWindowLongPtrW(w, GWL_EXSTYLE, want);
    SetLayeredWindowAttributes(w, 0, 0, LWA_ALPHA);   // 幂等，重复调不心疼

    // ② 再藏一次当兜底：万一分层没生效（老主题／输入法重置样式），至少还挡得住。
    if (IsWindowVisible(w)) ShowWindow(w, SW_HIDE);
    // ③ 记下它和它的高度，给定位用（见 win_set_ime_pos：候选窗贴着它的底边）。
    if (auto* st = reinterpret_cast<Win32State*>(reinterpret_cast<AinoImeHideCtx*>(lp)->st))
        st->ime_ghost = w;
    RECT gr{};
    if (GetWindowRect(w, &gr) && gr.bottom > gr.top)
        if (auto* st = reinterpret_cast<Win32State*>(reinterpret_cast<AinoImeHideCtx*>(lp)->st))
            st->ime_ghost_h = int(gr.bottom - gr.top);
    return TRUE;
}

inline void win_hide_ime_comp_window(Win32State* s) {
    if (!s || !s->owner) return;
    if (!s->owner->ui.impl->ime_owner) return;              // 没输入框接管 → 别动系统那份
    if (s->owner->ui.impl->ime_comp.empty()) return;        // 没在组字 → 别动
    AinoImeHideCtx ctx{GetCurrentProcessId(), s};
    EnumWindows(aino_hide_ime_comp_cb, reinterpret_cast<LPARAM>(&ctx));
}

inline void win_apply_cursor(Win32State* s) {
    if (!s->owner) return;
    LPCWSTR name = AINO_IDC_ARROW;
    switch (s->owner->cursor) {
        case Cursor::Hand:    name = AINO_IDC_HAND;   break;
        case Cursor::IBeam:   name = AINO_IDC_IBEAM;  break;
        case Cursor::ResizeH: name = AINO_IDC_SIZEWE; break;
        case Cursor::ResizeV: name = AINO_IDC_SIZENS; break;
        default:              name = AINO_IDC_ARROW;  break;
    }
    SetCursor(LoadCursorW(nullptr, name));
}

inline LRESULT CALLBACK win_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    Win32State* s = nullptr;
    if (m == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(l);
        s = reinterpret_cast<Win32State*>(cs->lpCreateParams);
        s->hwnd = h;
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
    } else {
        s = reinterpret_cast<Win32State*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    }
    if (!s) return DefWindowProcW(h, m, w, l);

    switch (m) {
    case WM_ERASEBKGND:
        return 1;   // 我们自己整块重画，禁止系统擦背景（防闪烁）

    case WM_PRINTCLIENT: {
        // 打印/截图（PrintWindow）走的路径 —— 直接把后备缓冲贴过去，
        // 顺带也支持了系统的窗口缩略图预览
        HDC dc = reinterpret_cast<HDC>(w);
        if (s->has_backbuffer && dc) BitBlt(dc, 0, 0, s->pw, s->ph, s->mem_dc, 0, 0, SRCCOPY);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(h, &ps);
        // 缓冲尺寸失效时补绘一帧；有效缓冲在 WM_PAINT 中直接呈现，避免窗口暴露触发无效渲染。
        if (!s->bb_valid) win_render_sync(s);
        if (s->has_backbuffer && s->bb_valid) {
            HDC dc = ps.hdc;
            BitBlt(dc, 0, 0, s->pw, s->ph, s->mem_dc, 0, 0, SRCCOPY);
        }
        EndPaint(h, &ps);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        // WindowConfig 的 min/max 落地点。不在 CreateWindowEx 里挡：Win32 没有
        // "创建时就带上尺寸约束"的参数，只有这条消息 —— 拖边框、最大化、程序里
        // SetWindowPos 都会先问它。0 表示不限制。
        MINMAXINFO* mm = reinterpret_cast<MINMAXINFO*>(l);
        if (mm) {
            if (s->owner->min_pw > 0) mm->ptMinTrackSize.x = s->owner->min_pw;
            if (s->owner->min_ph > 0) mm->ptMinTrackSize.y = s->owner->min_ph;
            if (s->owner->max_pw > 0) mm->ptMaxTrackSize.x = s->owner->max_pw;
            if (s->owner->max_ph > 0) mm->ptMaxTrackSize.y = s->owner->max_ph;
        }
        return 0;
    }

    case WM_SIZE: {
        int nw = LOWORD(l), nh = HIWORD(l);
        win_create_backbuffer(s, nw, nh);
        // 尺寸真的变了（缓冲被重建）→ 立刻同步画一帧再把标志交给主循环。
        // 不这么做的话，这里到主循环出帧之间（拖动时是**永远**）屏幕上就是黑的。
        if (!s->bb_valid) {
            win_render_sync(s);
            s->owner->need_frame = false;   // 刚画完，不必再排一帧
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        s->mx = float(GET_X_LPARAM(l));
        s->my = float(GET_Y_LPARAM(l));
        s->inside = true;
        if (!s->tracking) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = h;
            TrackMouseEvent(&tme);
            s->tracking = true;
        }
        // 指针在同一个控件内部移动时，画面一个像素都不会变 —— 不排帧。
        // 只有"悬停目标可能变"才重绘（离开旧控件、或进入某个可交互矩形）。
        // 这样悬停反馈不再需要 30fps 的合并预算：扫过一排条目是满刷新率的，
        // 而单纯抖鼠标是 0 帧。自绘内容跟着指针跑的应用可以关掉这个优化
        // （Window::set_hover_skip(false)）。
        if (!s->owner->need_frame && s->owner->hover_skip &&
            !s->owner->ui.impl->hover_would_change(s->mx / s->owner->scale,
                                                   s->my / s->owner->scale)) {
            return 0;
        }
        s->owner->need_frame = true;
        s->owner->move_only = true;      // 只有指针移动触发（预算见 move_budget_ms）
        return 0;
    }

    case WM_MOUSELEAVE:
        s->tracking = false;
        s->inside = false;
        s->mx = -1e6f; s->my = -1e6f;
        s->owner->need_frame = true;
        return 0;

    case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: {
        int idx = (m == WM_LBUTTONDOWN) ? 0 : (m == WM_RBUTTONDOWN ? 1 : 2);
        s->mx = float(GET_X_LPARAM(l)); s->my = float(GET_Y_LPARAM(l));
        s->inside = true;
        s->btn_down[idx] = true; s->btn_pressed[idx] = true;
        SetCapture(h);
        s->owner->need_frame = true;
        return 0;
    }

    case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP: {
        int idx = (m == WM_LBUTTONUP) ? 0 : (m == WM_RBUTTONUP ? 1 : 2);
        s->mx = float(GET_X_LPARAM(l)); s->my = float(GET_Y_LPARAM(l));
        s->btn_down[idx] = false; s->btn_released[idx] = true;
        if (!s->btn_down[0] && !s->btn_down[1] && !s->btn_down[2]) ReleaseCapture();
        s->owner->need_frame = true;
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        s->mx = float(GET_X_LPARAM(l)); s->my = float(GET_Y_LPARAM(l));
        s->btn_down[0] = true; s->btn_pressed[0] = true;
        s->clicks = 2;
        SetCapture(h);
        s->owner->need_frame = true;
        return 0;
    }

    case WM_MOUSEWHEEL: {
        s->wheel += float(GET_WHEEL_DELTA_WPARAM(w)) / 120.0f;
        s->owner->need_frame = true;
        return 0;
    }

    // ---- IME 组字串（拼音）----
    // 只靠 WM_CHAR 的话，拼音全在系统那个悬空的组合窗里，输入框看着是空的
    // （"没有预先填充"）。这里把 GCS_COMPSTR 抓下来存进 ui->ime_comp，由
    // input / textarea 画在光标处。上屏字符仍然走 WM_CHAR —— 所以这几个 case
    // 一律 break（交给 DefWindowProc），绝不自己插入 RESULTSTR：两条路都插就是双份。
    case WM_IME_SETCONTEXT:
        // 组字串我们自己画在框里了，别让系统再画一个悬在下面。
        // 候选窗（ISC_SHOWUIFIRSTCANDIDATE 等）不动 —— 那个只能系统画。
        l &= ~AINO_ISC_SHOWUICOMPOSITIONWINDOW;
        break;

    case WM_IME_STARTCOMPOSITION:
        s->owner->ui.impl->ime_comp.clear();
        s->owner->need_frame = true;
        break;

    case WM_IME_COMPOSITION: {
        Ui::Impl* ui = s->owner->ui.impl;
        const bool commit = (w & AINO_GCS_RESULTSTR) != 0;
        if (commit) ui->ime_comp.clear();  // 上屏：草稿消失，正文随后由 WM_CHAR 送到

        // TSF 经 CUAS 转发时，消息可能不含 GCS_COMPSTR 位，但 IMM 仍有组字串。
        // 除纯上屏消息外都查询 IMM，避免漏掉预编辑内容。
        if (!commit) {
            static ImmApi api;
            // ⚠️ 别用 lParam 那个 HIMC。CUAS（TSF 输入法给老式窗口做的 IMM 转发层）
            // 塞过来的句柄调 ImmGetCompositionString 一律返回 0，可同一个串用
            // ImmGetContext(hwnd) 拿到的上下文读就有 —— 实测 w=0x73 那条消息里
            // lParam 是 0x1b8/0x1c00，读出来 0 字节；换成 get_ctx(hwnd) 立刻是 2/6/10/14。
            HIMC himc = api.ok ? api.get_ctx(s->hwnd) : nullptr;
            if (!himc) himc = reinterpret_cast<HIMC>(l);
            LONG bytes = himc ? api.get_str(himc, AINO_GCS_COMPSTR, nullptr, 0) : 0;
            if (bytes > 0) {
                std::wstring buf(size_t(bytes) / sizeof(wchar_t) + 1, L'\0');
                api.get_str(himc, AINO_GCS_COMPSTR, &buf[0], bytes);
                ui->ime_comp = from_wide(buf.c_str());
            } else {
                ui->ime_comp.clear();      // 删光了拼音 / 首次进入组字
            }
            if (himc && himc != reinterpret_cast<HIMC>(l)) api.rel_ctx(s->hwnd, himc);
        }
        s->owner->need_frame = true;
        break;
    }

    case WM_IME_ENDCOMPOSITION:
        s->owner->ui.impl->ime_comp.clear();
        s->owner->need_frame = true;
        break;

    case WM_CHAR: {
        uint32_t unit = uint32_t(w);
        if (unit >= 32 || unit == 9) win_append_utf16(s->text_in, s->high_surrogate, unit);
        s->owner->need_frame = true;
        return 0;
    }

    case WM_KEYDOWN: case WM_SYSKEYDOWN: {
        int k = win_map_vk(w);
        if (k > 0 && k < KEY_COUNT) {
            // lParam bit 30 标识自动重复；次数存入 key_repeat，由编辑器和终端按需使用。
            const bool os_repeat = (l & (1 << 30)) != 0;
            if (!s->key_down[k] || !os_repeat) {
                // 首按。条件里带 !os_repeat 是兜底：焦点切回来、或我们漏收了
                // KEYUP 时 key_down 可能是陈旧的 true，此时必须仍按"首按"派发，
                // 否则这一下按键会被静默吞掉。
                s->key_pressed[k] = true;
                // 修饰位在**按下这一刻**定格：同一批消息里 Ctrl 的 KEYUP 可能
                // 已经先被处理掉，帧里再看 key_down 就晚了。
                s->key_ctrl[k]  = s->key_down[KEY_CTRL];
                s->key_shift[k] = s->key_down[KEY_SHIFT];
                s->key_alt[k]   = s->key_down[KEY_ALT];
            } else {
                if (s->key_repeat[k] < 255) ++s->key_repeat[k];
            }
            s->key_down[k] = true;
        }
        s->owner->need_frame = true;
        return 0;
    }

    case WM_KEYUP: case WM_SYSKEYUP: {
        int k = win_map_vk(w);
        if (k > 0 && k < KEY_COUNT) s->key_down[k] = false;
        s->owner->need_frame = true;
        return 0;
    }

    case WM_CAPTURECHANGED: case WM_CANCELMODE:
        // 正常松手也会触发 WM_CAPTURECHANGED，必须保留 released 供本帧控件处理。
        if (m == WM_CANCELMODE || s->btn_down[0] || s->btn_down[1] || s->btn_down[2]) {
            for (int i = 0; i < 3; ++i) s->btn_down[i] = s->btn_pressed[i] = s->btn_released[i] = false;
            s->owner->ui.impl->active = 0;
            s->owner->need_frame = true;
            if (m == WM_CANCELMODE && GetCapture() == h) ReleaseCapture();
        }
        return 0;

    case WM_KILLFOCUS:
        for (int i = 0; i < 3; ++i) s->btn_down[i] = s->btn_pressed[i] = s->btn_released[i] = false;
        s->owner->ui.impl->active = 0;
        for (int i = 0; i < KEY_COUNT; ++i) s->key_down[i] = false;
        if (GetCapture() == h) ReleaseCapture();
        s->owner->need_frame = true;
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) { win_apply_cursor(s); return TRUE; }
        break;

    case WM_DPICHANGED: {
        s->owner->scale = float(HIWORD(w)) / 96.0f;
        win_update_icons(s, HIWORD(w));
        RECT* r = reinterpret_cast<RECT*>(l);
        SetWindowPos(h, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        s->owner->need_frame = true;
        return 0;
    }

    case WM_CLOSE:
        // WM_QUIT 属于整个线程；关闭本窗口不能结束同线程的其它窗口。
        s->owner->running = false;
        return 0;

    case WM_DESTROY:
        s->owner->running = false;
        return 0;

    default: break;
    }
    LRESULT dw = DefWindowProcW(h, m, w, l);
    // 隐藏操作必须在初始 ShowWindow 调用后执行。
    if (m == WM_IME_COMPOSITION || m == WM_IME_STARTCOMPOSITION)
        detail::win_hide_ime_comp_window(s);
    return dw;
}

inline void w32_create(Window* self, const WindowConfig& cfg) {
    Window::Impl* o = self->impl;
    const int w = cfg.width > 0 ? cfg.width : 880;
    const int h = cfg.height > 0 ? cfg.height : 560;
    // high_dpi=false 时**不声明**DPI 感知，让系统整体缩放（字会糊但尺寸可控）。
    // 这是进程级开关，第一个窗口说了算 —— 文档里写明。
    if (cfg.high_dpi) win_enable_dpi_awareness();

    Win32State* s = new Win32State();
    s->owner = o;
    s->icon_path = to_wide(cfg.icon_path);
    o->native = s;

    // 字体文件：先注册进本进程（FR_PRIVATE，不污染别的程序），再按解析出的
    // 家族名排版。注册失败不致命 —— 家族名解析不到时上面已经退回 font_family。
    if (!cfg.font_path.empty()) {
        const std::wstring wp = to_wide(cfg.font_path);
        if (AddFontResourceExW(wp.c_str(), FR_PRIVATE, nullptr) > 0)
            SendMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
    }

    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
        wc.lpfnWndProc   = win_proc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursorW(nullptr, AINO_IDC_ARROW);
        wc.hIcon         = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(1));
        wc.hIconSm       = reinterpret_cast<HICON>(LoadImageW(wc.hInstance, MAKEINTRESOURCEW(1),
                              IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
        wc.lpszClassName = L"AinoWindowClass";
        wc.hbrBackground = nullptr;
        RegisterClassExW(&wc);
        class_registered = true;
    }

    UINT sys_dpi = win_dpi_of(nullptr);
    o->scale = float(sys_dpi) / 96.0f;

    // 不可缩放：去掉厚边框与最大化按钮。WS_THICKFRAME 才是"能拖大小"的那个位，
    // 留着它光去掉 WS_MAXIMIZEBOX 照样能拖。
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!cfg.resizable) style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    RECT rc = {0, 0, LONG(float(w) * o->scale), LONG(float(h) * o->scale)};
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    const int win_w = rc.right - rc.left, win_h = rc.bottom - rc.top;

    // 最小 / 最大：换算成**窗口**尺寸（含边框）存起来，WM_GETMINMAXINFO 直接用。
    // 注意 AdjustWindowRectEx 对 min 是对称的：把客户区尺寸换成窗口尺寸即可。
    o->min_pw = o->min_ph = o->max_pw = o->max_ph = 0;
    if (cfg.min_width > 0 || cfg.min_height > 0) {
        RECT r2 = {0, 0, LONG(float(cfg.min_width) * o->scale), LONG(float(cfg.min_height) * o->scale)};
        AdjustWindowRectEx(&r2, style, FALSE, 0);
        o->min_pw = r2.right - r2.left;
        o->min_ph = r2.bottom - r2.top;
    }
    if (cfg.max_width > 0 || cfg.max_height > 0) {
        RECT r3 = {0, 0, LONG(float(cfg.max_width) * o->scale), LONG(float(cfg.max_height) * o->scale)};
        AdjustWindowRectEx(&r3, style, FALSE, 0);
        o->max_pw = r3.right - r3.left;
        o->max_ph = r3.bottom - r3.top;
    }

    int px = CW_USEDEFAULT, py = CW_USEDEFAULT;
    if (cfg.center) {
        // 居中到工作区（排除任务栏）。不用屏幕尺寸 —— 任务栏会盖住下边缘。
        RECT wa = {0, 0, 0, 0};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
        px = wa.left + ((wa.right - wa.left) - win_w) / 2;
        py = wa.top + ((wa.bottom - wa.top) - win_h) / 2;
        if (px < wa.left) px = wa.left;
        if (py < wa.top) py = wa.top;
    }

    std::wstring wtitle = to_wide(cfg.title.empty() ? std::string("aino") : cfg.title);
    HWND hwnd = CreateWindowExW(0, L"AinoWindowClass", wtitle.c_str(), style,
                                px, py, win_w, win_h,
                                nullptr, nullptr, GetModuleHandleW(nullptr), s);
    if (!hwnd) { delete s; o->native = nullptr; return; }

    o->scale = float(win_dpi_of(hwnd)) / 96.0f;

    // 内嵌资源与显式 .ico 都按当前窗口 DPI 分别设置大小图标。
    win_update_icons(s, win_dpi_of(hwnd));

    GdiTextRenderer* tr = new GdiTextRenderer();
    tr->init(o->scale, o->theme.font_family, o->theme.font_family_mono);
    o->text = tr;
    o->ui.impl->text = tr;

    RECT cr;
    GetClientRect(hwnd, &cr);
    win_create_backbuffer(s, cr.right - cr.left, cr.bottom - cr.top);
    o->ui.impl->canvas = &o->canvas;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    o->need_frame = true;
}

inline void w32_destroy(Window* self) {
    Window::Impl* o = self->impl;
    Win32State* s = reinterpret_cast<Win32State*>(o->native);
    if (!s) return;
    if (s->mem_dc) {
        // 顺序有讲究：DeleteObject 对"仍被选进 DC 的位图"会失败，而 DC 又要
        // 在没有位图选着的时候删。所以：
        //   换入 1×1 替身 → 删被换出的后备位图 → 删 DC（顺带释放替身）→ 删替身。
        // 旧写法只做了第一步和 DeleteDC，那个替身一直删不掉（每窗口漏一个 GDI 位图）。
        HBITMAP tmp = CreateCompatibleBitmap(s->mem_dc, 1, 1);
        if (tmp) {
            HBITMAP old = reinterpret_cast<HBITMAP>(SelectObject(s->mem_dc, tmp));
            if (old && old != tmp) {
                DeleteObject(old);
                if (s->hbmp == old) s->hbmp = nullptr;   // 已经删过，别再删第二次
            }
        }
        DeleteDC(s->mem_dc);
        s->mem_dc = nullptr;
        if (tmp) DeleteObject(tmp);
    }
    if (s->hbmp) { DeleteObject(s->hbmp); s->hbmp = nullptr; }
    if (s->hwnd && IsWindow(s->hwnd)) DestroyWindow(s->hwnd);
    if (s->icon_big) DestroyIcon(s->icon_big);
    if (s->icon_small) DestroyIcon(s->icon_small);
    delete s;
    o->native = nullptr;
}

// 把这一轮累积的 Win32 消息转成 aino::Input
inline void win_feed_input(Win32State* s) {
    Window::Impl* o = s->owner;
    Ui::Impl* u = o->ui.impl;
    Input& in = u->input;

    const Vec2 prev = in.mouse;
    in.mouse = Vec2(s->mx / o->scale, s->my / o->scale);
    in.mouse_prev = prev;
    in.mouse_delta = Vec2(in.mouse.x - prev.x, in.mouse.y - prev.y);

    for (int i = 0; i < 3; ++i) {
        in.pressed[i]  = s->btn_pressed[i];
        in.released[i] = s->btn_released[i];
        in.down[i]     = s->btn_down[i];
    }
    in.wheel = s->wheel;
    in.double_click = (s->clicks >= 2);
    in.click_count = s->clicks;
    in.text_input = s->text_in;
    for (int i = 0; i < KEY_COUNT; ++i) {
        in.key_down[i]    = s->key_down[i];
        in.key_pressed[i] = s->key_pressed[i];
        in.key_repeat[i]  = s->key_repeat[i];
        in.key_ctrl[i]    = s->key_ctrl[i];
        in.key_shift[i]   = s->key_shift[i];
        in.key_alt[i]     = s->key_alt[i];
    }
    in.dragging = s->btn_down[0];

    s->btn_pressed[0] = s->btn_pressed[1] = s->btn_pressed[2] = false;
    s->btn_released[0] = s->btn_released[1] = s->btn_released[2] = false;
    s->wheel = 0.0f;
    s->clicks = 0;
    s->text_in.clear();
    for (int i = 0; i < KEY_COUNT; ++i) { s->key_pressed[i] = false; s->key_repeat[i] = 0; }
}

inline uint64_t win_ticks_ms() {
    return uint64_t(GetTickCount64());
}

inline uint64_t win_ticks_us() {
    static LARGE_INTEGER freq = [] {
        LARGE_INTEGER f{}; QueryPerformanceFrequency(&f); return f;
    }();
    LARGE_INTEGER c{}; QueryPerformanceCounter(&c);
    return uint64_t(double(c.QuadPart) * 1e6 / double(freq.QuadPart));
}

// 高精度睡眠。
//
// Sleep 的实际等待时间受系统时钟粒度影响，不适合补足短帧预算。
//
// CreateWaitableTimerExW + HIGH_RESOLUTION（Win10 1803+）不需要 timeBeginPeriod，
// 精度约 1 ms，且**只影响本进程等的那一下**，不会让整机时钟中断变快（timeBeginPeriod
// 会，代价是待机耗电）。老系统创建失败就退回 Sleep。
struct WinPreciseTimer {
    HANDLE h = nullptr;
    WinPreciseTimer() {
        h = CreateWaitableTimerExW(nullptr, nullptr, 0x00000002 /*HIGH_RESOLUTION*/,
                                   TIMER_ALL_ACCESS);
        if (!h) h = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    }
    ~WinPreciseTimer() { if (h) CloseHandle(h); }
    HANDLE arm_ms(int milliseconds) {
        if (!h || milliseconds <= 0) return nullptr;
        LARGE_INTEGER due{}; due.QuadPart = -LONGLONG(milliseconds) * 10000;
        return SetWaitableTimer(h, &due, 0, nullptr, nullptr, FALSE) ? h : nullptr;
    }
};

inline WinPreciseTimer& win_timer() {
    // 每个窗口线程持有独立定时器，避免到期设置相互覆盖或信号被其他线程消费。
    thread_local WinPreciseTimer t;
    return t;
}

// 每 120 帧汇总一次渲染耗时（只在 AINO_FRAME_LOG=1 时被调用）。
inline void win_frame_log(Window::Impl* o, uint64_t render_us) {
    ++o->frame_log_n;
    o->frame_log_sum += render_us;
    if (render_us > o->frame_log_worst) o->frame_log_worst = uint32_t(render_us);
    // 60 帧一块：交互驱动的帧是零散的（拖着鼠标扫过一排控件才有几帧），
    // 攒到 120 帧往往要等很久，探针还没采到样进程就被关掉了。
    if (o->frame_log_n >= 60) {
        std::FILE* out = o->frame_log_out ? o->frame_log_out.get() : stderr;
        const double n = double(o->frame_log_n);
        std::fprintf(out,
            "[aino] 60 帧  渲染均 %.2f ms  最差 %.2f ms  (预算 %.0f ms)\n"
            "        取输入 %.2f | 绘制 %.2f | 收尾 %.2f | 上屏 %.2f  (ms)\n",
            double(o->frame_log_sum) / n / 1000.0, double(o->frame_log_worst) / 1000.0,
            double(o->anim_budget_ms),
            double(o->t_feed) / n / 1000.0, double(o->t_cb) / n / 1000.0,
            double(o->t_end) / n / 1000.0, double(o->t_present) / n / 1000.0);
        if (o->ui.impl->prof_on) {
            // 分项。注意 draw_text 内部会调 fill_rect（下划线、底色），所以 fill
            // 里含着一部分文字时间；看"谁是大头"足够，别当严格互斥的分解看。
            auto per = [&](uint64_t acc) { return double(acc) / n / 1000.0; };
            const double fpx = double(o->ui.impl->pf_fill_px) / n;
            const double tch = double(o->ui.impl->pf_text_ch) / n;
            const double fms = per(o->ui.impl->pf_fill_us);
            const double tms = per(o->ui.impl->pf_text_us);
            std::fprintf(out,
                "        填充 %.2f ms (%u 次, %.0f kpx, %.0f Mpx/s) | 描边 %.2f (%u) | "
                "文字 %.2f ms (%u 段 %.0f 字, %.2f µs/段) | 圆 %.2f (%u)\n",
                fms, o->ui.impl->pf_fill_n / unsigned(n), fpx / 1000.0,
                fms > 0.001 ? fpx / fms / 1000.0 : 0.0,
                per(o->ui.impl->pf_stroke_us), o->ui.impl->pf_stroke_n / unsigned(n),
                tms, o->ui.impl->pf_text_n / unsigned(n), tch,
                o->ui.impl->pf_text_n ? tms * 1000.0 / double(o->ui.impl->pf_text_n / unsigned(n)) : 0.0,
                per(o->ui.impl->pf_circle_us), o->ui.impl->pf_circle_n / unsigned(n));
            o->ui.impl->pf_fill_us = o->ui.impl->pf_stroke_us = o->ui.impl->pf_text_us = 0;
            o->ui.impl->pf_circle_us = o->ui.impl->pf_shadow_us = 0;
            o->ui.impl->pf_fill_n = o->ui.impl->pf_stroke_n = o->ui.impl->pf_text_n = 0;
            o->ui.impl->pf_circle_n = o->ui.impl->pf_shadow_n = 0;
            o->ui.impl->pf_fill_px = o->ui.impl->pf_stroke_px = o->ui.impl->pf_text_ch = 0;
        }
        o->frame_log_n = 0; o->frame_log_sum = 0; o->frame_log_worst = 0;
        o->t_feed = o->t_cb = o->t_end = o->t_present = 0;
        // 必须 flush：文件是全缓冲，进程被 kill 掉就一行都看不到
        std::fflush(out);
    }
}

} // namespace detail

bool Window::poll() {
    Window::Impl* o = impl;
    detail::Win32State* s = reinterpret_cast<detail::Win32State*>(o->native);
    if (!s || !o->running) return false;
    detail::service_clipboard(impl);
    if (o->wake_signal.consume()) o->need_frame = true;

    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { o->running = false; break; }
        // 线程计时器（组字期间压输入法浮动组合窗用的）。这种消息 hwnd 为空、
        // 没有窗口过程可派发，只能在这儿消费。它**故意**不置 need_frame ——
        // 否则组字时会被这个计时器拖着 50fps 重绘。
        if (!msg.hwnd && msg.message == WM_TIMER) {
            detail::Win32State* target = s->ime_timer == msg.wParam ? s : nullptr;
            if (!target) for (Window* window : detail::scheduled_windows()) {
                auto* sibling = reinterpret_cast<detail::Win32State*>(window->impl->native);
                if (sibling && sibling->ime_timer == msg.wParam) { target = sibling; break; }
            }
            if (target) { detail::win_hide_ime_comp_window(target); continue; }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return o->running;
}

void Window::render() {
    Window::Impl* o = impl;
    detail::Win32State* s = reinterpret_cast<detail::Win32State*>(o->native);
    if (!s || !s->has_backbuffer) return;
    // 原生模态消息泵可能重入 WM_PAINT，不能在未结束的帧里再次构建 Ui。
    if (o->ui.impl->frame_open) { o->need_frame = true; return; }

    // 帧间隔基准必须是**每个窗口自己的**。写成函数内 static 时，两个窗口在同一
    // 进程里会轮流改写它：A 画完置为 t，B 紧接着画 → dt ≈ 0；再下一次 A 的 dt
    // 变成两帧之和。表现是子窗口的 spinner / 光标闪烁顿得没有规律。
    uint64_t now = detail::win_ticks_ms();
    float dt = o->last_frame_ms ? float(now - o->last_frame_ms) / 1000.0f : (1.0f / 60.0f);
    if (dt > 0.25f) {
        o->ui.impl->caret_blink += dt - 0.25f; // 闪烁按真实时间推进，动画仍限制大步长。
        dt = 0.25f;
    }
    o->last_frame_ms = now;

    const bool prof = o->frame_log;
    uint64_t p0 = prof ? detail::win_ticks_us() : 0;

    detail::win_feed_input(s);
    uint64_t pa = prof ? detail::win_ticks_us() : 0;

    // 必须在构建帧"之前"清标志：帧内任何 repaint()（动画、光标闪烁）都会
    // 重新把它置 true，从而让下一帧继续。放到帧末清会把请求吃掉，动画就卡在第一帧。
    o->need_frame = false;

    detail::ScopeExit frame_cleanup([o] {
        if (o->ui.impl->frame_open) { o->ui.impl->abort_frame(); o->need_frame = true; }
    });
    Ui& ui = o->ui;
    ui.begin_frame(float(s->pw) / o->scale, float(s->ph) / o->scale, dt, o->scale);
    if (o->frame_cb_ptr) o->frame_cb_ptr();
    // 组字期间开一个 20 ms 的低频计时器压住输入法的浮动组合窗。
    // 不能只靠"每帧压一次"：光标闪烁的 38% 相位不请求重绘，帧会停，
    // 停住之后输入法再把窗口亮出来就没人压了（实测多行 textarea 上就这么漏的）。
    // 必须是**线程计时器**（hwnd = nullptr）：窗口计时器的 WM_TIMER 只有在消息泵
    // 转起来时才被合成，而我们的循环没帧就阻塞在 WaitMessage 里 —— 那种情况下
    // 它一次都不会来。线程计时器的 WM_TIMER 是**真入队**的，能把 WaitMessage 叫醒，
    // 在 poll() 里消费（见 Window::poll）。
    if (o->ui.impl->ime_owner && !o->ui.impl->ime_comp.empty()) {
        // 无关联窗口的计时器由系统分配 ID，保存 SetTimer 返回值以匹配 WM_TIMER。
        if (!s->ime_timer) s->ime_timer = SetTimer(nullptr, detail::AINO_IME_TIMER_ID, 20, nullptr);
    } else if (s->ime_timer) {
        KillTimer(nullptr, s->ime_timer);
        s->ime_timer = 0;
    }
    ui.set_cursor(ui.cursor());
    uint64_t p1 = prof ? detail::win_ticks_us() : 0;
    ui.end_frame();
    uint64_t p2 = prof ? detail::win_ticks_us() : 0;

    detail::win_sync_caption_theme(s, ui.theme().is_dark());
    o->cursor = ui.cursor();
    detail::win_present(s);
    s->bb_valid = true;          // 这块缓冲现在装的是"当前尺寸下画过的画面"
    detail::win_apply_cursor(s);
    if (prof) {
        uint64_t p3 = detail::win_ticks_us();
        o->t_feed += pa - p0;      // 取输入
        o->t_cb += p1 - pa;        // begin_frame + 用户帧回调（即真正的绘制）
        o->t_end += p2 - p1;       // end_frame（延迟浮层等）
        o->t_present += p3 - p2;   // 上屏
    }
}


void Window::set_title(const std::string& title) {
    Window::Impl* o = impl;
    if (!o) return;
    detail::Win32State* s = reinterpret_cast<detail::Win32State*>(o->native);
    if (s && s->hwnd) SetWindowTextW(s->hwnd, detail::to_wide(title).c_str());
}

void Window::set_size(int w, int h) {
    Window::Impl* o = impl;
    detail::Win32State* s = reinterpret_cast<detail::Win32State*>(o->native);
    if (!s || !s->hwnd) return;
    RECT rc = {0, 0, LONG(float(w) * o->scale), LONG(float(h) * o->scale)};
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
    SetWindowPos(s->hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}


// OpenClipboard 可能因其它进程占用而失败；最多重试 8 次，每次间隔 1ms。
namespace detail {
inline bool clipboard_open_retry(HWND owner = nullptr) {
    for (int i = 0; i < 8; ++i) {
        if (OpenClipboard(owner)) return true;
        Sleep(1);
    }
    return false;
}
} // namespace detail

bool Window::try_set_clipboard(const std::string& text) {
    auto* state = reinterpret_cast<detail::Win32State*>(impl->native);
    if (!state || !state->hwnd) return false;
    const std::wstring w = detail::to_wide(text);
    const size_t bytes = (w.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!mem) return false;
    void* dst = GlobalLock(mem);
    if (!dst) { GlobalFree(mem); return false; }
    memcpy(dst, w.c_str(), bytes);
    GlobalUnlock(mem);
    if (!detail::clipboard_open_retry(state->hwnd)) { GlobalFree(mem); return false; }
    const bool stored = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, mem);
    if (!stored) GlobalFree(mem);
    CloseClipboard();
    return stored;
}

std::string Window::clipboard() const {
    std::string out;
    if (!detail::clipboard_open_retry()) return out;
    if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
        const wchar_t* p = reinterpret_cast<const wchar_t*>(GlobalLock(h));
        if (p) {
            // 规范要求 NUL 结尾，但真有程序不给 —— 用 GlobalSize 兜个上限，
            // 免得 std::wstring(p) 一路读出去踩到未映射页。
            const size_t cap = GlobalSize(h) / sizeof(wchar_t);
            size_t n = 0;
            while (n < cap && p[n]) ++n;
            out = detail::from_wide(std::wstring(p, n));
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return out;
}

#endif // AINO_PLATFORM_WINDOWS

// =============================================================================
//  8.19  Linux / X11 后端
//        文字走 FreeType + fontconfig（运行时 dlopen，不要开发包），字形在
//        **客户端**光栅化成灰度位图，逐像素 blend 进缓冲 —— 零 X 往返。
//        两者都加载不到时，退回核心 X 的 XLFD 点阵字体（能跑，只是没抗锯齿）。
// =============================================================================
namespace detail {
// XSETTINGS 的整数使用属性自身的字节序；截断或无效属性不采用部分解析结果。
struct DesktopScaleSettings { double dpi = 0; int factor = 0; std::string icon_theme; };
inline DesktopScaleSettings parse_xsettings_scale(const unsigned char* bytes, size_t length) {
    DesktopScaleSettings result;
    if (!bytes || length < 12 || length > 1024u * 1024u || bytes[0] > 1) return result;
    const bool little = bytes[0] == 0;
    size_t pos = 4;
    bool ok = true;
    auto read = [&](size_t n) -> uint32_t {
        if (n > length - pos) { ok = false; return 0; }
        uint32_t v = 0;
        for (size_t i = 0; i < n; ++i) v |= uint32_t(bytes[pos + i]) << (8u * (little ? i : n - i - 1));
        pos += n; return v;
    };
    read(4); const auto count = read(4);
    if (count > 4096) return {};
    auto skip = [&](size_t n) { if (n > length - pos) ok = false; else pos += n; };
    auto align = [&] { skip((4 - pos % 4) % 4); };
    for (uint32_t i = 0; i < count && ok; ++i) {
        const auto type = read(1); read(1); const auto len = read(2);
        if (!ok || len > length - pos) return {};
        const std::string name(reinterpret_cast<const char*>(bytes + pos), len);
        skip(len); align(); read(4);
        if (type == 0) {
            const auto value = read(4);
            if (name == "Xft/DPI" && value >= 48u * 1024u && value <= 384u * 1024u) result.dpi = double(value) / 1024.0;
            if (name == "Gdk/WindowScalingFactor" && value >= 1 && value <= 4) result.factor = int(value);
        } else if (type == 1) {
            const auto size = read(4);
            if (!ok || size > length - pos) return {};
            if (name == "Net/IconThemeName" && size <= 256)
                result.icon_theme.assign(reinterpret_cast<const char*>(bytes + pos), size);
            skip(size); align();
        }
        else if (type == 2) skip(8);
        else return {};
    }
    return ok ? result : DesktopScaleSettings{};
}
inline double parse_scale_number(const char* text, double low = .5, double high = 4) {
    if (!text || !*text) return 0;
    // 环境变量采用小数点格式，不受用户 locale 的小数分隔符影响。
    double value = 0, fraction = .1; bool point = false, digit = false;
    for (const char* p = text; *p; ++p) {
        if (*p == '.' && !point) { point = true; continue; }
        if (*p < '0' || *p > '9') return 0;
        digit = true;
        if (point) { value += (*p - '0') * fraction; fraction *= .1; }
        else value = value * 10 + (*p - '0');
        if (value > 1000) return 0;
    }
    return digit && value >= low && value <= high ? value : 0;
}
inline float desktop_scale(const char* override_scale, const char* gdk_scale,
                           const DesktopScaleSettings& settings, double resource_dpi) {
    if (const auto value = parse_scale_number(override_scale)) return float(value);
    if (const auto value = parse_scale_number(gdk_scale)) return float(value);
    const double dpi = settings.dpi > 0 ? settings.dpi : resource_dpi;
    double scale = dpi >= 48 && dpi <= 384 ? dpi / 96 : 1;
    if (settings.factor >= 1 && settings.factor <= 4) scale = std::max(scale, double(settings.factor));
    return float(scale);
}
} // namespace detail

#if defined(AINO_PLATFORM_UNIX)

namespace detail {

inline std::string& x11_icon_theme() { static thread_local std::string theme; return theme; }

// 诊断开关：AINO_X11_TRACE=1 时把 X11 文本路径的每一步打到 stderr。
// 为什么必须 fflush：崩溃（free(): invalid pointer / abort）时管道里的 stderr
// 是**全缓冲**的，不 flush 的话进程一死，前面打的字全部丢失 —— 日志里只剩
// 一行 glibc 的报错，看不出死在哪一步。
inline bool x11_trace_on() {
    static const bool on = (std::getenv("AINO_X11_TRACE") != nullptr || std::getenv("AINO_WAYLAND_TRACE") != nullptr);
    return on;
}
inline void x11_trace(const char* fmt, ...) {
    if (!x11_trace_on()) return;
    va_list ap;
    va_start(ap, fmt);
    std::fputs("[x11] ", stderr);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
    std::fflush(stderr);
    va_end(ap);
}

// FreeType 在客户端光栅化，fontconfig 负责字体匹配与缺字回退。Xft 服务端绘制会被 XPutImage 覆盖；Pixmap 回读会引入同步往返。结构体仅镜像所需公开 ABI 前缀。
struct FtBitmapLite {
    unsigned int rows, width;
    int pitch;
    unsigned char* buffer;
    unsigned short num_grays;
    unsigned char pixel_mode, palette_mode;
    void* palette;
};
struct FtMetricsLite {
    long width, height, horiBearingX, horiBearingY, horiAdvance;
    long vertBearingX, vertBearingY, vertAdvance;
};
struct FtVectorLite { long x, y; };
// FT_GlyphSlotRec 的前缀（到 bitmap_top 为止）
struct FtSlotLite {
    void* library;
    void* face;
    void* next;
    unsigned int glyph_index;
    void* generic_data;
    void (*generic_finalizer)(void*);
    FtMetricsLite metrics;
    long linear_hori_advance, linear_vert_advance;
    FtVectorLite advance;
    unsigned int format;
    FtBitmapLite bitmap;
    int bitmap_left, bitmap_top;
};
// FT_SizeRec 以 FT_Face 指针开头，镜像结构必须保留该字段以保证 metrics 偏移正确。
struct FtSizeLite {
    void* face;
    void* generic_data;
    void (*generic_finalizer)(void*);
    struct {
        unsigned short x_ppem, y_ppem;
        long x_scale, y_scale;
        long ascender, descender, height, max_advance;
    } metrics;
    void* internal;
};
// FT_FaceRec 的前缀（到 glyph / size 为止）
struct FtFaceLite {
    long num_faces, face_index, face_flags, style_flags, num_glyphs;
    char* family_name;
    char* style_name;
    int num_fixed_sizes;
    void* available_sizes;
    int num_charmaps;
    void* charmaps;
    void* generic_data;
    void (*generic_finalizer)(void*);
    struct { long xMin, yMin, xMax, yMax; } bbox;
    unsigned short units_per_EM;
    short ascender, descender, height, max_advance_width, max_advance_height;
    short underline_position, underline_thickness;
    void* glyph;      // FT_GlyphSlot
    void* size;       // FT_Size
};
// FcFontSet 是公开结构体，字段顺序自 1.0 起没变
struct FcFontSetLite { int nfont, sfont; void** fonts; };

struct FtApi {
    void* lib = nullptr;
    void* ft_lib = nullptr;      // FT_Library
    int  (*init_lib)(void** out) = nullptr;
    int  (*new_face)(void* lib, const char* path, long index, void** out) = nullptr;
    int  (*done_face)(void* face) = nullptr;
    int  (*set_char_size)(void* face, long w, long h, unsigned hdpi, unsigned vdpi) = nullptr;
    unsigned int (*char_index)(void* face, unsigned long cp) = nullptr;
    int  (*load_glyph)(void* face, unsigned int gidx, int flags) = nullptr;

    bool load() {
        if (lib) return true;
        const char* names[] = { "libfreetype.so.6", "libfreetype.so", nullptr };
        const char* used = nullptr;
        for (int i = 0; names[i]; ++i) {
            lib = dlopen(names[i], RTLD_LAZY);
            if (lib) { used = names[i]; break; }
        }
        if (!lib) {
            const char* e = dlerror();
            x11_trace("freetype: dlopen 失败: %s", e ? e : "?");
            return false;
        }
        init_lib      = reinterpret_cast<decltype(init_lib)>(dlsym(lib, "FT_Init_FreeType"));
        new_face      = reinterpret_cast<decltype(new_face)>(dlsym(lib, "FT_New_Face"));
        done_face     = reinterpret_cast<decltype(done_face)>(dlsym(lib, "FT_Done_Face"));
        set_char_size = reinterpret_cast<decltype(set_char_size)>(dlsym(lib, "FT_Set_Char_Size"));
        char_index    = reinterpret_cast<decltype(char_index)>(dlsym(lib, "FT_Get_Char_Index"));
        load_glyph    = reinterpret_cast<decltype(load_glyph)>(dlsym(lib, "FT_Load_Glyph"));
        if (!init_lib || !new_face || !done_face || !set_char_size || !char_index || !load_glyph) {
            x11_trace("freetype: dlsym 不全，放弃");
            dlclose(lib);
            lib = nullptr;
            return false;
        }
        if (init_lib(&ft_lib) != 0 || !ft_lib) {
            x11_trace("freetype: FT_Init_FreeType 失败");
            dlclose(lib);
            lib = nullptr;
            return false;
        }
        x11_trace("freetype: loaded %s -> %p", used ? used : "?", lib);
        return true;
    }
};

struct FcApi {
    void* lib = nullptr;
    int  (*init)(void) = nullptr;
    void* (*pattern_create)(void) = nullptr;
    void  (*pattern_destroy)(void*) = nullptr;
    int  (*add_string)(void*, const char*, const char*) = nullptr;
    int  (*add_double)(void*, const char*, double) = nullptr;
    int  (*add_integer)(void*, const char*, int) = nullptr;
    int  (*add_bool)(void*, const char*, int) = nullptr;
    int  (*config_substitute)(void*, void*, int) = nullptr;
    void  (*default_substitute)(void*) = nullptr;
    void* (*font_match)(void*, void*, int*) = nullptr;
    void* (*font_sort)(void*, void*, int, void*, int*) = nullptr;
    void  (*font_set_destroy)(void*) = nullptr;
    int  (*get_string)(void*, const char*, int, unsigned char**) = nullptr;
    int  (*get_integer)(void*, const char*, int, int*) = nullptr;
    // 这两个用来**验证一个字体到底覆盖不覆盖汉字**（FcPatternGetCharSet 取
    // pattern 上的 charset，FcCharSetHasChar 查某个码位在不在里面）。
    // 少了它们就只能靠「排序表前 N 条里碰运气」，见 build() 里的注释。
    int  (*get_charset)(void*, const char*, int, void**) = nullptr;
    int  (*charset_has)(const void*, unsigned int) = nullptr;
    void* (*charset_copy)(void*) = nullptr;
    void (*charset_destroy)(void*) = nullptr;

    bool load() {
        if (lib) return true;
        const char* names[] = { "libfontconfig.so.1", "libfontconfig.so", nullptr };
        const char* used = nullptr;
        for (int i = 0; names[i]; ++i) {
            lib = dlopen(names[i], RTLD_LAZY);
            if (lib) { used = names[i]; break; }
        }
        if (!lib) {
            const char* e = dlerror();
            x11_trace("fontconfig: dlopen 失败: %s", e ? e : "?");
            return false;
        }
        init              = reinterpret_cast<decltype(init)>(dlsym(lib, "FcInit"));
        pattern_create    = reinterpret_cast<decltype(pattern_create)>(dlsym(lib, "FcPatternCreate"));
        pattern_destroy   = reinterpret_cast<decltype(pattern_destroy)>(dlsym(lib, "FcPatternDestroy"));
        add_string        = reinterpret_cast<decltype(add_string)>(dlsym(lib, "FcPatternAddString"));
        add_double        = reinterpret_cast<decltype(add_double)>(dlsym(lib, "FcPatternAddDouble"));
        add_integer       = reinterpret_cast<decltype(add_integer)>(dlsym(lib, "FcPatternAddInteger"));
        add_bool          = reinterpret_cast<decltype(add_bool)>(dlsym(lib, "FcPatternAddBool"));
        config_substitute = reinterpret_cast<decltype(config_substitute)>(dlsym(lib, "FcConfigSubstitute"));
        default_substitute= reinterpret_cast<decltype(default_substitute)>(dlsym(lib, "FcDefaultSubstitute"));
        font_match        = reinterpret_cast<decltype(font_match)>(dlsym(lib, "FcFontMatch"));
        font_sort         = reinterpret_cast<decltype(font_sort)>(dlsym(lib, "FcFontSort"));
        font_set_destroy  = reinterpret_cast<decltype(font_set_destroy)>(dlsym(lib, "FcFontSetDestroy"));
        get_string        = reinterpret_cast<decltype(get_string)>(dlsym(lib, "FcPatternGetString"));
        get_integer       = reinterpret_cast<decltype(get_integer)>(dlsym(lib, "FcPatternGetInteger"));
        get_charset       = reinterpret_cast<decltype(get_charset)>(dlsym(lib, "FcPatternGetCharSet"));
        charset_has       = reinterpret_cast<decltype(charset_has)>(dlsym(lib, "FcCharSetHasChar"));
        charset_copy      = reinterpret_cast<decltype(charset_copy)>(dlsym(lib, "FcCharSetCopy"));
        charset_destroy   = reinterpret_cast<decltype(charset_destroy)>(dlsym(lib, "FcCharSetDestroy"));
        if (!init || !pattern_create || !pattern_destroy || !add_string || !add_double
            || !add_integer || !add_bool || !config_substitute || !default_substitute
            || !font_match || !font_sort || !font_set_destroy || !get_string || !get_integer) {
            x11_trace("fontconfig: dlsym 不全，放弃");
            dlclose(lib);
            lib = nullptr;
            return false;
        }
        init();          // 读配置文件 + 建字体缓存；失败也继续（FcFontMatch 会给一个空表）
        x11_trace("fontconfig: loaded %s -> %p", used ? used : "?", lib);
        return true;
    }
};

// 进程内只加载一次（多个窗口共用同一份库句柄和字体缓存）
inline FtApi& ft_api() {
    static FtApi api;
    static const bool once = api.load();
    (void)once;
    return api;
}
inline FcApi& fc_api() {
    static FcApi api;
    static const bool once = api.load();
    (void)once;
    return api;
}

// 回退链默认上限为 16，限制字体 face 和文件映射数量；CJK 字体置于主字体之后。AINO_X11_CHAIN_CAP 供测试逐窗口调整上限。
inline int x11_chain_cap() {
    const char* e = std::getenv("AINO_X11_CHAIN_CAP");
    const int v = e ? std::atoi(e) : 0;
    return v >= 2 ? v : 16;
}

class UnixTextRenderer : public TextRenderer {
public:
    void init(float, const std::string& family, const std::string& mono) override {
        // 两套名字：FreeType 按 fontconfig 家族名匹配（"sans"），
        // 核心字体按 XLFD 通配（"*"）。
        family_ = family.empty() ? std::string("*") : family;
        sans_   = family.empty() ? std::string("sans") : family;
        mono_   = mono.empty() ? std::string("monospace") : mono;
        ft_ready_ = ft_api().lib != nullptr && fc_api().lib != nullptr;
        x11_trace("text init: family=%s mono=%s freetype=%d", family.c_str(), mono.c_str(),
                  ft_ready_ ? 1 : 0);
    }
    void reconfigure(float dpi, const std::string& family, const std::string& mono) override {
#ifdef AINO_PLATFORM_X11
        Display* dpy = dpy_;
        const Drawable target = target_;
#endif
        release();
        init(dpi, family, mono);
#ifdef AINO_PLATFORM_X11
        set_drawable(dpy, target, nullptr);
#endif
    }
    void release() override {
        release_fonts();
#ifdef AINO_PLATFORM_X11
        if (dpy_) {
            for (auto& kv : sets_) {
                if (!kv.second) continue;
                x11_trace("  free fontset size*2=%d", kv.first);
                XFreeFontSet(dpy_, kv.second);
            }
            if (gc1_) { XFreeGC(dpy_, gc1_); gc1_ = nullptr; }
            if (pm1_) { XFreePixmap(dpy_, pm1_); pm1_ = 0; }
        }
        sets_.clear();
        dpy_ = nullptr;
#endif
        x11_trace("text release done");
    }
    float measure(const std::string& utf8, float size, int weight) override {
        if (utf8.empty()) return 0.0f;
        if (Font* f = font(size, weight, false)) {
            float w = 0.0f;
            size_t i = 0;
            while (i < utf8.size()) {
                uint32_t cp = 0;
                const int n = utf8_decode_n(utf8.c_str() + i, utf8.size() - i, &cp);
                i += size_t(n < 1 ? 1 : n);
                w += float(glyph(*f, cp).adv) * (1.0f / 64.0f);
            }
            if (w > 0.0f) return w;
        }
#ifdef AINO_PLATFORM_X11
        XFontSet fs = fontset(size);
        if (!fs) return float(utf8.size()) * size * 0.55f;
        XRectangle ink{}, logical{};
        Xutf8TextExtents(fs, utf8.c_str(), int(utf8.size()), &ink, &logical);
        // 字体集里缺这个字符时 width 会是 0 —— 直接返回 0 会让上层布局把整段
        // 文字算成"零宽"，居中/右对齐全错位。退回按字宽估算。
        if (logical.width <= 0) return float(utf8.size()) * size * 0.55f;
        return float(logical.width);
#else
        return float(utf8_count(utf8)) * size * 0.55f;
#endif
    }
    void caret_positions(const std::string& utf8, float size, int weight, std::vector<float>& out) override {
        Font* f = font(size, weight, false);
        if (!f) { TextRenderer::caret_positions(utf8, size, weight, out); return; }
        out.assign(1, 0.0f);
        float pen = 0;
        for (size_t i = 0; i < utf8.size();) {
            uint32_t cp;
            const int n = utf8_decode_n(utf8.data() + i, utf8.size() - i, &cp);
            i += size_t(std::max(1, n));
            pen += float(glyph(*f, cp).adv) / 64.0f;
            out.push_back(pen);
        }
    }
    float ascent(float size, int weight) override {
        if (Font* f = font(size, weight, false)) return float(f->asc);
#ifdef AINO_PLATFORM_X11
        XFontSet fs = fontset(size);
        if (!fs) return size * 0.8f;
        XFontSetExtents* e = XExtentsOfFontSet(fs);
        if (!e) return size * 0.8f;
        return float(-e->max_logical_extent.y);
#else
        return size * 0.8f;
#endif
    }
    float line_height(float size, int weight) override {
        if (Font* f = font(size, weight, false)) return float(f->asc + f->desc);
#ifdef AINO_PLATFORM_X11
        XFontSet fs = fontset(size);
        if (!fs) return size * 1.4f;
        XFontSetExtents* e = XExtentsOfFontSet(fs);
        if (!e) return size * 1.4f;
        return float(e->max_logical_extent.height);
#else
        return size * 1.4f;
#endif
    }
    // 等宽通道：代码块靠它按列对齐。Windows 用 Consolas，Linux 自然就是
    // fontconfig 的 monospace（DejaVu Sans Mono / Noto Sans Mono）。
    float measure_mono(const std::string& utf8, float size, int weight) override {
        if (utf8.empty()) return 0.0f;
        if (Font* f = font(size, weight, true)) {
            float w = 0.0f;
            size_t i = 0;
            while (i < utf8.size()) {
                uint32_t cp = 0;
                const int n = utf8_decode_n(utf8.c_str() + i, utf8.size() - i, &cp);
                i += size_t(n < 1 ? 1 : n);
                w += float(glyph(*f, cp).adv) * (1.0f / 64.0f);
            }
            if (w > 0.0f) return w;
        }
        return measure(utf8, size, weight);
    }
    float ascent_mono(float size, int weight) override {
        if (Font* f = font(size, weight, true)) return float(f->asc);
        return ascent(size, weight);
    }
    // 文字与几何统一绘入客户端缓冲，由 XPutImage 呈现。FreeType 本地光栅化，core_draw 仅作不可用时的回退。
    void draw(Canvas& cv, const std::string& utf8, float x, float y_top,
              float size, int weight, const Color& c, const Rect& clip) override {
        if (utf8.empty()) return;
        if (ft_draw(cv, utf8, x, y_top, size, weight, c, clip, false)) return;
#ifdef AINO_PLATFORM_X11
        if (dpy_) core_draw(cv, utf8, x, y_top, size, weight, c, clip);
#endif
    }
    void draw_mono(Canvas& cv, const std::string& utf8, float x, float y_top,
                   float size, int weight, const Color& c, const Rect& clip) override {
        if (utf8.empty()) return;
        if (ft_draw(cv, utf8, x, y_top, size, weight, c, clip, true)) return;
#ifdef AINO_PLATFORM_X11
        if (dpy_) core_draw(cv, utf8, x, y_top, size, weight, c, clip);
#endif
    }
#ifdef AINO_PLATFORM_X11
    void set_drawable(Display* d, Drawable target, GC) {
        dpy_ = d;
        target_ = target;
    }
#endif
    bool ready() const { return ft_ready_; }
    struct CacheStats { size_t fonts, faces, glyph_bytes; };
    CacheStats cache_stats() const { return {fonts_.size(), faces_.size(), glyph_bytes_}; }

private:
    // -----------------------------------------------------------------------
    //  一个 (字号, 字重, 是否等宽) 对应一组字体：主字体 + 兜底链
    //
    //  兜底链来自 FcFontSort —— fontconfig 按匹配度排好的整张字体表。西文
    //  几乎永远命中主字体（hit 记住上次命中的下标，省掉反复查找），汉字则
    //  一路找到 Noto Sans CJK。字体按需打开，同一文件与 face index 在渲染器内共享。
    //  字形位图按字号缓存；达到预算后清理，后续按需重新光栅化。
    // -----------------------------------------------------------------------
    struct Glyph {
        int adv = 0;       // 前进量，26.6 定点
        int left = 0, top = 0;
        int w = 0, h = 0;
        std::vector<unsigned char> px;   // w×h 灰度覆盖率
    };
    struct Font {
        double px = 0.0;
        bool ready = false;              // 度量已建立；不持有可被冷回收的裸 FT_Face。
        int asc = 0, desc = 0;
        std::vector<std::string> files;   // 兜底链：文件路径
        std::vector<long> indices;
        std::vector<std::shared_ptr<void>> charsets; // Fontconfig 的覆盖集合；查询缺字不打开字体文件。
        std::unordered_map<unsigned long, Glyph> glyphs;
        int hit = 0;                      // 上次命中的字体下标
    };

    Font* font(float size, int weight, bool mono) {
        if (!ft_ready_ || size <= 0.0f) return nullptr;
        reap_faces();
        const int q  = int(size * 2.0f + 0.5f);       // 0.5px 量化，一次开一种字号
        const int wq = weight >= 650 ? 200 : (weight >= 550 ? 180 : (weight <= 350 ? 50 : 80));
        const int key = (mono ? 100000000 : 0) + q * 1000 + wq;
        auto it = fonts_.find(key);
        if (it != fonts_.end()) return it->second.ready ? &it->second : nullptr;
        Font f;
        if (build(f, mono ? mono_ : sans_, double(q) * 0.5, wq)) {
            Font& slot = fonts_[key];
            slot = std::move(f);
            return &slot;
        }
        // 失败也记一笔，避免每帧重试一遍 fontconfig。
        fonts_[key] = Font();
        return nullptr;
    }

    bool build(Font& f, const std::string& family, double px, int weight) {
        FcApi& fc = fc_api();
        void* pat = fc.pattern_create();
        if (!pat) return false;
        // pixelsize 和 size 一起给：只给 size 的话 FcDefaultSubstitute 会按
        // dpi（默认 75）把它换算成 pixelsize，字会莫名大一圈。
        fc.add_string(pat, "family", family.c_str());
        fc.add_double(pat, "pixelsize", px);
        fc.add_double(pat, "size", px);
        fc.add_integer(pat, "weight", weight);
        fc.add_bool(pat, "antialias", 1);
        fc.add_bool(pat, "hinting", 1);
        fc.config_substitute(nullptr, pat, 0);     // 0 = FcMatchPattern
        fc.default_substitute(pat);

        int res = 1;
        void* sort = fc.font_sort(nullptr, pat, 1, nullptr, &res);
        if (sort) {
            FcFontSetLite* set = static_cast<FcFontSetLite*>(sort);
            // FcFontSort 包含同一家族的多个 style；按 charset 选择覆盖 U+4E2D 的字体并置于主字体之后，避免截断回退链时遗漏 CJK。
            const int cap = x11_chain_cap();
            std::string cjk_file;
            std::shared_ptr<void> cjk_charset;
            long cjk_index = 0;
            bool cjk_found = false;
            int  cjk_pos   = -1;      // 已在收集范围内时的位置
            for (int i = 0; i < set->nfont; ++i) {
                unsigned char* file = nullptr;
                int index = 0;
                if (fc.get_string(set->fonts[i], "file", 0, &file) != 0 || !file) continue;
                if (fc.get_integer(set->fonts[i], "index", 0, &index) != 0) index = 0;
                void* cs = nullptr;
                std::shared_ptr<void> charset;
                if (fc.get_charset) fc.get_charset(set->fonts[i], "charset", 0, &cs);
                const bool has_cjk = !cjk_found && cs && fc.charset_has && fc.charset_has(cs, 0x4E2Du);
                if ((int(f.files.size()) < cap || has_cjk) && cs && fc.charset_copy && fc.charset_destroy) {
                    const auto key = std::make_pair(std::string(reinterpret_cast<const char*>(file)), long(index));
                    auto found = charsets_.find(key);
                    if (found == charsets_.end())
                        found = charsets_.emplace(key, std::shared_ptr<void>(fc.charset_copy(cs), fc.charset_destroy)).first;
                    charset = found->second;
                }
                if (has_cjk) {
                    cjk_found = true;
                    cjk_file = reinterpret_cast<const char*>(file);
                    cjk_charset = charset;
                    cjk_index = long(index);
                }
                if (int(f.files.size()) >= cap) {
                    if (cjk_found) break;     // 已经拿到了需要的，不用再扫
                    continue;
                }
                if (has_cjk) cjk_pos = int(f.files.size());
                f.files.push_back(reinterpret_cast<const char*>(file));
                f.indices.push_back(long(index));
                f.charsets.push_back(std::move(charset));
            }
            // 第一条覆盖汉字的字体没进前 cap 条 → 手动插到主字体后面。
            if (cjk_found && cjk_pos < 0) {
                f.files.insert(f.files.begin() + (f.files.empty() ? 0 : 1), cjk_file);
                f.indices.insert(f.indices.begin() + (f.indices.empty() ? 0 : 1), cjk_index);
                f.charsets.insert(f.charsets.begin() + (f.charsets.empty() ? 0 : 1), cjk_charset);
                cjk_pos = f.files.empty() ? 0 : 1;
            }
            fc.font_set_destroy(sort);
            if (x11_trace_on())
                x11_trace("  chain %zu fonts, cjk at %d (%s)", f.files.size(), cjk_pos,
                          cjk_found ? cjk_file.c_str() : "none");
        }
        fc.pattern_destroy(pat);
        if (f.files.empty()) return false;

        f.px = px;
        void* face = open_face(f, 0);
        if (!face) {
            for (size_t i = 1; i < f.files.size(); ++i) {
                if (open_face(f, i)) { f.hit = int(i); face = open_face(f, i); break; }
            }
        }
        if (!face) return false;
        f.ready = true;

        // 升降部采用主字体 metrics，避免使用单个 CJK 字形的墨迹高度改变行高和垂直对齐。
        FtFaceLite* fr = static_cast<FtFaceLite*>(face);
        if (fr->size) {
            FtSizeLite* sz = static_cast<FtSizeLite*>(fr->size);
            f.asc  = int((sz->metrics.ascender + 63) >> 6);
            f.desc = int((-sz->metrics.descender + 63) >> 6);
        }
        // 范围校验：一行字再高也不会超过 2em。越界说明 metrics 读错了（结构体
        // 错位时就是 448 这种数），直接退回 hhea 的 ascender/descender。
        const int em = int(px * 2.0 + 0.5);
        if (f.asc <= 0 || f.asc > em) f.asc = 0;
        if (f.desc < 0 || f.desc > em) f.desc = 0;
        if (fr->units_per_EM) {
            if (f.asc <= 0)  f.asc  = int(px * double(fr->ascender) / double(fr->units_per_EM) + 0.5);
            if (f.desc <= 0) f.desc = int(px * double(-fr->descender) / double(fr->units_per_EM) + 0.5);
        }
        if (f.desc < 1) f.desc = int(px * 0.22 + 0.5);
        if (f.asc <= 0) f.asc = int(px * 0.8 + 0.5);
        x11_trace("font \"%s\" %.1fpx w=%d -> %s asc=%d desc=%d fallbacks=%zu",
                  family.c_str(), px, weight, f.files[0].c_str(), f.asc, f.desc, f.files.size());
        return true;
    }

    void* open_face(Font& f, size_t i) {
        if (i >= f.files.size()) return nullptr;
        // FT_Face 含字体表；按字号重复打开大型 CJK 字体会占用大量内存。
        // 字形位图仍按字号缓存，字体文件及 face 在渲染器内共享。
        const auto key = std::make_pair(f.files[i], f.indices[i]);
        auto inserted = faces_.emplace(key, SharedFace{});
        auto& shared = inserted.first->second;
        shared.touched = face_time();
        FtApi& ft = ft_api();
        if (inserted.second) {
            if (ft.new_face(ft.ft_lib, f.files[i].c_str(), f.indices[i], &shared.face) != 0) shared.face = nullptr;
        }
        if (!shared.face) return nullptr;
        // 72dpi + 26.6 定点：char_size 的数值就等于像素，还能带小数
        const long size = long(f.px * 64.0 + 0.5);
        if (shared.size != size) {
            if (ft.set_char_size(shared.face, 0, size, 72, 72) != 0) return nullptr;
            shared.size = size;
        }
        return shared.face;
    }

    static uint64_t face_time() {
        timespec value{}; clock_gettime(CLOCK_MONOTONIC, &value);
        return uint64_t(value.tv_sec) * 1000 + uint64_t(value.tv_nsec) / 1000000;
    }
    // 已复制的字形和行高不依赖 FT_Face；只关闭十秒未参与缺字光栅化的文件。
    // 下一次缓存缺字时按相同文件、face index、字号重新打开，不改变选字与显示。
    void reap_faces() {
        const auto now = face_time();
        if (now < face_reap_ + 1000) return;
        face_reap_ = now;
        for (auto it = faces_.begin(); it != faces_.end();) {
            if (now >= it->second.touched + 10000) {
                if (it->second.face) ft_api().done_face(it->second.face);
                it = faces_.erase(it);
            } else ++it;
        }
    }

    bool covers(const Font& f, size_t index, unsigned long cp) const {
        if (index >= f.files.size()) return false;
        const auto& fc = fc_api();
        return index >= f.charsets.size() || !f.charsets[index] || !fc.charset_has ||
               fc.charset_has(f.charsets[index].get(), unsigned(cp)) != 0;
    }

    // 取一个码位的字形：先试上次命中的字体，再沿兜底链找第一个覆盖它的。
    // 缺少字体时画 .notdef 并报告一次，不能把不可读的内容静默变成空白。
    const Glyph& glyph(Font& f, unsigned long cp) {
        uncached_glyph_ = Glyph{};
        auto it = f.glyphs.find(cp);
        if (it != f.glyphs.end()) return it->second;
        FtApi& ft = ft_api();
        Glyph g;
        int hit = -1;
        // 优先查询主字体，再查询最近命中的回退字体，避免 CJK 缓存改变西文字形来源。
        if (covers(f, 0, cp)) {
            if (void* primary = open_face(f, 0)) {
                if (ft.char_index(primary, cp)) hit = 0;
            }
        }
        if (hit < 0 && f.hit != 0 && covers(f, size_t(f.hit), cp)) {
            if (void* face = open_face(f, size_t(f.hit))) {
                if (ft.char_index(face, cp)) hit = f.hit;
            }
        }
        if (hit < 0) {
            for (size_t i = 1; i < f.files.size(); ++i) {
                if (int(i) == f.hit || !covers(f, i, cp)) continue;
                void* cand = open_face(f, i);
                if (!cand) continue;
                if (ft.char_index(cand, cp)) { hit = int(i); break; }
            }
        }
        if (hit >= 0) {
            f.hit = hit;
            void* face = open_face(f, size_t(hit));
            const unsigned int gi = face ? ft.char_index(face, cp) : 0;
            // 0x4 = FT_LOAD_RENDER：加载后直接光栅化成灰度位图。
            // 渲染目标是 normal（灰度），不是 LCD 子像素 —— 这里是拿覆盖率
            // 当 alpha 逐像素 blend，子像素会把彩色边缘算进去。
            const int lerr = gi ? ft.load_glyph(face, gi, 0x4) : -1;
            if (x11_trace_on()) {
                FtSlotLite* sl = gi ? static_cast<FtSlotLite*>(static_cast<FtFaceLite*>(face)->glyph) : nullptr;
                x11_trace("  glyph U+%04lX face=%d gidx=%u load=%d slot=%p bm=%ux%u mode=%d adv=%ld",
                          cp, hit, gi, lerr, (void*)sl,
                          sl ? sl->bitmap.width : 0u, sl ? sl->bitmap.rows : 0u,
                          sl ? int(sl->bitmap.pixel_mode) : -1, sl ? sl->advance.x : 0L);
            }
            if (lerr == 0) {
                FtSlotLite* slot = static_cast<FtSlotLite*>(static_cast<FtFaceLite*>(face)->glyph);
                if (slot) {
                    g.adv = int(slot->advance.x);
                    copy_bitmap(slot, g);
                }
            }
        } else {
            g.adv = int(f.px * 0.5 * 64.0 + 0.5);
            if (cp >= 32 && cp != 127) {
                if (!missing_font_reported_) {
                    std::fprintf(stderr, "[aino/font] No installed font covers U+%04lX; showing a missing-glyph marker. "
                        "Install suitable fonts (CJK: fonts-noto-cjk).\n", cp);
                    missing_font_reported_ = true;
                }
                if (void* face = open_face(f, 0)) {
                    if (ft.load_glyph(face, 0, 0x4) == 0) {
                        auto* slot = static_cast<FtSlotLite*>(static_cast<FtFaceLite*>(face)->glyph);
                        if (slot) { g.adv = int(slot->advance.x); copy_bitmap(slot, g); }
                    }
                }
            }
        }
        constexpr size_t budget = 4u * 1024u * 1024u;
        // 单个超大字形只供本次调用使用，不让它撑破持久缓存预算。
        if (g.px.size() > budget) {
            uncached_glyph_ = std::move(g);
            return uncached_glyph_;
        }
        if (glyph_bytes_ + g.px.size() > budget || glyph_count_ >= 16384) {
            for (auto& cached : fonts_) cached.second.glyphs.clear();
            glyph_bytes_ = 0; glyph_count_ = 0;
        }
        glyph_bytes_ += g.px.size();
        ++glyph_count_;
        return f.glyphs.emplace(cp, std::move(g)).first->second;
    }

    static void copy_bitmap(FtSlotLite* slot, Glyph& g) {
        const FtBitmapLite& bm = slot->bitmap;
        if (!bm.rows || !bm.width || !bm.buffer) return;
        g.w = int(bm.width);
        g.h = int(bm.rows);
        g.left = slot->bitmap_left;
        g.top  = slot->bitmap_top;
        g.px.assign(size_t(g.w) * size_t(g.h), 0);
        int pitch = bm.pitch;
        if (bm.pixel_mode == 2) {                 // 8bit 灰度：一个字节一个像素
            if (pitch < 0) return;
            for (int r = 0; r < g.h; ++r) {
                const unsigned char* src = bm.buffer + ptrdiff_t(r) * ptrdiff_t(pitch);
                std::memcpy(&g.px[size_t(r) * size_t(g.w)], src, size_t(g.w));
            }
        } else if (bm.pixel_mode == 1) {          // 1bit 点阵（字体关掉抗锯齿时）
            const int bpr = (g.w + 7) / 8;
            int stride = pitch < 0 ? -pitch : pitch;
            if (stride < bpr) stride = bpr;
            for (int r = 0; r < g.h; ++r) {
                const unsigned char* src = bm.buffer + ptrdiff_t(r) * ptrdiff_t(stride);
                for (int c = 0; c < g.w; ++c)
                    if (src[c >> 3] & (0x80u >> (c & 7)))
                        g.px[size_t(r) * size_t(g.w) + size_t(c)] = 255;
            }
        } else {
            g.w = g.h = 0;                        // 彩色/其它模式：不画，别糊
            g.px.clear();
        }
    }

    bool ft_draw(Canvas& cv, const std::string& s, float x, float y_top,
                 float size, int weight, const Color& c, const Rect& clip, bool mono) {
        Font* f = font(size, weight, mono);
        if (!f) return false;
        const int baseline = int(y_top + 0.5f) + f->asc;
        int pen = int(x * 64.0f + 0.5f);        // 26.6 定点，避免逐字累积误差
        const bool use_clip = clip.w >= 0.0f;
        size_t i = 0;
        while (i < s.size()) {
            uint32_t cp = 0;
            const int n = utf8_decode_n(s.c_str() + i, s.size() - i, &cp);
            i += size_t(n < 1 ? 1 : n);
            const Glyph& g = glyph(*f, cp);
            const int gx = (pen >> 6) + g.left;
            const int gy = baseline - g.top;
            if (g.w > 0 && g.h > 0) {
                const bool inside = !use_clip
                    || (float(gx + g.w) > clip.x && float(gx) < clip.right()
                        && float(gy + g.h) > clip.y && float(gy) < clip.bottom());
                if (inside) {
                    for (int r = 0; r < g.h; ++r) {
                        const int py = gy + r;
                        if (use_clip && (float(py) < clip.y || float(py) >= clip.bottom())) continue;
                        const unsigned char* row = &g.px[size_t(r) * size_t(g.w)];
                        for (int q = 0; q < g.w; ++q) {
                            const unsigned char a = row[q];
                            if (!a) continue;
                            const int px = gx + q;
                            if (use_clip && (float(px) < clip.x || float(px) >= clip.right())) continue;
                            cv.blend(px, py, c, float(a) * (1.0f / 255.0f));
                        }
                    }
                }
            }
            pen += g.adv;
        }
        if (x11_trace_on())
            x11_trace("  ft_draw \"%s\" size=%.1f weight=%d asc=%d baseline=%d x=%.1f",
                      s.c_str(), size, weight, f->asc, baseline, x);
        return true;
    }

    void release_fonts() {
        FtApi& ft = ft_api();
        if (ft.lib) {
            for (auto& kv : faces_) if (kv.second.face) ft.done_face(kv.second.face);
        }
        fonts_.clear(); faces_.clear(); charsets_.clear(); face_reap_ = 0; glyph_bytes_ = 0; glyph_count_ = 0;
        uncached_glyph_ = Glyph{};
    }

    // 核心 XLFD 字体那条老路：1bit 点阵、无抗锯齿、字重拉不开。
    // 只在 FreeType 加载不到（极简容器）时才会走到这里。
#ifdef AINO_PLATFORM_X11
    void core_draw(Canvas& cv, const std::string& utf8, float x, float y_top,
                   float size, int weight, const Color& c, const Rect& clip) {
        if (!dpy_ || utf8.empty()) return;
        XFontSet fs = fontset(size);
        if (!fs) return;
        ensure_scratch();
        if (!gc1_ || !pm1_) return;
        (void)weight;

        XFontSetExtents* ex = XExtentsOfFontSet(fs);
        if (!ex) return;
        const int asc = int(-ex->max_logical_extent.y + 0.5f);
        int desc = int(ex->max_logical_extent.height + 0.5f) - asc;
        if (asc <= 0) return;
        if (desc <= 0) desc = asc / 3;

        XRectangle ink{}, logical{};
        Xutf8TextExtents(fs, utf8.c_str(), int(utf8.size()), &ink, &logical);
        const int tw = int(logical.width) + 2;
        const int th = asc + desc + 2;
        x11_trace("core draw \"%s\" tw=%d th=%d asc=%d desc=%d", utf8.c_str(), tw, th, asc, desc);
        if (tw <= 2) return;

        Pixmap pm = XCreatePixmap(dpy_, pm1_, unsigned(tw), unsigned(th), 1);
        if (!pm) return;
        XSetForeground(dpy_, gc1_, 0);
        XFillRectangle(dpy_, pm, gc1_, 0, 0, unsigned(tw), unsigned(th));
        XSetForeground(dpy_, gc1_, 1);
        Xutf8DrawString(dpy_, pm, fs, gc1_, 1, 1 + asc, utf8.c_str(), int(utf8.size()));

        XImage* im = XGetImage(dpy_, pm, 0, 0, unsigned(tw), unsigned(th), AllPlanes, ZPixmap);
        if (im) {
            // 1bit Pixmap 里 bit=1 的像素就是字形覆盖到的地方（core X 字体没有
            // 灰度，所以覆盖率只有 0/1 —— 这也是"点阵"的由来）。
            const int ox = int(x) - 1;      // pixmap 第 0 列对应 x-1
            const int oy = int(y_top) - 1;
            const bool use_clip = clip.w >= 0.0f;
            for (int gy = 0; gy < th; ++gy) {
                const int py = oy + gy;
                if (use_clip && (float(py) < clip.y || float(py) >= clip.bottom())) continue;
                for (int gx = 0; gx < tw; ++gx) {
                    if (!XGetPixel(im, gx, gy)) continue;
                    const int px = ox + gx;
                    if (use_clip && (float(px) < clip.x || float(px) >= clip.right())) continue;
                    cv.blend(px, py, c, 1.0f);   // 内部还会再过一遍 Canvas 自己的裁剪区
                }
            }
            XDestroyImage(im);
        }
        XFreePixmap(dpy_, pm);
    }

    // GC 的深度必须与被画 drawable 的深度一致，所以往 1bit Pixmap 上画不能复用
    // 窗口那份 24bit 的 GC。这里常驻一个 1×1 的 1bit pixmap 专门用来建那个 GC。
    void ensure_scratch() {
        if (gc1_ || !dpy_ || !target_) return;
        pm1_ = XCreatePixmap(dpy_, target_, 1, 1, 1);
        if (pm1_) gc1_ = XCreateGC(dpy_, pm1_, 0, nullptr);
    }

    // 使用固定字符串测量候选 XFontSet，排除尺寸退化的点阵字体。
    bool plausible(XFontSet fs, float size) {
        if (!fs) return false;
        XRectangle ink{}, logical{};
        Xutf8TextExtents(fs, "Hg", 2, &ink, &logical);
        // 正常字体 "Hg" 在 size 号字下大约是 1.1~1.3 倍 size 宽；
        // 门槛取 0.7 倍，只用来挡"退化字体"，不参与排版判断。
        return float(logical.width) >= size * 0.7f;
    }

    XFontSet fontset(float size) {
        const int q = int(size * 2.0f + 0.5f);
        auto it = sets_.find(q);
        if (it != sets_.end()) return it->second;
        if (!dpy_) return nullptr;
        const int px = int(size + 0.5f);

        // XLFD 第 7 个字段为 pixel_size，第 8 个为 point_size；此处按像素字号匹配。
        const char* tmpl[] = {
            "-*-%s-*-*-*-*-%d-*-*-*-*-*-*-*",     // 指定族名 + 像素尺寸
            "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*",      // 任意族名 + 像素尺寸
            "fixed",                              // 经典别名，几乎所有 X 都有
            "-*-fixed-*-*-*-*-%d-*-*-*-*-*-*-*",  // 指定 fixed 族
            "*",                                  // 兜底
        };

        XFontSet first = nullptr;
        XFontSet chosen = nullptr;
        for (const char* t : tmpl) {
            char cand[512];
            snprintf(cand, sizeof(cand), t, family_.c_str(), px);
            char** missing = nullptr;
            int nmissing = 0;
            char* dict = nullptr;
            if (x11_trace_on()) x11_trace("fontset(%d) try: %s", px, cand);
            XFontSet fs = XCreateFontSet(dpy_, cand, &missing, &nmissing, &dict);
            if (missing) XFreeStringList(missing);
            // dict 故意不 free：Xlib 文档说要用 XFree 释放，但不同版本的实现里
            // 它未必是 malloc 出来的（CI 上就报 free(): invalid pointer）。
            // 代价是每个字号泄漏一个短字符串，量级可以忽略。
            (void)dict;
            if (!fs) {
                if (x11_trace_on()) x11_trace("fontset(%d)   -> null", px);
                continue;
            }
            const bool ok = plausible(fs, size);
            if (x11_trace_on()) {
                XRectangle ink2{}, lg2{};
                Xutf8TextExtents(fs, "Hg", 2, &ink2, &lg2);
                x11_trace("fontset(%d)   -> %p width(Hg)=%d plausible=%d", px, (void*)fs,
                          int(lg2.width), ok ? 1 : 0);
            }
            if (!first) first = fs;
            if (ok) { chosen = fs; break; }
            // 不合适的立刻释放，免得攒一堆字体集
            if (fs) XFreeFontSet(dpy_, fs);
        }
        // 所有候选都不像字 —— 留第一个能用的（总比完全没有强），
        // 但 measure() 那边还会再兜一层估算，避免布局塌成零宽。
        if (!chosen) chosen = first;
        sets_[q] = chosen;
        if (x11_trace_on()) x11_trace("fontset(%d) done -> %p", px, (void*)chosen);
        return chosen;
    }

    Display* dpy_ = nullptr;
    Drawable target_ = 0;      // 窗口：只用来创建 Pixmap
    Pixmap pm1_ = 0;           // 1×1 depth 1 —— 只为建下面那个 GC
    GC gc1_ = nullptr;         // depth 1 的 GC
#endif
    bool ft_ready_ = false;    // FreeType + fontconfig 都加载成功
    bool missing_font_reported_ = false;
    std::string family_;       // 核心字体的 XLFD 家族（空 = "*"）
    std::string sans_;         // fontconfig 家族名（空 = "sans"）
    std::string mono_;
#ifdef AINO_PLATFORM_X11
    std::unordered_map<int, XFontSet> sets_;
#endif
    std::unordered_map<int, Font> fonts_;
    struct SharedFace { void* face = nullptr; long size = 0; uint64_t touched = 0; };
    std::map<std::pair<std::string, long>, SharedFace> faces_;
    std::map<std::pair<std::string, long>, std::shared_ptr<void>> charsets_;
    uint64_t face_reap_ = 0;
    size_t glyph_bytes_ = 0;
    size_t glyph_count_ = 0;
    Glyph uncached_glyph_;
};


#ifdef AINO_PLATFORM_X11
struct X11State {
    Window::Impl* owner = nullptr;
    Display* dpy = nullptr;
    ::Window win = 0;
    GC gc = nullptr;
    XImage* img = nullptr;
    std::vector<uint8_t> buffer;
    Atom wm_delete = 0;
    XIM im = nullptr;
    XIC ic = nullptr;
    XFontSet im_font = nullptr;
    bool im_position = false, im_focused = false, window_focused = false;
    XPoint im_spot{-1, -1};
    int pw = 0, ph = 0;
    ::Window settings_owner = 0;
    Atom settings_selection = 0, settings_property = 0, resource_manager = 0, manager = 0;
    WindowConfig config;

    std::string clipboard_text, clipboard_received;
    bool clipboard_waiting = false, clipboard_done = false, clipboard_ok = false, clipboard_incr = false;
    Atom clipboard_property = 0, clipboard_selection = 0, clipboard_utf8 = 0, clipboard_increment = 0;
    struct ClipboardSend {
        ::Window window; Atom property, type; std::string text; size_t offset = 0;
        std::chrono::steady_clock::time_point updated = std::chrono::steady_clock::now();
    };
    std::vector<ClipboardSend> clipboard_sends;
    ::Cursor cursors[6] = {};
    unsigned cursor_shape = 0;

    float mx = -1e6f, my = -1e6f;
    bool  btn_down[3] = {false, false, false};
    bool  btn_pressed[3] = {false, false, false};
    bool  btn_released[3] = {false, false, false};
    float wheel = 0.0f;
    int   clicks = 0;
    std::string text_in;
    bool  key_down[KEY_COUNT] = {};
    bool  key_pressed[KEY_COUNT] = {};
    bool  key_ctrl[KEY_COUNT]  = {};
    bool  key_shift[KEY_COUNT] = {};
    bool  key_alt[KEY_COUNT]   = {};
};

inline std::vector<X11State*>& x11_clipboard_windows() {
    thread_local std::vector<X11State*> windows;
    return windows;
}

// 外部选区 requestor 可以随时销毁。仅在这段同步事务中捕获其 BadWindow，
// 其它 Display / Thread 的错误继续交回宿主处理器；退出后恢复全局 Xlib handler。
class X11SelectionErrors {
    Display* display_;
    std::unique_lock<std::mutex> lock_;
    static std::mutex& mutex() { static std::mutex value; return value; }
    static std::atomic<XErrorHandler>& previous() { static std::atomic<XErrorHandler> value{nullptr}; return value; }
    static X11SelectionErrors*& current() { thread_local X11SelectionErrors* value = nullptr; return value; }
    static int handle(Display* display, XErrorEvent* error) {
        auto* scope = current();
        if (scope && scope->display_ == display && (error->error_code == BadWindow || error->error_code == BadAtom
            || error->error_code == BadMatch)) return 0;
        auto handler = previous().load(); return handler ? handler(display, error) : 0;
    }
public:
    explicit X11SelectionErrors(Display* display) : display_(display), lock_(mutex()) {
        XSync(display_, False);
        previous().store(XSetErrorHandler(handle)); current() = this;
    }
    ~X11SelectionErrors() {
        XSync(display_, False); current() = nullptr; XSetErrorHandler(previous().load());
    }
};

inline void x11_clipboard_atoms(X11State* s) {
    if (s->clipboard_selection) return;
    s->clipboard_selection = XInternAtom(s->dpy, "CLIPBOARD", False);
    s->clipboard_utf8 = XInternAtom(s->dpy, "UTF8_STRING", False);
    s->clipboard_increment = XInternAtom(s->dpy, "INCR", False);
    s->clipboard_property = XInternAtom(s->dpy, "AINO_CLIPBOARD", False);
}
inline Bool x11_clipboard_filter(Display*, XEvent* e, XPointer arg) {
    auto* s = reinterpret_cast<X11State*>(arg);
    if (e->type == SelectionRequest || e->type == SelectionNotify || e->type == SelectionClear) return True;
    if (e->type != PropertyNotify) return False;
    if (e->xproperty.window == s->win && e->xproperty.atom == s->clipboard_property) return True;
    for (const auto& send : s->clipboard_sends)
        if (send.window == e->xproperty.window && send.property == e->xproperty.atom) return True;
    return False;
}
inline bool x11_clipboard_event(X11State* s, XEvent& event) {
    if (event.type == DestroyNotify && event.xdestroywindow.window != s->win) {
        const auto count = s->clipboard_sends.size();
        s->clipboard_sends.erase(std::remove_if(s->clipboard_sends.begin(), s->clipboard_sends.end(),
            [&](const X11State::ClipboardSend& send) { return send.window == event.xdestroywindow.window; }), s->clipboard_sends.end());
        if (count != s->clipboard_sends.size()) return true;
    }
    if (event.type != SelectionRequest && event.type != SelectionNotify
        && event.type != SelectionClear && event.type != PropertyNotify) return false;
    x11_clipboard_atoms(s);
    constexpr size_t limit = 32u << 20, chunk = 64u << 10;
    const Atom selection = s->clipboard_selection, utf8 = s->clipboard_utf8, incr = s->clipboard_increment;
    if (event.type == SelectionRequest) {
        const auto& request = event.xselectionrequest;
        if (request.selection != selection) return false;
        X11SelectionErrors errors(s->dpy);
        const auto now = std::chrono::steady_clock::now();
        s->clipboard_sends.erase(std::remove_if(s->clipboard_sends.begin(), s->clipboard_sends.end(),
            [&](const X11State::ClipboardSend& send) { return now - send.updated > std::chrono::seconds(30); }), s->clipboard_sends.end());
        XEvent reply{};
        reply.xselection.type = SelectionNotify; reply.xselection.display = request.display;
        reply.xselection.requestor = request.requestor; reply.xselection.selection = request.selection;
        reply.xselection.target = request.target; reply.xselection.time = request.time;
        const Atom property = request.property ? request.property : request.target;
        const Atom targets = XInternAtom(s->dpy, "TARGETS", False), text = XInternAtom(s->dpy, "TEXT", False);
        if (request.target == targets) {
            const Atom supported[] = {targets, utf8, text};
            XChangeProperty(s->dpy, request.requestor, property, XA_ATOM, 32, PropModeReplace,
                reinterpret_cast<const unsigned char*>(supported), 3);
            reply.xselection.property = property;
        } else if (request.target == utf8 || request.target == text) {
            if (s->clipboard_text.size() <= chunk) {
                XChangeProperty(s->dpy, request.requestor, property, utf8, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(s->clipboard_text.data()), int(s->clipboard_text.size()));
                reply.xselection.property = property;
            } else {
                size_t bytes = s->clipboard_text.size();
                for (const auto& send : s->clipboard_sends) bytes += send.text.size();
                XWindowAttributes attributes{};
                if (bytes <= limit && s->clipboard_sends.size() < 8
                    && XGetWindowAttributes(s->dpy, request.requestor, &attributes)) {
                    XSelectInput(s->dpy, request.requestor, attributes.your_event_mask | PropertyChangeMask | StructureNotifyMask);
                    s->clipboard_sends.push_back({request.requestor, property, utf8, s->clipboard_text, 0});
                    const unsigned long length = static_cast<unsigned long>(s->clipboard_text.size());
                    XChangeProperty(s->dpy, request.requestor, property, incr, 32, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(&length), 1);
                    reply.xselection.property = property;
                }
            }
        }
        XSendEvent(s->dpy, request.requestor, False, 0, &reply); XFlush(s->dpy);
        return true;
    }
    if (event.type == SelectionClear && event.xselectionclear.selection == selection) {
        s->clipboard_text.clear(); return true;
    }
    if (event.type == PropertyNotify && event.xproperty.state == PropertyDelete) {
        for (auto it = s->clipboard_sends.begin(); it != s->clipboard_sends.end(); ++it) {
            if (it->window != event.xproperty.window || it->property != event.xproperty.atom) continue;
            X11SelectionErrors errors(s->dpy);
            it->updated = std::chrono::steady_clock::now();
            const size_t size = std::min(chunk, it->text.size() - it->offset);
            XChangeProperty(s->dpy, it->window, it->property, it->type, 8, PropModeReplace,
                reinterpret_cast<const unsigned char*>(it->text.data() + it->offset), int(size));
            it->offset += size;
            if (!size) s->clipboard_sends.erase(it);
            XFlush(s->dpy); return true;
        }
    }
    const bool notify = event.type == SelectionNotify && s->clipboard_waiting
        && event.xselection.requestor == s->win && event.xselection.selection == selection;
    const bool next = event.type == PropertyNotify && s->clipboard_waiting && s->clipboard_incr
        && event.xproperty.window == s->win && event.xproperty.atom == s->clipboard_property
        && event.xproperty.state == PropertyNewValue;
    if (!notify && !next) return false;
    if (notify && !event.xselection.property) { s->clipboard_done = true; return true; }
    Atom type = 0; int format = 0;
    unsigned long count = 0, remaining = 0;
    unsigned char* data = nullptr;
    const int rc = XGetWindowProperty(s->dpy, s->win, s->clipboard_property, 0, long(limit / 4),
        True, AnyPropertyType, &type, &format, &count, &remaining, &data);
    ScopeExit release([&] { if (data) XFree(data); });
    if (rc != Success || remaining || (type != incr && (format != 8 || type != utf8))) {
        s->clipboard_done = true; return true;
    }
    if (type == incr) {
        if (format != 32 || count != 1 || !data || *reinterpret_cast<unsigned long*>(data) > limit) {
            s->clipboard_done = true; return true;
        }
        s->clipboard_incr = true;
        XDeleteProperty(s->dpy, s->win, s->clipboard_property);
    } else if (count <= limit - s->clipboard_received.size()) {
        if (count) s->clipboard_received.append(reinterpret_cast<const char*>(data), size_t(count));
        if (!s->clipboard_incr || !count) { s->clipboard_ok = true; s->clipboard_done = true; }
    } else s->clipboard_done = true;
    XFlush(s->dpy); return true;
}

// 输入法坐标始终是物理客户区坐标，随输入框滚动和 DPI 更新。
inline void x11_set_ime_pos(X11State* s, float caret_x, float top, float bottom) {
    (void)top;
    if (!s || !s->ic) return;
    if (s->window_focused && !s->im_focused) { XSetICFocus(s->ic); s->im_focused = true; }
    const XPoint spot{short(std::max(-32768.0f, std::min(32767.0f, caret_x))),
                      short(std::max(-32768.0f, std::min(32767.0f, bottom)))};
    if (s->im_position && (spot.x != s->im_spot.x || spot.y != s->im_spot.y)) {
        auto attrs = XVaCreateNestedList(0, XNSpotLocation, &spot, nullptr);
        if (attrs) {
            XSetICValues(s->ic, XNPreeditAttributes, attrs, nullptr);
            XFree(attrs); s->im_spot = spot; XFlush(s->dpy);
        }
    }
}

inline int x11_map_keysym(KeySym ks) {
    if (ks >= XK_a && ks <= XK_z) return int('A' + (ks - XK_a));
    if (ks >= XK_A && ks <= XK_Z) return int(ks);
    if (ks >= XK_0 && ks <= XK_9) return int(ks);
    switch (ks) {
        case XK_BackSpace: return KEY_BACKSPACE;
        case XK_Tab:       return KEY_TAB;
        case XK_Return:    return KEY_ENTER;
        case XK_Escape:    return KEY_ESC;
        case XK_space:     return KEY_SPACE;
        case XK_Delete:    return KEY_DELETE;
        case XK_Left:      return KEY_LEFT;
        case XK_Right:     return KEY_RIGHT;
        case XK_Up:        return KEY_UP;
        case XK_Down:      return KEY_DOWN;
        case XK_Home:      return KEY_HOME;
        case XK_End:       return KEY_END;
        case XK_Prior:     return KEY_PAGEUP;
        case XK_Next:      return KEY_PAGEDOWN;
        case XK_Shift_L: case XK_Shift_R: return KEY_SHIFT;
        case XK_Control_L: case XK_Control_R: return KEY_CTRL;
        case XK_Alt_L: case XK_Alt_R: return KEY_ALT;
        case XK_Super_L: case XK_Super_R: return KEY_SUPER;
        case XK_F1: return KEY_F1; case XK_F2: return KEY_F2; case XK_F3: return KEY_F3;
        case XK_F4: return KEY_F4; case XK_F5: return KEY_F5; case XK_F6: return KEY_F6;
        case XK_F7: return KEY_F7; case XK_F8: return KEY_F8; case XK_F9: return KEY_F9;
        case XK_F10: return KEY_F10; case XK_F11: return KEY_F11; case XK_F12: return KEY_F12;
        default: return 0;
    }
}

inline void x11_create_backbuffer(X11State* s, int w, int h) {
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384 || uint64_t(w) * h > 16777216u) return;
    if (s->pw == w && s->ph == h && s->img) return;

    XWindowAttributes wa{};
    if (!XGetWindowAttributes(s->dpy, s->win, &wa)) return;
    std::vector<uint8_t> buffer(size_t(w) * size_t(h) * 4u, 0);
    XImage* image = XCreateImage(s->dpy, wa.visual, unsigned(wa.depth), ZPixmap, 0,
                          reinterpret_cast<char*>(buffer.data()), unsigned(w), unsigned(h),
                          32, w * 4);
    if (!image) return;
    if (s->img) { s->img->data = nullptr; XDestroyImage(s->img); }
    s->buffer.swap(buffer);
    s->img = image;
    s->pw = w; s->ph = h;

    Window::Impl* o = s->owner;
    o->canvas.px = s->buffer.data();
    o->canvas.w = w; o->canvas.h = h;
    o->canvas.stride = w * 4;
    o->canvas.platform_dc = s->dpy;
    o->canvas.platform_gc = s->gc;
    o->canvas.clear_clip();
    o->client_w = w; o->client_h = h;
}

inline void x11_present(X11State* s) {
    if (!s->img) return;
    XPutImage(s->dpy, s->win, s->gc, s->img, 0, 0, 0, 0, unsigned(s->pw), unsigned(s->ph));
    XFlush(s->dpy);
}

inline void x11_apply_cursor(X11State* s) {
    if (!s->owner || !s->win) return;
    unsigned shape = XC_left_ptr;
    switch (s->owner->cursor) {
        case Cursor::Hand:    shape = XC_hand2; break;
        case Cursor::Text:
        case Cursor::IBeam:   shape = XC_xterm; break;
        case Cursor::ResizeH: shape = XC_sb_h_double_arrow; break;
        case Cursor::ResizeV: shape = XC_sb_v_double_arrow; break;
        default:              shape = XC_left_ptr; break;
    }
    const int index = std::max(0, std::min(5, int(s->owner->cursor)));
    if (s->cursor_shape == shape && s->cursors[index]) return;
    if (!s->cursors[index]) s->cursors[index] = XCreateFontCursor(s->dpy, shape);
    if (s->cursors[index]) { XDefineCursor(s->dpy, s->win, s->cursors[index]); s->cursor_shape = shape; }
}

inline void x11_feed_input(X11State* s) {
    Window::Impl* o = s->owner;
    Ui::Impl* u = o->ui.impl;
    Input& in = u->input;
    const Vec2 prev = in.mouse;
    in.mouse = Vec2(s->mx / o->scale, s->my / o->scale);
    in.mouse_prev = prev;
    in.mouse_delta = Vec2(in.mouse.x - prev.x, in.mouse.y - prev.y);
    for (int i = 0; i < 3; ++i) { in.pressed[i] = s->btn_pressed[i]; in.released[i] = s->btn_released[i]; in.down[i] = s->btn_down[i]; }
    in.wheel = s->wheel;
    in.double_click = (s->clicks >= 2);
    in.click_count = s->clicks;
    in.text_input = s->text_in;
    for (int i = 0; i < KEY_COUNT; ++i) {
        in.key_down[i] = s->key_down[i]; in.key_pressed[i] = s->key_pressed[i];
        in.key_ctrl[i] = s->key_ctrl[i]; in.key_shift[i] = s->key_shift[i]; in.key_alt[i] = s->key_alt[i];
    }
    in.dragging = s->btn_down[0];
    for (int i = 0; i < 3; ++i) { s->btn_pressed[i] = false; s->btn_released[i] = false; }
    s->wheel = 0.0f; s->clicks = 0; s->text_in.clear();
    for (int i = 0; i < KEY_COUNT; ++i) s->key_pressed[i] = false;
}

inline std::vector<unsigned char> x11_property(Display* dpy, ::Window win, Atom name, Atom expected) {
    Atom actual = 0; int format = 0; unsigned long count = 0, remaining = 0;
    unsigned char* data = nullptr;
    std::vector<unsigned char> out;
    if (XGetWindowProperty(dpy, win, name, 0, 262144, False, expected, &actual, &format, &count, &remaining, &data) == Success &&
        actual == expected && format == 8 && !remaining && data) out.assign(data, data + count);
    if (data) XFree(data);
    return out;
}
inline float x11_desktop_scale(X11State* s) {
    if (!s->config.high_dpi) return 1.0f;
    const auto root = DefaultRootWindow(s->dpy);
    // 读取 owner 和订阅事件期间避免设置管理器恰好销毁窗口。
    XGrabServer(s->dpy);
    s->settings_owner = XGetSelectionOwner(s->dpy, s->settings_selection);
    std::vector<unsigned char> bytes;
    if (s->settings_owner) {
        XSelectInput(s->dpy, s->settings_owner, PropertyChangeMask | StructureNotifyMask);
        bytes = x11_property(s->dpy, s->settings_owner, s->settings_property, s->settings_property);
    }
    XUngrabServer(s->dpy);
    XFlush(s->dpy);
    const auto settings = parse_xsettings_scale(bytes.data(), bytes.size());
    x11_icon_theme() = settings.icon_theme;
    const auto resource = x11_property(s->dpy, root, s->resource_manager, XA_STRING);
    double dpi = 0;
    if (!resource.empty()) {
        const std::string text(resource.begin(), resource.end());
        XrmInitialize();
        if (auto database = XrmGetStringDatabase(text.c_str())) {
            XrmValue value{}; char* type = nullptr;
            if (XrmGetResource(database, "Xft.dpi", "Xft.Dpi", &type, &value) && value.addr) {
                const std::string number(value.addr, value.size ? value.size - 1 : 0);
                dpi = parse_scale_number(number.c_str(), 48, 384);
            }
            XrmDestroyDatabase(database);
        }
    }
    return desktop_scale(std::getenv("AINO_SCALE"), std::getenv("GDK_SCALE"), settings, dpi);
}
inline void x11_size_hints(X11State* s, int width, int height) {
    XSizeHints hints{};
    const auto& cfg = s->config;
    const auto sc = s->owner->scale;
    const int maxw = std::max(1, int(DisplayWidth(s->dpy, DefaultScreen(s->dpy)) * .96));
    const int maxh = std::max(1, int(DisplayHeight(s->dpy, DefaultScreen(s->dpy)) * .92));
    if (!cfg.resizable) {
        hints.flags = PMinSize | PMaxSize;
        hints.min_width = hints.max_width = width; hints.min_height = hints.max_height = height;
    } else {
        hints.flags = PMinSize;
        hints.min_width = std::min(maxw, std::max(1, int(cfg.min_width * sc + .5f)));
        hints.min_height = std::min(maxh, std::max(1, int(cfg.min_height * sc + .5f)));
        if (cfg.max_width > 0 || cfg.max_height > 0) {
            hints.flags |= PMaxSize;
            hints.max_width = cfg.max_width > 0 ? std::max(hints.min_width, int(cfg.max_width * sc + .5f)) : 32767;
            hints.max_height = cfg.max_height > 0 ? std::max(hints.min_height, int(cfg.max_height * sc + .5f)) : 32767;
        }
    }
    XSetWMNormalHints(s->dpy, s->win, &hints);
}
inline void x11_refresh_scale(X11State* s) {
    auto* o = s->owner;
    const float scale = x11_desktop_scale(s);
    if (std::fabs(o->scale - scale) < .001f) return;
    const int w = std::max(1, std::min(int(s->pw / o->scale * scale + .5f), int(DisplayWidth(s->dpy, DefaultScreen(s->dpy)) * .96)));
    const int h = std::max(1, std::min(int(s->ph / o->scale * scale + .5f), int(DisplayHeight(s->dpy, DefaultScreen(s->dpy)) * .92)));
    o->scale = scale;
    // 新字号重新生成位图，释放旧 DPI 下的字体和字形缓存。
    if (o->text) {
        o->text->release(); o->text->init(scale, o->ui.theme().font_family, o->ui.theme().font_family_mono);
        static_cast<UnixTextRenderer*>(o->text)->set_drawable(s->dpy, s->win, s->gc);
    }
    x11_size_hints(s, w, h);
    XResizeWindow(s->dpy, s->win, unsigned(w), unsigned(h));
    o->need_frame = true;
}

inline bool x11_handle(X11State* s, XEvent& e) {
    if (x11_clipboard_event(s, e)) return true;
    Window::Impl* o = s->owner;
    // XIM 先消费组字按键及协议事件，避免拼音同时落入正文和快捷键。
    if (XFilterEvent(&e, None)) { o->need_frame = true; return true; }
    switch (e.type) {
    case Expose:
        o->need_frame = true;
        break;
    case ConfigureNotify:
        if (e.xconfigure.window != s->win) break;
        x11_create_backbuffer(s, e.xconfigure.width, e.xconfigure.height);
        o->need_frame = true;
        break;
    case MotionNotify:
        s->mx = float(e.xmotion.x); s->my = float(e.xmotion.y);
        // 与 Windows 后端同一条规则：悬停目标没变就不排帧。
        if (!o->need_frame && o->hover_skip &&
            !o->ui.impl->hover_would_change(s->mx / o->scale, s->my / o->scale)) break;
        o->need_frame = true;
        break;
    case ButtonPress: {
        s->mx = float(e.xbutton.x); s->my = float(e.xbutton.y);
        if (e.xbutton.button == Button4) { s->wheel += 1.0f; o->need_frame = true; break; }
        if (e.xbutton.button == Button5) { s->wheel -= 1.0f; o->need_frame = true; break; }
        int idx = (e.xbutton.button == Button1) ? 0 : (e.xbutton.button == Button3 ? 1 : 2);
        s->btn_down[idx] = true; s->btn_pressed[idx] = true;
        o->need_frame = true;
        break;
    }
    case ButtonRelease: {
        s->mx = float(e.xbutton.x); s->my = float(e.xbutton.y);
        int idx = (e.xbutton.button == Button1) ? 0 : (e.xbutton.button == Button3 ? 1 : 2);
        s->btn_down[idx] = false; s->btn_released[idx] = true;
        o->need_frame = true;
        break;
    }
    case KeyPress: {
        KeySym ks = XLookupKeysym(&e.xkey, 0);
        int k = x11_map_keysym(ks);
        if (k > 0 && k < KEY_COUNT) {
            if (!s->key_down[k]) {
                s->key_pressed[k] = true;
                s->key_ctrl[k]  = s->key_down[KEY_CTRL];
                s->key_shift[k] = s->key_down[KEY_SHIFT];
                s->key_alt[k]   = s->key_down[KEY_ALT];
            }
            s->key_down[k] = true;
        }
        if (s->ic) {
            char buf[64];
            Status st = 0;
            KeySym dummy;
            int n = Xutf8LookupString(s->ic, &e.xkey, buf, sizeof(buf), &dummy, &st);
            std::vector<char> overflow;
            const char* text = buf;
            if (st == XBufferOverflow && n > 0 && n <= 65536) {
                overflow.resize(size_t(n));
                n = Xutf8LookupString(s->ic, &e.xkey, overflow.data(), n, &dummy, &st);
                text = overflow.data();
            }
            if (n > 0 && (st == XLookupChars || st == XLookupBoth)) {
                for (int i = 0; i < n; ++i) if (uint8_t(text[i]) >= 32 || text[i] == '\t') s->text_in.push_back(text[i]);
            }
        } else {
            char buf[16];
            int n = XLookupString(&e.xkey, buf, sizeof(buf) - 1, nullptr, nullptr);
            for (int i = 0; i < n; ++i) if (uint8_t(buf[i]) >= 32) s->text_in.push_back(buf[i]);
        }
        o->need_frame = true;
        break;
    }
    case KeyRelease: {
        KeySym ks = XLookupKeysym(&e.xkey, 0);
        int k = x11_map_keysym(ks);
        if (k > 0 && k < KEY_COUNT) s->key_down[k] = false;
        o->need_frame = true;
        break;
    }
    case ClientMessage:
        if (e.xclient.message_type == s->manager && e.xclient.format == 32 &&
            Atom(e.xclient.data.l[1]) == s->settings_selection) {
            x11_refresh_scale(s);
            break;
        }
        if (Atom(e.xclient.data.l[0]) == s->wm_delete) { o->running = false; return false; }
        break;
    case PropertyNotify:
        if ((e.xproperty.window == DefaultRootWindow(s->dpy) && e.xproperty.atom == s->resource_manager) ||
            (e.xproperty.window == s->settings_owner && e.xproperty.atom == s->settings_property))
            x11_refresh_scale(s);
        break;
    case DestroyNotify:
        if (e.xdestroywindow.window == s->settings_owner) x11_refresh_scale(s);
        break;
    case LeaveNotify:
        s->mx = -1e6f; s->my = -1e6f;
        o->need_frame = true;
        break;
    case FocusIn:
        s->window_focused = true;
        if (s->ic && o->ui.impl->ime_owner && !s->im_focused) { XSetICFocus(s->ic); s->im_focused = true; }
        o->need_frame = true;
        break;
    case FocusOut:
        s->window_focused = false;
        if (s->ic && s->im_focused) { XUnsetICFocus(s->ic); s->im_focused = false; }
        for (int i = 0; i < 3; ++i) s->btn_down[i] = s->btn_pressed[i] = s->btn_released[i] = false;
        for (int i = 0; i < KEY_COUNT; ++i) s->key_down[i] = false;
        o->ui.impl->active = 0;
        o->need_frame = true;
        break;
    default: break;
    }
    return true;
}

// 设置窗口标题。
//
// ICCCM 规定 WM_NAME 是 ISO-8859-1，XStoreName 塞进去的 UTF-8 字节会被 GTK/GNOME/
// KDE 当成 latin1 **逐字节**解码 —— 标题栏里就成了 "æœ€å°ç¤ºä¾‹"，中间那些落进
// U+0080..U+009F 的字节还会被画成带十六进制的方框。
// 现代窗口管理器读的是 EWMH 的 _NET_WM_NAME，而且类型必须是 UTF8_STRING，所以两个
// 都要设：_NET_WM_NAME 给现代桌面，WM_NAME 留作兜底（只认它的老 WM）。
// 兜底那份仍写 UTF-8 字节：中文本来就没有 latin1 表示，转不过去，而现代 WM 都以
// _NET_WM_NAME 为准，不会看它。
inline void x11_set_title(Display* dpy, ::Window win, const std::string& utf8) {
    if (!dpy || !win) return;
    const char* p = utf8.empty() ? "aino" : utf8.c_str();
    const size_t n = utf8.empty() ? 4u : utf8.size();
    const Atom net_name = XInternAtom(dpy, "_NET_WM_NAME", False);
    const Atom net_icon = XInternAtom(dpy, "_NET_WM_ICON_NAME", False);
    const Atom utf8_atom = XInternAtom(dpy, "UTF8_STRING", False);
    XChangeProperty(dpy, win, net_name, utf8_atom, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(p), int(n));
    XChangeProperty(dpy, win, net_icon, utf8_atom, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(p), int(n));
    XStoreName(dpy, win, p);
}


inline void x11_create(Window* self, const WindowConfig& cfg) {
    Window::Impl* o = self->impl;
    X11State* s = new X11State();
    s->config = cfg;
    s->owner = o;
    o->native = s;

    setlocale(LC_ALL, "");
    s->dpy = XOpenDisplay(nullptr);
    if (!s->dpy) {
        // 上一次 XCloseDisplay 之后服务器可能还在收尾，紧接着的 connect 会拿到
        // EAGAIN（11）—— 不是"连不上"，是"现在别连"。0.4.7 把兜底链的扫描缩短
        // 之后，连续建窗口的间隔从几十毫秒掉到几毫秒，CI 上就撞出来了：
        // ci_pixels 的第四个窗口（S4）稳定 XOpenDisplay 返回 NULL、errno=11。
        // 退避重试，别在这里判死刑 —— 真实桌面上重建窗口也会走这条路。
        // 递增间隔（25/50/75…封顶 250ms），累计约 1.4s：真连不上时这个等待毫无
        // 代价（本来就什么都做不了），能救回来的情况都救回来了。
        for (int i = 0; i < 10 && !s->dpy; ++i) {
            int ms = 25 * (i + 1);
            if (ms > 250) ms = 250;
            usleep(useconds_t(ms) * 1000);
            s->dpy = XOpenDisplay(nullptr);
            if (x11_trace_on())
                x11_trace("XOpenDisplay retry %d -> %p errno=%d", i + 1,
                          reinterpret_cast<void*>(s->dpy), errno);
        }
    }
    if (!s->dpy) {
        // 无条件打一行：CI 上"窗口创建失败"后面什么都没有，光靠退出码根本猜不到
        // 是 DISPLAY 没设、Xvfb 挂了、还是服务器在重建（0.4.7 前后为这个查了两轮）。
        const char* d = std::getenv("DISPLAY");
        std::fprintf(stderr, "[aino] XOpenDisplay 失败（10 次重试后仍失败）DISPLAY=%s errno=%d\n",
                     d ? d : "(null)", errno);
        std::fflush(stderr);
        delete s;
        o->native = nullptr;
        return;
    }

    int screen = DefaultScreen(s->dpy);
    ::Window root = RootWindow(s->dpy, screen);
    s->settings_selection = XInternAtom(s->dpy, ("_XSETTINGS_S" + std::to_string(screen)).c_str(), False);
    s->settings_property = XInternAtom(s->dpy, "_XSETTINGS_SETTINGS", False);
    s->resource_manager = XInternAtom(s->dpy, "RESOURCE_MANAGER", False);
    s->manager = XInternAtom(s->dpy, "MANAGER", False);
    XSelectInput(s->dpy, root, PropertyChangeMask | StructureNotifyMask);
    o->scale = x11_desktop_scale(s);
    const int scr_w = DisplayWidth(s->dpy, screen);
    const int scr_h = DisplayHeight(s->dpy, screen);
    const int w = std::max(1, std::min(int((cfg.width > 0 ? cfg.width : 880) * o->scale + .5f), int(scr_w * .96)));
    const int h = std::max(1, std::min(int((cfg.height > 0 ? cfg.height : 560) * o->scale + .5f), int(scr_h * .92)));
    const int wx = cfg.center ? (scr_w - w) / 2 : 0;
    const int wy = cfg.center ? (scr_h - h) / 2 : 0;
    s->win = XCreateSimpleWindow(s->dpy, root, wx, wy, unsigned(w), unsigned(h), 0,
                                 BlackPixel(s->dpy, screen), WhitePixel(s->dpy, screen));
    x11_set_title(s->dpy, s->win, cfg.title);
    if (!cfg.icon_path.empty()) {
        Image icon;
        std::string error;
        if (icon.load(cfg.icon_path, &error)) {
            const float scale = std::min(1.f, 128.f / float(std::max(icon.width, icon.height)));
            const int iw = std::max(1, int(icon.width * scale)), ih = std::max(1, int(icon.height * scale));
            std::vector<unsigned long> data(2u + size_t(iw) * size_t(ih));
            data[0] = static_cast<unsigned long>(iw);
            data[1] = static_cast<unsigned long>(ih);
            for (int y = 0; y < ih; ++y) for (int x = 0; x < iw; ++x) {
                const int sx = int(int64_t(x) * icon.width / iw), sy = int(int64_t(y) * icon.height / ih);
                const auto* p = &icon.pixels[(size_t(sy) * size_t(icon.width) + size_t(sx)) * 4u];
                data[2u + size_t(y) * size_t(iw) + size_t(x)] = (static_cast<unsigned long>(p[3]) << 24) | (static_cast<unsigned long>(p[0]) << 16) |
                              (static_cast<unsigned long>(p[1]) << 8) | static_cast<unsigned long>(p[2]);
            }
            const Atom property = XInternAtom(s->dpy, "_NET_WM_ICON", False);
            XChangeProperty(s->dpy, s->win, property, XA_CARDINAL, 32, PropModeReplace,
                            reinterpret_cast<const unsigned char*>(data.data()), int(data.size()));
        } else std::fprintf(stderr, "[aino] %s\n", error.c_str());
    }
    XSelectInput(s->dpy, s->win,
                 PropertyChangeMask | ExposureMask | StructureNotifyMask | PointerMotionMask |
                 ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask |
                 EnterWindowMask | LeaveWindowMask | FocusChangeMask);
    s->wm_delete = XInternAtom(s->dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s->dpy, s->win, &s->wm_delete, 1);
    s->gc = XCreateGC(s->dpy, s->win, 0, nullptr);

    // 尺寸约束。X11 没有"不可缩放"这个位，只能把 min 和 max 设成同一个值 ——
    // 主流 WM 都按这个理解（ICCCM 里没人保证，但这是唯一可移植的做法）。
    x11_size_hints(s, w, h);
    // X11 字体按 cfg.font_family 选择系统已安装的家族。

    // 输入法：中文输入需要 XIM/XIC
    if (XSetLocaleModifiers("") != nullptr) {
        s->im = XOpenIM(s->dpy, nullptr, nullptr, nullptr);
        if (s->im) {
            XIMStyles* styles = nullptr;
            XGetIMValues(s->im, XNQueryInputStyle, &styles, nullptr);
            if (styles) {
                for (unsigned i = 0; i < styles->count_styles; ++i)
                    if (styles->supported_styles[i] == (XIMPreeditPosition | XIMStatusNothing)) s->im_position = true;
                XFree(styles);
            }
            if (s->im_position) {
                char** missing = nullptr; int count = 0; char* fallback = nullptr;
                s->im_font = XCreateFontSet(s->dpy, "fixed", &missing, &count, &fallback);
                if (missing) XFreeStringList(missing);
                if (s->im_font) {
                    XPoint spot{0, 0};
                    auto attrs = XVaCreateNestedList(0, XNSpotLocation, &spot, XNFontSet, s->im_font, nullptr);
                    if (attrs) {
                        s->ic = XCreateIC(s->im, XNInputStyle, XIMPreeditPosition | XIMStatusNothing,
                            XNClientWindow, s->win, XNFocusWindow, s->win, XNPreeditAttributes, attrs, nullptr);
                        XFree(attrs);
                    }
                }
            }
            if (!s->ic) {
                s->im_position = false;
                if (s->im_font) { XFreeFontSet(s->dpy, s->im_font); s->im_font = nullptr; }
                s->ic = XCreateIC(s->im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                    XNClientWindow, s->win, XNFocusWindow, s->win, nullptr);
            }
            if (s->ic) {
                long filter = 0;
                XGetICValues(s->ic, XNFilterEvents, &filter, nullptr);
                XWindowAttributes attributes{};
                if (XGetWindowAttributes(s->dpy, s->win, &attributes))
                    XSelectInput(s->dpy, s->win, attributes.your_event_mask | filter);
            }
        }
    }

    XMapWindow(s->dpy, s->win);
    XFlush(s->dpy);

    UnixTextRenderer* tr = new UnixTextRenderer();
    tr->init(o->scale, o->theme.font_family, o->theme.font_family_mono);
    tr->set_drawable(s->dpy, s->win, s->gc);
    o->text = tr;
    o->ui.impl->text = tr;

    x11_create_backbuffer(s, w, h);
    o->ui.impl->canvas = &o->canvas;
    o->need_frame = true;
    x11_clipboard_windows().push_back(s);
}

inline void x11_destroy(Window* self) {
    Window::Impl* o = self->impl;
    X11State* s = reinterpret_cast<X11State*>(o->native);
    if (!s) return;
    auto& windows = x11_clipboard_windows();
    windows.erase(std::remove(windows.begin(), windows.end(), s), windows.end());
    // 文本渲染器持有此 Display 的 XFontSet / GC / Pixmap，必须在关闭连接前释放。
    if (o->text) o->text->release();
    if (s->img) { s->img->data = nullptr; XDestroyImage(s->img); s->img = nullptr; }
    if (s->ic) { XDestroyIC(s->ic); s->ic = nullptr; }
    if (s->im) { XCloseIM(s->im); s->im = nullptr; }
    if (s->im_font) { XFreeFontSet(s->dpy, s->im_font); s->im_font = nullptr; }
    if (s->gc) { XFreeGC(s->dpy, s->gc); s->gc = nullptr; }
    for (auto cursor : s->cursors) if (cursor) XFreeCursor(s->dpy, cursor);
    if (s->win) XDestroyWindow(s->dpy, s->win);
    if (s->dpy) XCloseDisplay(s->dpy);
    delete s;
    o->native = nullptr;
}

inline uint64_t x11_ticks_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return uint64_t(ts.tv_sec) * 1000u + uint64_t(ts.tv_nsec / 1000000);
}

} // namespace detail

bool Window::poll() {
    Window::Impl* o = impl;
    detail::X11State* s = reinterpret_cast<detail::X11State*>(o->native);
    if (!s || !o->running) return false;
    detail::service_clipboard(impl);
    if (o->wake_signal.consume()) o->need_frame = true;
    while (XPending(s->dpy)) {
        XEvent e;
        XNextEvent(s->dpy, &e);
        if (!detail::x11_handle(s, e)) break;
    }
    if (o->running) detail::service_clipboard(impl);
    return o->running;
}

void Window::render() {
    Window::Impl* o = impl;
    detail::X11State* s = reinterpret_cast<detail::X11State*>(o->native);
    if (!s || !s->img) return;

    if (o->ui.impl->frame_open) { o->need_frame = true; return; }
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = uint64_t(ts.tv_sec) * 1000u + uint64_t(ts.tv_nsec / 1000000);
    float dt = o->last_frame_ms ? float(now - o->last_frame_ms) / 1000.0f : (1.0f / 60.0f);
    if (dt > 0.25f) { o->ui.impl->caret_blink += dt - 0.25f; dt = 0.25f; }
    o->last_frame_ms = now;

    detail::x11_feed_input(s);
    o->need_frame = false;   // 同 Windows：帧前清，帧内 repaint() 可再次置位
    detail::ScopeExit frame_cleanup([o] {
        if (o->ui.impl->frame_open) { o->ui.impl->abort_frame(); o->need_frame = true; }
    });
    Ui& ui = o->ui;
    ui.begin_frame(float(s->pw) / o->scale, float(s->ph) / o->scale, dt, o->scale);
    if (o->frame_cb_ptr) o->frame_cb_ptr();
    ui.end_frame();
    if (!ui.impl->ime_owner && s->ic && s->im_focused) { XUnsetICFocus(s->ic); s->im_focused = false; }
    o->cursor = ui.cursor();
    detail::x11_present(s);
    detail::x11_apply_cursor(s);
}


void Window::set_title(const std::string& title) {
    detail::X11State* s = reinterpret_cast<detail::X11State*>(impl->native);
    if (s && s->dpy && s->win) detail::x11_set_title(s->dpy, s->win, title);
}
void Window::set_size(int w, int h) {
    detail::X11State* s = reinterpret_cast<detail::X11State*>(impl->native);
    if (s && s->dpy && s->win) XResizeWindow(s->dpy, s->win, unsigned(std::max(1, int(w * impl->scale + .5f))), unsigned(std::max(1, int(h * impl->scale + .5f))));
}

bool Window::try_set_clipboard(const std::string& text) {
    auto* s = reinterpret_cast<detail::X11State*>(impl->native);
    if (!s || !s->dpy || text.size() > (32u << 20)) return false;
    detail::x11_clipboard_atoms(s);
    s->clipboard_text = text;
    XSetSelectionOwner(s->dpy, s->clipboard_selection, s->win, CurrentTime);
    XFlush(s->dpy);
    return XGetSelectionOwner(s->dpy, s->clipboard_selection) == s->win;
}
std::string Window::clipboard() const {
    auto* s = reinterpret_cast<detail::X11State*>(impl->native);
    if (!s || !s->dpy || s->clipboard_waiting) return {};
    detail::x11_clipboard_atoms(s);
    const auto owner = XGetSelectionOwner(s->dpy, s->clipboard_selection);
    if (!owner) return {};
    if (owner == s->win) return s->clipboard_text;
    s->clipboard_received.clear(); s->clipboard_done = s->clipboard_ok = s->clipboard_incr = false;
    s->clipboard_waiting = true;
    detail::ScopeExit reset([&] {
        s->clipboard_waiting = false; XDeleteProperty(s->dpy, s->win, s->clipboard_property);
    });
    XConvertSelection(s->dpy, s->clipboard_selection, s->clipboard_utf8,
        s->clipboard_property, s->win, CurrentTime);
    XFlush(s->dpy);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!s->clipboard_done && std::chrono::steady_clock::now() < deadline) {
        // 同线程的选区 owner 可能在另一个 Display 连接上；一起服务协议以免互等超时。
        // 帧内不派发 resize 或其它 UI 事件，避免重建正在使用的画布。
        std::vector<pollfd> descriptors;
        bool saturated = false;
        for (auto* window : detail::x11_clipboard_windows()) {
            XEvent event{};
            int handled = 0;
            while (handled < 64 && std::chrono::steady_clock::now() < deadline
                && XCheckIfEvent(window->dpy, &event, detail::x11_clipboard_filter, reinterpret_cast<XPointer>(window))) {
                detail::x11_clipboard_event(window, event);
                ++handled;
            }
            saturated |= handled == 64;
            descriptors.push_back({ConnectionNumber(window->dpy), POLLIN, 0});
        }
        if (s->clipboard_done) break;
        if (saturated) continue; // 队列已读入 Xlib 时无需再等 fd，同时保留总截止时间。
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        ::poll(descriptors.data(), descriptors.size(), int(std::max<int64_t>(0, left)));
    }
    return s->clipboard_ok ? s->clipboard_received : std::string();
}

#else
} // namespace detail
#endif // AINO_PLATFORM_X11
#endif // AINO_PLATFORM_UNIX

// =============================================================================
//  8.20  Wayland 后端：CPU 画布、共享内存呈现与原生输入协议
// =============================================================================
#ifdef AINO_PLATFORM_WAYLAND
namespace detail {
namespace wl {
struct Proxy;
struct Interface;
struct Message { const char* name; const char* signature; const Interface** types; };
struct Interface {
    const char* name;
    int version, method_count;
    const Message* methods;
    int event_count;
    const Message* events;
};
struct Array { size_t size, alloc; void* data; };
// BEGIN AINO WAYLAND PROTOCOLS (tools/generate_wayland_protocols.py)
// 协议布局来自 Wayland / wayland-protocols XML，保留原协议名称与版本。
/* wayland
Copyright © 2008-2011 Kristian Høgsberg
Copyright © 2010-2011 Intel Corporation
Copyright © 2012-2013 Collabora, Ltd.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/* xdg_shell
Copyright © 2008-2013 Kristian Høgsberg
Copyright © 2013      Rafael Antognolli
Copyright © 2013      Jasper St. Pierre
Copyright © 2010-2013 Intel Corporation
Copyright © 2015-2017 Samsung Electronics Co., Ltd
Copyright © 2015-2017 Red Hat Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
/* viewporter
Copyright © 2013-2016 Collabora, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
/* fractional_scale_v1
Copyright © 2022 Kenny Levinsen

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
/* text_input_unstable_v3
Copyright © 2012, 2013 Intel Corporation
Copyright © 2015, 2016 Jan Arne Petersen
Copyright © 2017, 2018 Red Hat, Inc.
Copyright © 2018       Purism SPC

Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice appear in
all copies and that both that copyright notice and this permission
notice appear in supporting documentation, and that the name of
the copyright holders not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.  The copyright holders make no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied
warranty.

THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/
/* xdg_decoration_unstable_v1
Copyright © 2018 Simon Ser

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
extern const Interface wl_display;
extern const Interface wl_registry;
extern const Interface wl_callback;
extern const Interface wl_compositor;
extern const Interface wl_shm_pool;
extern const Interface wl_shm;
extern const Interface wl_buffer;
extern const Interface wl_data_offer;
extern const Interface wl_data_source;
extern const Interface wl_data_device;
extern const Interface wl_data_device_manager;
extern const Interface wl_shell;
extern const Interface wl_shell_surface;
extern const Interface wl_surface;
extern const Interface wl_seat;
extern const Interface wl_pointer;
extern const Interface wl_keyboard;
extern const Interface wl_touch;
extern const Interface wl_output;
extern const Interface wl_region;
extern const Interface wl_subcompositor;
extern const Interface wl_subsurface;
extern const Interface xdg_wm_base;
extern const Interface xdg_positioner;
extern const Interface xdg_surface;
extern const Interface xdg_toplevel;
extern const Interface xdg_popup;
extern const Interface wp_viewporter;
extern const Interface wp_viewport;
extern const Interface wp_fractional_scale_manager_v1;
extern const Interface wp_fractional_scale_v1;
extern const Interface zwp_text_input_v3;
extern const Interface zwp_text_input_manager_v3;
extern const Interface zxdg_decoration_manager_v1;
extern const Interface zxdg_toplevel_decoration_v1;
inline const Interface* wl_display_request_types_0[] = {&wl_callback};
inline const Interface* wl_display_request_types_1[] = {&wl_registry};
inline const Message wl_display_requests[] = {
        {"sync", "n", wl_display_request_types_0},
        {"get_registry", "n", wl_display_request_types_1},
};
inline const Interface* wl_display_event_types_0[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_display_event_types_1[] = {nullptr};
inline const Message wl_display_events[] = {
        {"error", "ous", wl_display_event_types_0},
        {"delete_id", "u", wl_display_event_types_1},
};
inline const Interface wl_display{"wl_display", 1, 2, wl_display_requests, 2, wl_display_events};
inline const Interface* wl_registry_request_types_0[] = {nullptr, nullptr, nullptr, nullptr};
inline const Message wl_registry_requests[] = {
        {"bind", "usun", wl_registry_request_types_0},
};
inline const Interface* wl_registry_event_types_0[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_registry_event_types_1[] = {nullptr};
inline const Message wl_registry_events[] = {
        {"global", "usu", wl_registry_event_types_0},
        {"global_remove", "u", wl_registry_event_types_1},
};
inline const Interface wl_registry{"wl_registry", 1, 1, wl_registry_requests, 2, wl_registry_events};
inline const Interface* wl_callback_event_types_0[] = {nullptr};
inline const Message wl_callback_events[] = {
        {"done", "u", wl_callback_event_types_0},
};
inline const Interface wl_callback{"wl_callback", 1, 0, nullptr, 1, wl_callback_events};
inline const Interface* wl_compositor_request_types_0[] = {&wl_surface};
inline const Interface* wl_compositor_request_types_1[] = {&wl_region};
inline const Message wl_compositor_requests[] = {
        {"create_surface", "n", wl_compositor_request_types_0},
        {"create_region", "n", wl_compositor_request_types_1},
};
inline const Interface wl_compositor{"wl_compositor", 6, 2, wl_compositor_requests, 0, nullptr};
inline const Interface* wl_shm_pool_request_types_0[] = {&wl_buffer, nullptr, nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_shm_pool_request_types_2[] = {nullptr};
inline const Message wl_shm_pool_requests[] = {
        {"create_buffer", "niiiiu", wl_shm_pool_request_types_0},
        {"destroy", "", nullptr},
        {"resize", "i", wl_shm_pool_request_types_2},
};
inline const Interface wl_shm_pool{"wl_shm_pool", 1, 3, wl_shm_pool_requests, 0, nullptr};
inline const Interface* wl_shm_request_types_0[] = {&wl_shm_pool, nullptr, nullptr};
inline const Message wl_shm_requests[] = {
        {"create_pool", "nhi", wl_shm_request_types_0},
};
inline const Interface* wl_shm_event_types_0[] = {nullptr};
inline const Message wl_shm_events[] = {
        {"format", "u", wl_shm_event_types_0},
};
inline const Interface wl_shm{"wl_shm", 1, 1, wl_shm_requests, 1, wl_shm_events};
inline const Message wl_buffer_requests[] = {
        {"destroy", "", nullptr},
};
inline const Message wl_buffer_events[] = {
        {"release", "", nullptr},
};
inline const Interface wl_buffer{"wl_buffer", 1, 1, wl_buffer_requests, 1, wl_buffer_events};
inline const Interface* wl_data_offer_request_types_0[] = {nullptr, nullptr};
inline const Interface* wl_data_offer_request_types_1[] = {nullptr, nullptr};
inline const Interface* wl_data_offer_request_types_4[] = {nullptr, nullptr};
inline const Message wl_data_offer_requests[] = {
        {"accept", "u?s", wl_data_offer_request_types_0},
        {"receive", "sh", wl_data_offer_request_types_1},
        {"destroy", "", nullptr},
        {"finish", "3", nullptr},
        {"set_actions", "3uu", wl_data_offer_request_types_4},
};
inline const Interface* wl_data_offer_event_types_0[] = {nullptr};
inline const Interface* wl_data_offer_event_types_1[] = {nullptr};
inline const Interface* wl_data_offer_event_types_2[] = {nullptr};
inline const Message wl_data_offer_events[] = {
        {"offer", "s", wl_data_offer_event_types_0},
        {"source_actions", "3u", wl_data_offer_event_types_1},
        {"action", "3u", wl_data_offer_event_types_2},
};
inline const Interface wl_data_offer{"wl_data_offer", 3, 5, wl_data_offer_requests, 3, wl_data_offer_events};
inline const Interface* wl_data_source_request_types_0[] = {nullptr};
inline const Interface* wl_data_source_request_types_2[] = {nullptr};
inline const Message wl_data_source_requests[] = {
        {"offer", "s", wl_data_source_request_types_0},
        {"destroy", "", nullptr},
        {"set_actions", "3u", wl_data_source_request_types_2},
};
inline const Interface* wl_data_source_event_types_0[] = {nullptr};
inline const Interface* wl_data_source_event_types_1[] = {nullptr, nullptr};
inline const Interface* wl_data_source_event_types_5[] = {nullptr};
inline const Message wl_data_source_events[] = {
        {"target", "?s", wl_data_source_event_types_0},
        {"send", "sh", wl_data_source_event_types_1},
        {"cancelled", "", nullptr},
        {"dnd_drop_performed", "3", nullptr},
        {"dnd_finished", "3", nullptr},
        {"action", "3u", wl_data_source_event_types_5},
};
inline const Interface wl_data_source{"wl_data_source", 3, 3, wl_data_source_requests, 6, wl_data_source_events};
inline const Interface* wl_data_device_request_types_0[] = {&wl_data_source, &wl_surface, &wl_surface, nullptr};
inline const Interface* wl_data_device_request_types_1[] = {&wl_data_source, nullptr};
inline const Message wl_data_device_requests[] = {
        {"start_drag", "?oo?ou", wl_data_device_request_types_0},
        {"set_selection", "?ou", wl_data_device_request_types_1},
        {"release", "2", nullptr},
};
inline const Interface* wl_data_device_event_types_0[] = {&wl_data_offer};
inline const Interface* wl_data_device_event_types_1[] = {nullptr, &wl_surface, nullptr, nullptr, &wl_data_offer};
inline const Interface* wl_data_device_event_types_3[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_data_device_event_types_5[] = {&wl_data_offer};
inline const Message wl_data_device_events[] = {
        {"data_offer", "n", wl_data_device_event_types_0},
        {"enter", "uoff?o", wl_data_device_event_types_1},
        {"leave", "", nullptr},
        {"motion", "uff", wl_data_device_event_types_3},
        {"drop", "", nullptr},
        {"selection", "?o", wl_data_device_event_types_5},
};
inline const Interface wl_data_device{"wl_data_device", 3, 3, wl_data_device_requests, 6, wl_data_device_events};
inline const Interface* wl_data_device_manager_request_types_0[] = {&wl_data_source};
inline const Interface* wl_data_device_manager_request_types_1[] = {&wl_data_device, &wl_seat};
inline const Message wl_data_device_manager_requests[] = {
        {"create_data_source", "n", wl_data_device_manager_request_types_0},
        {"get_data_device", "no", wl_data_device_manager_request_types_1},
};
inline const Interface wl_data_device_manager{"wl_data_device_manager", 3, 2, wl_data_device_manager_requests, 0, nullptr};
inline const Interface* wl_shell_request_types_0[] = {&wl_shell_surface, &wl_surface};
inline const Message wl_shell_requests[] = {
        {"get_shell_surface", "no", wl_shell_request_types_0},
};
inline const Interface wl_shell{"wl_shell", 1, 1, wl_shell_requests, 0, nullptr};
inline const Interface* wl_shell_surface_request_types_0[] = {nullptr};
inline const Interface* wl_shell_surface_request_types_1[] = {&wl_seat, nullptr};
inline const Interface* wl_shell_surface_request_types_2[] = {&wl_seat, nullptr, nullptr};
inline const Interface* wl_shell_surface_request_types_4[] = {&wl_surface, nullptr, nullptr, nullptr};
inline const Interface* wl_shell_surface_request_types_5[] = {nullptr, nullptr, &wl_output};
inline const Interface* wl_shell_surface_request_types_6[] = {&wl_seat, nullptr, &wl_surface, nullptr, nullptr, nullptr};
inline const Interface* wl_shell_surface_request_types_7[] = {&wl_output};
inline const Interface* wl_shell_surface_request_types_8[] = {nullptr};
inline const Interface* wl_shell_surface_request_types_9[] = {nullptr};
inline const Message wl_shell_surface_requests[] = {
        {"pong", "u", wl_shell_surface_request_types_0},
        {"move", "ou", wl_shell_surface_request_types_1},
        {"resize", "ouu", wl_shell_surface_request_types_2},
        {"set_toplevel", "", nullptr},
        {"set_transient", "oiiu", wl_shell_surface_request_types_4},
        {"set_fullscreen", "uu?o", wl_shell_surface_request_types_5},
        {"set_popup", "ouoiiu", wl_shell_surface_request_types_6},
        {"set_maximized", "?o", wl_shell_surface_request_types_7},
        {"set_title", "s", wl_shell_surface_request_types_8},
        {"set_class", "s", wl_shell_surface_request_types_9},
};
inline const Interface* wl_shell_surface_event_types_0[] = {nullptr};
inline const Interface* wl_shell_surface_event_types_1[] = {nullptr, nullptr, nullptr};
inline const Message wl_shell_surface_events[] = {
        {"ping", "u", wl_shell_surface_event_types_0},
        {"configure", "uii", wl_shell_surface_event_types_1},
        {"popup_done", "", nullptr},
};
inline const Interface wl_shell_surface{"wl_shell_surface", 1, 10, wl_shell_surface_requests, 3, wl_shell_surface_events};
inline const Interface* wl_surface_request_types_1[] = {&wl_buffer, nullptr, nullptr};
inline const Interface* wl_surface_request_types_2[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_surface_request_types_3[] = {&wl_callback};
inline const Interface* wl_surface_request_types_4[] = {&wl_region};
inline const Interface* wl_surface_request_types_5[] = {&wl_region};
inline const Interface* wl_surface_request_types_7[] = {nullptr};
inline const Interface* wl_surface_request_types_8[] = {nullptr};
inline const Interface* wl_surface_request_types_9[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_surface_request_types_10[] = {nullptr, nullptr};
inline const Message wl_surface_requests[] = {
        {"destroy", "", nullptr},
        {"attach", "?oii", wl_surface_request_types_1},
        {"damage", "iiii", wl_surface_request_types_2},
        {"frame", "n", wl_surface_request_types_3},
        {"set_opaque_region", "?o", wl_surface_request_types_4},
        {"set_input_region", "?o", wl_surface_request_types_5},
        {"commit", "", nullptr},
        {"set_buffer_transform", "2i", wl_surface_request_types_7},
        {"set_buffer_scale", "3i", wl_surface_request_types_8},
        {"damage_buffer", "4iiii", wl_surface_request_types_9},
        {"offset", "5ii", wl_surface_request_types_10},
};
inline const Interface* wl_surface_event_types_0[] = {&wl_output};
inline const Interface* wl_surface_event_types_1[] = {&wl_output};
inline const Interface* wl_surface_event_types_2[] = {nullptr};
inline const Interface* wl_surface_event_types_3[] = {nullptr};
inline const Message wl_surface_events[] = {
        {"enter", "o", wl_surface_event_types_0},
        {"leave", "o", wl_surface_event_types_1},
        {"preferred_buffer_scale", "6i", wl_surface_event_types_2},
        {"preferred_buffer_transform", "6u", wl_surface_event_types_3},
};
inline const Interface wl_surface{"wl_surface", 6, 11, wl_surface_requests, 4, wl_surface_events};
inline const Interface* wl_seat_request_types_0[] = {&wl_pointer};
inline const Interface* wl_seat_request_types_1[] = {&wl_keyboard};
inline const Interface* wl_seat_request_types_2[] = {&wl_touch};
inline const Message wl_seat_requests[] = {
        {"get_pointer", "n", wl_seat_request_types_0},
        {"get_keyboard", "n", wl_seat_request_types_1},
        {"get_touch", "n", wl_seat_request_types_2},
        {"release", "5", nullptr},
};
inline const Interface* wl_seat_event_types_0[] = {nullptr};
inline const Interface* wl_seat_event_types_1[] = {nullptr};
inline const Message wl_seat_events[] = {
        {"capabilities", "u", wl_seat_event_types_0},
        {"name", "2s", wl_seat_event_types_1},
};
inline const Interface wl_seat{"wl_seat", 9, 4, wl_seat_requests, 2, wl_seat_events};
inline const Interface* wl_pointer_request_types_0[] = {nullptr, &wl_surface, nullptr, nullptr};
inline const Message wl_pointer_requests[] = {
        {"set_cursor", "u?oii", wl_pointer_request_types_0},
        {"release", "3", nullptr},
};
inline const Interface* wl_pointer_event_types_0[] = {nullptr, &wl_surface, nullptr, nullptr};
inline const Interface* wl_pointer_event_types_1[] = {nullptr, &wl_surface};
inline const Interface* wl_pointer_event_types_2[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_pointer_event_types_3[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_pointer_event_types_4[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_pointer_event_types_6[] = {nullptr};
inline const Interface* wl_pointer_event_types_7[] = {nullptr, nullptr};
inline const Interface* wl_pointer_event_types_8[] = {nullptr, nullptr};
inline const Interface* wl_pointer_event_types_9[] = {nullptr, nullptr};
inline const Interface* wl_pointer_event_types_10[] = {nullptr, nullptr};
inline const Message wl_pointer_events[] = {
        {"enter", "uoff", wl_pointer_event_types_0},
        {"leave", "uo", wl_pointer_event_types_1},
        {"motion", "uff", wl_pointer_event_types_2},
        {"button", "uuuu", wl_pointer_event_types_3},
        {"axis", "uuf", wl_pointer_event_types_4},
        {"frame", "5", nullptr},
        {"axis_source", "5u", wl_pointer_event_types_6},
        {"axis_stop", "5uu", wl_pointer_event_types_7},
        {"axis_discrete", "5ui", wl_pointer_event_types_8},
        {"axis_value120", "8ui", wl_pointer_event_types_9},
        {"axis_relative_direction", "9uu", wl_pointer_event_types_10},
};
inline const Interface wl_pointer{"wl_pointer", 9, 2, wl_pointer_requests, 11, wl_pointer_events};
inline const Message wl_keyboard_requests[] = {
        {"release", "3", nullptr},
};
inline const Interface* wl_keyboard_event_types_0[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_keyboard_event_types_1[] = {nullptr, &wl_surface, nullptr};
inline const Interface* wl_keyboard_event_types_2[] = {nullptr, &wl_surface};
inline const Interface* wl_keyboard_event_types_3[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_keyboard_event_types_4[] = {nullptr, nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_keyboard_event_types_5[] = {nullptr, nullptr};
inline const Message wl_keyboard_events[] = {
        {"keymap", "uhu", wl_keyboard_event_types_0},
        {"enter", "uoa", wl_keyboard_event_types_1},
        {"leave", "uo", wl_keyboard_event_types_2},
        {"key", "uuuu", wl_keyboard_event_types_3},
        {"modifiers", "uuuuu", wl_keyboard_event_types_4},
        {"repeat_info", "4ii", wl_keyboard_event_types_5},
};
inline const Interface wl_keyboard{"wl_keyboard", 9, 1, wl_keyboard_requests, 6, wl_keyboard_events};
inline const Message wl_touch_requests[] = {
        {"release", "3", nullptr},
};
inline const Interface* wl_touch_event_types_0[] = {nullptr, nullptr, &wl_surface, nullptr, nullptr, nullptr};
inline const Interface* wl_touch_event_types_1[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_touch_event_types_2[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_touch_event_types_5[] = {nullptr, nullptr, nullptr};
inline const Interface* wl_touch_event_types_6[] = {nullptr, nullptr};
inline const Message wl_touch_events[] = {
        {"down", "uuoiff", wl_touch_event_types_0},
        {"up", "uui", wl_touch_event_types_1},
        {"motion", "uiff", wl_touch_event_types_2},
        {"frame", "", nullptr},
        {"cancel", "", nullptr},
        {"shape", "6iff", wl_touch_event_types_5},
        {"orientation", "6if", wl_touch_event_types_6},
};
inline const Interface wl_touch{"wl_touch", 9, 1, wl_touch_requests, 7, wl_touch_events};
inline const Message wl_output_requests[] = {
        {"release", "3", nullptr},
};
inline const Interface* wl_output_event_types_0[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_output_event_types_1[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_output_event_types_3[] = {nullptr};
inline const Interface* wl_output_event_types_4[] = {nullptr};
inline const Interface* wl_output_event_types_5[] = {nullptr};
inline const Message wl_output_events[] = {
        {"geometry", "iiiiissi", wl_output_event_types_0},
        {"mode", "uiii", wl_output_event_types_1},
        {"done", "2", nullptr},
        {"scale", "2i", wl_output_event_types_3},
        {"name", "4s", wl_output_event_types_4},
        {"description", "4s", wl_output_event_types_5},
};
inline const Interface wl_output{"wl_output", 4, 1, wl_output_requests, 6, wl_output_events};
inline const Interface* wl_region_request_types_1[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wl_region_request_types_2[] = {nullptr, nullptr, nullptr, nullptr};
inline const Message wl_region_requests[] = {
        {"destroy", "", nullptr},
        {"add", "iiii", wl_region_request_types_1},
        {"subtract", "iiii", wl_region_request_types_2},
};
inline const Interface wl_region{"wl_region", 1, 3, wl_region_requests, 0, nullptr};
inline const Interface* wl_subcompositor_request_types_1[] = {&wl_subsurface, &wl_surface, &wl_surface};
inline const Message wl_subcompositor_requests[] = {
        {"destroy", "", nullptr},
        {"get_subsurface", "noo", wl_subcompositor_request_types_1},
};
inline const Interface wl_subcompositor{"wl_subcompositor", 1, 2, wl_subcompositor_requests, 0, nullptr};
inline const Interface* wl_subsurface_request_types_1[] = {nullptr, nullptr};
inline const Interface* wl_subsurface_request_types_2[] = {&wl_surface};
inline const Interface* wl_subsurface_request_types_3[] = {&wl_surface};
inline const Message wl_subsurface_requests[] = {
        {"destroy", "", nullptr},
        {"set_position", "ii", wl_subsurface_request_types_1},
        {"place_above", "o", wl_subsurface_request_types_2},
        {"place_below", "o", wl_subsurface_request_types_3},
        {"set_sync", "", nullptr},
        {"set_desync", "", nullptr},
};
inline const Interface wl_subsurface{"wl_subsurface", 1, 6, wl_subsurface_requests, 0, nullptr};
inline const Interface* xdg_wm_base_request_types_1[] = {&xdg_positioner};
inline const Interface* xdg_wm_base_request_types_2[] = {&xdg_surface, &wl_surface};
inline const Interface* xdg_wm_base_request_types_3[] = {nullptr};
inline const Message xdg_wm_base_requests[] = {
        {"destroy", "", nullptr},
        {"create_positioner", "n", xdg_wm_base_request_types_1},
        {"get_xdg_surface", "no", xdg_wm_base_request_types_2},
        {"pong", "u", xdg_wm_base_request_types_3},
};
inline const Interface* xdg_wm_base_event_types_0[] = {nullptr};
inline const Message xdg_wm_base_events[] = {
        {"ping", "u", xdg_wm_base_event_types_0},
};
inline const Interface xdg_wm_base{"xdg_wm_base", 7, 4, xdg_wm_base_requests, 1, xdg_wm_base_events};
inline const Interface* xdg_positioner_request_types_1[] = {nullptr, nullptr};
inline const Interface* xdg_positioner_request_types_2[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* xdg_positioner_request_types_3[] = {nullptr};
inline const Interface* xdg_positioner_request_types_4[] = {nullptr};
inline const Interface* xdg_positioner_request_types_5[] = {nullptr};
inline const Interface* xdg_positioner_request_types_6[] = {nullptr, nullptr};
inline const Interface* xdg_positioner_request_types_8[] = {nullptr, nullptr};
inline const Interface* xdg_positioner_request_types_9[] = {nullptr};
inline const Message xdg_positioner_requests[] = {
        {"destroy", "", nullptr},
        {"set_size", "ii", xdg_positioner_request_types_1},
        {"set_anchor_rect", "iiii", xdg_positioner_request_types_2},
        {"set_anchor", "u", xdg_positioner_request_types_3},
        {"set_gravity", "u", xdg_positioner_request_types_4},
        {"set_constraint_adjustment", "u", xdg_positioner_request_types_5},
        {"set_offset", "ii", xdg_positioner_request_types_6},
        {"set_reactive", "3", nullptr},
        {"set_parent_size", "3ii", xdg_positioner_request_types_8},
        {"set_parent_configure", "3u", xdg_positioner_request_types_9},
};
inline const Interface xdg_positioner{"xdg_positioner", 7, 10, xdg_positioner_requests, 0, nullptr};
inline const Interface* xdg_surface_request_types_1[] = {&xdg_toplevel};
inline const Interface* xdg_surface_request_types_2[] = {&xdg_popup, &xdg_surface, &xdg_positioner};
inline const Interface* xdg_surface_request_types_3[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* xdg_surface_request_types_4[] = {nullptr};
inline const Message xdg_surface_requests[] = {
        {"destroy", "", nullptr},
        {"get_toplevel", "n", xdg_surface_request_types_1},
        {"get_popup", "n?oo", xdg_surface_request_types_2},
        {"set_window_geometry", "iiii", xdg_surface_request_types_3},
        {"ack_configure", "u", xdg_surface_request_types_4},
};
inline const Interface* xdg_surface_event_types_0[] = {nullptr};
inline const Message xdg_surface_events[] = {
        {"configure", "u", xdg_surface_event_types_0},
};
inline const Interface xdg_surface{"xdg_surface", 7, 5, xdg_surface_requests, 1, xdg_surface_events};
inline const Interface* xdg_toplevel_request_types_1[] = {&xdg_toplevel};
inline const Interface* xdg_toplevel_request_types_2[] = {nullptr};
inline const Interface* xdg_toplevel_request_types_3[] = {nullptr};
inline const Interface* xdg_toplevel_request_types_4[] = {&wl_seat, nullptr, nullptr, nullptr};
inline const Interface* xdg_toplevel_request_types_5[] = {&wl_seat, nullptr};
inline const Interface* xdg_toplevel_request_types_6[] = {&wl_seat, nullptr, nullptr};
inline const Interface* xdg_toplevel_request_types_7[] = {nullptr, nullptr};
inline const Interface* xdg_toplevel_request_types_8[] = {nullptr, nullptr};
inline const Interface* xdg_toplevel_request_types_11[] = {&wl_output};
inline const Message xdg_toplevel_requests[] = {
        {"destroy", "", nullptr},
        {"set_parent", "?o", xdg_toplevel_request_types_1},
        {"set_title", "s", xdg_toplevel_request_types_2},
        {"set_app_id", "s", xdg_toplevel_request_types_3},
        {"show_window_menu", "ouii", xdg_toplevel_request_types_4},
        {"move", "ou", xdg_toplevel_request_types_5},
        {"resize", "ouu", xdg_toplevel_request_types_6},
        {"set_max_size", "ii", xdg_toplevel_request_types_7},
        {"set_min_size", "ii", xdg_toplevel_request_types_8},
        {"set_maximized", "", nullptr},
        {"unset_maximized", "", nullptr},
        {"set_fullscreen", "?o", xdg_toplevel_request_types_11},
        {"unset_fullscreen", "", nullptr},
        {"set_minimized", "", nullptr},
};
inline const Interface* xdg_toplevel_event_types_0[] = {nullptr, nullptr, nullptr};
inline const Interface* xdg_toplevel_event_types_2[] = {nullptr, nullptr};
inline const Interface* xdg_toplevel_event_types_3[] = {nullptr};
inline const Message xdg_toplevel_events[] = {
        {"configure", "iia", xdg_toplevel_event_types_0},
        {"close", "", nullptr},
        {"configure_bounds", "4ii", xdg_toplevel_event_types_2},
        {"wm_capabilities", "5a", xdg_toplevel_event_types_3},
};
inline const Interface xdg_toplevel{"xdg_toplevel", 7, 14, xdg_toplevel_requests, 4, xdg_toplevel_events};
inline const Interface* xdg_popup_request_types_1[] = {&wl_seat, nullptr};
inline const Interface* xdg_popup_request_types_2[] = {&xdg_positioner, nullptr};
inline const Message xdg_popup_requests[] = {
        {"destroy", "", nullptr},
        {"grab", "ou", xdg_popup_request_types_1},
        {"reposition", "3ou", xdg_popup_request_types_2},
};
inline const Interface* xdg_popup_event_types_0[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* xdg_popup_event_types_2[] = {nullptr};
inline const Message xdg_popup_events[] = {
        {"configure", "iiii", xdg_popup_event_types_0},
        {"popup_done", "", nullptr},
        {"repositioned", "3u", xdg_popup_event_types_2},
};
inline const Interface xdg_popup{"xdg_popup", 7, 3, xdg_popup_requests, 3, xdg_popup_events};
inline const Interface* wp_viewporter_request_types_1[] = {&wp_viewport, &wl_surface};
inline const Message wp_viewporter_requests[] = {
        {"destroy", "", nullptr},
        {"get_viewport", "no", wp_viewporter_request_types_1},
};
inline const Interface wp_viewporter{"wp_viewporter", 1, 2, wp_viewporter_requests, 0, nullptr};
inline const Interface* wp_viewport_request_types_1[] = {nullptr, nullptr, nullptr, nullptr};
inline const Interface* wp_viewport_request_types_2[] = {nullptr, nullptr};
inline const Message wp_viewport_requests[] = {
        {"destroy", "", nullptr},
        {"set_source", "ffff", wp_viewport_request_types_1},
        {"set_destination", "ii", wp_viewport_request_types_2},
};
inline const Interface wp_viewport{"wp_viewport", 1, 3, wp_viewport_requests, 0, nullptr};
inline const Interface* wp_fractional_scale_manager_v1_request_types_1[] = {&wp_fractional_scale_v1, &wl_surface};
inline const Message wp_fractional_scale_manager_v1_requests[] = {
        {"destroy", "", nullptr},
        {"get_fractional_scale", "no", wp_fractional_scale_manager_v1_request_types_1},
};
inline const Interface wp_fractional_scale_manager_v1{"wp_fractional_scale_manager_v1", 1, 2, wp_fractional_scale_manager_v1_requests, 0, nullptr};
inline const Message wp_fractional_scale_v1_requests[] = {
        {"destroy", "", nullptr},
};
inline const Interface* wp_fractional_scale_v1_event_types_0[] = {nullptr};
inline const Message wp_fractional_scale_v1_events[] = {
        {"preferred_scale", "u", wp_fractional_scale_v1_event_types_0},
};
inline const Interface wp_fractional_scale_v1{"wp_fractional_scale_v1", 1, 1, wp_fractional_scale_v1_requests, 1, wp_fractional_scale_v1_events};
inline const Interface* zwp_text_input_v3_request_types_3[] = {nullptr, nullptr, nullptr};
inline const Interface* zwp_text_input_v3_request_types_4[] = {nullptr};
inline const Interface* zwp_text_input_v3_request_types_5[] = {nullptr, nullptr};
inline const Interface* zwp_text_input_v3_request_types_6[] = {nullptr, nullptr, nullptr, nullptr};
inline const Message zwp_text_input_v3_requests[] = {
        {"destroy", "", nullptr},
        {"enable", "", nullptr},
        {"disable", "", nullptr},
        {"set_surrounding_text", "sii", zwp_text_input_v3_request_types_3},
        {"set_text_change_cause", "u", zwp_text_input_v3_request_types_4},
        {"set_content_type", "uu", zwp_text_input_v3_request_types_5},
        {"set_cursor_rectangle", "iiii", zwp_text_input_v3_request_types_6},
        {"commit", "", nullptr},
};
inline const Interface* zwp_text_input_v3_event_types_0[] = {&wl_surface};
inline const Interface* zwp_text_input_v3_event_types_1[] = {&wl_surface};
inline const Interface* zwp_text_input_v3_event_types_2[] = {nullptr, nullptr, nullptr};
inline const Interface* zwp_text_input_v3_event_types_3[] = {nullptr};
inline const Interface* zwp_text_input_v3_event_types_4[] = {nullptr, nullptr};
inline const Interface* zwp_text_input_v3_event_types_5[] = {nullptr};
inline const Message zwp_text_input_v3_events[] = {
        {"enter", "o", zwp_text_input_v3_event_types_0},
        {"leave", "o", zwp_text_input_v3_event_types_1},
        {"preedit_string", "?sii", zwp_text_input_v3_event_types_2},
        {"commit_string", "?s", zwp_text_input_v3_event_types_3},
        {"delete_surrounding_text", "uu", zwp_text_input_v3_event_types_4},
        {"done", "u", zwp_text_input_v3_event_types_5},
};
inline const Interface zwp_text_input_v3{"zwp_text_input_v3", 1, 8, zwp_text_input_v3_requests, 6, zwp_text_input_v3_events};
inline const Interface* zwp_text_input_manager_v3_request_types_1[] = {&zwp_text_input_v3, &wl_seat};
inline const Message zwp_text_input_manager_v3_requests[] = {
        {"destroy", "", nullptr},
        {"get_text_input", "no", zwp_text_input_manager_v3_request_types_1},
};
inline const Interface zwp_text_input_manager_v3{"zwp_text_input_manager_v3", 1, 2, zwp_text_input_manager_v3_requests, 0, nullptr};
inline const Interface* zxdg_decoration_manager_v1_request_types_1[] = {&zxdg_toplevel_decoration_v1, &xdg_toplevel};
inline const Message zxdg_decoration_manager_v1_requests[] = {
        {"destroy", "", nullptr},
        {"get_toplevel_decoration", "no", zxdg_decoration_manager_v1_request_types_1},
};
inline const Interface zxdg_decoration_manager_v1{"zxdg_decoration_manager_v1", 1, 2, zxdg_decoration_manager_v1_requests, 0, nullptr};
inline const Interface* zxdg_toplevel_decoration_v1_request_types_1[] = {nullptr};
inline const Message zxdg_toplevel_decoration_v1_requests[] = {
        {"destroy", "", nullptr},
        {"set_mode", "u", zxdg_toplevel_decoration_v1_request_types_1},
        {"unset_mode", "", nullptr},
};
inline const Interface* zxdg_toplevel_decoration_v1_event_types_0[] = {nullptr};
inline const Message zxdg_toplevel_decoration_v1_events[] = {
        {"configure", "u", zxdg_toplevel_decoration_v1_event_types_0},
};
inline const Interface zxdg_toplevel_decoration_v1{"zxdg_toplevel_decoration_v1", 1, 3, zxdg_toplevel_decoration_v1_requests, 1, zxdg_toplevel_decoration_v1_events};
// END AINO WAYLAND PROTOCOLS

struct Api {
    void* library = nullptr;
    Proxy* (*connect)(const char*) = nullptr;
    void (*disconnect)(Proxy*) = nullptr;
    int (*get_fd)(Proxy*) = nullptr;
    int (*flush)(Proxy*) = nullptr;
    int (*dispatch_pending)(Proxy*) = nullptr;
    int (*prepare_read)(Proxy*) = nullptr;
    int (*read_events)(Proxy*) = nullptr;
    void (*cancel_read)(Proxy*) = nullptr;
    int (*roundtrip)(Proxy*) = nullptr;
    Proxy* (*marshal)(Proxy*, uint32_t, const Interface*, uint32_t, uint32_t, ...) = nullptr;
    uint32_t (*version)(Proxy*) = nullptr;
    int (*listen)(Proxy*, void (**)(void), void*) = nullptr;
    void (*destroy)(Proxy*) = nullptr;
    bool load() {
        library = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!library) return false;
#define AINO_WL_LOAD(field, name) \
        field = reinterpret_cast<decltype(field)>(dlsym(library, name)); \
        if (!field) return false
        AINO_WL_LOAD(connect, "wl_display_connect");
        AINO_WL_LOAD(disconnect, "wl_display_disconnect");
        AINO_WL_LOAD(get_fd, "wl_display_get_fd");
        AINO_WL_LOAD(flush, "wl_display_flush");
        AINO_WL_LOAD(dispatch_pending, "wl_display_dispatch_pending");
        AINO_WL_LOAD(prepare_read, "wl_display_prepare_read");
        AINO_WL_LOAD(read_events, "wl_display_read_events");
        AINO_WL_LOAD(cancel_read, "wl_display_cancel_read");
        AINO_WL_LOAD(roundtrip, "wl_display_roundtrip");
        AINO_WL_LOAD(marshal, "wl_proxy_marshal_flags");
        AINO_WL_LOAD(version, "wl_proxy_get_version");
        AINO_WL_LOAD(listen, "wl_proxy_add_listener");
        AINO_WL_LOAD(destroy, "wl_proxy_destroy");
#undef AINO_WL_LOAD
        return true;
    }
};
inline Api& api() { static Api value; return value; }
inline bool available() { static const bool ready = api().load(); return ready; }
template<class... Args> inline Proxy* request(Proxy* object, uint32_t opcode, Args... args) {
    return api().marshal(object, opcode, nullptr, api().version(object), 0, args...);
}
template<class... Args> inline Proxy* create(Proxy* object, uint32_t opcode, const Interface& type, Args... args) {
    return api().marshal(object, opcode, &type, std::min(api().version(object), uint32_t(type.version)), 0, nullptr, args...);
}
inline void destroy(Proxy*& object, uint32_t opcode = 0) {
    if (object) api().marshal(object, opcode, nullptr, api().version(object), 1);
    object = nullptr;
}
inline void local_destroy(Proxy*& object) {
    if (object) api().destroy(object);
    object = nullptr;
}
template<class Listener> inline void listen(Proxy* object, const Listener& listener, void* data) {
    api().listen(object, reinterpret_cast<void (**)(void)>(const_cast<Listener*>(&listener)), data);
}

struct Xkb {
    void* library = nullptr;
    void* (*context_new)(int) = nullptr;
    void (*context_unref)(void*) = nullptr;
    void* (*keymap_new)(void*, const char*, int, int) = nullptr;
    void (*keymap_unref)(void*) = nullptr;
    void* (*state_new)(void*) = nullptr;
    void (*state_unref)(void*) = nullptr;
    uint32_t (*update_mask)(void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) = nullptr;
    uint32_t (*sym)(void*, uint32_t) = nullptr;
    uint32_t (*layout)(void*, uint32_t) = nullptr;
    int (*symbols)(void*, uint32_t, uint32_t, uint32_t, const uint32_t**) = nullptr;
    int (*utf8)(void*, uint32_t, char*, size_t) = nullptr;
    int (*mod_active)(void*, const char*, int) = nullptr;
    int (*repeats)(void*, uint32_t) = nullptr;
    void* (*compose_table)(void*, const char*, int) = nullptr;
    void (*compose_table_unref)(void*) = nullptr;
    void* (*compose_new)(void*, int) = nullptr;
    void (*compose_unref)(void*) = nullptr;
    int (*compose_feed)(void*, uint32_t) = nullptr;
    int (*compose_status)(void*) = nullptr;
    int (*compose_utf8)(void*, char*, size_t) = nullptr;
    void (*compose_reset)(void*) = nullptr;
    bool load() {
        library = dlopen("libxkbcommon.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!library) return false;
#define AINO_XKB_LOAD(field, name) \
        field = reinterpret_cast<decltype(field)>(dlsym(library, "xkb_" name)); \
        if (!field) return false
        AINO_XKB_LOAD(context_new, "context_new");
        AINO_XKB_LOAD(context_unref, "context_unref");
        AINO_XKB_LOAD(keymap_new, "keymap_new_from_string");
        AINO_XKB_LOAD(keymap_unref, "keymap_unref");
        AINO_XKB_LOAD(state_new, "state_new");
        AINO_XKB_LOAD(state_unref, "state_unref");
        AINO_XKB_LOAD(update_mask, "state_update_mask");
        AINO_XKB_LOAD(sym, "state_key_get_one_sym");
        AINO_XKB_LOAD(layout, "state_key_get_layout");
        AINO_XKB_LOAD(symbols, "keymap_key_get_syms_by_level");
        AINO_XKB_LOAD(utf8, "state_key_get_utf8");
        AINO_XKB_LOAD(mod_active, "state_mod_name_is_active");
        AINO_XKB_LOAD(repeats, "keymap_key_repeats");
        AINO_XKB_LOAD(compose_table, "compose_table_new_from_locale");
        AINO_XKB_LOAD(compose_table_unref, "compose_table_unref");
        AINO_XKB_LOAD(compose_new, "compose_state_new");
        AINO_XKB_LOAD(compose_unref, "compose_state_unref");
        AINO_XKB_LOAD(compose_feed, "compose_state_feed");
        AINO_XKB_LOAD(compose_status, "compose_state_get_status");
        AINO_XKB_LOAD(compose_utf8, "compose_state_get_utf8");
        AINO_XKB_LOAD(compose_reset, "compose_state_reset");
#undef AINO_XKB_LOAD
        return true;
    }
};
inline Xkb& xkb() { static Xkb value; return value; }
inline bool keyboard_available() { static const bool ready = xkb().load(); return ready; }

struct CursorImage { uint32_t width, height, hotspot_x, hotspot_y, delay; };
struct ThemeCursor { unsigned image_count; CursorImage** images; char* name; };
struct CursorApi {
    void* library = nullptr;
    void* (*load)(const char*, int, Proxy*) = nullptr;
    void (*destroy)(void*) = nullptr;
    ThemeCursor* (*get)(void*, const char*) = nullptr;
    Proxy* (*buffer)(CursorImage*) = nullptr;
    CursorApi() {
        library = dlopen("libwayland-cursor.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!library) return;
        load = reinterpret_cast<decltype(load)>(dlsym(library, "wl_cursor_theme_load"));
        destroy = reinterpret_cast<decltype(destroy)>(dlsym(library, "wl_cursor_theme_destroy"));
        get = reinterpret_cast<decltype(get)>(dlsym(library, "wl_cursor_theme_get_cursor"));
        buffer = reinterpret_cast<decltype(buffer)>(dlsym(library, "wl_cursor_image_get_buffer"));
        if (!load || !destroy || !get || !buffer) load = nullptr;
    }
};
inline CursorApi& cursors() { static CursorApi value; return value; }
} // namespace wl

struct WaylandState {
    Window::Impl* owner = nullptr;
    wl::Proxy *display = nullptr, *registry = nullptr, *compositor = nullptr, *shm = nullptr;
    wl::Proxy *shell = nullptr, *surface = nullptr, *xdg_surface = nullptr, *toplevel = nullptr;
    wl::Proxy *seat = nullptr, *pointer = nullptr, *keyboard = nullptr, *frame = nullptr;
    wl::Proxy *data_manager = nullptr, *data_device = nullptr, *selection = nullptr;
    wl::Proxy *text_manager = nullptr, *text_input = nullptr;
    wl::Proxy *viewporter = nullptr, *viewport = nullptr, *fractional_manager = nullptr, *fractional = nullptr;
    wl::Proxy *decoration_manager = nullptr, *decoration = nullptr, *cursor_surface = nullptr;
    uint32_t seat_name = 0, compositor_name = 0, shm_name = 0, shell_name = 0;
    struct Output { wl::Proxy* proxy = nullptr; int scale = 1; uint32_t name = 0; bool entered = false; };
    std::list<Output> outputs;
    struct Buffer {
        wl::Proxy* proxy = nullptr;
        void* pixels = MAP_FAILED;
        size_t bytes = 0;
        int width = 0, height = 0;
        bool busy = false;
        ~Buffer() { wl::destroy(proxy); if (pixels != MAP_FAILED) munmap(pixels, bytes); }
    };
    std::vector<std::unique_ptr<Buffer>> buffers;
    std::vector<uint8_t> pixels;
    struct Offer {
        wl::Proxy* proxy;
        std::string mime;
        bool uri_list = false;
        uint32_t action = 0;
    };
    std::list<Offer> offers;
    std::function<bool(float, float)> file_drag;
    std::function<std::function<void(std::string)>(float, float)> file_drop;
    wl::Proxy* drag_offer = nullptr;
    wl::Proxy* drop_offer = nullptr;
    uint32_t drag_serial = 0;
    float drag_x = 0, drag_y = 0;
    bool drag_accepted = false;
    int drop_fd = -1;
    std::string drop_data;
    std::function<void(std::string)> drop_complete;
    std::chrono::steady_clock::time_point drop_deadline;
    struct Source { wl::Proxy* proxy; std::shared_ptr<const std::string> text; };
    std::list<Source> sources;
    struct Send {
        int fd; std::shared_ptr<const std::string> text; size_t offset = 0;
        std::chrono::steady_clock::time_point deadline;
    };
    std::list<Send> sends;
    uint32_t serial = 0, pointer_serial = 0, text_serial = 0;
    uint32_t content_hint = 0, content_purpose = 0, sent_hint = 0, sent_purpose = 0;
    uint64_t text_focus = 0;
    bool configured = false, failed = false, flush_blocked = false;
    bool completed_frame = false;
    bool pointer_inside = false, keyboard_inside = false, text_entered = false, text_enabled = false;
    bool clipboard_reading = false;
    bool preedit_pending = false, commit_pending = false;
    std::string preedit, commit, text_in;
    Rect caret, sent_caret{-1, -1, -1, -1};
    int width = 1, height = 1, pending_width = 0, pending_height = 0;
    int title_height = 32, header_rows = 0, buffer_height = 0, title_pressed = 0;
    bool maximized = false;
    int output_scale = 1, preferred_scale = 0;
    bool resize_pending = true;
    float mx = -1e6f, my = -1e6f, wheel = 0;
    bool btn_down[3]{}, btn_pressed[3]{}, btn_released[3]{};
    int clicks = 0;
    uint32_t click_time = 0;
    Vec2 click_position;
    bool key_down[KEY_COUNT]{}, key_pressed[KEY_COUNT]{};
    int mapped_keys[768]{};
    bool key_ctrl[KEY_COUNT]{}, key_shift[KEY_COUNT]{}, key_alt[KEY_COUNT]{};
    unsigned char key_repeat[KEY_COUNT]{};
    void *context = nullptr, *keymap = nullptr, *xkb_state = nullptr;
    void *compose_table = nullptr, *compose = nullptr, *cursor_theme = nullptr;
    int cursor_scale = 0, cursor_index = -1;
    int repeat_rate = 25, repeat_delay = 600;
    uint32_t repeat_key = 0;
    std::chrono::steady_clock::time_point repeat_at{};
};
inline std::vector<WaylandState*>& wayland_windows() {
    thread_local std::vector<WaylandState*> windows;
    return windows;
}
inline void wayland_fail(WaylandState* s, const char* message) {
    if (!s->failed) std::fprintf(stderr, "[aino/wayland] %s\n", message);
    s->failed = true;
    s->owner->running = false;
}
inline void wayland_flush(WaylandState* s) {
    if (s->failed) return;
    s->flush_blocked = wl::api().flush(s->display) < 0;
    if (s->flush_blocked && errno != EAGAIN) wayland_fail(s, "display flush failed");
}

inline void wayland_dispatch(WaylandState* s) {
    if (!s->display || s->failed) return;
    auto& api = wl::api();
    // 单个 UI 线程拥有连接；prepare/read/cancel 成对，后台仅通过 WakeSignal 通知。
    while (api.prepare_read(s->display) != 0) {
        if (api.dispatch_pending(s->display) < 0) { wayland_fail(s, "display disconnected"); return; }
    }
    wayland_flush(s);
    pollfd fd{api.get_fd(s->display), POLLIN, 0};
    const int ready = ::poll(&fd, 1, 0);
    if (ready > 0 && (fd.revents & (POLLIN | POLLHUP | POLLERR))) {
        if (api.read_events(s->display) < 0) wayland_fail(s, "display read failed");
    } else api.cancel_read(s->display);
    if (!s->failed && api.dispatch_pending(s->display) < 0) wayland_fail(s, "display dispatch failed");
}

inline bool wayland_can_render(const WaylandState* s) {
    if (!s || !s->configured || s->failed || s->frame) return false;
    if (s->buffers.size() < 3) return true;
    for (const auto& buffer : s->buffers) if (!buffer->busy) return true;
    return false;
}
inline void wayland_scale_changed(WaylandState* s) {
    s->resize_pending = true;
    s->owner->need_frame = true;
}
inline bool wayland_resize(WaylandState* s) {
    auto* o = s->owner;
    if (!s->resize_pending || o->ui.impl->frame_open) return true;
    if (s->pending_width > 0) s->width = s->pending_width;
    if (s->pending_height > 0) s->height = std::max(1, s->pending_height - s->title_height);
    s->pending_width = s->pending_height = 0;
    float scale = 1;
    if (o->cfg.high_dpi) scale = s->preferred_scale > 0 && s->viewport
        ? s->preferred_scale / 120.f : float(s->output_scale);
    scale = clampf(scale, .5f, 8.f);
    const int width = int(std::ceil(s->width * scale));
    const int height = int(std::ceil(s->height * scale));
    const int header_rows = int(std::ceil(s->title_height * scale));
    const int buffer_height = height + header_rows;
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
        buffer_height > 16384 || uint64_t(width) * buffer_height > 16777216u) {
        wayland_fail(s, "requested buffer exceeds CPU surface budget");
        return false;
    }
    if (o->client_w != width || o->client_h != height || o->scale != scale || s->header_rows != header_rows) {
        s->pixels.resize(size_t(width) * buffer_height * 4);
        s->header_rows = header_rows;
        s->buffer_height = buffer_height;
        o->client_w = width;
        o->client_h = height;
        o->scale = scale;
        o->canvas.w = width;
        o->canvas.h = height;
        o->canvas.stride = width * 4;
        o->canvas.px = s->pixels.data() + size_t(header_rows) * width * 4;
        o->canvas.clear_clip();
        o->ui.impl->canvas = &o->canvas;
        if (o->text) o->text->reconfigure(scale, o->theme.font_family, o->theme.font_family_mono);
    }
    s->resize_pending = false;
    return true;
}
inline std::unique_ptr<WaylandState::Buffer> wayland_buffer(WaylandState* s) {
    const int width = s->owner->client_w, height = s->buffer_height;
    const size_t bytes = size_t(width) * height * 4;
    // mkstemp + unlink 兼容旧内核；共享内存只活到 buffer release，不留下命名文件。
    const char* runtime = std::getenv("XDG_RUNTIME_DIR");
    if (!runtime || !*runtime) return {};
    std::string path = std::string(runtime) + "/aino-shm-XXXXXX";
    const int fd = mkstemp(&path[0]);
    if (fd < 0) return {};
    unlink(path.c_str());
    ScopeExit close_fd([fd] { ::close(fd); });
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0 || ftruncate(fd, off_t(bytes)) < 0) return {};
    auto buffer = std::make_unique<WaylandState::Buffer>();
    buffer->pixels = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer->pixels == MAP_FAILED) return {};
    buffer->bytes = bytes;
    auto* pool = wl::create(s->shm, 0, wl::wl_shm_pool, fd, int32_t(bytes));
    if (!pool) return {};
    buffer->proxy = wl::create(pool, 0, wl::wl_buffer, 0, width, height, width * 4, uint32_t(1));
    wl::destroy(pool, 1);
    if (!buffer->proxy) return {};
    buffer->width = width;
    buffer->height = height;
    static const struct { void (*release)(void*, wl::Proxy*); } listener{
        [](void* data, wl::Proxy*) {
            auto* buffer = static_cast<WaylandState::Buffer*>(data);
            buffer->busy = false;
        }
    };
    wl::listen(buffer->proxy, listener, buffer.get());
    return buffer;
}
inline void wayland_present(WaylandState* s) {
    auto* o = s->owner;
    WaylandState::Buffer* target = nullptr;
    for (auto& buffer : s->buffers) if (!buffer->busy) {
        if (buffer->width != o->client_w || buffer->height != s->buffer_height) {
            buffer = wayland_buffer(s);
            if (!buffer) { wayland_fail(s, "cannot allocate shared memory buffer"); return; }
        }
        target = buffer.get();
        break;
    }
    if (!target && s->buffers.size() < 3) {
        auto buffer = wayland_buffer(s);
        if (!buffer) { wayland_fail(s, "cannot allocate shared memory buffer"); return; }
        target = buffer.get();
        s->buffers.push_back(std::move(buffer));
    }
    if (!target) return;
#if AINO_BIG_ENDIAN
    auto* output = static_cast<uint32_t*>(target->pixels);
    for (size_t i = 0; i < target->bytes / 4; ++i) {
        const auto* pixel = s->pixels.data() + i * 4;
        output[i] = 0xff000000u | uint32_t(pixel[CH_R]) << 16 | uint32_t(pixel[CH_G]) << 8 | pixel[CH_B];
    }
#else
    std::memcpy(target->pixels, s->pixels.data(), target->bytes);
#endif
    if (s->viewport) {
        wl::request(s->surface, 8, 1);
        wl::request(s->viewport, 2, s->width, s->height + s->title_height);
    } else wl::request(s->surface, 8, int(o->scale));
    wl::request(s->surface, 1, target->proxy, 0, 0);
    wl::request(s->surface, 9, 0, 0, o->client_w, s->buffer_height);
    wl::request(s->xdg_surface, 3, 0, 0, s->width, s->height + s->title_height);
    s->frame = wl::create(s->surface, 3, wl::wl_callback);
    static const struct { void (*done)(void*, wl::Proxy*, uint32_t); } listener{
        [](void* data, wl::Proxy*, uint32_t) {
            auto* s = static_cast<WaylandState*>(data);
            wl::local_destroy(s->frame);
        }
    };
    if (s->frame) wl::listen(s->frame, listener, s);
    target->busy = true;
    s->completed_frame = false;
    wl::request(s->surface, 6);
    wayland_flush(s);
}

inline int wayland_key(uint32_t symbol) {
    if (symbol >= 'a' && symbol <= 'z') return int(symbol - 'a' + 'A');
    if (symbol >= 32 && symbol <= 126) return int(symbol);
    if (symbol >= 0xffbe && symbol <= 0xffc9) return KEY_F1 + int(symbol - 0xffbe);
    if (symbol >= 0xffb0 && symbol <= 0xffb9) return '0' + int(symbol - 0xffb0);
    switch (symbol) {
    case 0xff08: return KEY_BACKSPACE;
    case 0xff09: case 0xfe20: return KEY_TAB;
    case 0xff0d: case 0xff8d: return KEY_ENTER;
    case 0xffaa: return '*';
    case 0xffab: return '+';
    case 0xffad: return '-';
    case 0xffae: return '.';
    case 0xffaf: return '/';
    case 0xff1b: return KEY_ESC;
    case 0xffff: case 0xff9f: return KEY_DELETE;
    case 0xff51: case 0xff96: return KEY_LEFT;
    case 0xff53: case 0xff98: return KEY_RIGHT;
    case 0xff52: case 0xff97: return KEY_UP;
    case 0xff54: case 0xff99: return KEY_DOWN;
    case 0xff50: case 0xff95: return KEY_HOME;
    case 0xff57: case 0xff9c: return KEY_END;
    case 0xff55: case 0xff9a: return KEY_PAGEUP;
    case 0xff56: case 0xff9b: return KEY_PAGEDOWN;
    case 0xffe1: case 0xffe2: return KEY_SHIFT;
    case 0xffe3: case 0xffe4: return KEY_CTRL;
    case 0xffe9: case 0xffea: case 0xfe03: return KEY_ALT;
    case 0xffeb: case 0xffec: return KEY_SUPER;
    default: return KEY_NONE;
    }
}
inline void wayland_reset_input(WaylandState* s) {
    std::fill_n(s->mapped_keys, 768, KEY_NONE);
    std::fill_n(s->key_down, KEY_COUNT, false);
    std::fill_n(s->key_pressed, KEY_COUNT, false);
    std::fill_n(s->key_repeat, KEY_COUNT, 0);
    std::fill_n(s->btn_down, 3, false);
    std::fill_n(s->btn_pressed, 3, false);
    std::fill_n(s->btn_released, 3, false);
    s->repeat_key = 0;
    s->text_in.clear();
    s->owner->ui.impl->active = 0;
    s->owner->ui.impl->ime_comp.clear();
    if (s->compose) wl::xkb().compose_reset(s->compose);
    s->owner->need_frame = true;
}
inline void wayland_modifiers(WaylandState* s) {
    if (!s->xkb_state) return;
    auto& xkb = wl::xkb();
    s->key_down[KEY_SHIFT] = xkb.mod_active(s->xkb_state, "Shift", 8) > 0;
    s->key_down[KEY_CTRL] = xkb.mod_active(s->xkb_state, "Control", 8) > 0;
    s->key_down[KEY_ALT] = xkb.mod_active(s->xkb_state, "Mod1", 8) > 0;
    s->key_down[KEY_SUPER] = xkb.mod_active(s->xkb_state, "Mod4", 8) > 0;
}
inline void wayland_key_event(WaylandState* s, uint32_t key, bool down, bool repeat) {
    if (!s->xkb_state || key >= 768) return;
    auto& xkb = wl::xkb();
    const uint32_t symbol = xkb.sym(s->xkb_state, key + 8);
    int mapped = s->mapped_keys[key];
    if (down && !repeat) {
        uint32_t base = symbol;
        const uint32_t* symbols = nullptr;
        if (xkb.symbols(s->keymap, key + 8, xkb.layout(s->xkb_state, key + 8), 0, &symbols) == 1 &&
            symbols && symbols[0] >= 32 && symbols[0] <= 126) base = symbols[0];
        mapped = s->mapped_keys[key] = wayland_key(base);
    }
    if (mapped > 0 && mapped < KEY_COUNT) {
        if (repeat) {
            if (s->key_repeat[mapped] < 255) ++s->key_repeat[mapped];
        } else {
            s->key_down[mapped] = down;
            if (down) s->key_pressed[mapped] = true;
        }
        if (down) {
            s->key_ctrl[mapped] = s->key_down[KEY_CTRL];
            s->key_shift[mapped] = s->key_down[KEY_SHIFT];
            s->key_alt[mapped] = s->key_down[KEY_ALT];
        }
    }
    if (!down) s->mapped_keys[key] = KEY_NONE;
    if (down && !s->key_down[KEY_CTRL] && !s->key_down[KEY_ALT] && !s->key_down[KEY_SUPER]) {
        // v3 可用不代表输入法正在消费按键；普通输入仍走 xkb，预编辑期间由输入法提交。
        if (s->owner->ui.impl->ime_comp.empty()) {
            char buffer[128];
            int length = 0;
            bool composed = false;
            if (s->compose) {
                xkb.compose_feed(s->compose, symbol);
                const int status = xkb.compose_status(s->compose);
                if (status == 1) composed = true;
                else if (status == 2) {
                    length = xkb.compose_utf8(s->compose, buffer, sizeof buffer);
                    xkb.compose_reset(s->compose);
                    composed = true;
                } else if (status == 3) { xkb.compose_reset(s->compose); composed = true; }
            }
            if (!composed) length = xkb.utf8(s->xkb_state, key + 8, buffer, sizeof buffer);
            if (length > 0 && size_t(length) < sizeof buffer)
                for (int i = 0; i < length; ++i) if (uint8_t(buffer[i]) >= 32 || buffer[i] == '\t')
                    s->text_in.push_back(buffer[i]);
        }
    }
    s->owner->need_frame = true;
}
inline void wayland_repeat(WaylandState* s) {
    if (!s->repeat_key || s->repeat_rate <= 0 || !s->keyboard_inside) return;
    const auto now = std::chrono::steady_clock::now();
    const auto step = std::chrono::microseconds(1000000 / std::max(1, s->repeat_rate));
    for (int count = 0; count < 32 && now >= s->repeat_at; ++count) {
        wayland_key_event(s, s->repeat_key, true, true);
        s->repeat_at += step;
    }
    if (now >= s->repeat_at) s->repeat_at = now + step;
}

inline void wayland_cursor(WaylandState* s) {
    if (!s->pointer || !s->pointer_inside || !s->pointer_serial || !s->cursor_surface) return;
    auto& api = wl::cursors();
    if (!api.load) return;
    const int scale = std::max(1, int(std::ceil(s->owner->scale)));
    if (s->cursor_scale != scale) {
        if (s->cursor_theme) api.destroy(s->cursor_theme);
        const char* theme = std::getenv("XCURSOR_THEME");
        int size = 24;
        if (const char* value = std::getenv("XCURSOR_SIZE")) size = std::max(8, std::min(128, std::atoi(value)));
        s->cursor_theme = api.load(theme, size * scale, s->shm);
        s->cursor_scale = scale;
        s->cursor_index = -1;
    }
    const int index = s->my < 0 ? 0 : std::max(0, std::min(5, int(s->owner->cursor)));
    if (!s->cursor_theme || s->cursor_index == index) return;
    const char* names[] = {"left_ptr", "hand2", "xterm", "sb_h_double_arrow", "sb_v_double_arrow", "xterm"};
    auto* cursor = api.get(s->cursor_theme, names[index]);
    if (!cursor) cursor = api.get(s->cursor_theme, "left_ptr");
    if (!cursor || !cursor->image_count) return;
    auto* image = cursor->images[0];
    auto* buffer = api.buffer(image);
    if (!buffer) return;
    wl::request(s->pointer, 0, s->pointer_serial, s->cursor_surface,
        int(image->hotspot_x / scale), int(image->hotspot_y / scale));
    wl::request(s->cursor_surface, 8, scale);
    wl::request(s->cursor_surface, 1, buffer, 0, 0);
    wl::request(s->cursor_surface, 2, 0, 0, int(image->width), int(image->height));
    wl::request(s->cursor_surface, 6);
    s->cursor_index = index;
}

inline void wayland_set_ime_pos(WaylandState* s, float x, float top, float bottom) {
    if (!s) return;
    const float scale = s->owner->scale;
    s->caret = {x / scale, top / scale + s->title_height, 1, std::max(1.f, (bottom - top) / scale)};
}
inline void wayland_text_state(WaylandState* s) {
    if (!s->text_input) return;
    const bool enabled = s->text_entered && s->owner->ui.impl->ime_owner;
    const bool reset = enabled && s->text_focus != s->owner->ui.impl->focus_id;
    bool changed = enabled != s->text_enabled || reset;
    if (changed) {
        wl::request(s->text_input, enabled ? 1 : 2);
        s->text_enabled = enabled;
        s->owner->ui.impl->ime_comp.clear();
        if (s->compose) wl::xkb().compose_reset(s->compose);
        s->text_focus = enabled ? s->owner->ui.impl->focus_id : 0;
    }
    if (enabled && (changed || s->content_hint != s->sent_hint || s->content_purpose != s->sent_purpose)) {
        wl::request(s->text_input, 5, s->content_hint, s->content_purpose);
        s->sent_hint = s->content_hint;
        s->sent_purpose = s->content_purpose;
        changed = true;
    }
    if (enabled && (changed || s->caret.x != s->sent_caret.x || s->caret.y != s->sent_caret.y ||
        s->caret.h != s->sent_caret.h)) {
        wl::request(s->text_input, 6, int(s->caret.x), int(s->caret.y), 1, int(std::ceil(s->caret.h)));
        s->sent_caret = s->caret;
        changed = true;
    }
    if (changed) { wl::request(s->text_input, 7); ++s->text_serial; }
}

inline void wayland_size_hints(WaylandState* s) {
    const auto& cfg = s->owner->cfg;
    const int width = std::max(1, cfg.width);
    const int height = std::max(1, cfg.height);
    const int min_w = cfg.resizable ? std::max(0, cfg.min_width) : width;
    const int min_h = cfg.resizable ? std::max(0, cfg.min_height) : height;
    const int max_w = cfg.resizable ? std::max(0, cfg.max_width) : width;
    const int max_h = cfg.resizable ? std::max(0, cfg.max_height) : height;
    wl::request(s->toplevel, 8, min_w, min_h ? min_h + s->title_height : 0);
    wl::request(s->toplevel, 7, max_w ? std::max(min_w, max_w) : 0,
        max_h ? std::max(min_h, max_h) + s->title_height : 0);
}
inline int wayland_title_button(const WaylandState* s) {
    if (!s->title_height || s->my < -s->title_height || s->my >= 0) return 0;
    const float y = s->my + s->title_height;
    if (y < 6 || y >= 26) return 0;
    if (s->mx >= 10 && s->mx < 30) return 1;
    if (s->mx >= 30 && s->mx < 50) return 2;
    if (s->mx >= 50 && s->mx < 70 && s->owner->cfg.resizable) return 3;
    return 0;
}
inline bool wayland_title_event(WaylandState* s, uint32_t serial, uint32_t time, bool down) {
    if (!s->title_height) return false;
    if (s->title_pressed) {
        if (!down) {
            if (wayland_title_button(s) == s->title_pressed) {
                if (s->title_pressed == 1) s->owner->running = false;
                else if (s->title_pressed == 2) wl::request(s->toplevel, 13);
                else wl::request(s->toplevel, s->maximized ? 10 : 9);
            }
            s->title_pressed = 0;
        }
        s->owner->need_frame = true;
        return true;
    }
    if (!down) return false;
    const float surface_y = s->my + s->title_height;
    uint32_t edge = s->mx < 4 ? 4 : s->mx >= s->width - 4 ? 8 : 0;
    if (surface_y < 4) edge |= 1;
    else if (s->my >= s->height - 4) edge |= 2;
    if (edge && s->owner->cfg.resizable && !s->maximized) {
        wl::request(s->toplevel, 6, s->seat, serial, edge);
        return true;
    }
    if (s->my >= 0) return false;
    s->title_pressed = wayland_title_button(s);
    if (!s->title_pressed) {
        if (s->owner->cfg.resizable && time - s->click_time < 400 &&
            std::abs(s->mx - s->click_position.x) < 5 && std::abs(s->my - s->click_position.y) < 5)
            wl::request(s->toplevel, s->maximized ? 10 : 9);
        else wl::request(s->toplevel, 5, s->seat, serial);
        s->click_time = time;
        s->click_position = {s->mx, s->my};
    }
    s->owner->need_frame = true;
    return true;
}
inline void wayland_draw_title(WaylandState* s) {
    if (!s->header_rows) return;
    auto* o = s->owner;
    Canvas canvas;
    canvas.px = s->pixels.data();
    canvas.w = o->client_w;
    canvas.h = s->header_rows;
    canvas.stride = o->canvas.stride;
    const auto& theme = o->ui.theme();
    const float scale = o->scale;
    canvas.clear(theme.window_bg);
    const Color colors[] = {Color::hex(0xff5f57), Color::hex(0xfebb2e), Color::hex(0x28c840)};
    for (int i = 0; i < 3; ++i) {
        Color color = i == 2 && !o->cfg.resizable ? theme.separator : colors[i];
        if (s->title_pressed == i + 1) color = color.darken(.15f);
        raster_rrect(canvas, Rect((14.f + i * 20) * scale, 10 * scale, 12 * scale, 12 * scale), color, 6 * scale);
    }
    const float font = 12 * scale;
    const auto& title = o->cfg.title;
    const float available = std::max(0.f, (s->width - 160.f) * scale);
    std::string fitted = title;
    if (o->text->measure(fitted, font, 600) > available) {
        size_t first = 0, last = size_t(utf8_count(title));
        while (first < last) {
            const size_t count = first + (last - first + 1) / 2;
            const auto candidate = title.substr(0, utf8_byte_offset(title, int(count))) + "…";
            if (o->text->measure(candidate, font, 600) <= available) first = count;
            else last = count - 1;
        }
        fitted = first ? title.substr(0, utf8_byte_offset(title, int(first))) + "…" : "";
    }
    const float text_width = o->text->measure(fitted, font, 600);
    o->text->draw(canvas, fitted, (canvas.w - text_width) * .5f,
        (s->header_rows - o->text->line_height(font, 600)) * .5f, font, 600, theme.label,
        Rect(80 * scale, 0, available, float(s->header_rows)));
    canvas.blend_row(s->header_rows - 1, 0, canvas.w, theme.separator);
}

inline void wayland_attach_input(WaylandState* s, uint32_t capabilities) {
    if (!(capabilities & 1)) {
        wl::destroy(s->pointer, 1);
        s->pointer_inside = false;
        std::fill_n(s->btn_down, 3, false);
        s->owner->ui.impl->active = 0;
    } else if (!s->pointer) {
        s->pointer = wl::create(s->seat, 0, wl::wl_pointer);
        static const struct {
            void (*enter)(void*, wl::Proxy*, uint32_t, wl::Proxy*, int32_t, int32_t);
            void (*leave)(void*, wl::Proxy*, uint32_t, wl::Proxy*);
            void (*motion)(void*, wl::Proxy*, uint32_t, int32_t, int32_t);
            void (*button)(void*, wl::Proxy*, uint32_t, uint32_t, uint32_t, uint32_t);
            void (*axis)(void*, wl::Proxy*, uint32_t, uint32_t, int32_t);
            void (*frame)(void*, wl::Proxy*);
            void (*source)(void*, wl::Proxy*, uint32_t);
            void (*stop)(void*, wl::Proxy*, uint32_t, uint32_t);
            void (*discrete)(void*, wl::Proxy*, uint32_t, int32_t);
        } listener{
            [](void* data, wl::Proxy*, uint32_t serial, wl::Proxy*, int32_t x, int32_t y) {
                auto* s = static_cast<WaylandState*>(data);
                s->pointer_serial = serial;
                s->pointer_inside = true;
                s->mx = x / 256.f; s->my = y / 256.f - s->title_height;
                s->cursor_index = -1;
                wayland_cursor(s);
                s->owner->need_frame = true;
            },
            [](void* data, wl::Proxy*, uint32_t, wl::Proxy*) {
                auto* s = static_cast<WaylandState*>(data);
                s->pointer_inside = false;
                s->mx = s->my = -1e6f;
                s->owner->need_frame = true;
            },
            [](void* data, wl::Proxy*, uint32_t, int32_t x, int32_t y) {
                auto* s = static_cast<WaylandState*>(data);
                s->mx = x / 256.f; s->my = y / 256.f - s->title_height;
                if (!s->owner->hover_skip || s->title_pressed || s->owner->ui.impl->hover_would_change(s->mx, s->my)) {
                    s->owner->need_frame = true;
                    s->owner->move_only = true;
                }
            },
            [](void* data, wl::Proxy*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
                auto* s = static_cast<WaylandState*>(data);
                s->serial = serial;
                const int index = button == 0x110 ? 0 : button == 0x111 ? 1 : button == 0x112 ? 2 : -1;
                if (index < 0) return;
                if (index == 0 && wayland_title_event(s, serial, time, state != 0)) return;
                s->btn_down[index] = state != 0;
                if (state) {
                    s->btn_pressed[index] = true;
                    if (index == 0) {
                        const float dx = s->mx - s->click_position.x, dy = s->my - s->click_position.y;
                        s->clicks = time - s->click_time < 400 && dx * dx + dy * dy < 25 ? 2 : 1;
                        s->click_time = time;
                        s->click_position = {s->mx, s->my};
                    }
                } else s->btn_released[index] = true;
                s->owner->move_only = false;
                s->owner->need_frame = true;
            },
            [](void* data, wl::Proxy*, uint32_t, uint32_t axis, int32_t value) {
                auto* s = static_cast<WaylandState*>(data);
                if (axis == 0) { s->wheel -= value / 2560.f; s->owner->need_frame = true; }
            },
            [](void*, wl::Proxy*) {}, [](void*, wl::Proxy*, uint32_t) {},
            [](void*, wl::Proxy*, uint32_t, uint32_t) {}, [](void*, wl::Proxy*, uint32_t, int32_t) {}
        };
        wl::listen(s->pointer, listener, s);
    }
    if (!(capabilities & 2)) {
        wl::destroy(s->keyboard);
        s->keyboard_inside = false;
        wayland_reset_input(s);
    } else if (!s->keyboard) {
        s->keyboard = wl::create(s->seat, 1, wl::wl_keyboard);
        static const struct {
            void (*keymap)(void*, wl::Proxy*, uint32_t, int32_t, uint32_t);
            void (*enter)(void*, wl::Proxy*, uint32_t, wl::Proxy*, wl::Array*);
            void (*leave)(void*, wl::Proxy*, uint32_t, wl::Proxy*);
            void (*key)(void*, wl::Proxy*, uint32_t, uint32_t, uint32_t, uint32_t);
            void (*modifiers)(void*, wl::Proxy*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
            void (*repeat)(void*, wl::Proxy*, int32_t, int32_t);
        } listener{
            [](void* data, wl::Proxy*, uint32_t format, int32_t fd, uint32_t size) {
                auto* s = static_cast<WaylandState*>(data);
                ScopeExit close_fd([fd] { ::close(fd); });
                if (format != 1 || !size || size > 16u * 1024 * 1024) return;
                void* bytes = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
                if (bytes == MAP_FAILED) return;
                ScopeExit unmap([=] { munmap(bytes, size); });
                if (static_cast<const char*>(bytes)[size - 1] != 0) return;
                auto& api = wl::xkb();
                auto* keymap = api.keymap_new(s->context, static_cast<const char*>(bytes), 1, 0);
                auto* state = keymap ? api.state_new(keymap) : nullptr;
                if (!state) { if (keymap) api.keymap_unref(keymap); return; }
                if (s->xkb_state) api.state_unref(s->xkb_state);
                if (s->keymap) api.keymap_unref(s->keymap);
                s->keymap = keymap; s->xkb_state = state;
                wayland_reset_input(s);
            },
            [](void* data, wl::Proxy*, uint32_t serial, wl::Proxy*, wl::Array* keys) {
                auto* s = static_cast<WaylandState*>(data);
                s->serial = serial; s->keyboard_inside = true;
                if (s->xkb_state && keys) for (size_t i = 0; i < keys->size / sizeof(uint32_t); ++i) {
                    const auto key = static_cast<uint32_t*>(keys->data)[i];
                    const int mapped = wayland_key(wl::xkb().sym(s->xkb_state, key + 8));
                    if (mapped > 0 && mapped < KEY_COUNT && key < 768) {
                        s->key_down[mapped] = true;
                        s->mapped_keys[key] = mapped;
                    }
                }
                wayland_modifiers(s);
                s->owner->need_frame = true;
            },
            [](void* data, wl::Proxy*, uint32_t, wl::Proxy*) {
                auto* s = static_cast<WaylandState*>(data);
                s->keyboard_inside = false;
                wayland_reset_input(s);
            },
            [](void* data, wl::Proxy*, uint32_t serial, uint32_t, uint32_t key, uint32_t state) {
                auto* s = static_cast<WaylandState*>(data);
                s->serial = serial;
                wayland_key_event(s, key, state != 0, false);
                if (state && s->keymap && wl::xkb().repeats(s->keymap, key + 8) && s->repeat_rate > 0) {
                    s->repeat_key = key;
                    s->repeat_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(s->repeat_delay);
                } else if (!state && key == s->repeat_key) s->repeat_key = 0;
            },
            [](void* data, wl::Proxy*, uint32_t, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
                auto* s = static_cast<WaylandState*>(data);
                if (s->xkb_state) wl::xkb().update_mask(s->xkb_state, depressed, latched, locked, 0, 0, group);
                wayland_modifiers(s);
            },
            [](void* data, wl::Proxy*, int32_t rate, int32_t delay) {
                auto* s = static_cast<WaylandState*>(data);
                s->repeat_rate = std::max(0, std::min(1000, rate));
                s->repeat_delay = std::max(0, std::min(10000, delay));
                if (!rate) s->repeat_key = 0;
            }
        };
        wl::listen(s->keyboard, listener, s);
    }
}

inline ssize_t wayland_write(int fd, const char* bytes, size_t size) {
    // compositor 给出的通常是 pipe；局部屏蔽 SIGPIPE，不能修改应用的全局信号处理。
    sigset_t blocked, old, pending;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &blocked, &old) != 0) { errno = EIO; return -1; }
    sigpending(&pending);
    const bool had_pipe = sigismember(&pending, SIGPIPE) == 1;
    const auto written = ::write(fd, bytes, size);
    const int saved = errno;
    if (written < 0 && saved == EPIPE && !had_pipe) {
        timespec timeout{};
        while (sigtimedwait(&blocked, nullptr, &timeout) < 0 && errno == EINTR) {}
    }
    pthread_sigmask(SIG_SETMASK, &old, nullptr);
    errno = saved;
    return written;
}
inline void wayland_send_clipboard(WaylandState* s) {
    const auto now = std::chrono::steady_clock::now();
    for (auto it = s->sends.begin(); it != s->sends.end();) {
        bool done = now >= it->deadline;
        if (!done) {
            const size_t remaining = it->text->size() - it->offset;
            const auto written = wayland_write(it->fd, it->text->data() + it->offset, std::min<size_t>(remaining, 65536));
            if (written > 0) it->offset += size_t(written);
            else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) done = true;
            done |= it->offset == it->text->size();
        }
        if (done) { ::close(it->fd); it = s->sends.erase(it); }
        else ++it;
    }
}
inline void wayland_trim_offers(WaylandState* s) {
    for (auto it = s->offers.begin(); it != s->offers.end();) {
        if (it->proxy == s->selection || it->proxy == s->drag_offer || it->proxy == s->drop_offer) ++it;
        else { wl::destroy(it->proxy, 2); it = s->offers.erase(it); }
    }
}

inline void wayland_drag_update(WaylandState* s, int32_t x, int32_t y, bool enter) {
    if (!s->drag_offer) return;
    s->drag_x = x / 256.f;
    s->drag_y = y / 256.f - s->title_height;
    const auto offer = std::find_if(s->offers.begin(), s->offers.end(), [&](const WaylandState::Offer& item) {
        return item.proxy == s->drag_offer;
    });
    const bool accept = offer != s->offers.end() && offer->uri_list && s->file_drag &&
                        s->drop_fd < 0 && s->file_drag(s->drag_x, s->drag_y);
    if (enter || accept != s->drag_accepted) {
        wl::request(s->drag_offer, 0, s->drag_serial, accept ? "text/uri-list" : nullptr);
        wl::request(s->drag_offer, 4, accept ? 1u : 0u, accept ? 1u : 0u);
        s->drag_accepted = accept;
    }
}

inline void wayland_drag_leave(WaylandState* s) {
    if (s->file_drag) s->file_drag(-1, -1);
    s->drag_offer = nullptr;
    s->drag_accepted = false;
    wayland_trim_offers(s);
}

inline void wayland_receive_drop(WaylandState* s) {
    const auto offer = std::find_if(s->offers.begin(), s->offers.end(), [&](const WaylandState::Offer& item) {
        return item.proxy == s->drag_offer;
    });
    if (!s->drag_accepted || !s->file_drop || offer == s->offers.end() || offer->action != 1) return;
    auto complete = s->file_drop(s->drag_x, s->drag_y);
    if (!complete) return;
    int transfer[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, transfer) < 0) return;
    if (fcntl(transfer[0], F_SETFL, O_NONBLOCK) < 0) {
        ::close(transfer[0]);
        ::close(transfer[1]);
        return;
    }
    s->drop_offer = s->drag_offer;
    s->drop_fd = transfer[0];
    s->drop_complete = std::move(complete);
    s->drop_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    wl::request(s->drop_offer, 1, "text/uri-list", transfer[1]);
    ::close(transfer[1]);
}

inline void wayland_read_drop(WaylandState* s) {
    if (s->drop_fd < 0) return;
    bool received = false;
    while (std::chrono::steady_clock::now() < s->drop_deadline) {
        char chunk[8192];
        const auto count = ::read(s->drop_fd, chunk, sizeof chunk);
        if (count == 0) { received = true; break; }
        if (count < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            break;
        }
        if (s->drop_data.size() + size_t(count) > (1u << 20)) break;
        s->drop_data.append(chunk, size_t(count));
    }
    ::close(s->drop_fd);
    s->drop_fd = -1;
    if (received) wl::request(s->drop_offer, 3);
    s->drop_offer = nullptr;
    wayland_trim_offers(s);
    auto complete = std::move(s->drop_complete);
    std::string payload;
    if (received) payload.swap(s->drop_data);
    s->drop_data.clear();
    complete(std::move(payload));
}
inline void wayland_attach_data(WaylandState* s) {
    if (!s->data_manager || !s->seat || s->data_device) return;
    s->data_device = wl::create(s->data_manager, 1, wl::wl_data_device, s->seat);
    static const struct {
        void (*offer)(void*, wl::Proxy*, wl::Proxy*);
        void (*enter)(void*, wl::Proxy*, uint32_t, wl::Proxy*, int32_t, int32_t, wl::Proxy*);
        void (*leave)(void*, wl::Proxy*);
        void (*motion)(void*, wl::Proxy*, uint32_t, int32_t, int32_t);
        void (*drop)(void*, wl::Proxy*);
        void (*selection)(void*, wl::Proxy*, wl::Proxy*);
    } listener{
        [](void* data, wl::Proxy*, wl::Proxy* offer) {
            auto* s = static_cast<WaylandState*>(data);
            s->offers.push_back({offer, {}});
            static const struct {
                void (*mime)(void*, wl::Proxy*, const char*);
                void (*actions)(void*, wl::Proxy*, uint32_t);
                void (*action)(void*, wl::Proxy*, uint32_t);
            } offer_listener{
                [](void* data, wl::Proxy* offer, const char* mime) {
                    auto* s = static_cast<WaylandState*>(data);
                    if (!mime) return;
                    for (auto& item : s->offers) if (item.proxy == offer) {
                        if (std::strcmp(mime, "text/plain;charset=utf-8") == 0 ||
                            (item.mime.empty() && std::strcmp(mime, "text/plain") == 0)) item.mime = mime;
                        else if (std::strcmp(mime, "text/uri-list") == 0) item.uri_list = true;
                    }
                },
                [](void*, wl::Proxy*, uint32_t) {},
                [](void* data, wl::Proxy* proxy, uint32_t action) {
                    auto* s = static_cast<WaylandState*>(data);
                    for (auto& item : s->offers) if (item.proxy == proxy) item.action = action;
                }
            };
            wl::listen(offer, offer_listener, s);
        },
        [](void* data, wl::Proxy*, uint32_t serial, wl::Proxy*, int32_t x, int32_t y, wl::Proxy* offer) {
            auto* s = static_cast<WaylandState*>(data);
            s->drag_offer = offer;
            s->drag_serial = serial;
            wayland_drag_update(s, x, y, true);
            wayland_trim_offers(s);
        },
        [](void* data, wl::Proxy*) { wayland_drag_leave(static_cast<WaylandState*>(data)); },
        [](void* data, wl::Proxy*, uint32_t, int32_t x, int32_t y) {
            wayland_drag_update(static_cast<WaylandState*>(data), x, y, false);
        },
        [](void* data, wl::Proxy*) {
            auto* s = static_cast<WaylandState*>(data);
            wayland_receive_drop(s);
            wayland_drag_leave(s);
        },
        [](void* data, wl::Proxy*, wl::Proxy* offer) {
            auto* s = static_cast<WaylandState*>(data);
            s->selection = offer;
            wayland_trim_offers(s);
        }
    };
    wl::listen(s->data_device, listener, s);
}
inline bool wayland_set_clipboard(WaylandState* s, const std::string& text) {
    if (!s || s->failed || !s->data_device || !s->serial || !s->keyboard_inside ||
        text.size() > (32u << 20) || s->sources.size() >= 8) return false;
    auto* proxy = wl::create(s->data_manager, 0, wl::wl_data_source);
    if (!proxy) return false;
    s->sources.push_back({proxy, std::make_shared<const std::string>(text)});
    static const struct {
        void (*target)(void*, wl::Proxy*, const char*);
        void (*send)(void*, wl::Proxy*, const char*, int32_t);
        void (*cancelled)(void*, wl::Proxy*);
        void (*drop)(void*, wl::Proxy*);
        void (*finished)(void*, wl::Proxy*);
        void (*action)(void*, wl::Proxy*, uint32_t);
    } listener{
        [](void*, wl::Proxy*, const char*) {},
        [](void* data, wl::Proxy* proxy, const char* mime, int32_t fd) {
            auto* s = static_cast<WaylandState*>(data);
            const bool supported = mime && (std::strcmp(mime, "text/plain;charset=utf-8") == 0 ||
                std::strcmp(mime, "text/plain") == 0);
            if (supported && s->sends.size() < 8) for (const auto& source : s->sources) if (source.proxy == proxy) {
                const int flags = fcntl(fd, F_GETFL);
                if (flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0 && fcntl(fd, F_SETFD, FD_CLOEXEC) == 0) {
                    s->sends.push_back({fd, source.text, 0, std::chrono::steady_clock::now() + std::chrono::seconds(2)});
                    return;
                }
                break;
            }
            ::close(fd);
        },
        [](void* data, wl::Proxy* proxy) {
            auto* s = static_cast<WaylandState*>(data);
            for (auto it = s->sources.begin(); it != s->sources.end(); ++it) if (it->proxy == proxy) {
                wl::destroy(it->proxy, 1);
                s->sources.erase(it);
                break;
            }
        },
        [](void*, wl::Proxy*) {}, [](void*, wl::Proxy*) {}, [](void*, wl::Proxy*, uint32_t) {}
    };
    wl::listen(proxy, listener, s);
    wl::request(proxy, 0, "text/plain;charset=utf-8");
    wl::request(proxy, 0, "text/plain");
    wl::request(s->data_device, 1, proxy, s->serial);
    wayland_flush(s);
    return !s->failed;
}

inline void wayland_attach_text(WaylandState* s) {
    if (!s->text_manager || !s->seat || s->text_input) return;
    s->text_input = wl::create(s->text_manager, 1, wl::zwp_text_input_v3, s->seat);
    static const struct {
        void (*enter)(void*, wl::Proxy*, wl::Proxy*);
        void (*leave)(void*, wl::Proxy*, wl::Proxy*);
        void (*preedit)(void*, wl::Proxy*, const char*, int32_t, int32_t);
        void (*commit)(void*, wl::Proxy*, const char*);
        void (*remove)(void*, wl::Proxy*, uint32_t, uint32_t);
        void (*done)(void*, wl::Proxy*, uint32_t);
    } listener{
        [](void* data, wl::Proxy*, wl::Proxy*) {
            auto* s = static_cast<WaylandState*>(data);
            s->text_entered = true;
            s->owner->need_frame = true;
        },
        [](void* data, wl::Proxy*, wl::Proxy*) {
            auto* s = static_cast<WaylandState*>(data);
            s->text_entered = false;
            s->preedit_pending = s->commit_pending = false;
            s->preedit.clear(); s->commit.clear();
            s->owner->ui.impl->ime_comp.clear();
            wayland_text_state(s);
            s->owner->need_frame = true;
        },
        [](void* data, wl::Proxy*, const char* text, int32_t, int32_t) {
            auto* s = static_cast<WaylandState*>(data);
            s->preedit = text ? text : "";
            s->preedit_pending = true;
        },
        [](void* data, wl::Proxy*, const char* text) {
            auto* s = static_cast<WaylandState*>(data);
            s->commit = text ? text : "";
            s->commit_pending = true;
        },
        // 未声明 surrounding-text 能力；compositor 不应发送依赖上下文的删除。
        [](void*, wl::Proxy*, uint32_t, uint32_t) {},
        [](void* data, wl::Proxy*, uint32_t serial) {
            auto* s = static_cast<WaylandState*>(data);
            if (s->text_entered && s->text_enabled && serial == s->text_serial) {
                if (s->commit_pending) {
                    s->text_in += s->commit;
                    s->owner->ui.impl->ime_comp.clear();
                }
                if (s->preedit_pending) s->owner->ui.impl->ime_comp = s->preedit;
                if (s->commit_pending || s->preedit_pending) s->owner->need_frame = true;
            }
            s->preedit_pending = s->commit_pending = false;
            s->preedit.clear(); s->commit.clear();
        }
    };
    wl::listen(s->text_input, listener, s);
}

inline void wayland_output_scale(WaylandState* s) {
    int scale = 1;
    for (const auto& output : s->outputs) if (output.entered) scale = std::max(scale, output.scale);
    if (s->output_scale != scale) { s->output_scale = scale; wayland_scale_changed(s); }
}
inline void wayland_registry(WaylandState* s) {
    static const struct {
        void (*global)(void*, wl::Proxy*, uint32_t, const char*, uint32_t);
        void (*remove)(void*, wl::Proxy*, uint32_t);
    } listener{
        [](void* data, wl::Proxy*, uint32_t name, const char* interface, uint32_t version) {
            auto* s = static_cast<WaylandState*>(data);
            auto bind = [&](const wl::Interface& type, uint32_t cap) {
                const uint32_t bound = std::min(version, cap);
                return wl::api().marshal(s->registry, 0, &type, bound, 0, name, type.name, bound, nullptr);
            };
            if (std::strcmp(interface, "wl_compositor") == 0 && !s->compositor && version >= 4) {
                s->compositor = bind(wl::wl_compositor, 4); s->compositor_name = name;
            } else if (std::strcmp(interface, "wl_shm") == 0 && !s->shm) {
                s->shm = bind(wl::wl_shm, 1); s->shm_name = name;
                static const struct { void (*format)(void*, wl::Proxy*, uint32_t); } formats{
                    [](void*, wl::Proxy*, uint32_t) {}
                };
                wl::listen(s->shm, formats, s);
            } else if (std::strcmp(interface, "xdg_wm_base") == 0 && !s->shell) {
                s->shell = bind(wl::xdg_wm_base, 1); s->shell_name = name;
                static const struct { void (*ping)(void*, wl::Proxy*, uint32_t); } shell{
                    [](void*, wl::Proxy* object, uint32_t serial) { wl::request(object, 3, serial); }
                };
                wl::listen(s->shell, shell, s);
            } else if (std::strcmp(interface, "wl_seat") == 0 && !s->seat && version >= 5) {
                s->seat = bind(wl::wl_seat, 7); s->seat_name = name;
                static const struct {
                    void (*capabilities)(void*, wl::Proxy*, uint32_t);
                    void (*name)(void*, wl::Proxy*, const char*);
                } seat{
                    [](void* data, wl::Proxy*, uint32_t caps) { wayland_attach_input(static_cast<WaylandState*>(data), caps); },
                    [](void*, wl::Proxy*, const char*) {}
                };
                wl::listen(s->seat, seat, s);
                wayland_attach_data(s);
                wayland_attach_text(s);
            } else if (std::strcmp(interface, "wl_data_device_manager") == 0 && !s->data_manager && version >= 3) {
                s->data_manager = bind(wl::wl_data_device_manager, 3);
                wayland_attach_data(s);
            } else if (std::strcmp(interface, "zwp_text_input_manager_v3") == 0 && !s->text_manager) {
                s->text_manager = bind(wl::zwp_text_input_manager_v3, 1);
                wayland_attach_text(s);
            } else if (std::strcmp(interface, "wp_viewporter") == 0 && !s->viewporter) {
                s->viewporter = bind(wl::wp_viewporter, 1);
            } else if (std::strcmp(interface, "wp_fractional_scale_manager_v1") == 0 && !s->fractional_manager) {
                s->fractional_manager = bind(wl::wp_fractional_scale_manager_v1, 1);
            } else if (std::strcmp(interface, "zxdg_decoration_manager_v1") == 0 && !s->decoration_manager) {
                s->decoration_manager = bind(wl::zxdg_decoration_manager_v1, 1);
            } else if (std::strcmp(interface, "wl_output") == 0) {
                auto* proxy = bind(wl::wl_output, 2);
                s->outputs.push_back({proxy, 1, name, false});
                static const struct {
                    void (*geometry)(void*, wl::Proxy*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t);
                    void (*mode)(void*, wl::Proxy*, uint32_t, int32_t, int32_t, int32_t);
                    void (*done)(void*, wl::Proxy*);
                    void (*scale)(void*, wl::Proxy*, int32_t);
                } output{
                    [](void*, wl::Proxy*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {},
                    [](void*, wl::Proxy*, uint32_t, int32_t, int32_t, int32_t) {},
                    [](void* data, wl::Proxy*) { wayland_output_scale(static_cast<WaylandState*>(data)); },
                    [](void* data, wl::Proxy* proxy, int32_t factor) {
                        auto* s = static_cast<WaylandState*>(data);
                        for (auto& output : s->outputs) if (output.proxy == proxy) output.scale = std::max(1, std::min(8, factor));
                    }
                };
                wl::listen(proxy, output, s);
            }
        },
        [](void* data, wl::Proxy*, uint32_t name) {
            auto* s = static_cast<WaylandState*>(data);
            if (name == s->compositor_name || name == s->shm_name || name == s->shell_name)
                wayland_fail(s, "required compositor global removed");
            if (name == s->seat_name) {
                wayland_attach_input(s, 0);
                wl::destroy(s->text_input);
                wl::destroy(s->data_device, 2);
                wl::destroy(s->seat, 3);
                s->seat_name = 0;
                s->serial = s->pointer_serial = s->text_serial = 0;
                s->text_entered = s->text_enabled = false;
                s->selection = nullptr;
                wayland_trim_offers(s);
            }
            for (auto it = s->outputs.begin(); it != s->outputs.end(); ++it) if (it->name == name) {
                wl::local_destroy(it->proxy);
                s->outputs.erase(it);
                wayland_output_scale(s);
                break;
            }
        }
    };
    wl::listen(s->registry, listener, s);
}

inline void wayland_destroy(Window* self) {
    auto* o = self->impl;
    auto* s = static_cast<WaylandState*>(o->native);
    if (!s) return;
    auto& windows = wayland_windows();
    windows.erase(std::remove(windows.begin(), windows.end(), s), windows.end());
    for (const auto& send : s->sends) ::close(send.fd);
    if (s->drop_fd >= 0) ::close(s->drop_fd);
    for (auto& source : s->sources) wl::destroy(source.proxy, 1);
    s->selection = nullptr;
    s->drag_offer = s->drop_offer = nullptr;
    wayland_trim_offers(s);
    wl::local_destroy(s->frame);
    wl::destroy(s->text_input);
    wl::destroy(s->data_device, 2);
    wl::destroy(s->pointer, 1);
    wl::destroy(s->keyboard);
    wl::destroy(s->seat, 3);
    wl::destroy(s->fractional);
    wl::destroy(s->viewport);
    wl::destroy(s->decoration);
    wl::destroy(s->toplevel);
    wl::destroy(s->xdg_surface);
    wl::destroy(s->surface);
    wl::destroy(s->cursor_surface);
    if (s->cursor_theme) wl::cursors().destroy(s->cursor_theme);
    s->buffers.clear();
    for (auto& output : s->outputs) wl::local_destroy(output.proxy);
    wl::destroy(s->decoration_manager);
    wl::destroy(s->fractional_manager);
    wl::destroy(s->viewporter);
    wl::destroy(s->text_manager);
    wl::local_destroy(s->data_manager);
    wl::destroy(s->shell);
    wl::local_destroy(s->compositor);
    wl::local_destroy(s->shm);
    wl::local_destroy(s->registry);
    if (s->display) { wl::api().flush(s->display); wl::api().disconnect(s->display); }
    auto& xkb = wl::xkb();
    if (s->compose) xkb.compose_unref(s->compose);
    if (s->compose_table) xkb.compose_table_unref(s->compose_table);
    if (s->xkb_state) xkb.state_unref(s->xkb_state);
    if (s->keymap) xkb.keymap_unref(s->keymap);
    if (s->context) xkb.context_unref(s->context);
    o->canvas.px = nullptr;
    o->canvas.w = o->canvas.h = o->canvas.stride = 0;
    delete s;
    o->native = nullptr;
}

inline void wayland_create(Window* self, const WindowConfig& cfg) {
    if (!wl::available() || !wl::keyboard_available()) {
        std::fprintf(stderr, "[aino/wayland] requires libwayland-client >= 1.20 and libxkbcommon\n");
        return;
    }
    auto* o = self->impl;
    auto state = std::make_unique<WaylandState>();
    auto* s = state.get();
    s->owner = o;
    s->display = wl::api().connect(nullptr);
    if (!s->display) { std::fprintf(stderr, "[aino/wayland] cannot connect to WAYLAND_DISPLAY\n"); return; }
    o->native = state.release();
    bool ready = false;
    ScopeExit cleanup([&] { if (!ready) wayland_destroy(self); });
    s->context = wl::xkb().context_new(0);
    if (!s->context) return;
    const char* locale = std::setlocale(LC_CTYPE, nullptr);
    if (!locale || std::strcmp(locale, "C") == 0) {
        locale = std::getenv("LC_ALL");
        if (!locale || !*locale) locale = std::getenv("LC_CTYPE");
        if (!locale || !*locale) locale = std::getenv("LANG");
    }
    s->compose_table = wl::xkb().compose_table(s->context, locale && *locale ? locale : "C.UTF-8", 0);
    if (s->compose_table) s->compose = wl::xkb().compose_new(s->compose_table, 0);
    s->registry = wl::create(s->display, 1, wl::wl_registry);
    if (!s->registry) return;
    wayland_registry(s);
    if (wl::api().roundtrip(s->display) < 0 || !s->compositor || !s->shm || !s->shell) {
        std::fprintf(stderr, "[aino/wayland] compositor v4, wl_shm and xdg-shell are required\n");
        return;
    }
    s->width = std::max(1, cfg.width);
    s->height = std::max(1, cfg.height);
    s->surface = wl::create(s->compositor, 0, wl::wl_surface);
    s->cursor_surface = wl::create(s->compositor, 0, wl::wl_surface);
    if (!s->surface || !s->cursor_surface) return;
    static const struct {
        void (*enter)(void*, wl::Proxy*, wl::Proxy*);
        void (*leave)(void*, wl::Proxy*, wl::Proxy*);
    } surface{
        [](void* data, wl::Proxy*, wl::Proxy* proxy) {
            auto* s = static_cast<WaylandState*>(data);
            for (auto& output : s->outputs) if (output.proxy == proxy) output.entered = true;
            wayland_output_scale(s);
        },
        [](void* data, wl::Proxy*, wl::Proxy* proxy) {
            auto* s = static_cast<WaylandState*>(data);
            for (auto& output : s->outputs) if (output.proxy == proxy) output.entered = false;
            wayland_output_scale(s);
        }
    };
    wl::listen(s->surface, surface, s);
    if (s->viewporter) s->viewport = wl::create(s->viewporter, 1, wl::wp_viewport, s->surface);
    if (s->fractional_manager && s->viewport) {
        s->fractional = wl::create(s->fractional_manager, 1, wl::wp_fractional_scale_v1, s->surface);
        static const struct { void (*scale)(void*, wl::Proxy*, uint32_t); } listener{
            [](void* data, wl::Proxy*, uint32_t scale) {
                auto* s = static_cast<WaylandState*>(data);
                if (scale >= 60 && scale <= 960 && int(scale) != s->preferred_scale) {
                    s->preferred_scale = int(scale); wayland_scale_changed(s);
                }
            }
        };
        if (s->fractional) wl::listen(s->fractional, listener, s);
    }
    s->xdg_surface = wl::create(s->shell, 2, wl::xdg_surface, s->surface);
    if (!s->xdg_surface) return;
    static const struct { void (*configure)(void*, wl::Proxy*, uint32_t); } configure{
        [](void* data, wl::Proxy* object, uint32_t serial) {
            auto* s = static_cast<WaylandState*>(data);
            wl::request(object, 4, serial);
            s->configured = true;
            wayland_scale_changed(s);
        }
    };
    wl::listen(s->xdg_surface, configure, s);
    s->toplevel = wl::create(s->xdg_surface, 1, wl::xdg_toplevel);
    if (!s->toplevel) return;
    static const struct {
        void (*configure)(void*, wl::Proxy*, int32_t, int32_t, wl::Array*);
        void (*close)(void*, wl::Proxy*);
    } toplevel{
        [](void* data, wl::Proxy*, int32_t width, int32_t height, wl::Array* states) {
            auto* s = static_cast<WaylandState*>(data);
            if (width > 0) s->pending_width = width;
            if (height > 0) s->pending_height = height;
            s->maximized = false;
            if (states) for (size_t i = 0; i < states->size / sizeof(uint32_t); ++i)
                if (static_cast<uint32_t*>(states->data)[i] == 1) s->maximized = true;
        },
        [](void* data, wl::Proxy*) { static_cast<WaylandState*>(data)->owner->running = false; }
    };
    wl::listen(s->toplevel, toplevel, s);
    wl::request(s->toplevel, 2, cfg.title.c_str());
    wl::request(s->toplevel, 3, cfg.app_id.empty() ? "aino" : cfg.app_id.c_str());
    wayland_size_hints(s);
    if (s->decoration_manager) {
        s->decoration = wl::create(s->decoration_manager, 1, wl::zxdg_toplevel_decoration_v1, s->toplevel);
        static const struct { void (*configure)(void*, wl::Proxy*, uint32_t); } decoration{
            [](void* data, wl::Proxy*, uint32_t mode) {
                auto* s = static_cast<WaylandState*>(data);
                const int height = mode == 2 ? 0 : 32;
                if (s->title_height != height) {
                    s->title_height = height;
                    wayland_size_hints(s);
                    wayland_scale_changed(s);
                }
            }
        };
        if (s->decoration) {
            wl::listen(s->decoration, decoration, s);
            wl::request(s->decoration, 1, uint32_t(2));
        }
    }
    auto text = std::make_unique<UnixTextRenderer>();
    text->init(1, o->theme.font_family, o->theme.font_family_mono);
    if (!text->ready()) {
        std::fprintf(stderr, "[aino/wayland] requires FreeType and fontconfig runtime libraries\n");
        return;
    }
    o->text = text.release();
    o->ui.impl->text = o->text;
    o->ui.impl->ime_content_type = [s](bool password, bool multiline) {
        s->content_hint = password ? 64u | 128u : multiline ? 512u : 0u;
        s->content_purpose = password ? 8u : 0u;
    };
    if (!wayland_resize(s)) return;
    wl::request(s->surface, 6);
    wayland_flush(s);
    wayland_windows().push_back(s);
    ready = !s->failed;
}

inline void wayland_feed_input(WaylandState* s) {
    Input& input = s->owner->ui.impl->input;
    input.mouse_prev = input.mouse;
    input.mouse = {s->mx, s->my};
    input.mouse_delta = input.mouse - input.mouse_prev;
    for (int i = 0; i < 3; ++i) {
        input.down[i] = s->btn_down[i];
        input.pressed[i] = s->btn_pressed[i];
        input.released[i] = s->btn_released[i];
        s->btn_pressed[i] = s->btn_released[i] = false;
    }
    input.wheel = s->wheel; s->wheel = 0;
    input.click_count = s->clicks; input.double_click = s->clicks >= 2; s->clicks = 0;
    input.dragging = s->btn_down[0];
    input.text_input.swap(s->text_in); s->text_in.clear();
    for (int i = 0; i < KEY_COUNT; ++i) {
        input.key_down[i] = s->key_down[i];
        input.key_pressed[i] = s->key_pressed[i];
        input.key_repeat[i] = s->key_repeat[i];
        input.key_ctrl[i] = s->key_ctrl[i];
        input.key_shift[i] = s->key_shift[i];
        input.key_alt[i] = s->key_alt[i];
        s->key_pressed[i] = false; s->key_repeat[i] = 0;
    }
}

inline void wayland_wait_fds(WaylandState* s, std::vector<pollfd>& fds, int& timeout, bool input = true) {
    if (!s || s->failed) return;
    const auto now = std::chrono::steady_clock::now();
    auto deadline = [&](std::chrono::steady_clock::time_point end) {
        const int delay = int(std::max<int64_t>(0, std::chrono::ceil<std::chrono::milliseconds>(end - now).count()));
        if (timeout < 0 || delay < timeout) timeout = delay;
    };
    if (input && s->repeat_key && s->keyboard_inside && s->repeat_rate > 0) deadline(s->repeat_at);
    fds.push_back({wl::api().get_fd(s->display), short(POLLIN | (s->flush_blocked ? POLLOUT : 0)), 0});
    if (input) fds.push_back({s->owner->wake_signal.read_descriptor(), POLLIN, 0});
    if (input && s->owner->clipboard_fd >= 0) fds.push_back({s->owner->clipboard_fd, POLLIN, 0});
    for (const auto& send : s->sends) { fds.push_back({send.fd, POLLOUT, 0}); deadline(send.deadline); }
    if (input && s->drop_fd >= 0) { fds.push_back({s->drop_fd, POLLIN, 0}); deadline(s->drop_deadline); }
}
} // namespace detail

int Window::width() const {
    auto* s = static_cast<const detail::WaylandState*>(impl->native);
    return s ? s->width : 0;
}
int Window::height() const {
    auto* s = static_cast<const detail::WaylandState*>(impl->native);
    return s ? s->height : 0;
}
bool Window::poll() {
    auto* s = static_cast<detail::WaylandState*>(impl->native);
    if (!s || !impl->running) return false;
    detail::service_clipboard(impl);
    if (impl->wake_signal.consume()) impl->need_frame = true;
    detail::wayland_dispatch(s);
    if (impl->running) detail::service_clipboard(impl);
    detail::wayland_send_clipboard(s);
    detail::wayland_read_drop(s);
    detail::wayland_repeat(s);
    if (s->completed_frame && s->resize_pending && !impl->ui.impl->frame_open) {
        const int width = impl->client_w;
        const int height = impl->client_h;
        const int header = s->header_rows;
        const float scale = impl->scale;
        if (!detail::wayland_resize(s)) return false;
        // 已完成的图像属于旧尺寸，不能在确认新 configure 后作为新尺寸的帧提交。
        if (width != impl->client_w || height != impl->client_h || header != s->header_rows || scale != impl->scale) {
            s->completed_frame = false;
            impl->need_frame = true;
        }
    }
    if (s->completed_frame && !impl->ui.impl->frame_open && detail::wayland_can_render(s))
        detail::wayland_present(s);
    detail::wayland_flush(s);
    return impl->running;
}
void Window::render() {
    auto* o = impl;
    auto* s = static_cast<detail::WaylandState*>(o->native);
    if (!s || o->ui.impl->frame_open) return;
    if (s->failed) return;
    if (!detail::wayland_resize(s)) return;
    const auto now = detail::prof_now() / 1000;
    float dt = o->last_frame_ms ? (now - o->last_frame_ms) / 1000.f : 1.f / 60;
    if (dt > .25f) { o->ui.impl->caret_blink += dt - .25f; dt = .25f; }
    o->last_frame_ms = now;
    detail::wayland_feed_input(s);
    o->need_frame = false;
    detail::ScopeExit cleanup([o] {
        if (o->ui.impl->frame_open) { o->ui.impl->abort_frame(); o->need_frame = true; }
    });
    o->ui.begin_frame(float(s->width), float(s->height), dt, o->scale);
    if (o->frame_cb_ptr) o->frame_cb_ptr();
    o->ui.end_frame();
    detail::wayland_draw_title(s);
    detail::wayland_text_state(s);
    o->cursor = o->ui.cursor();
    detail::wayland_cursor(s);
    s->completed_frame = true;
    if (detail::wayland_can_render(s)) detail::wayland_present(s);
    else detail::wayland_flush(s);
}
void Window::set_title(const std::string& title) {
    auto* s = static_cast<detail::WaylandState*>(impl->native);
    if (s && !s->failed) {
        impl->cfg.title = title;
        impl->need_frame = true;
        detail::wl::request(s->toplevel, 2, title.c_str());
        detail::wayland_flush(s);
    }
}
void Window::set_size(int width, int height) {
    auto* s = static_cast<detail::WaylandState*>(impl->native);
    if (!s || s->failed) return;
    auto clamp = [](int value, int minimum, int maximum) {
        return std::max(std::max(1, minimum), maximum > 0 ? std::min(value, maximum) : value);
    };
    s->pending_width = clamp(width, impl->cfg.min_width, impl->cfg.max_width);
    s->pending_height = clamp(height, impl->cfg.min_height, impl->cfg.max_height);
    impl->cfg.width = s->pending_width;
    impl->cfg.height = s->pending_height;
    s->pending_height += s->title_height;
    if (!impl->cfg.resizable) detail::wayland_size_hints(s);
    detail::wayland_scale_changed(s);
}
bool Window::try_set_clipboard(const std::string& text) {
    return detail::wayland_set_clipboard(static_cast<detail::WaylandState*>(impl->native), text);
}
std::string Window::clipboard() const {
    auto* s = static_cast<detail::WaylandState*>(impl->native);
    if (!s || s->failed || !s->selection || s->clipboard_reading) return {};
    std::string mime;
    for (const auto& offer : s->offers) if (offer.proxy == s->selection) mime = offer.mime;
    if (mime.empty()) return {};
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) < 0) return {};
    if (fcntl(sockets[0], F_SETFL, O_NONBLOCK) < 0) { ::close(sockets[0]); ::close(sockets[1]); return {}; }
    detail::ScopeExit cleanup([&] { ::close(sockets[0]); s->clipboard_reading = false; });
    s->clipboard_reading = true;
    detail::wl::request(s->selection, 1, mime.c_str(), sockets[1]);
    ::close(sockets[1]);
    detail::wayland_flush(s);
    std::string result;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline && !s->failed) {
        char bytes[65536];
        const auto count = ::read(sockets[0], bytes, sizeof bytes);
        if (count == 0) return result;
        if (count > 0) {
            if (result.size() + size_t(count) > (32u << 20)) return {};
            result.append(bytes, size_t(count));
            continue;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return {};
        std::vector<pollfd> fds{{sockets[0], POLLIN, 0}};
        int timeout = int(std::max<int64_t>(0,
            std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count()));
        // 同线程其他窗口可能是选区 owner；仅派发协议，resize 延迟到下一帧。
        for (auto* window : detail::wayland_windows()) {
            detail::wayland_dispatch(window);
            detail::wayland_send_clipboard(window);
            detail::wayland_flush(window);
            detail::wayland_wait_fds(window, fds, timeout, false);
        }
        ::poll(fds.data(), fds.size(), timeout);
    }
    return {};
}
#endif // AINO_PLATFORM_WAYLAND


// =============================================================================
//  8.21  macOS 后端
//        纯 C++ 通过 objc runtime 调 AppKit；绘制用 CoreGraphics，文字用 CoreText。
//        尚未经真机验证。
// =============================================================================
#if defined(AINO_PLATFORM_MACOS)
// 这些 objc runtime 函数由 <objc/runtime.h> / <objc/message.h> 提供，别再手写声明
// —— 手写会报 conflicting types。

namespace detail {

// arm64 上 Apple 不允许直接变参调 objc_msgSend（x86_64 能编但 arm64 直接报
// "no matching function"），必须先把函数指针转成具体签名再调。统一走这里：
// 返回类型用手写（id / void / BOOL / long / CGPoint ...），实参类型由模板推导。
template <class R, class Recv, class... Args>
inline R oc_send(Recv recv, SEL op, Args... args) {
    // 接收者类型也不能写死成 id —— objc_getClass 返回的是 Class，
    // 这是纯 C++ 编译单元，Class → id 没有隐式转换，所以先 reinterpret_cast。
#if defined(__x86_64__)
    if constexpr (std::is_same<R, CGRect>::value) {
        R result{};
        reinterpret_cast<void (*)(R*, id, SEL, Args...)>(objc_msgSend_stret)(
            &result, reinterpret_cast<id>(recv), op, args...);
        return result;
    }
#endif
    return reinterpret_cast<R (*)(id, SEL, Args...)>(objc_msgSend)(
        reinterpret_cast<id>(recv), op, args...);
}

class CoreTextRenderer : public TextRenderer {
public:
    void init(float, const std::string& family, const std::string& mono) override {
        family_ = family.empty() ? std::string("Helvetica Neue") : family;
        mono_ = mono.empty() ? std::string("Menlo") : mono;
    }
    void release() override {
        for (auto& entry : lines_) CFRelease(entry.second.line);
        lines_.clear(); line_bytes_ = 0;
        for (auto& kv : fonts_) if (kv.second) CFRelease(kv.second);
        fonts_.clear();
    }

    float measure_mono(const std::string& text, float size, int weight) override { return measure_impl(text, size, weight, true); }
    float ascent_mono(float size, int weight) override {
        CTFontRef f = font(size, weight, true); return f ? float(CTFontGetAscent(f)) : size * .8f;
    }
    float measure(const std::string& text, float size, int weight) override { return measure_impl(text, size, weight, false); }
    void caret_positions(const std::string& text, float size, int weight, std::vector<float>& out) override {
        CTLineRef line = make_line(text, size, weight, Color(0, 0, 0), false);
        if (!line) { TextRenderer::caret_positions(text, size, weight, out); return; }
        ScopeExit release([&] { CFRelease(line); });
        out.assign(1, 0.0f);
        CFIndex units = 0;
        for (size_t i = 0; i < text.size();) {
            uint32_t cp;
            const int n = utf8_decode_n(text.data() + i, text.size() - i, &cp);
            i += size_t(std::max(1, n)); units += cp > 0xffff ? 2 : 1;
            out.push_back(float(CTLineGetOffsetForStringIndex(line, units, nullptr)));
        }
    }
    float measure_impl(const std::string& utf8, float size, int weight, bool mono) {
        CTLineRef line = make_line(utf8, size, weight, Color(0, 0, 0), mono);
        if (!line) return 0.0f;
        double w = CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
        CFRelease(line);
        return float(w);
    }
    float ascent(float size, int weight) override {
        CTFontRef f = font(size, weight);
        if (!f) return size * 0.8f;
        return float(CTFontGetAscent(f));
    }
    float line_height(float size, int weight) override {
        CTFontRef f = font(size, weight);
        if (!f) return size * 1.4f;
        return float(CTFontGetAscent(f) + CTFontGetDescent(f) + CTFontGetLeading(f));
    }
    void draw(Canvas& cv, const std::string& utf8, float x, float y_top,
              float size, int weight, const Color& c, const Rect& clip) override {
        draw_impl(cv, utf8, x, y_top, size, weight, c, clip, false);
    }
    void draw_mono(Canvas& cv, const std::string& utf8, float x, float y_top,
                   float size, int weight, const Color& c, const Rect& clip) override {
        draw_impl(cv, utf8, x, y_top, size, weight, c, clip, true);
    }
    void draw_impl(Canvas& cv, const std::string& utf8, float x, float y_top,
                   float size, int weight, const Color& c, const Rect& clip, bool mono) {
        CGContextRef ctx = reinterpret_cast<CGContextRef>(cv.platform_dc);
        if (!ctx) return;
        CTLineRef line = make_line(utf8, size, weight, c, mono);
        if (!line) return;

        CGContextSaveGState(ctx);
        // 裁剪矩形必须镜像 y：这个 CGContext 是 CGBitmapContextCreate 出来的、
        // 原点在**左下**，而 clip 是"从顶上往下量"的逻辑坐标（和 px 缓冲一致）。
        // 不镜像的话裁剪带会落到垂直对称的位置上 —— 上方的字被切一半，靠下的字
        // 整块被裁掉，看起来就是"只有第一行文字画出来了"。
        if (clip.w >= 0.0f)
            CGContextClipToRect(ctx, CGRectMake(clip.x, CGFloat(cv.h) - clip.bottom(),
                                                clip.w, clip.h));
        // CoreGraphics 原点在左下角，需要翻转
        CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
        CGContextTranslateCTM(ctx, 0.0, CGFloat(cv.h));
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextSetTextPosition(ctx, x, y_top + (mono ? ascent_mono(size, weight) : ascent(size, weight)));
        CTLineDraw(line, ctx);
        CGContextRestoreGState(ctx);
        CFRelease(line);
    }

private:
    CTFontRef font(float size, int weight, bool mono = false) {
        int q = int(size * 2.0f + 0.5f);
        uint64_t key = (uint64_t(uint32_t(q)) << 32) | (mono ? (uint64_t(1) << 63) : 0) | uint32_t(weight < 0 ? 400 : weight);
        auto it = fonts_.find(key);
        if (it != fonts_.end()) return it->second;
        CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, (mono ? mono_ : family_).c_str(), kCFStringEncodingUTF8);
        const CTFontSymbolicTraits traits = (weight >= 600) ? kCTFontBoldTrait : 0;
        CTFontRef base = CTFontCreateWithName(name ? name : CFSTR("Helvetica"), CGFloat(float(q) / 2.0f), nullptr);
        CTFontRef f = traits && base ? CTFontCreateCopyWithSymbolicTraits(base, 0.0, nullptr, traits, traits) : base;
        if (f != base && base) CFRelease(base);
        if (!f) f = CTFontCreateWithName(CFSTR("Helvetica"), CGFloat(float(q) / 2.0f), nullptr);
        if (name) CFRelease(name);
        if (fonts_.size() >= 128) { auto old = fonts_.begin(); if (old->second) CFRelease(old->second); fonts_.erase(old); }
        fonts_[key] = f;
        return f;
    }
    CTLineRef make_line(const std::string& utf8, float size, int weight, const Color& c, bool mono) {
        std::string key;
        if (utf8.size() <= 4096) {
            key = std::to_string(int(size * 2.0f + .5f)) + ":" + std::to_string(weight) + ":"
                + (mono ? "m:" : "p:") + std::to_string(c.r) + ":" + std::to_string(c.g) + ":"
                + std::to_string(c.b) + ":" + std::to_string(c.a) + ":" + utf8;
            auto found = lines_.find(key);
            if (found != lines_.end()) {
                found->second.used = ++line_serial_;
                CFRetain(found->second.line); return found->second.line;
            }
        }
        CTFontRef f = font(size, weight, mono);
        if (!f) return nullptr;
        CFStringRef str = CFStringCreateWithBytes(kCFAllocatorDefault,
            reinterpret_cast<const UInt8*>(utf8.c_str()), CFIndex(utf8.size()),
            kCFStringEncodingUTF8, false);
        if (!str) return nullptr;
        CFMutableAttributedStringRef attr = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
        if (!attr) { CFRelease(str); return nullptr; }
        CFAttributedStringReplaceString(attr, CFRangeMake(0, 0), str);
        CFRange full = CFRangeMake(0, CFStringGetLength(str));
        CFAttributedStringSetAttribute(attr, full, kCTFontAttributeName, f);
        CGFloat comps[4] = { CGFloat(c.r) / 255.0, CGFloat(c.g) / 255.0, CGFloat(c.b) / 255.0, CGFloat(c.a) / 255.0 };
        CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
        if (!space) { CFRelease(str); CFRelease(attr); return nullptr; }
        CGColorRef cg = CGColorCreate(space, comps);
        if (!cg) { CGColorSpaceRelease(space); CFRelease(str); CFRelease(attr); return nullptr; }
        CFAttributedStringSetAttribute(attr, full, kCTForegroundColorAttributeName, cg);
        CTLineRef line = CTLineCreateWithAttributedString(attr);
        CGColorRelease(cg);
        CGColorSpaceRelease(space);
        CFRelease(str);
        CFRelease(attr);
        if (line && !key.empty()) {
            // CFLine 的 glyph/run 分配不可直接查询，按 UTF-8 字节计保守预算并限制条目数。
            const size_t cost = 512 + key.capacity() + utf8.size() * 128;
            while (!lines_.empty() && (lines_.size() >= 256 || line_bytes_ + cost > (2u << 20))) {
                auto oldest = lines_.begin();
                for (auto it = lines_.begin(); it != lines_.end(); ++it)
                    if (it->second.used < oldest->second.used) oldest = it;
                line_bytes_ -= oldest->second.cost;
                CFRelease(oldest->second.line); lines_.erase(oldest);
            }
            lines_.emplace(std::move(key), Line{line, cost, ++line_serial_});
            line_bytes_ += cost; CFRetain(line);
        }
        return line;
    }
    struct Line { CTLineRef line; size_t cost; uint64_t used; };
    std::unordered_map<std::string, Line> lines_;
    size_t line_bytes_ = 0;
    uint64_t line_serial_ = 0;
    std::string family_, mono_;
    std::unordered_map<uint64_t, CTFontRef> fonts_;
};

struct CocoaState {
    Window::Impl* owner = nullptr;
    id window = nullptr;
    id view = nullptr;
    CGContextRef ctx = nullptr;
    std::vector<uint8_t> buffer;
    int pw = 0, ph = 0;

    bool geometry_dirty = true, closed = false;
    std::string marked_text;
    CFIndex marked_units = 0;
    CGRect ime_rect = CGRectMake(0, 0, 1, 18);
    id tracking = nullptr;
    CFFileDescriptorRef wake_descriptor = nullptr;
    CFRunLoopSourceRef wake_source = nullptr;
    unsigned char key_repeat[KEY_COUNT] = {};

    float mx = -1e6f, my = -1e6f;
    bool  btn_down[3] = {false, false, false};
    bool  btn_pressed[3] = {false, false, false};
    bool  btn_released[3] = {false, false, false};
    float wheel = 0.0f;
    int   clicks = 0;
    std::string text_in;
    bool  key_down[KEY_COUNT] = {};
    bool  key_pressed[KEY_COUNT] = {};
    // 修饰键锁存：和 Windows / X11 后端保持同一套语义（Input::ctrl_of 靠它）。
    bool  key_ctrl[KEY_COUNT]  = {};
    bool  key_shift[KEY_COUNT] = {};
    bool  key_alt[KEY_COUNT]   = {};
    // present 生成、drawRect: 取用的图像。换新的一帧前要把旧的放掉，否则逐帧泄漏。
    CGImageRef pending_image = nullptr;
};

inline bool cocoa_create_backbuffer(CocoaState* s, int w, int h) {
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384 || uint64_t(w) * h > 16777216u) return false;
    if (s->pw == w && s->ph == h && s->ctx) return true;
    // 先准备新缓冲，失败时保留仍被 Canvas 引用的原始资源。
    std::vector<uint8_t> buffer(size_t(w) * size_t(h) * 4u, 0);
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    if (!space) return false;
    CGContextRef context = CGBitmapContextCreate(buffer.data(), size_t(w), size_t(h), 8, size_t(w) * 4u,
                                   space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if (!context) return false;
    if (s->ctx) CGContextRelease(s->ctx);
    s->buffer.swap(buffer); s->ctx = context;
    s->pw = w; s->ph = h;
    Window::Impl* o = s->owner;
    o->canvas.px = s->buffer.data();
    o->canvas.w = w; o->canvas.h = h;
    o->canvas.stride = w * 4;
    o->canvas.platform_dc = s->ctx;
    o->canvas.clear_clip();
    o->client_w = w; o->client_h = h;
    return true;
}

inline void cocoa_present(CocoaState* s) {
    if (!s->ctx || !s->view) return;
    CGImageRef image = CGBitmapContextCreateImage(s->ctx);
    if (!image) return;
    oc_send<void>(s->view, sel_registerName("setNeedsDisplay:"), (BOOL)YES);
    // 真正的绘制发生在 NSView 的 drawRect: 回调里，这里把图像先收起来
    if (s->pending_image) CGImageRelease(s->pending_image);
    s->pending_image = image;
    oc_send<void>(s->view, sel_registerName("display"));
}

inline void cocoa_feed_input(CocoaState* s) {
    Window::Impl* o = s->owner;
    Ui::Impl* u = o->ui.impl;
    Input& in = u->input;
    u->ime_comp = s->marked_text;
    const Vec2 prev = in.mouse;
    in.mouse = Vec2(s->mx / o->scale, s->my / o->scale);
    in.mouse_prev = prev;
    in.mouse_delta = Vec2(in.mouse.x - prev.x, in.mouse.y - prev.y);
    for (int i = 0; i < 3; ++i) { in.pressed[i] = s->btn_pressed[i]; in.released[i] = s->btn_released[i]; in.down[i] = s->btn_down[i]; }
    in.wheel = s->wheel;
    in.click_count = s->clicks;
    in.double_click = (s->clicks >= 2);
    in.text_input = s->text_in;
    for (int i = 0; i < KEY_COUNT; ++i) {
        in.key_down[i] = s->key_down[i]; in.key_pressed[i] = s->key_pressed[i];
        in.key_repeat[i] = s->key_repeat[i];
        in.key_ctrl[i] = s->key_ctrl[i]; in.key_shift[i] = s->key_shift[i]; in.key_alt[i] = s->key_alt[i];
    }
    in.dragging = s->btn_down[0];
    for (int i = 0; i < 3; ++i) { s->btn_pressed[i] = false; s->btn_released[i] = false; }
    s->wheel = 0.0f; s->clicks = 0; s->text_in.clear();
    for (int i = 0; i < KEY_COUNT; ++i) { s->key_pressed[i] = false; s->key_repeat[i] = 0; }
}

// --- NSView 子类的事件回调（C 函数，通过 class_addMethod 注册）---
// 原来用 setTag:/tag 把 CocoaState* 塞进 view，但 NSView 根本没有 tag 方法
// （那是 NSControl 的），真机上直接 unrecognized selector 崩掉。改成一张表。
inline std::unordered_map<void*, CocoaState*>& cocoa_view_map() {
    static std::unordered_map<void*, CocoaState*> m;
    return m;
}

inline CocoaState* cocoa_state_of(id self) {
    auto& m = cocoa_view_map();
    auto it = m.find(reinterpret_cast<void*>(self));
    return it == m.end() ? nullptr : it->second;
}

inline void cocoa_view_draw(id self, SEL, CGRect) {
    CocoaState* s = cocoa_state_of(self);
    if (!s) return;
    id ctx_obj = oc_send<id>(objc_getClass("NSGraphicsContext"), sel_registerName("currentContext"));
    CGContextRef cg = oc_send<CGContextRef>(ctx_obj, sel_registerName("CGContext"));
    if (!cg || !s->pending_image) return;
    CGImageRef img = s->pending_image;
    CGRect r = CGRectMake(0, 0, s->pw / s->owner->scale, s->ph / s->owner->scale);
    // NSView 默认原点在左下，图片需要翻转
    CGContextSaveGState(cg);
    CGContextTranslateCTM(cg, 0, CGFloat(s->ph / s->owner->scale));
    CGContextScaleCTM(cg, 1, -1);
    CGContextDrawImage(cg, r, img);
    CGContextRestoreGState(cg);
}

inline void cocoa_mouse_moved(id self, SEL, id ev) {
    CocoaState* s = cocoa_state_of(self);
    if (!s) return;
    CGPoint p = oc_send<CGPoint>(ev, sel_registerName("locationInWindow"));
    p = oc_send<CGPoint>(self, sel_registerName("convertPoint:fromView:"), p, (id)nullptr);
    s->mx = float(p.x) * s->owner->scale;
    s->my = float(s->ph) - float(p.y) * s->owner->scale;
    // 与 Windows / X11 同一条规则：悬停目标没变就不排帧。
    if (!s->owner->need_frame && s->owner->hover_skip &&
        !s->owner->ui.impl->hover_would_change(s->mx / s->owner->scale,
                                               s->my / s->owner->scale)) return;
    s->owner->need_frame = true;
}

inline void cocoa_mouse_down(id self, SEL, id ev) {
    CocoaState* s = cocoa_state_of(self);
    if (!s) return;
    cocoa_mouse_moved(self, nullptr, ev);
    const int button = int(oc_send<long>(ev, sel_registerName("buttonNumber")));
    if (button >= 0 && button < 3) { s->btn_down[button] = true; s->btn_pressed[button] = true; }
    s->clicks = int(oc_send<long>(ev, sel_registerName("clickCount")));
    s->owner->need_frame = true;
    (void)ev;
}

inline void cocoa_mouse_up(id self, SEL, id ev) {
    CocoaState* s = cocoa_state_of(self);
    if (!s) return;
    cocoa_mouse_moved(self, nullptr, ev);
    const int button = int(oc_send<long>(ev, sel_registerName("buttonNumber")));
    if (button >= 0 && button < 3) { s->btn_down[button] = false; s->btn_released[button] = true; }
    s->owner->need_frame = true;
}

struct CocoaRange { uintptr_t location, length; };
inline std::string cocoa_string(id value) {
    if (!value) return {};
    if (oc_send<BOOL>(value, sel_registerName("isKindOfClass:"), objc_getClass("NSAttributedString")))
        value = oc_send<id>(value, sel_registerName("string"));
    const auto str = reinterpret_cast<CFStringRef>(value);
    const CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8) + 1;
    if (size <= 0 || size > (32 << 20)) return {};
    std::vector<char> buffer(size_t(size), '\0');
    return CFStringGetCString(str, buffer.data(), size, kCFStringEncodingUTF8) ? std::string(buffer.data()) : std::string();
}
inline void cocoa_modifiers(CocoaState* s, id event) {
    const auto flags = oc_send<unsigned long>(event, sel_registerName("modifierFlags"));
    s->key_down[KEY_SHIFT] = (flags & (1ul << 17)) != 0;
    s->key_down[KEY_CTRL] = (flags & ((1ul << 18) | (1ul << 20))) != 0;
    s->key_down[KEY_ALT] = (flags & (1ul << 19)) != 0;
    s->key_down[KEY_SUPER] = (flags & (1ul << 20)) != 0;
}
inline int cocoa_key(id event) {
    const auto code = oc_send<unsigned short>(event, sel_registerName("keyCode"));
    switch (code) {
    case 51: return KEY_BACKSPACE; case 48: return KEY_TAB; case 36: case 76: return KEY_ENTER;
    case 53: return KEY_ESC; case 49: return KEY_SPACE; case 117: return KEY_DELETE;
    case 123: return KEY_LEFT; case 124: return KEY_RIGHT; case 125: return KEY_DOWN; case 126: return KEY_UP;
    case 115: return KEY_HOME; case 119: return KEY_END; case 116: return KEY_PAGEUP; case 121: return KEY_PAGEDOWN;
    case 122: return KEY_F1; case 120: return KEY_F2; case 99: return KEY_F3; case 118: return KEY_F4;
    case 96: return KEY_F5; case 97: return KEY_F6; case 98: return KEY_F7; case 100: return KEY_F8;
    case 101: return KEY_F9; case 109: return KEY_F10; case 103: return KEY_F11; case 111: return KEY_F12;
    default: break;
    }
    const auto chars = oc_send<CFStringRef>(event, sel_registerName("charactersIgnoringModifiers"));
    if (!chars || !CFStringGetLength(chars)) return KEY_NONE;
    const UniChar c = CFStringGetCharacterAtIndex(chars, 0);
    return c < 128 ? ((c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c) : KEY_NONE;
}
inline void cocoa_flags_changed(id self, SEL, id event) {
    if (auto* s = cocoa_state_of(self)) { cocoa_modifiers(s, event); s->owner->need_frame = true; }
}
inline void cocoa_key_down(id self, SEL, id event) {
    auto* s = cocoa_state_of(self);
    if (!s) return;
    cocoa_modifiers(s, event);
    const int key = cocoa_key(event);
    if (key > 0 && key < KEY_COUNT) {
        if (oc_send<BOOL>(event, sel_registerName("isARepeat"))) {
            s->key_repeat[key] = uint8_t(std::min(255, int(s->key_repeat[key]) + 1));
        } else s->key_pressed[key] = true;
        s->key_down[key] = true;
        s->key_ctrl[key] = s->key_down[KEY_CTRL];
        s->key_shift[key] = s->key_down[KEY_SHIFT];
        s->key_alt[key] = s->key_down[KEY_ALT];
    }
    if (!s->key_down[KEY_CTRL] || s->marked_units) {
        id events = oc_send<id>(objc_getClass("NSArray"), sel_registerName("arrayWithObject:"), event);
        oc_send<void>(self, sel_registerName("interpretKeyEvents:"), events);
    }
    s->owner->need_frame = true;
}
inline void cocoa_key_up(id self, SEL, id event) {
    if (auto* s = cocoa_state_of(self)) {
        const int key = cocoa_key(event);
        if (key > 0 && key < KEY_COUNT) s->key_down[key] = false;
        cocoa_modifiers(s, event); s->owner->need_frame = true;
    }
}
inline void cocoa_insert_text(id self, SEL, id text, CocoaRange) {
    if (auto* s = cocoa_state_of(self)) {
        const std::string value = cocoa_string(text);
        for (unsigned char c : value) if (c >= 32 && c != 127) s->text_in.push_back(char(c));
        s->marked_text.clear(); s->marked_units = 0; s->owner->need_frame = true;
    }
}
inline void cocoa_mark_text(id self, SEL, id text, CocoaRange, CocoaRange) {
    if (auto* s = cocoa_state_of(self)) {
        s->marked_text = cocoa_string(text);
        CFStringRef str = CFStringCreateWithBytes(nullptr,
            reinterpret_cast<const UInt8*>(s->marked_text.data()), CFIndex(s->marked_text.size()), kCFStringEncodingUTF8, false);
        s->marked_units = str ? CFStringGetLength(str) : 0;
        if (str) CFRelease(str);
        s->owner->need_frame = true;
    }
}
inline void cocoa_unmark_text(id self, SEL) {
    if (auto* s = cocoa_state_of(self)) { s->marked_text.clear(); s->marked_units = 0; s->owner->need_frame = true; }
}
inline BOOL cocoa_has_marked(id self, SEL) { auto* s = cocoa_state_of(self); return s && s->marked_units ? YES : NO; }
inline CocoaRange cocoa_marked_range(id self, SEL) {
    auto* s = cocoa_state_of(self);
    return {s && s->marked_units ? 0u : uintptr_t(-1) >> 1, s ? uintptr_t(s->marked_units) : 0u};
}
inline CocoaRange cocoa_selected_range(id, SEL) { return {0, 0}; }
inline id cocoa_marked_attributes(id, SEL) { return oc_send<id>(objc_getClass("NSArray"), sel_registerName("array")); }
inline id cocoa_substring(id, SEL, CocoaRange, CocoaRange* actual) {
    if (actual) *actual = {uintptr_t(-1) >> 1, 0};
    return nullptr;
}
inline uintptr_t cocoa_character_index(id, SEL, CGPoint) { return 0; }
inline CGRect cocoa_first_rect(id self, SEL, CocoaRange range, CocoaRange* actual) {
    auto* s = cocoa_state_of(self);
    if (actual) *actual = range;
    if (!s) return CGRectZero;
    CGRect rect = s->ime_rect;
    rect.origin.y = s->ph / s->owner->scale - rect.origin.y - rect.size.height;
    rect = oc_send<CGRect>(self, sel_registerName("convertRect:toView:"), rect, (id)nullptr);
    return oc_send<CGRect>(s->window, sel_registerName("convertRectToScreen:"), rect);
}
inline void cocoa_command(id, SEL, SEL) {} // 命令键由统一 Input 消费，文本提交只走 insertText。
inline BOOL cocoa_accepts_first(id, SEL) { return YES; }
inline void cocoa_scroll(id self, SEL, id event) {
    if (auto* s = cocoa_state_of(self)) {
        const bool precise = oc_send<BOOL>(event, sel_registerName("hasPreciseScrollingDeltas"));
        s->wheel += float(oc_send<CGFloat>(event, sel_registerName("scrollingDeltaY"))) / (precise ? 10.0f : 1.0f);
        s->owner->need_frame = true;
    }
}
inline void cocoa_mouse_exited(id self, SEL, id) {
    if (auto* s = cocoa_state_of(self)) { s->mx = s->my = -1e6f; s->owner->need_frame = true; }
}
inline void cocoa_window_closed(id self, SEL, id) {
    if (auto* s = cocoa_state_of(self)) { s->closed = true; s->owner->running = false; s->owner->wake_signal.notify(); }
}
inline void cocoa_window_changed(id self, SEL, id) {
    if (auto* s = cocoa_state_of(self)) { s->geometry_dirty = true; s->owner->need_frame = true; }
}
inline void cocoa_window_blur(id self, SEL, id) {
    if (auto* s = cocoa_state_of(self)) {
        for (int i = 0; i < KEY_COUNT; ++i) { s->key_down[i] = s->key_pressed[i] = false; s->key_repeat[i] = 0; }
        for (int i = 0; i < 3; ++i) { s->btn_released[i] = s->btn_down[i]; s->btn_down[i] = false; }
        s->marked_text.clear(); s->marked_units = 0;
        s->owner->ui.impl->active = 0; drop_focus(s->owner->ui.impl);
        s->owner->need_frame = true;
    }
}
inline void cocoa_sync_geometry(CocoaState* s) {
    if (!s->geometry_dirty || s->closed || s->owner->ui.impl->frame_open) return;
    const CGRect bounds = oc_send<CGRect>(s->view, sel_registerName("bounds"));
    const float scale = s->owner->cfg.high_dpi
        ? std::max(1.0f, float(oc_send<CGFloat>(s->window, sel_registerName("backingScaleFactor")))) : 1.0f;
    if (s->owner->scale != scale && s->owner->text)
        s->owner->text->reconfigure(scale, s->owner->ui.theme().font_family, s->owner->ui.theme().font_family_mono);
    s->owner->scale = scale;
    if (cocoa_create_backbuffer(s, std::max(1, int(bounds.size.width * scale + .5)),
        std::max(1, int(bounds.size.height * scale + .5)))) s->geometry_dirty = false;
}
inline void cocoa_wake(CFFileDescriptorRef descriptor, CFOptionFlags, void* context) {
    auto* s = static_cast<CocoaState*>(context);
    if (s->owner->wake_signal.consume()) s->owner->need_frame = true;
    CFFileDescriptorEnableCallBacks(descriptor, kCFFileDescriptorReadCallBack);
}

inline void cocoa_create(Window* self, const WindowConfig& cfg) {
    Window::Impl* o = self->impl;
    const int w = cfg.width > 0 ? cfg.width : 880;
    const int h = cfg.height > 0 ? cfg.height : 560;
    if (!pthread_main_np()) { std::fprintf(stderr, "[aino] AppKit windows require the main thread\n"); return; }
    id pool = oc_send<id>(oc_send<id>(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    ScopeExit drain([&] { oc_send<void>(pool, sel_registerName("drain")); });
    CocoaState* s = new CocoaState();
    s->owner = o;
    o->native = s;

    // 字体文件：注册到**进程**作用域（kCTFontManagerScopeProcess = 1），
    // 只影响本程序，不需要用户装字体。注册失败就还是按家族名走。
    if (!cfg.font_path.empty()) {
        CFURLRef furl = CFURLCreateFromFileSystemRepresentation(
            kCFAllocatorDefault, reinterpret_cast<const UInt8*>(cfg.font_path.c_str()),
            CFIndex(cfg.font_path.size()), false);
        if (furl) {
            CTFontManagerRegisterFontsForURL(furl, (CTFontManagerScope)1, nullptr);
            CFRelease(furl);
        }
    }

    Class app_cls = objc_getClass("NSApplication");
    id app = oc_send<id>(app_cls, sel_registerName("sharedApplication"));
    oc_send<void>(app, sel_registerName("setActivationPolicy:"), (long)0);

    // 动态注册 NSView 子类
    static Class view_cls = nullptr;
    if (!view_cls) {
        view_cls = objc_allocateClassPair(objc_getClass("NSView"), "AinoView", 0);
        class_addMethod(view_cls, sel_registerName("drawRect:"), reinterpret_cast<IMP>(cocoa_view_draw), "v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
        class_addMethod(view_cls, sel_registerName("mouseMoved:"), reinterpret_cast<IMP>(cocoa_mouse_moved), "v@:@");
        class_addMethod(view_cls, sel_registerName("mouseDown:"), reinterpret_cast<IMP>(cocoa_mouse_down), "v@:@");
        class_addMethod(view_cls, sel_registerName("mouseUp:"), reinterpret_cast<IMP>(cocoa_mouse_up), "v@:@");
        class_addMethod(view_cls, sel_registerName("keyDown:"), reinterpret_cast<IMP>(cocoa_key_down), "v@:@");
        if (auto* protocol = objc_getProtocol("NSTextInputClient")) class_addProtocol(view_cls, protocol);
        class_addMethod(view_cls, sel_registerName("acceptsFirstResponder"), reinterpret_cast<IMP>(cocoa_accepts_first), "c@:");
        class_addMethod(view_cls, sel_registerName("keyUp:"), reinterpret_cast<IMP>(cocoa_key_up), "v@:@");
        class_addMethod(view_cls, sel_registerName("flagsChanged:"), reinterpret_cast<IMP>(cocoa_flags_changed), "v@:@");
        class_addMethod(view_cls, sel_registerName("scrollWheel:"), reinterpret_cast<IMP>(cocoa_scroll), "v@:@");
        class_addMethod(view_cls, sel_registerName("rightMouseDown:"), reinterpret_cast<IMP>(cocoa_mouse_down), "v@:@");
        class_addMethod(view_cls, sel_registerName("rightMouseUp:"), reinterpret_cast<IMP>(cocoa_mouse_up), "v@:@");
        class_addMethod(view_cls, sel_registerName("otherMouseDown:"), reinterpret_cast<IMP>(cocoa_mouse_down), "v@:@");
        class_addMethod(view_cls, sel_registerName("otherMouseUp:"), reinterpret_cast<IMP>(cocoa_mouse_up), "v@:@");
        for (const char* method : {"mouseDragged:", "rightMouseDragged:", "otherMouseDragged:"})
            class_addMethod(view_cls, sel_registerName(method), reinterpret_cast<IMP>(cocoa_mouse_moved), "v@:@");
        class_addMethod(view_cls, sel_registerName("mouseExited:"), reinterpret_cast<IMP>(cocoa_mouse_exited), "v@:@");
        class_addMethod(view_cls, sel_registerName("insertText:replacementRange:"), reinterpret_cast<IMP>(cocoa_insert_text), "v@:@{_NSRange=QQ}");
        class_addMethod(view_cls, sel_registerName("setMarkedText:selectedRange:replacementRange:"), reinterpret_cast<IMP>(cocoa_mark_text), "v@:@{_NSRange=QQ}{_NSRange=QQ}");
        class_addMethod(view_cls, sel_registerName("unmarkText"), reinterpret_cast<IMP>(cocoa_unmark_text), "v@:");
        class_addMethod(view_cls, sel_registerName("hasMarkedText"), reinterpret_cast<IMP>(cocoa_has_marked), "c@:");
        class_addMethod(view_cls, sel_registerName("markedRange"), reinterpret_cast<IMP>(cocoa_marked_range), "{_NSRange=QQ}@:");
        class_addMethod(view_cls, sel_registerName("selectedRange"), reinterpret_cast<IMP>(cocoa_selected_range), "{_NSRange=QQ}@:");
        class_addMethod(view_cls, sel_registerName("validAttributesForMarkedText"), reinterpret_cast<IMP>(cocoa_marked_attributes), "@@:");
        class_addMethod(view_cls, sel_registerName("attributedSubstringForProposedRange:actualRange:"), reinterpret_cast<IMP>(cocoa_substring), "@@:{_NSRange=QQ}^{_NSRange=QQ}");
        class_addMethod(view_cls, sel_registerName("characterIndexForPoint:"), reinterpret_cast<IMP>(cocoa_character_index), "Q@:{CGPoint=dd}");
        class_addMethod(view_cls, sel_registerName("firstRectForCharacterRange:actualRange:"), reinterpret_cast<IMP>(cocoa_first_rect), "{CGRect={CGPoint=dd}{CGSize=dd}}@:{_NSRange=QQ}^{_NSRange=QQ}");
        class_addMethod(view_cls, sel_registerName("doCommandBySelector:"), reinterpret_cast<IMP>(cocoa_command), "v@::");
        class_addMethod(view_cls, sel_registerName("windowWillClose:"), reinterpret_cast<IMP>(cocoa_window_closed), "v@:@");
        class_addMethod(view_cls, sel_registerName("windowDidResize:"), reinterpret_cast<IMP>(cocoa_window_changed), "v@:@");
        class_addMethod(view_cls, sel_registerName("windowDidChangeBackingProperties:"), reinterpret_cast<IMP>(cocoa_window_changed), "v@:@");
        class_addMethod(view_cls, sel_registerName("windowDidResignKey:"), reinterpret_cast<IMP>(cocoa_window_blur), "v@:@");
        objc_registerClassPair(view_cls);
    }

    CGRect rect = CGRectMake(0, 0, CGFloat(w), CGFloat(h));
    // styleMask 15 = Titled(1) | Closable(2) | Miniaturizable(4) | Resizable(8)。
    // 不可缩放就把 Resizable 位拿掉（7）—— 和 Win32 去掉 WS_THICKFRAME 对齐。
    const unsigned long style_mask = cfg.resizable ? 15ul : 7ul;
    id window = oc_send<id>(oc_send<id>(objc_getClass("NSWindow"), sel_registerName("alloc")),
                            sel_registerName("initWithContentRect:styleMask:backing:defer:"),
                            rect, style_mask, (unsigned long)2, (BOOL)NO);
    id view = oc_send<id>(oc_send<id>(view_cls, sel_registerName("alloc")),
                          sel_registerName("initWithFrame:"), rect);
    cocoa_view_map()[reinterpret_cast<void*>(view)] = s;
    oc_send<void>(view, sel_registerName("setWantsLayer:"), (BOOL)YES);
    oc_send<void>(window, sel_registerName("setContentView:"), view);
    oc_send<void>(window, sel_registerName("setReleasedWhenClosed:"), (BOOL)NO);
    oc_send<void>(window, sel_registerName("setDelegate:"), view);
    oc_send<void>(window, sel_registerName("setAcceptsMouseMovedEvents:"), (BOOL)YES);
    oc_send<void>(window, sel_registerName("makeFirstResponder:"), view);
    s->tracking = oc_send<id>(oc_send<id>(objc_getClass("NSTrackingArea"), sel_registerName("alloc")),
        sel_registerName("initWithRect:options:owner:userInfo:"), CGRectZero, (unsigned long)(1 | 2 | 32 | 512), view, (id)nullptr);
    if (s->tracking) oc_send<void>(view, sel_registerName("addTrackingArea:"), s->tracking);

    // 尺寸约束用 content 版本：NSWindow 的 setMinSize: 算的是**窗口**尺寸，
    // 而我们手里的 min/max 是客户区（= 逻辑像素），用 setContentMinSize: 才对齐。
    if (cfg.min_width > 0 || cfg.min_height > 0)
        oc_send<void>(window, sel_registerName("setContentMinSize:"),
                      CGSizeMake(CGFloat(cfg.min_width > 0 ? cfg.min_width : 1),
                                 CGFloat(cfg.min_height > 0 ? cfg.min_height : 1)));
    if (cfg.max_width > 0 || cfg.max_height > 0)
        oc_send<void>(window, sel_registerName("setContentMaxSize:"),
                      CGSizeMake(CGFloat(cfg.max_width > 0 ? cfg.max_width : 100000),
                                 CGFloat(cfg.max_height > 0 ? cfg.max_height : 100000)));
    if (cfg.center) oc_send<void>(window, sel_registerName("center"));

    CFStringRef t = CFStringCreateWithCString(kCFAllocatorDefault,
                                              cfg.title.empty() ? "aino" : cfg.title.c_str(),
                                              kCFStringEncodingUTF8);
    if (t) oc_send<void>(window, sel_registerName("setTitle:"), t);
    if (t) CFRelease(t);

    // 图标：macOS 的窗口没有标题栏图标位，只能设应用图标（Dock / Cmd+Tab）。
    if (!cfg.icon_path.empty()) {
        CFStringRef ip = CFStringCreateWithCString(kCFAllocatorDefault, cfg.icon_path.c_str(),
                                                   kCFStringEncodingUTF8);
        id img = ip ? oc_send<id>(oc_send<id>(objc_getClass("NSImage"), sel_registerName("alloc")),
                             sel_registerName("initWithContentsOfFile:"), ip) : nullptr;
        if (img) {
            oc_send<void>(app, sel_registerName("setApplicationIconImage:"), img);
            oc_send<void>(img, sel_registerName("release"));
        } else std::fprintf(stderr, "[aino] Cannot load icon: %s\n", cfg.icon_path.c_str());
        if (ip) CFRelease(ip);
    }

    oc_send<void>(window, sel_registerName("makeKeyAndOrderFront:"), (id)nullptr);
    oc_send<void>(app, sel_registerName("activateIgnoringOtherApps:"), (BOOL)YES);

    s->window = window;
    s->view = view;
    o->scale = 1.0f;

    CoreTextRenderer* tr = new CoreTextRenderer();
    tr->init(o->scale, o->theme.font_family, o->theme.font_family_mono);
    o->text = tr;
    o->ui.impl->text = tr;

    cocoa_sync_geometry(s);
    CFFileDescriptorContext context{0, s, nullptr, nullptr, nullptr};
    s->wake_descriptor = CFFileDescriptorCreate(nullptr, o->wake_signal.read_descriptor(), false, cocoa_wake, &context);
    if (s->wake_descriptor) {
        s->wake_source = CFFileDescriptorCreateRunLoopSource(nullptr, s->wake_descriptor, 0);
        if (s->wake_source) CFRunLoopAddSource(CFRunLoopGetMain(), s->wake_source, kCFRunLoopCommonModes);
        CFFileDescriptorEnableCallBacks(s->wake_descriptor, kCFFileDescriptorReadCallBack);
    }
    o->ui.impl->ime_pos = [s](float x, float top, float bottom) {
        const float scale = s->owner->scale;
        s->owner->ui.impl->ime_owner = true;
        s->ime_rect = CGRectMake(x / scale, top / scale, 1, std::max(1.0f, (bottom - top) / scale));
    };
    o->ui.impl->canvas = &o->canvas;
    o->need_frame = true;
}

inline void cocoa_destroy(Window* self) {
    auto* o = self->impl;
    auto* s = reinterpret_cast<CocoaState*>(o->native);
    if (!s) return;
    id pool = oc_send<id>(oc_send<id>(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    ScopeExit drain([&] { oc_send<void>(pool, sel_registerName("drain")); });
    if (s->wake_source) { CFRunLoopRemoveSource(CFRunLoopGetMain(), s->wake_source, kCFRunLoopCommonModes); CFRelease(s->wake_source); }
    if (s->wake_descriptor) { CFFileDescriptorInvalidate(s->wake_descriptor); CFRelease(s->wake_descriptor); }
    if (s->ctx) CGContextRelease(s->ctx);
    if (s->pending_image) CGImageRelease(s->pending_image);
    if (s->window) oc_send<void>(s->window, sel_registerName("setDelegate:"), (id)nullptr);
    if (s->view) {
        cocoa_view_map().erase(reinterpret_cast<void*>(s->view));
        if (s->tracking) { oc_send<void>(s->view, sel_registerName("removeTrackingArea:"), s->tracking); oc_send<void>(s->tracking, sel_registerName("release")); }
    }
    if (s->window) { oc_send<void>(s->window, sel_registerName("close")); oc_send<void>(s->window, sel_registerName("release")); }
    if (s->view) oc_send<void>(s->view, sel_registerName("release"));
    delete s;
    o->native = nullptr;
}

} // namespace detail

using detail::oc_send;   // 下面 Window::xxx 在 aino 命名空间里，别每次都写 detail::

bool Window::poll() {
    Window::Impl* o = impl;
    detail::CocoaState* s = reinterpret_cast<detail::CocoaState*>(o->native);
    if (!s || !o->running) return false;
    detail::service_clipboard(impl);
    id pool = oc_send<id>(oc_send<id>(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    detail::ScopeExit drain([&] { oc_send<void>(pool, sel_registerName("drain")); });
    if (o->wake_signal.consume()) o->need_frame = true;
    // 抽干一次 runloop
    id distant = oc_send<id>(objc_getClass("NSDate"), sel_registerName("distantPast"));
    while (true) {
        id ev = oc_send<id>(oc_send<id>(objc_getClass("NSApplication"), sel_registerName("sharedApplication")),
                            sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
                            (unsigned long)-1, distant,
                            oc_send<id>(objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
                                        "NSDefaultRunLoopMode"),   // 不是 kCFRunLoopDefaultMode
                            (BOOL)YES);
        if (!ev) break;
        oc_send<void>(oc_send<id>(objc_getClass("NSApplication"), sel_registerName("sharedApplication")),
                      sel_registerName("sendEvent:"), ev);
    }
    oc_send<void>(oc_send<id>(objc_getClass("NSApplication"), sel_registerName("sharedApplication")),
                  sel_registerName("updateWindows"));
    detail::cocoa_sync_geometry(s);
    return o->running;
}

void Window::render() {
    Window::Impl* o = impl;
    detail::CocoaState* s = reinterpret_cast<detail::CocoaState*>(o->native);
    if (!s || !s->ctx) return;
    if (o->ui.impl->frame_open) { o->need_frame = true; return; }
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = uint64_t(ts.tv_sec) * 1000u + uint64_t(ts.tv_nsec / 1000000);
    float dt = o->last_frame_ms ? float(now - o->last_frame_ms) / 1000.0f : (1.0f / 60.0f);
    if (dt > 0.25f) { o->ui.impl->caret_blink += dt - 0.25f; dt = 0.25f; }
    o->last_frame_ms = now;

    detail::cocoa_feed_input(s);
    o->need_frame = false;   // 同 Windows：帧前清
    detail::ScopeExit frame_cleanup([o] {
        if (o->ui.impl->frame_open) { o->ui.impl->abort_frame(); o->need_frame = true; }
    });
    Ui& ui = o->ui;
    ui.begin_frame(float(s->pw) / o->scale, float(s->ph) / o->scale, dt, o->scale);
    if (o->frame_cb_ptr) o->frame_cb_ptr();
    ui.end_frame();
    o->cursor = ui.cursor();
    if (s->view && oc_send<BOOL>(s->window, sel_registerName("isKeyWindow"))) {
        const char* selector = "arrowCursor";
        if (o->cursor == Cursor::Hand) selector = "pointingHandCursor";
        else if (o->cursor == Cursor::IBeam || o->cursor == Cursor::Text) selector = "IBeamCursor";
        else if (o->cursor == Cursor::ResizeH) selector = "resizeLeftRightCursor";
        else if (o->cursor == Cursor::ResizeV) selector = "resizeUpDownCursor";
        oc_send<void>(oc_send<id>(objc_getClass("NSCursor"), sel_registerName(selector)), sel_registerName("set"));
    }
    detail::cocoa_present(s);
}


void Window::set_title(const std::string& title) {
    detail::CocoaState* s = reinterpret_cast<detail::CocoaState*>(impl->native);
    if (!s || !s->window) return;
    CFStringRef t = CFStringCreateWithCString(kCFAllocatorDefault, title.c_str(), kCFStringEncodingUTF8);
    if (t) oc_send<void>(s->window, sel_registerName("setTitle:"), t);
    if (t) CFRelease(t);
}
void Window::set_size(int w, int h) {
    detail::CocoaState* s = reinterpret_cast<detail::CocoaState*>(impl->native);
    if (s && s->window) {
        oc_send<void>(s->window, sel_registerName("setContentSize:"), CGSizeMake(CGFloat(std::max(1, w)), CGFloat(std::max(1, h))));
        s->geometry_dirty = true; s->owner->need_frame = true;
        detail::cocoa_sync_geometry(s);
    }
}
bool Window::try_set_clipboard(const std::string& text) {
    if (!pthread_main_np()) return false;
    id pool = oc_send<id>(oc_send<id>(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    detail::ScopeExit drain([&] { oc_send<void>(pool, sel_registerName("drain")); });
    CFStringRef value = CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(text.data()),
        CFIndex(text.size()), kCFStringEncodingUTF8, false);
    if (!value) return false;
    detail::ScopeExit release([&] { CFRelease(value); });
    id pasteboard = oc_send<id>(objc_getClass("NSPasteboard"), sel_registerName("generalPasteboard"));
    if (!pasteboard) return false;
    oc_send<long>(pasteboard, sel_registerName("clearContents"));
    return oc_send<BOOL>(pasteboard, sel_registerName("setString:forType:"), value, CFSTR("public.utf8-plain-text"));
}
std::string Window::clipboard() const {
    if (!pthread_main_np()) return {};
    id pool = oc_send<id>(oc_send<id>(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    detail::ScopeExit drain([&] { oc_send<void>(pool, sel_registerName("drain")); });
    id pasteboard = oc_send<id>(objc_getClass("NSPasteboard"), sel_registerName("generalPasteboard"));
    return detail::cocoa_string(oc_send<id>(pasteboard, sel_registerName("stringForType:"), CFSTR("public.utf8-plain-text")));
}

#endif // AINO_PLATFORM_MACOS


void Window::request_clipboard(std::function<void(std::string)> complete) {
    const auto serial = ++impl->clipboard_serial;
    if (impl->clipboard_complete) impl->finish_clipboard({});
    // 被取消请求的回调可以立即发起更新的请求；不能覆盖它的 callback 或接收 fd。
    if (impl->clipboard_serial != serial) { if (complete) complete({}); return; }
    if (!complete || !ok()) { if (complete) complete({}); return; }
    impl->clipboard_complete = std::move(complete);
    impl->clipboard_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
#if defined(AINO_PLATFORM_X11)
    auto* s = static_cast<detail::X11State*>(impl->native);
    detail::x11_clipboard_atoms(s);
    const auto owner = XGetSelectionOwner(s->dpy, s->clipboard_selection);
    if (!owner || owner == s->win) { impl->finish_clipboard(owner ? s->clipboard_text : std::string()); return; }
    s->clipboard_received.clear(); s->clipboard_done = s->clipboard_ok = s->clipboard_incr = false;
    s->clipboard_waiting = true;
    XConvertSelection(s->dpy, s->clipboard_selection, s->clipboard_utf8, s->clipboard_property, s->win, CurrentTime);
    XFlush(s->dpy);
#elif defined(AINO_PLATFORM_WAYLAND)
    auto* s = static_cast<detail::WaylandState*>(impl->native);
    std::string mime;
    for (const auto& offer : s->offers) if (offer.proxy == s->selection) mime = offer.mime;
    int sockets[2];
    if (mime.empty() || socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) < 0) {
        impl->finish_clipboard({}); return;
    }
    if (fcntl(sockets[0], F_SETFL, O_NONBLOCK) < 0) {
        ::close(sockets[0]); ::close(sockets[1]); impl->finish_clipboard({}); return;
    }
    impl->clipboard_fd = sockets[0];
    detail::wl::request(s->selection, 1, mime.c_str(), sockets[1]); ::close(sockets[1]);
    detail::wayland_flush(s);
#else
    impl->finish_clipboard(clipboard());
#endif
    wake();
}

namespace detail {
inline void service_clipboard(Window::Impl* o) {
    if (!o->clipboard_complete) return;
    const bool expired = std::chrono::steady_clock::now() >= o->clipboard_deadline;
#if defined(AINO_PLATFORM_X11)
    auto* s = static_cast<X11State*>(o->native);
    if (s->clipboard_done || expired) {
        auto result = !expired && s->clipboard_ok ? std::move(s->clipboard_received) : std::string();
        s->clipboard_waiting = false;
        XDeleteProperty(s->dpy, s->win, s->clipboard_property);
        o->finish_clipboard(std::move(result));
    }
#elif defined(AINO_PLATFORM_WAYLAND)
    if (expired) { o->finish_clipboard({}); return; }
    size_t budget = 256u << 10;
    while (budget && o->clipboard_fd >= 0) {
        char bytes[65536];
        const auto n = ::read(o->clipboard_fd, bytes, std::min(sizeof bytes, budget));
        if (n > 0) {
            if (o->clipboard_data.size() + size_t(n) > (32u << 20)) { o->finish_clipboard({}); return; }
            o->clipboard_data.append(bytes, size_t(n)); budget -= size_t(n);
        } else if (!n) {
            auto result = std::move(o->clipboard_data); o->finish_clipboard(std::move(result)); return;
        } else if (errno == EINTR) continue;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        else { o->finish_clipboard({}); return; }
    }
#else
    (void)expired;
#endif
}
} // namespace detail

void Window::set_frame_callback(std::function<void(Ui&)> frame) {
    auto& windows = detail::scheduled_windows();
    if (frame) {
        impl->frame_cb_ptr = [this, callback = std::move(frame)] { callback(impl->ui); };
        impl->render_fn = [this] { render(); };
        if (std::find(windows.begin(), windows.end(), this) == windows.end()) windows.push_back(this);
        impl->need_frame = true;
    } else {
        windows.erase(std::remove(windows.begin(), windows.end(), this), windows.end());
        impl->frame_cb_ptr = nullptr;
        impl->render_fn = nullptr;
    }
}

void Window::run(const std::function<void(Ui&)>& frame) {
    if (!ok()) return;
    auto& windows = detail::scheduled_windows();
    const bool registered = std::find(windows.begin(), windows.end(), this) != windows.end();
    detail::ScopeExit restore([this, registered, previous = std::move(impl->frame_cb_ptr),
                               render_fn = std::move(impl->render_fn)]() mutable {
        if (!registered) {
            auto& list = detail::scheduled_windows();
            list.erase(std::remove(list.begin(), list.end(), this), list.end());
        }
        impl->frame_cb_ptr = std::move(previous);
        impl->render_fn = std::move(render_fn);
    });
    set_frame_callback([&frame](Ui& ui) { frame(ui); });
    using Clock = std::chrono::steady_clock;
    while (impl->running) {
        // 回调可以创建或销毁其他窗口；快照只用于遍历，每次解引用前验证注册仍存在。
        const auto snapshot = windows;
        auto live = [&](Window* window) {
            return std::find(windows.begin(), windows.end(), window) != windows.end();
        };
        auto notify_closed = [&] {
            bool closed = false;
            for (Window* window : snapshot) if (live(window) && !window->impl->running
                && !window->impl->close_reported) { window->impl->close_reported = true; closed = true; }
            if (closed) for (Window* window : windows) window->impl->need_frame = true;
        };
        for (Window* window : snapshot) if (live(window) && window->impl->running) {
            window->poll();
        }
        notify_closed();
        if (!impl->running) break;
        for (Window* window : snapshot) {
            if (!live(window)) continue;
            auto* o = window->impl;
            if (!o->running || !o->frame_cb_ptr) continue;
#ifdef AINO_PLATFORM_WAYLAND
            if (!detail::wayland_can_render(static_cast<detail::WaylandState*>(o->native))) continue;
#endif
            const auto now = Clock::now();
            if ((o->need_frame || o->ui.animating() || o->ui.impl->repaint_timeout_ms() == 0)
                && now >= o->next_frame_at) {
                const auto budget = (o->move_only && !o->ui.impl->active)
                    ? std::max(o->anim_budget_ms, o->move_budget_ms) : o->anim_budget_ms;
                o->self_repaint = false;
                window->render();
#ifdef AINO_PLATFORM_WINDOWS
                if (o->frame_log) detail::win_frame_log(o, uint64_t(
                    std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - now).count()));
#endif
                o->move_only = false;
                o->next_frame_at = now + std::chrono::milliseconds(budget);
            }
        }
        if (!impl->running) break;
        notify_closed();
        if (snapshot != windows) continue;
        int timeout = -1;
        auto limit = [&](int delay) { if (delay >= 0 && (timeout < 0 || delay < timeout)) timeout = delay; };
        for (Window* window : windows) {
            auto* o = window->impl;
            if (!o->running || !o->frame_cb_ptr) continue;
            if (o->wake_signal.pending()) limit(0);
            if (o->clipboard_complete) limit(int(std::max<int64_t>(0,
                std::chrono::ceil<std::chrono::milliseconds>(o->clipboard_deadline - Clock::now()).count())));
#ifdef AINO_PLATFORM_WAYLAND
            if (!detail::wayland_can_render(static_cast<detail::WaylandState*>(o->native))) continue;
#endif
            const int until_frame = int(std::max<int64_t>(0,
                std::chrono::ceil<std::chrono::milliseconds>(o->next_frame_at - Clock::now()).count()));
            const int timer = o->ui.impl->repaint_timeout_ms();
            if (o->need_frame || o->ui.animating()) limit(until_frame);
            else if (timer >= 0) limit(std::max(timer, until_frame));
        }
#ifdef AINO_PLATFORM_WINDOWS
        // MsgWait 的句柄上限为 MAXIMUM_WAIT_OBJECTS - 1；每组之外的窗口用一个有界等待兜底。
        std::vector<HANDLE> handles;
        for (Window* window : windows) if (window->impl->running) {
            HANDLE event = window->impl->wake_signal.wait_handle();
            if (event && handles.size() < MAXIMUM_WAIT_OBJECTS - 2) handles.push_back(event);
            else limit(16);
        }
        HANDLE timer = detail::win_timer().arm_ms(timeout);
        if (timer) handles.push_back(timer);
        MsgWaitForMultipleObjectsEx(DWORD(handles.size()), handles.data(),
            timer || timeout < 0 ? INFINITE : DWORD(timeout), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
#elif defined(AINO_PLATFORM_X11)
        std::vector<pollfd> descriptors;
        for (Window* window : windows) if (window->impl->running) {
            auto* state = reinterpret_cast<detail::X11State*>(window->impl->native);
            if (state) descriptors.push_back({ConnectionNumber(state->dpy), POLLIN, 0});
            descriptors.push_back({window->impl->wake_signal.read_descriptor(), POLLIN, 0});
        }
        ::poll(descriptors.data(), descriptors.size(), timeout);
#elif defined(AINO_PLATFORM_WAYLAND)
        std::vector<pollfd> descriptors;
        for (Window* window : windows) if (window->impl->running)
            detail::wayland_wait_fds(static_cast<detail::WaylandState*>(window->impl->native), descriptors, timeout);
        ::poll(descriptors.data(), descriptors.size(), timeout);
#elif defined(AINO_PLATFORM_MACOS)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, timeout < 0 ? 1e9 : double(timeout) / 1000.0, true);
#endif
    }
}

namespace detail {
struct SystemIconMask {
    int width = 0, height = 0;
    std::vector<uint8_t> alpha;
};

#if defined(AINO_PLATFORM_UNIX)
// 只使用 GdkPixbuf 的图像加载，不初始化 GTK 或创建额外窗口。
struct X11IconPixels {
    void* lib = nullptr;
    void* (*load)(const char*, int, int, int, void**) = nullptr;
    void* (*file_info)(const char*, int*, int*) = nullptr;
    int (*width)(const void*) = nullptr;
    int (*height)(const void*) = nullptr;
    int (*stride)(const void*) = nullptr;
    int (*channels)(const void*) = nullptr;
    int (*has_alpha)(const void*) = nullptr;
    unsigned char* (*pixels)(const void*) = nullptr;
    void (*unref)(void*) = nullptr;
    X11IconPixels() {
        lib = dlopen("libgdk_pixbuf-2.0.so.0", RTLD_LAZY | RTLD_LOCAL);
        if (!lib) return;
        load = reinterpret_cast<decltype(load)>(dlsym(lib, "gdk_pixbuf_new_from_file_at_scale"));
        file_info = reinterpret_cast<decltype(file_info)>(dlsym(lib, "gdk_pixbuf_get_file_info"));
        width = reinterpret_cast<decltype(width)>(dlsym(lib, "gdk_pixbuf_get_width"));
        height = reinterpret_cast<decltype(height)>(dlsym(lib, "gdk_pixbuf_get_height"));
        stride = reinterpret_cast<decltype(stride)>(dlsym(lib, "gdk_pixbuf_get_rowstride"));
        channels = reinterpret_cast<decltype(channels)>(dlsym(lib, "gdk_pixbuf_get_n_channels"));
        has_alpha = reinterpret_cast<decltype(has_alpha)>(dlsym(lib, "gdk_pixbuf_get_has_alpha"));
        pixels = reinterpret_cast<decltype(pixels)>(dlsym(lib, "gdk_pixbuf_get_pixels"));
        unref = reinterpret_cast<decltype(unref)>(dlsym(lib, "g_object_unref"));
        if (!load || !file_info || !width || !height || !stride || !channels || !has_alpha || !pixels || !unref) {
            dlclose(lib); lib = nullptr;
        }
    }
};
inline std::string x11_icon_trim(std::string value) {
    const auto begin = value.find_first_not_of(" \t\r\n\"'");
    return begin == std::string::npos ? std::string() : value.substr(begin, value.find_last_not_of(" \t\r\n\"'") - begin + 1);
}
inline void x11_icon_split(const std::string& value, char separator, std::vector<std::string>& out) {
    size_t start = 0;
    while (start < value.size()) {
        const auto end = value.find(separator, start);
        const auto part = x11_icon_trim(value.substr(start, end - start));
        if (!part.empty() && std::find(out.begin(), out.end(), part) == out.end()) out.push_back(part);
        if (end == std::string::npos) break;
        start = end + 1;
    }
}
inline bool x11_icon_relative(const std::string& name) {
    return !name.empty() && name.front() != '/' && name.find("..") == std::string::npos && name.find('\\') == std::string::npos;
}
inline std::string x11_effective_icon_theme() {
    if (!x11_icon_theme().empty()) return x11_icon_theme();
    static const std::string fallback = [] {
        const char* home = std::getenv("HOME");
        const char* config = std::getenv("XDG_CONFIG_HOME");
        const std::string base = config && *config ? config : home ? std::string(home) + "/.config" : std::string();
        for (const auto& file : {base + "/gtk-3.0/settings.ini", base + "/gtk-4.0/settings.ini", std::string("/etc/gtk-3.0/settings.ini")}) {
            std::ifstream input(file); std::string line;
            while (std::getline(input, line)) {
                const auto equal = line.find('=');
                if (equal != std::string::npos && x11_icon_trim(line.substr(0, equal)) == "gtk-icon-theme-name") {
                    const auto theme = x11_icon_trim(line.substr(equal + 1));
                    if (x11_icon_relative(theme) && theme.find('/') == std::string::npos) return theme;
                }
            }
        }
        const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
        return desktop && std::string(desktop).find("ubuntu") != std::string::npos ? std::string("Yaru") : std::string("Adwaita");
    }();
    return fallback;
}
struct X11IconTheme {
    std::vector<std::string> dirs, parents;
};
struct X11IconLookup {
    std::string active;
    std::vector<std::string> roots;
    std::map<std::string, X11IconTheme> themes;
    std::map<std::string, std::string> files;
    X11IconLookup() {
        const char* home = std::getenv("HOME");
        const char* data = std::getenv("XDG_DATA_HOME");
        if (data && *data) roots.push_back(std::string(data) + "/icons");
        else if (home) roots.push_back(std::string(home) + "/.local/share/icons");
        if (home) roots.push_back(std::string(home) + "/.icons");
        std::vector<std::string> bases;
        const char* dirs = std::getenv("XDG_DATA_DIRS");
        x11_icon_split(dirs && *dirs ? dirs : "/usr/local/share:/usr/share", ':', bases);
        for (const auto& base : bases) roots.push_back(base + "/icons");
    }
    const X11IconTheme& theme(const std::string& name) {
        auto found = themes.find(name);
        if (found != themes.end()) return found->second;
        X11IconTheme value;
        if (x11_icon_relative(name) && name.find('/') == std::string::npos) for (const auto& root : roots) {
            std::ifstream input(root + "/" + name + "/index.theme");
            std::string line; bool header = false;
            while (std::getline(input, line)) {
                line = x11_icon_trim(line);
                if (line.empty() || line[0] == '#') continue;
                if (line[0] == '[') { header = line == "[Icon Theme]"; continue; }
                if (!header) continue;
                const auto equal = line.find('=');
                if (equal == std::string::npos) continue;
                const auto key = x11_icon_trim(line.substr(0, equal));
                if (key == "Directories" || key == "ScaledDirectories") x11_icon_split(line.substr(equal + 1), ',', value.dirs);
                else if (key == "Inherits") x11_icon_split(line.substr(equal + 1), ',', value.parents);
            }
        }
        // Symbolic/scalable 源优先，系统按请求像素尺寸栅格化，不依赖字体字形。
        std::stable_sort(value.dirs.begin(), value.dirs.end(), [](const std::string& a, const std::string& b) {
            auto rank = [](const std::string& v) { return v.find("symbolic") != std::string::npos ? 0 : v.find("scalable") != std::string::npos ? 1 : 2; };
            return rank(a) < rank(b);
        });
        return themes.emplace(name, std::move(value)).first->second;
    }
    std::string find_in(const std::string& name, const std::string& icon, std::vector<std::string>& visited) {
        if (visited.size() >= 32 || std::find(visited.begin(), visited.end(), name) != visited.end()) return {};
        visited.push_back(name);
        const auto& entry = theme(name);
        for (const auto& dir : entry.dirs) {
            if (!x11_icon_relative(dir)) continue;
            for (const auto& root : roots) for (const auto* suffix : {".svg", ".png"}) {
                const auto file = root + "/" + name + "/" + dir + "/" + icon + suffix;
                if (auto* input = std::fopen(file.c_str(), "rb")) { std::fclose(input); return file; }
            }
        }
        for (const auto& parent : entry.parents) {
            const auto file = find_in(parent, icon, visited);
            if (!file.empty()) return file;
        }
        return {};
    }
    std::string find(const std::string& icon) {
        const auto current = x11_effective_icon_theme();
        if (active != current) { active = current; themes.clear(); files.clear(); }
        const auto cached = files.find(icon);
        if (cached != files.end()) return cached->second;
        std::vector<std::string> visited;
        std::string file;
        for (const auto& name : {active, std::string("Adwaita"), std::string("hicolor")}) {
            file = find_in(name, icon, visited);
            if (!file.empty()) break;
        }
        files[icon] = file;
        return file;
    }
};
inline std::string x11_system_icon_file(SystemIcon icon, int size) {
    static const char* names[] = {
        "input-mouse", "insert-text", "object-select", "preferences-system", "view-list", "open-menu",
        "x-office-calendar", "applications-graphics", "preferences-system-notifications", "view-grid",
        "utilities-system-monitor", "view-refresh", "view-app-grid", "sidebar-show", "view-list", "edit-copy",
        "starred", "view-more", "go-up", "go-down", "document-edit", "user-trash", "go-previous", "go-next",
        "weather-clear-night", "weather-clear", "preferences-desktop-display", "window-close", "view-reveal",
        "list-add", "folder", "system-search", "pan-down", "pan-up", "object-select", "list-remove",
        "emblem-system", "network-wireless", "image-x-generic", "dialog-information", "dialog-warning", "system-users",
        "media-playback-start", "media-playback-pause", "media-skip-backward", "media-skip-forward",
        "media-playlist-shuffle", "media-playlist-repeat-song", "media-playlist-repeat", "alarm"
    };
    static_assert(sizeof(names) / sizeof(names[0]) == size_t(SystemIcon::Count));
    if (size <= 0 || size > 512 || size_t(icon) >= size_t(SystemIcon::Count)) return {};
    static thread_local X11IconLookup lookup;
    std::string file = lookup.find(std::string(names[size_t(icon)]) + "-symbolic");
    if (file.empty()) file = lookup.find(names[size_t(icon)]);
    if (file.empty() && icon == SystemIcon::RepeatOne) file = lookup.find("media-playlist-repeat-symbolic");
    if (file.empty() && icon == SystemIcon::Timer) file = lookup.find("preferences-system-time-symbolic");
    if (file.empty()) x11_trace("system icon unavailable: %s", names[size_t(icon)]);
    return file;
}
inline SystemIconMask x11_decode_system_icon(const std::string& file, int size) {
    SystemIconMask mask;
    static X11IconPixels api;
    if (!api.lib || file.empty()) return mask;
    void* image = api.load(file.c_str(), size, size, 1, nullptr);
    if (!image) return mask;
    ScopeExit release([&] { api.unref(image); });
    const int w = api.width(image), h = api.height(image), stride = api.stride(image), channels = api.channels(image);
    const auto* pixels = api.pixels(image);
    if (pixels && w > 0 && h > 0 && w <= size && h <= size &&
        (channels == 3 || channels == 4) && stride >= w * channels) {
        const bool alpha = api.has_alpha(image) && channels == 4;
        mask.width = w; mask.height = h; mask.alpha.resize(size_t(w) * h);
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            const auto* pixel = pixels + size_t(y) * stride + x * channels;
            mask.alpha[size_t(y) * w + x] = alpha ? pixel[3] :
                uint8_t(255 - (299 * pixel[0] + 587 * pixel[1] + 114 * pixel[2]) / 1000);
        }
    }
    return mask;
}
#if defined(__linux__)
inline bool& x11_icon_worker_enabled() { static bool enabled = false; return enabled; }
// 缓存只存最终 alpha；主题源文件、时间戳、尺寸和协议版本共同参与键。
inline std::string x11_icon_cache_dir() {
    const char* cache = std::getenv("XDG_CACHE_HOME");
    const char* home = std::getenv("HOME");
    const std::string base = cache && *cache ? cache : home && *home ? std::string(home) + "/.cache" : "";
    if (base.empty() || base.front() != '/') return {};
    std::error_code ec;
    const auto dir = std::filesystem::path(base) / "aino-system-icons-v1";
    std::filesystem::create_directories(dir, ec);
    if (ec) return {};
    struct stat status{};
    if (lstat(dir.c_str(), &status) != 0 || !S_ISDIR(status.st_mode) || status.st_uid != getuid() ||
        (status.st_mode & 0077) != 0) {
        // 只收紧本用户创建的真实目录，不跟随符号链接。
        if (!S_ISDIR(status.st_mode) || status.st_uid != getuid() || chmod(dir.c_str(), 0700) != 0) return {};
    }
    return dir.string();
}
inline std::string x11_icon_cache_file(const std::string& dir, const std::string& source, int size) {
    struct stat status{};
    if (dir.empty() || source.empty() || stat(source.c_str(), &status) != 0 || !S_ISREG(status.st_mode)) return {};
    const auto key = std::string("alpha-v1|") + source + "|" + std::to_string(size) + "|" +
        std::to_string(status.st_size) + "|" + std::to_string(status.st_mtim.tv_sec) + "|" + std::to_string(status.st_mtim.tv_nsec);
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char byte : key) { hash ^= byte; hash *= 1099511628211ull; }
    char name[40]; std::snprintf(name, sizeof name, "/%016llx.mask", static_cast<unsigned long long>(hash));
    return dir + name;
}
inline bool x11_read_icon_cache(const std::string& path, int size, SystemIconMask& mask) {
    if (path.empty()) return false;
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return false;
    FILE* input = fdopen(fd, "rb");
    if (!input) { close(fd); return false; }
    struct stat info{}; uint32_t header[3]{};
    bool ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_uid == getuid() &&
        std::fread(header, sizeof header, 1, input) == 1 && header[0] == 0x41494e31 &&
        header[1] > 0 && header[2] > 0 && header[1] <= uint32_t(size) && header[2] <= uint32_t(size) &&
        info.st_size == static_cast<off_t>(sizeof header + size_t(header[1]) * header[2]);
    if (ok) {
        mask.width = int(header[1]); mask.height = int(header[2]);
        mask.alpha.resize(size_t(mask.width) * mask.height);
        ok = std::fread(mask.alpha.data(), 1, mask.alpha.size(), input) == mask.alpha.size();
    }
    std::fclose(input);
    if (!ok) mask = {};
    return ok;
}
inline bool x11_write_icon_cache(const std::string& path, const SystemIconMask& mask) {
    if (path.empty() || mask.alpha.empty()) return false;
    std::string temporary = path + ".XXXXXX";
    const int fd = mkstemp(&temporary[0]);
    if (fd < 0) return false;
    FILE* output = fdopen(fd, "wb");
    if (!output) { close(fd); unlink(temporary.c_str()); return false; }
    const uint32_t header[]{0x41494e31, uint32_t(mask.width), uint32_t(mask.height)};
    bool ok = std::fwrite(header, sizeof header, 1, output) == 1 &&
        std::fwrite(mask.alpha.data(), 1, mask.alpha.size(), output) == mask.alpha.size();
    ok = std::fclose(output) == 0 && ok;
    if (ok) ok = rename(temporary.c_str(), path.c_str()) == 0;
    if (!ok) unlink(temporary.c_str());
    return ok;
}
inline void x11_trim_icon_cache(const std::string& dir) {
    namespace fs = std::filesystem;
    std::error_code ec; uintmax_t bytes = 0;
    std::vector<std::pair<fs::file_time_type, fs::path>> files;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        const auto status = it->symlink_status(ec);
        if (ec || !fs::is_regular_file(status) || it->path().extension() != ".mask") continue;
        const auto count = it->file_size(ec); if (ec) break;
        const auto time = it->last_write_time(ec); if (ec) break;
        bytes += count; files.push_back({time, it->path()});
    }
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& file : files) {
        if (bytes <= 8u * 1024u * 1024u) break;
        const auto count = fs::file_size(file.second, ec); if (ec) continue;
        if (fs::remove(file.second, ec)) bytes -= count;
    }
}
inline bool x11_run_icon_worker(int size) {
    std::string scale = std::to_string(size), theme = x11_effective_icon_theme();
    char executable[] = "/proc/self/exe", mode[] = "--aino-system-icons-v1";
    char* args[]{executable, mode, &scale[0], &theme[0], nullptr};
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) return false;
    int error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    if (!error) error = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    if (!error) error = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t pid = 0;
    if (!error) error = posix_spawn(&pid, executable, &actions, nullptr, args, ::environ);
    posix_spawn_file_actions_destroy(&actions);
    if (error) return false;
    timespec start{}; clock_gettime(CLOCK_MONOTONIC, &start);
    for (;;) {
        int status = 0; const auto result = waitpid(pid, &status, WNOHANG);
        if (result == pid) return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        if (result < 0 && errno != EINTR) return false;
        timespec now{}; clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec - start.tv_sec >= 5) {
            kill(pid, SIGKILL); while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            return false;
        }
        timespec delay{0, 2000000}; nanosleep(&delay, nullptr);
    }
}
#endif
inline SystemIconMask x11_load_system_icon(SystemIcon icon, int size) {
    const auto file = x11_system_icon_file(icon, size);
    if (file.empty()) return {};
#if defined(__linux__)
    if (x11_icon_worker_enabled()) {
        const auto path = x11_icon_cache_file(x11_icon_cache_dir(), file, size);
        SystemIconMask mask;
        if (x11_read_icon_cache(path, size, mask)) return mask;
        // 同尺寸图标批量生成一次；失败时保留原解码路径，不能让图标消失。
        if (!path.empty() && x11_run_icon_worker(size) && x11_read_icon_cache(path, size, mask)) return mask;
    }
#endif
    return x11_decode_system_icon(file, size);
}
#endif

inline SystemIconMask load_system_icon(SystemIcon icon, int size) {
    SystemIconMask mask;
#if defined(AINO_PLATFORM_WINDOWS)
    // 来自系统 Segoe Fluent Icons 字符表；Win10 回退到系统 Segoe MDL2 Assets。
    static const wchar_t codes[] = {
        0xE7C9, 0xE8D2, 0xEC12, 0xE9E9, 0xE8F9, 0xE700, 0xE787, 0xE790,
        0xEA8F, 0xE8A9, 0xE9D9, 0xE895, 0xE7C3, 0xE99C, 0xE8FD, 0xE8C8,
        0xE734, 0xE712, 0xE898, 0xE896, 0xE70F, 0xE74D, 0xE76B, 0xE76C,
        0xE708, 0xE706, 0xE7F4, 0xE711, 0xE890, 0xE710, 0xE8B7, 0xE721,
        0xE70D, 0xE70E, 0xE73E, 0xE738, 0xE713, 0xE701, 0xEB9F, 0xE946, 0xE7BA, 0xE716,
        0xE768, 0xE769, 0xE892, 0xE893, 0xE8B1, 0xE8ED, 0xE8EE, 0xE916
    };
    static_assert(sizeof(codes) / sizeof(codes[0]) == size_t(SystemIcon::Count));
    const size_t index = size_t(icon);
    if (index >= sizeof(codes) / sizeof(codes[0])) return mask;
    static const wchar_t* family = font_face_installed(L"Segoe Fluent Icons")
        ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets";
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return mask;
    HFONT font = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                            DEFAULT_PITCH, family);
    if (!font) { DeleteDC(dc); return mask; }
    const auto previous = SelectObject(dc, font);
    WORD glyph_index = 0xffff;
    GetGlyphIndicesW(dc, &codes[index], 1, &glyph_index, GGI_MARK_NONEXISTING_GLYPHS);
    if (glyph_index != 0xffff) {
        MAT2 identity{};
        identity.eM11.value = identity.eM22.value = 1;
        GLYPHMETRICS metrics{};
        const DWORD bytes = GetGlyphOutlineW(dc, codes[index], GGO_GRAY8_BITMAP, &metrics, 0, nullptr,
                                             &identity);
        if (bytes != GDI_ERROR && bytes > 0) {
            std::vector<uint8_t> bitmap(bytes);
            if (GetGlyphOutlineW(dc, codes[index], GGO_GRAY8_BITMAP, &metrics, bytes, bitmap.data(),
                                 &identity) != GDI_ERROR) {
                mask.width = int(metrics.gmBlackBoxX);
                mask.height = int(metrics.gmBlackBoxY);
                mask.alpha.resize(size_t(mask.width) * mask.height);
                const size_t stride = (size_t(mask.width) + 3) & ~size_t(3);
                for (int y = 0; y < mask.height; ++y)
                    for (int x = 0; x < mask.width; ++x)
                        mask.alpha[size_t(y) * mask.width + x] = uint8_t(
                            std::min(255u, unsigned(bitmap[size_t(y) * stride + x]) * 255u / 64u));
            }
        }
    }
    SelectObject(dc, previous);
    DeleteObject(font);
    DeleteDC(dc);
#elif defined(AINO_PLATFORM_MACOS)
    // AppKit 只在主线程使用；图标取系统 SF Symbols，不捆绑第三方字体或仿绘路径。
    static const char* names[] = {
        "cursorarrow", "textformat", "switch.2", "slider.horizontal.3", "rectangle.split.3x1",
        "line.3.horizontal", "calendar", "paintpalette", "bell", "tablecells", "chart.bar",
        "arrow.triangle.2.circlepath", "rectangle", "sidebar.left", "list.bullet", "doc.on.doc",
        "star", "ellipsis", "square.and.arrow.up", "square.and.arrow.down", "pencil", "trash",
        "chevron.left", "chevron.right", "moon", "sun.max", "desktopcomputer", "xmark",
        "eye", "plus", "folder", "magnifyingglass", "chevron.down", "chevron.up", "checkmark", "minus",
        "gearshape", "wifi", "photo", "info.circle", "exclamationmark.triangle", "person.2",
        "play.fill", "pause.fill", "backward.end.fill", "forward.end.fill", "shuffle", "repeat.1", "repeat", "timer"
    };
    static_assert(sizeof(names) / sizeof(names[0]) == size_t(SystemIcon::Count));
    const size_t index = size_t(icon);
    if (index >= sizeof(names) / sizeof(names[0])) return mask;
    const auto image_class = objc_getClass("NSImage");
    const auto selector = sel_registerName("imageWithSystemSymbolName:accessibilityDescription:");
    if (!oc_send<BOOL>(image_class, sel_registerName("respondsToSelector:"), selector)) return mask;
    id pool = oc_send<id>(oc_send<id>(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")),
                          sel_registerName("init"));
    ScopeExit drain([&] { oc_send<void>(pool, sel_registerName("drain")); });
    CFStringRef name = CFStringCreateWithCString(nullptr, names[index], kCFStringEncodingUTF8);
    ScopeExit release([&] { if (name) CFRelease(name); });
    if (!name) return mask;
    id image = oc_send<id>(image_class, selector, reinterpret_cast<id>(const_cast<void*>(
                              reinterpret_cast<const void*>(name))), static_cast<id>(nullptr));
    CGRect proposed = CGRectMake(0, 0, size, size);
    CGImageRef cg_image = image ? oc_send<CGImageRef>(image,
        sel_registerName("CGImageForProposedRect:context:hints:"), &proposed,
        static_cast<id>(nullptr), static_cast<id>(nullptr)) : nullptr;
    if (cg_image) {
        mask.width = mask.height = size;
        mask.alpha.resize(size_t(size) * size, 0);
        CGContextRef context = CGBitmapContextCreate(mask.alpha.data(), size, size, 8, size, nullptr,
                                                     kCGImageAlphaOnly);
        if (context) {
            const double w = CGImageGetWidth(cg_image), h = CGImageGetHeight(cg_image);
            const double fit = size / std::max(w, h);
            CGContextDrawImage(context, CGRectMake((size - w * fit) / 2, (size - h * fit) / 2,
                                                   w * fit, h * fit), cg_image);
            CGContextRelease(context);
        } else mask = SystemIconMask{};
    }
#elif defined(AINO_PLATFORM_UNIX)
    return x11_load_system_icon(icon, size);
#else
    (void)icon; (void)size;
#endif
    return mask;
}
} // namespace detail

std::string executable_directory() {
    std::string path;
#if defined(AINO_PLATFORM_WINDOWS)
    std::vector<wchar_t> buffer(32768);
    const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
    if (n && n < buffer.size()) path = detail::from_wide(std::wstring(buffer.data(), n));
#elif defined(AINO_PLATFORM_MACOS)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (size && _NSGetExecutablePath(buffer.data(), &size) == 0) path = buffer.data();
#elif defined(AINO_PLATFORM_UNIX)
    std::vector<char> buffer(4096);
    const ssize_t n = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (n > 0 && size_t(n) < buffer.size() - 1) path.assign(buffer.data(), size_t(n));
#endif
    const auto separator = path.find_last_of("/\\");
    if (separator == std::string::npos) {
        std::fprintf(stderr, "[aino] Cannot locate executable directory\n");
        return {};
    }
    return path.substr(0, separator + 1);
}

#if defined(AINO_PLATFORM_WINDOWS)
namespace detail {
// 文件与资源共用解码状态机；任一步失败或分配异常都释放 COM 所有权。
struct WicDecode {
    HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    WicDecode() = default;
    WicDecode(const WicDecode&) = delete;
    WicDecode& operator=(const WicDecode&) = delete;
    ~WicDecode() {
        if (converter) converter->Release();
        if (frame) frame->Release();
        if (decoder) decoder->Release();
        if (stream) stream->Release();
        if (factory) factory->Release();
        if (SUCCEEDED(initialized)) CoUninitialize();
    }
    HRESULT create() {
        if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return initialized;
        return CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory, reinterpret_cast<void**>(&factory));
    }
    HRESULT pixels(Image& result) {
        UINT w = 0, h = 0;
        HRESULT hr = decoder ? decoder->GetFrame(0, &frame) : E_FAIL;
        if (SUCCEEDED(hr)) hr = frame->GetSize(&w, &h);
        if (SUCCEEDED(hr) && (!w || !h || w > 16384 || h > 16384 || uint64_t(w) * h > 67108864u)) hr = E_FAIL;
        if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr)) hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
        if (SUCCEEDED(hr)) {
            result.width = int(w); result.height = int(h);
            result.pixels.resize(size_t(w) * size_t(h) * 4u);
            hr = converter->CopyPixels(nullptr, w * 4u, UINT(result.pixels.size()), result.pixels.data());
        }
        if (FAILED(hr)) result = Image{};
        return hr;
    }
};
} // namespace detail
#endif

bool Image::load(const std::string& path, std::string* error) {
    Image result;
    if (error) error->clear();
#if defined(AINO_PLATFORM_WINDOWS)
    detail::WicDecode decode;
    HRESULT hr = decode.create();
    if (SUCCEEDED(hr)) hr = decode.factory->CreateDecoderFromFilename(detail::to_wide(path).c_str(),
        nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decode.decoder);
    if (SUCCEEDED(hr)) hr = decode.pixels(result);
#elif defined(AINO_PLATFORM_MACOS)
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr,
        reinterpret_cast<const UInt8*>(path.data()), CFIndex(path.size()), false);
    detail::ScopeExit release_url([&] { if (url) CFRelease(url); });
    CGImageSourceRef source = url ? CGImageSourceCreateWithURL(url, nullptr) : nullptr;
    detail::ScopeExit release_source([&] { if (source) CFRelease(source); });
    CFDictionaryRef properties = source ? CGImageSourceCopyPropertiesAtIndex(source, 0, nullptr) : nullptr;
    int64_t metadata_w = 0, metadata_h = 0;
    if (properties) {
        auto width = static_cast<CFNumberRef>(CFDictionaryGetValue(properties, kCGImagePropertyPixelWidth));
        auto height = static_cast<CFNumberRef>(CFDictionaryGetValue(properties, kCGImagePropertyPixelHeight));
        if (width && CFGetTypeID(width) == CFNumberGetTypeID()) CFNumberGetValue(width, kCFNumberSInt64Type, &metadata_w);
        if (height && CFGetTypeID(height) == CFNumberGetTypeID()) CFNumberGetValue(height, kCFNumberSInt64Type, &metadata_h);
        CFRelease(properties);
    }
    const bool bounded = metadata_w > 0 && metadata_h > 0 && metadata_w <= 16384 && metadata_h <= 16384
        && uint64_t(metadata_w) * uint64_t(metadata_h) <= 67108864u;
    CGImageRef image = source && bounded ? CGImageSourceCreateImageAtIndex(source, 0, nullptr) : nullptr;
    detail::ScopeExit release_image([&] { if (image) CGImageRelease(image); });
    const size_t w = image ? CGImageGetWidth(image) : 0, h = image ? CGImageGetHeight(image) : 0;
    if (w && h && w <= 16384 && h <= 16384 && w * h <= 67108864u) {
        result.width = int(w); result.height = int(h);
        result.pixels.resize(w * h * 4u);
        CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = space ? CGBitmapContextCreate(result.pixels.data(), w, h, 8, w * 4u,
            space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big) : nullptr;
        if (context) {
            CGContextTranslateCTM(context, 0, CGFloat(h));
            CGContextScaleCTM(context, 1, -1);
            CGContextDrawImage(context, CGRectMake(0, 0, w, h), image);
            CGContextRelease(context);
            for (size_t i = 0; i < result.pixels.size(); i += 4) {
                const unsigned a = result.pixels[i + 3];
                if (a) for (int c = 0; c < 3; ++c)
                    result.pixels[i + c] = uint8_t(std::min(255u, (unsigned(result.pixels[i + c]) * 255u + a / 2) / a));
            }
        } else result = Image{};
        if (space) CGColorSpaceRelease(space);
    }
#elif defined(AINO_PLATFORM_UNIX)
    static detail::X11IconPixels api;
    int metadata_w = 0, metadata_h = 0;
    const bool bounded = api.lib && api.file_info(path.c_str(), &metadata_w, &metadata_h)
        && metadata_w > 0 && metadata_h > 0 && metadata_w <= 16384 && metadata_h <= 16384
        && uint64_t(metadata_w) * uint64_t(metadata_h) <= 67108864u;
    void* image = bounded ? api.load(path.c_str(), -1, -1, 1, nullptr) : nullptr;
    detail::ScopeExit release_image([&] { if (image) api.unref(image); });
    if (image) {
        const int w = api.width(image), h = api.height(image), n = api.channels(image);
        const int stride = api.stride(image);
        const auto* src = api.pixels(image);
        if (src && w > 0 && h > 0 && w <= 16384 && h <= 16384 &&
            uint64_t(w) * uint64_t(h) <= 67108864u && (n == 3 || n == 4) && stride >= w * n) {
            result.width = w; result.height = h;
            result.pixels.resize(size_t(w) * size_t(h) * 4u);
            const bool alpha = n == 4 && api.has_alpha(image);
            for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
                const auto* p = src + size_t(y) * size_t(stride) + size_t(x) * size_t(n);
                auto* d = &result.pixels[(size_t(y) * size_t(w) + size_t(x)) * 4u];
                d[0] = p[0]; d[1] = p[1]; d[2] = p[2]; d[3] = alpha ? p[3] : 255;
            }
        }
    }
#endif
    if (!result.valid()) {
        if (error) *error = "Unable to decode image: " + path;
        return false;
    }
    *this = std::move(result);
    return true;
}

bool Image::load_resource(int resource_id, std::string* error) {
    if (error) error->clear();
#if defined(AINO_PLATFORM_WINDOWS)
    Image result;
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = resource_id > 0
        ? FindResourceW(module, MAKEINTRESOURCEW(resource_id), MAKEINTRESOURCEW(10)) : nullptr;
    HGLOBAL loaded = resource ? LoadResource(module, resource) : nullptr;
    const DWORD size = resource ? SizeofResource(module, resource) : 0;
    const BYTE* bytes = loaded && size ? static_cast<const BYTE*>(LockResource(loaded)) : nullptr;
    detail::WicDecode decode;
    HRESULT hr = bytes && size ? decode.create() : E_FAIL;
    if (SUCCEEDED(hr)) hr = decode.factory->CreateStream(&decode.stream);
    if (SUCCEEDED(hr)) hr = decode.stream->InitializeFromMemory(const_cast<BYTE*>(bytes), size);
    if (SUCCEEDED(hr)) hr = decode.factory->CreateDecoderFromStream(decode.stream, nullptr,
        WICDecodeMetadataCacheOnDemand, &decode.decoder);
    if (SUCCEEDED(hr)) hr = decode.pixels(result);
    if (!result.valid()) {
        if (error) *error = "Unable to decode Windows image resource " + std::to_string(resource_id);
        return false;
    }
    *this = std::move(result);
    return true;
#else
    if (error) *error = "Image resources are only supported on Windows";
    (void)resource_id;
    return false;
#endif
}

int system_icon_worker_entry(int argc, char** argv) {
#if defined(AINO_PLATFORM_UNIX) && defined(__linux__)
    if (argc > 1 && std::strcmp(argv[1], "--aino-system-icons-v1") == 0) {
        if (argc != 4 || !argv[2] || !*argv[2]) return 2;
        char* end = nullptr; const long size = std::strtol(argv[2], &end, 10);
        if (!end || *end || size < 1 || size > 512) return 2;
        detail::x11_icon_theme() = argv[3];
        const auto dir = detail::x11_icon_cache_dir();
        if (dir.empty()) return 3;
        for (size_t i = 0; i < size_t(SystemIcon::Count); ++i) {
            const auto file = detail::x11_system_icon_file(SystemIcon(i), int(size));
            const auto path = detail::x11_icon_cache_file(dir, file, int(size));
            detail::SystemIconMask cached;
            if (path.empty() || detail::x11_read_icon_cache(path, int(size), cached)) continue;
            const auto mask = detail::x11_decode_system_icon(file, int(size));
            if (!mask.alpha.empty() && !detail::x11_write_icon_cache(path, mask)) return 4;
        }
        detail::x11_trim_icon_cache(dir);
        return 0;
    }
    detail::x11_icon_worker_enabled() = true;
#else
    (void)argc; (void)argv;
#endif
    return -1;
}

bool Ui::system_icon(SystemIcon icon, const Rect& box, const Color& color) {
    if (impl->measure_on) { impl->measure_add(box); return true; }
    if (!impl->canvas || box.empty() || color.a == 0) return false;
#if defined(AINO_PLATFORM_WINDOWS) || defined(AINO_PLATFORM_MACOS) || defined(AINO_PLATFORM_UNIX)
    const Rect physical = impl->xf_phys(box);
    const int size = std::max(1, int(std::floor(std::min(physical.w, physical.h))));
    const uint64_t key = uint64_t(icon) << 32 | uint32_t(size);
    // 按线程缓存栅格化结果，复用时只混合像素；尺寸变化不保留系统 DC、字体或 NSImage。
    static thread_local std::unordered_map<uint64_t, detail::SystemIconMask> masks;
    static thread_local size_t mask_bytes = 0;
#ifdef AINO_PLATFORM_UNIX
    static thread_local std::string last_theme;
    const auto current_theme = detail::x11_effective_icon_theme();
    if (last_theme != current_theme) { masks.clear(); mask_bytes = 0; last_theme = current_theme; }
#endif
    if (size > 512 || size_t(icon) >= size_t(SystemIcon::Count)) return false;
    auto found = masks.find(key);
    if (found == masks.end()) {
        auto loaded = detail::load_system_icon(icon, size);
        const size_t cost = loaded.alpha.capacity();
        constexpr size_t budget = 8u << 20;
        while (!masks.empty() && (masks.size() >= 512 || mask_bytes + cost > budget)) {
            auto old = masks.begin(); mask_bytes -= old->second.alpha.capacity(); masks.erase(old);
        }
        found = masks.emplace(key, std::move(loaded)).first;
        mask_bytes += cost;
    }
    const auto& mask = found->second;
    if (mask.alpha.empty()) return false;
    const int left = int(std::floor(physical.cx() - mask.width * .5f));
    const int top = int(std::floor(physical.cy() - mask.height * .5f));
    // 掩膜本身就在 box 之内（size ≤ min(w,h) 且居中），所以只需要裁到画布和
    // 当前裁剪区；把裁剪算成一次区间，循环里就不再逐像素判边界。
    // 早先是 push_clip(box) + 逐像素 blend：一枚 24×24 图标要 576 次边界判断、
    // 两次裁剪栈重算（实测 2.1 µs），而它每帧的形状和颜色都没变。
    Canvas& cv = *impl->canvas;
    int x0 = left, x1 = left + mask.width;
    int y0 = top,  y1 = top + mask.height;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > cv.w) x1 = cv.w;
    if (y1 > cv.h) y1 = cv.h;
    if (cv.clipped) {
        if (x0 < cv.clip_x0) x0 = cv.clip_x0;
        if (y0 < cv.clip_y0) y0 = cv.clip_y0;
        if (x1 > cv.clip_x1) x1 = cv.clip_x1;
        if (y1 > cv.clip_y1) y1 = cv.clip_y1;
    }
    if (x1 <= x0) return true;
    for (int y = y0; y < y1; ++y) {
        const uint8_t* row = mask.alpha.data() + size_t(y - top) * size_t(mask.width)
                           + size_t(x0 - left);
        cv.blend_mask_row8(y, x0, x1, color, row);
    }
    return true;
#else
    return false;
#endif
}

} // namespace aino

#endif // AINO_IMPLEMENTED
#endif // AINO_IMPLEMENTATION
