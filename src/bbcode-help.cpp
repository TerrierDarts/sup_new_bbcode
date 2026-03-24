// =========================================================================
// SUP BBCode Text — Help popup (Win32 dark-themed Tag Reference)
// =========================================================================
#include "bbcode-source.h"

#ifdef _WIN32

// Help popup palette (reuses SUP_ palette)
#define HELP_CARD_BG     SUP_BG_PRIMARY
#define HELP_BG          SUP_BG_DARKEST
#define HELP_FG          SUP_TEXT_PRIMARY
#define HELP_ACCENT      SUP_ACCENT
#define HELP_TAG         RGB(255, 134, 189)  // rgba(255, 134, 189, 1) green for tag examples
#define HELP_DIM         SUP_TEXT_MUTED
#define HELP_BORDER      SUP_BORDER

static const wchar_t *HELP_WND_CLASS = L"SUPBBCodeHelpWindow";
static HWND g_help_hwnd = nullptr;

struct HelpSection {
    const char *title;
    const char *lines;
};

static const HelpSection HELP_SECTIONS[] = {
    {"Text Formatting",
     ">[b]Bold[/b]  [i]Italic[/i]  [u]Underline[/u]  [s]Strikethrough[/s]\n"
     ">[dropcap]Drop Cap[/dropcap]  — first letter at 3x size"},

    {"Font & Size",
     ">[font=Comic Sans MS]Custom font[/font]\n"
     ">[size=48]Big text[/size]  — size in points (1-500)"},

    {"Colors",
     ">[color=#FF0000]Hex color[/color]\n"
     ">[color=red]Named color[/color]\n"
     " Supports ~140 Godot/X11 named colors!\n"
     " Use underscores, hyphens, or none: dark_red, dark-red, darkred\n"
     " Common: red, green, blue, yellow, cyan, magenta, orange, pink,\n"
     " purple, gray, gold, brown, coral, salmon, crimson, violet..."},

    {"Opacity",
     ">[opacity=0.5]Semi-transparent[/opacity]\n"
     " Value from 0.0 (invisible) to 1.0 (fully opaque)"},

    {"Background / Highlight",
     ">[bg=yellow]Highlighted text[/bg]\n"
     ">[bg=#FF0000]Red background[/bg]"},

    {"Gradient",
     ">[gradient from=red to=blue]Smooth blend[/gradient]\n"
     " from/to = hex (#FF0000) or named color"},

    {"Shadow",
     ">[shadow]Default shadow[/shadow]\n"
     ">[shadow color=gray x=3 y=3]Custom shadow[/shadow]\n"
     " color = shadow color, x/y = pixel offset (default 2)"},

    {"Outline",
     ">[outline color=#000000 size=2]Outlined[/outline]\n"
     " color = outline color (hex or named), size = 1-20"},

    {"New Lines",
     " Press Enter in the text box, or use [/n] tag"},

    {"Animated Effects  (all combinable!)",
     ">[wave amp=50 freq=5]Wavy[/wave]\n"
     " amp = height, freq = speed\n"
     "\n"
     ">[rainbow freq=1 sat=0.8 val=0.8 speed=1]Rainbow[/rainbow]\n"
     " freq = color spread, sat/val = color, speed = cycle/sec\n"
     "\n"
     ">[pulse freq=1 ease=-2]Pulse[/pulse]\n"
     " freq = pulse speed, ease = easing exponent\n"
     "\n"
     ">[tornado radius=10 freq=1]Tornado[/tornado]\n"
     " radius = circle size, freq = rotation speed\n"
     "\n"
     ">[shake rate=20 level=5]Shake[/shake]\n"
     " rate = speed, level = distance\n"
     "\n"
     ">[fade start=4 length=14]Fade[/fade]\n"
     " start = char offset, length = chars to fade over\n"
     "\n"
     ">[bounce amp=20 freq=3]Bounce[/bounce]\n"
     " amp = hop height, freq = hops/sec\n"
     "\n"
     ">[typewriter speed=10 loop=0]Typewriter[/typewriter]\n"
     " speed = chars/sec, loop = pause secs (0 = play once)\n"
     "\n"
     ">[glitch rate=5 intensity=1]Glitch[/glitch]\n"
     " rate = scrambles/sec, intensity = fraction affected\n"
     "\n"
     ">[scroll speed=50]Scrolling marquee[/scroll]\n"
     " speed = pixels/sec, text scrolls within its space\n"
     "\n"
     ">[curve radius=200 angle=180]Curved arc[/curve]\n"
     " radius = arc size, angle = arc degrees\n"
     "\n"
     ">[hacker speed=5 loop=2]Hacker reveal[/hacker]\n"
     " speed = chars revealed/sec, loop = pause secs (0 = play once)\n"
     " All chars scramble randomly, then lock in left-to-right"},

    {"Combining Effects",
     ">[rainbow][bounce]Rainbow + Bounce[/bounce][/rainbow]\n"
     ">[wave][shake]Wavy + Shaky[/shake][/wave]\n"
     " All effects can be nested and will stack!"},

    {"Global Tags",
     " Use the 'Global Tags' box to wrap ALL text automatically.\n"
     " Type opening tags and closing tags are added for you.\n"
     "\n"
     " Example: typing [rainbow][wave] in Global Tags wraps\n"
     " your entire text in [rainbow][wave]...[/wave][/rainbow]\n"
     "\n"
     " Great for applying effects without editing the text,\n"
     " and works when other plugins supply the text content!"},

    {"External Control (Proc Handlers)",
     " Other plugins, scripts, and obs-websocket can control\n"
     " this source programmatically via proc handlers:\n"
     "\n"
     ">set_text(text)        — set the BBCode text\n"
     ">get_text()            — read the current text\n"
     ">set_global_tags(tags) — set global wrapper tags\n"
     ">get_global_tags()     — read current global tags\n"
     "\n"
     " Works with obs-websocket CallVendorRequest,\n"
     " OBS Lua/Python scripts, and StreamerBot!"},

    {"Style Macros  [use=name]",
     " Define reusable style presets in the StreamUP group!\n"
     " Set a macro Name (e.g. alert) and Tags (e.g. [color=red][b])\n"
     "\n"
     ">1. In settings: Name=alert  Tags=[color=red][b][shake]\n"
     ">2. In text: [use=alert]Warning![/use]\n"
     "\n"
     " [use=name] applies all the macro's tags at once\n"
     " [/use] resets style back to defaults\n"
     " Up to 10 macros — great for consistent styling!"},

    {"Reset Tags",
     ">[reset]  or  [/all]\n"
     " Instantly closes ALL open tags and returns to defaults\n"
     "\n"
     " Auto-Close Tags checkbox: when enabled, automatically\n"
     " closes all open tags at the end of your text"},

    {"Source Settings",
     "\n"
     ">[refresh_on_active] Refresh effects when source becomes active\n"
     "\n"
     " This setting allows the typewriter or hacker effect to restart\n"
     " whenever the source becomes active. Useful for looping animations\n"
     " or ensuring consistent behavior during scene transitions."},
};
static const int HELP_SECTION_COUNT = sizeof(HELP_SECTIONS) / sizeof(HELP_SECTIONS[0]);

static LRESULT CALLBACK help_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static int scroll_y = 0;
    static int content_h = 0;

    switch (msg) {
    case WM_CREATE: {
        scroll_y = 0;
        content_h = 0;
        int y = 20;
        for (int s = 0; s < HELP_SECTION_COUNT; s++) {
            y += 30;
            const char *p = HELP_SECTIONS[s].lines;
            while (*p) { if (*p == '\n') y += 18; p++; }
            y += 18 + 12;
        }
        content_h = y + 140;
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT cr;
        GetClientRect(hwnd, &cr);
        int cw = cr.right, ch = cr.bottom;

        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, cw, ch);
        HGDIOBJ old_bmp = SelectObject(mem, bmp);

        HBRUSH bg_brush = CreateSolidBrush(HELP_BG);
        FillRect(mem, &cr, bg_brush);
        DeleteObject(bg_brush);

        SetBkMode(mem, TRANSPARENT);

        HFONT title_font  = CreateFontW(22, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT card_title  = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT header_font = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT tag_font    = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, FIXED_PITCH,  L"Consolas");
        HFONT desc_font   = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT small_font  = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        int y = 12 - scroll_y;
        int pad_x = 20;
        int card_pad = 12;

        // Main title
        SelectObject(mem, title_font);
        SetTextColor(mem, HELP_FG);
        {
            const wchar_t *ttl = L"\x2728 SUP BBCode Text \u2014 Tag Reference";
            TextOutW(mem, pad_x, y, ttl, (int)wcslen(ttl));
            y += 28;
        }
        SelectObject(mem, desc_font);
        SetTextColor(mem, HELP_DIM);
        {
            const wchar_t *sub = L"Complete reference for all supported BBCode tags and effects";
            TextOutW(mem, pad_x, y, sub, (int)wcslen(sub));
            y += 24;
        }

        // Section cards
        for (int si = 0; si < HELP_SECTION_COUNT; si++) {
            int sec_h = 28;
            const char *pc = HELP_SECTIONS[si].lines;
            while (*pc) { if (*pc == '\n') sec_h += 18; pc++; }
            sec_h += 18 + 8;

            {
                HBRUSH cbr = CreateSolidBrush(HELP_CARD_BG);
                HPEN cpen = CreatePen(PS_SOLID, 1, HELP_BORDER);
                HBRUSH old_br = (HBRUSH)SelectObject(mem, cbr);
                HPEN old_pen = (HPEN)SelectObject(mem, cpen);
                RoundRect(mem, pad_x, y, cw - pad_x, y + sec_h, 12, 12);
                SelectObject(mem, old_br);
                SelectObject(mem, old_pen);
                DeleteObject(cbr);
                DeleteObject(cpen);
            }

            SelectObject(mem, header_font);
            SetTextColor(mem, HELP_ACCENT);

            std::wstring sec_title;
            {
                const char *t = HELP_SECTIONS[si].title;
                while (*t) sec_title += (wchar_t)(unsigned char)*t++;
            }
            TextOutW(mem, pad_x + card_pad, y + 6, sec_title.c_str(), (int)sec_title.size());

            int line_y = y + 28;

            const char *line_start = HELP_SECTIONS[si].lines;
            while (*line_start) {
                const char *line_end = line_start;
                while (*line_end && *line_end != '\n') line_end++;

                std::string line(line_start, line_end);

                if (line.empty() || (line.size() == 1 && line[0] == ' ')) {
                    line_y += 6;
                } else if (!line.empty() && line[0] == '>') {
                    SelectObject(mem, tag_font);
                    SetTextColor(mem, HELP_TAG);
                    std::wstring wline;
                    for (size_t i = 1; i < line.size(); i++)
                        wline += (wchar_t)(unsigned char)line[i];
                    TextOutW(mem, pad_x + card_pad + 8, line_y, wline.c_str(), (int)wline.size());
                    line_y += 18;
                } else {
                    SelectObject(mem, desc_font);
                    SetTextColor(mem, HELP_DIM);
                    std::wstring wline;
                    for (size_t i = 0; i < line.size(); i++)
                        wline += (wchar_t)(unsigned char)line[i];
                    TextOutW(mem, pad_x + card_pad + 8, line_y, wline.c_str(), (int)wline.size());
                    line_y += 18;
                }

                line_start = (*line_end == '\n') ? line_end + 1 : line_end;
            }
            y += sec_h + 8;
        }

        // Version Information card
        {
            int vi_h = 52;
            HBRUSH cbr = CreateSolidBrush(HELP_CARD_BG);
            HPEN cpen = CreatePen(PS_SOLID, 1, HELP_BORDER);
            HBRUSH old_br = (HBRUSH)SelectObject(mem, cbr);
            HPEN old_pen = (HPEN)SelectObject(mem, cpen);
            RoundRect(mem, pad_x, y, cw - pad_x, y + vi_h, 12, 12);
            SelectObject(mem, old_br);
            SelectObject(mem, old_pen);
            DeleteObject(cbr);
            DeleteObject(cpen);

            SelectObject(mem, card_title);
            SetTextColor(mem, HELP_ACCENT);
            const wchar_t *vi_ttl = L"\xD83D\xDCC1  Version Information";
            TextOutW(mem, pad_x + card_pad, y + 6, vi_ttl, (int)wcslen(vi_ttl));

            SelectObject(mem, small_font);
            SetTextColor(mem, HELP_DIM);
            const wchar_t *vi_line = L"Version: 0.1.0   \u2022   Build: Release   \u2022   Author: TerrierDarts";
            TextOutW(mem, pad_x + card_pad, y + 28, vi_line, (int)wcslen(vi_line));

            y += vi_h + 8;
        }

        // Useful Links card
        {
            int ul_h = 52;
            HBRUSH cbr = CreateSolidBrush(HELP_CARD_BG);
            HPEN cpen = CreatePen(PS_SOLID, 1, HELP_BORDER);
            HBRUSH old_br = (HBRUSH)SelectObject(mem, cbr);
            HPEN old_pen = (HPEN)SelectObject(mem, cpen);
            RoundRect(mem, pad_x, y, cw - pad_x, y + ul_h, 12, 12);
            SelectObject(mem, old_br);
            SelectObject(mem, old_pen);
            DeleteObject(cbr);
            DeleteObject(cpen);

            SelectObject(mem, card_title);
            SetTextColor(mem, HELP_ACCENT);
            const wchar_t *ul_ttl = L"\xD83D\xDD17  Useful Links";
            TextOutW(mem, pad_x + card_pad, y + 6, ul_ttl, (int)wcslen(ul_ttl));

            int lx = pad_x + card_pad;
            int ly = y + 28;
            
            // Simple button replacements for draw_pill calls
            HBRUSH pbr = CreateSolidBrush(SUP_BG_SECONDARY);
            HPEN ppen = CreatePen(PS_SOLID, 1, HELP_BORDER);
            HBRUSH opb = (HBRUSH)SelectObject(mem, pbr);
            HPEN opp = (HPEN)SelectObject(mem, ppen);
            
            // Documentation button
            Rectangle(mem, lx, ly, lx + 110, ly + 18);
            RECT doc_rect = {lx, ly, lx + 110, ly + 18};
            SetTextColor(mem, SUP_TEXT_SECONDARY);
            DrawTextW(mem, L"📄 Documentation", -1, &doc_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            lx += 118;
            
            // Discord button
            Rectangle(mem, lx, ly, lx + 90, ly + 18);
            RECT discord_rect = {lx, ly, lx + 90, ly + 18};
            DrawTextW(mem, L"💬 Discord", -1, &discord_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            lx += 98;
            
            // Website button
            Rectangle(mem, lx, ly, lx + 85, ly + 18);
            RECT web_rect = {lx, ly, lx + 85, ly + 18};
            DrawTextW(mem, L"🌐 Website", -1, &web_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(mem, opb);
            SelectObject(mem, opp);
            DeleteObject(pbr);
            DeleteObject(ppen);

            y += ul_h + 8;
        }

        // Close pill
        {
            int close_w = 90, close_h = 26;
            int cx = (cw - close_w) / 2;
            HBRUSH cbr = CreateSolidBrush(SUP_BG_SECONDARY);
            HPEN cpen = CreatePen(PS_SOLID, 1, HELP_BORDER);
            HBRUSH old_br = (HBRUSH)SelectObject(mem, cbr);
            HPEN old_pen = (HPEN)SelectObject(mem, cpen);
            RoundRect(mem, cx, y, cx + close_w, y + close_h, close_h, close_h);
            SelectObject(mem, old_br);
            SelectObject(mem, old_pen);
            DeleteObject(cbr);
            DeleteObject(cpen);

            SelectObject(mem, small_font);
            SetTextColor(mem, SUP_TEXT_SECONDARY);
            RECT crc = {cx, y, cx + close_w, y + close_h};
            DrawTextW(mem, L"Close", -1, &crc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            y += close_h + 16;
        }

        content_h = y + scroll_y + 20;

        BitBlt(hdc, 0, 0, cw, ch, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old_bmp);
        DeleteObject(bmp);
        DeleteDC(mem);

        DeleteObject(title_font);
        DeleteObject(card_title);
        DeleteObject(header_font);
        DeleteObject(tag_font);
        DeleteObject(desc_font);
        DeleteObject(small_font);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONUP: {
        int mx = LOWORD(lp);
        int my = HIWORD(lp) + scroll_y;

        int pad_x = 20, card_pad = 12;
        int y = 64;
        for (int si = 0; si < HELP_SECTION_COUNT; si++) {
            int sec_h = 28;
            const char *pc = HELP_SECTIONS[si].lines;
            while (*pc) { if (*pc == '\n') sec_h += 18; pc++; }
            sec_h += 18 + 8;
            y += sec_h + 8;
        }
        int vi_h = 52;
        y += vi_h + 8;
        int ul_h = 52;
        int lx = pad_x + card_pad;
        int ly = y + 28;
        
        // Check if clicks are within button areas (simple rectangles now)
        // Documentation button area: lx, ly, 110x18
        // Discord button area: lx+118, ly, 90x18  
        // Website button area: lx+216, ly, 85x18

        y += ul_h + 8;
        RECT clr;
        GetClientRect(hwnd, &clr);
        int close_w = 90, close_h = 26;
        int ccx = (clr.right - close_w) / 2;
        if (mx >= ccx && mx <= ccx + close_w && my >= y && my <= y + close_h) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        scroll_y -= delta / 3;
        if (scroll_y < 0) scroll_y = 0;
        RECT cr;
        GetClientRect(hwnd, &cr);
        int max_scroll = content_h - cr.bottom;
        if (max_scroll < 0) max_scroll = 0;
        if (scroll_y > max_scroll) scroll_y = max_scroll;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_DESTROY:
        g_help_hwnd = nullptr;
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void show_help_window()
{
    if (g_help_hwnd && IsWindow(g_help_hwnd)) {
        SetForegroundWindow(g_help_hwnd);
        return;
    }

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = help_wnd_proc;
        wc.hInstance      = GetModuleHandleW(nullptr);
        wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground  = CreateSolidBrush(HELP_BG);
        wc.lpszClassName  = HELP_WND_CLASS;
        RegisterClassW(&wc);
        registered = true;
    }

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = 580, wh = 740;
    int wx = (sw - ww) / 2;
    int wy = (sh - wh) / 2;

    g_help_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        HELP_WND_CLASS,
        L"\x2728 SUP BBCode Text \u2014 Help",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        wx, wy, ww, wh,
        nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);

    ShowWindow(g_help_hwnd, SW_SHOW);
    UpdateWindow(g_help_hwnd);
}

bool help_button_clicked(obs_properties_t *, obs_property_t *, void *)
{
    show_help_window();
    return false;
}

#else  // !_WIN32
bool help_button_clicked(obs_properties_t *, obs_property_t *, void *)
{
    blog(LOG_WARNING, "[sup-bbcode] Help window is only available on Windows");
    return false;
}
#endif // _WIN32
