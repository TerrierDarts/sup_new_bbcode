// =========================================================================
// SUP BBCode Text — Global macros system + Win32 Macro Editor
// =========================================================================
#include "bbcode-source.h"
#include <obs-frontend-api.h>

// ---------------------------------------------------------------------------
// Global macro storage
// ---------------------------------------------------------------------------
std::vector<macro_def> g_macros;
const int MAX_MACROS = 10;

void load_global_macros()
{
    g_macros.clear();
    char *path = obs_module_config_path("macros.json");
    if (!path) return;

    obs_data_t *data = obs_data_create_from_json_file(path);
    bfree(path);
    if (!data) return;

    obs_data_array_t *arr = obs_data_get_array(data, "macros");
    if (arr) {
        size_t count = obs_data_array_count(arr);
        for (size_t i = 0; i < count && i < (size_t)MAX_MACROS; i++) {
            obs_data_t *item = obs_data_array_item(arr, i);
            const char *n = obs_data_get_string(item, "name");
            const char *t = obs_data_get_string(item, "tags");
            if (n && *n) {
                macro_def md;
                md.name = n;
                md.tags = t ? t : "";
                g_macros.push_back(std::move(md));
            }
            obs_data_release(item);
        }
        obs_data_array_release(arr);
    }
    obs_data_release(data);
}

void save_global_macros()
{
    char *path = obs_module_config_path("macros.json");
    if (!path) return;

    // Ensure directory exists
    char *dir = obs_module_config_path("");
    if (dir) { os_mkdirs(dir); bfree(dir); }

    obs_data_t *data = obs_data_create();
    obs_data_array_t *arr = obs_data_array_create();
    for (auto &m : g_macros) {
        obs_data_t *item = obs_data_create();
        obs_data_set_string(item, "name", m.name.c_str());
        obs_data_set_string(item, "tags", m.tags.c_str());
        obs_data_array_push_back(arr, item);
        obs_data_release(item);
    }
    obs_data_set_array(data, "macros", arr);
    obs_data_save_json(data, path);
    obs_data_array_release(arr);
    obs_data_release(data);
    bfree(path);
}

#ifdef _WIN32
// ---------------------------------------------------------------------------
// Macro Editor Dialog (Tools menu -> "StreamUP - BBCode Macros")
// Styled to match the StreamUP OBS plugin design system
// ---------------------------------------------------------------------------
static const wchar_t *MACRO_WND_CLASS = L"SUPBBCodeMacroEditor";
static HWND g_macro_hwnd = nullptr;
static HWND g_macro_name_edits[10] = {};
static HWND g_macro_tags_edits[10] = {};

static HBRUSH g_sup_bg_brush      = nullptr;
static HBRUSH g_sup_field_brush   = nullptr;
static HBRUSH g_sup_btn_brush     = nullptr;

// Forward declare bbcode_text_update for source refresh after save
static void bbcode_text_update(void *data, obs_data_t *settings);

static LRESULT CALLBACK macro_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        HWND ctrl = (HWND)lp;
        int id = GetDlgCtrlID(ctrl);
        if (id == 1 || id == 4)
            SetTextColor(hdc, SUP_TEXT_PRIMARY);
        else if (id == 3 || id == 6)
            SetTextColor(hdc, SUP_TEXT_MUTED);
        else if (id == 5)
            SetTextColor(hdc, SUP_ACCENT);
        else
            SetTextColor(hdc, SUP_TEXT_SECONDARY);
        SetBkColor(hdc, SUP_BG_DARKEST);
        if (!g_sup_bg_brush)
            g_sup_bg_brush = CreateSolidBrush(SUP_BG_DARKEST);
        return (LRESULT)g_sup_bg_brush;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, SUP_TEXT_PRIMARY);
        SetBkColor(hdc, SUP_BG_PRIMARY);
        if (!g_sup_field_brush)
            g_sup_field_brush = CreateSolidBrush(SUP_BG_PRIMARY);
        return (LRESULT)g_sup_field_brush;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wp;
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(SUP_BG_DARKEST);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);
        return 1;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lp;
        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;
        bool pressed = (dis->itemState & ODS_SELECTED) != 0;
        int id = (int)dis->CtlID;

        COLORREF bg_col, text_col;
        if (id == 9999) {
            bg_col = pressed ? SUP_ACCENT_HOVER : SUP_ACCENT;
            text_col = SUP_TEXT_PRIMARY;
        } else if (id >= 8000 && id < 8010) {
            bg_col = pressed ? SUP_BG_TERTIARY : SUP_BG_PRIMARY;
            text_col = SUP_TEXT_SECONDARY;
        } else if (id == 9998) {
            bg_col = pressed ? SUP_BG_TERTIARY : SUP_BG_SECONDARY;
            text_col = SUP_TEXT_SECONDARY;
        } else {
            break;
        }

        HBRUSH br = CreateSolidBrush(bg_col);
        HPEN pen = CreatePen(PS_SOLID, 1, bg_col);
        HBRUSH old_br = (HBRUSH)SelectObject(hdc, br);
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 18, 18);
        SelectObject(hdc, old_br);
        SelectObject(hdc, old_pen);
        DeleteObject(br);
        DeleteObject(pen);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, text_col);
        HFONT font = CreateFontW(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT old_font = (HFONT)SelectObject(hdc, font);
        wchar_t text[128] = {};
        GetWindowTextW(dis->hwndItem, text, 127);
        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old_font);
        DeleteObject(font);
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT cr;
        GetClientRect(hwnd, &cr);
        int cw = cr.right;

        auto draw_card = [&](int x, int y, int w, int h) {
            HBRUSH cbr = CreateSolidBrush(SUP_BG_PRIMARY);
            HPEN cpen = CreatePen(PS_SOLID, 1, SUP_BORDER);
            HBRUSH old_br = (HBRUSH)SelectObject(hdc, cbr);
            HPEN old_pen = (HPEN)SelectObject(hdc, cpen);
            RoundRect(hdc, x, y, x + w, y + h, 14, 14);
            SelectObject(hdc, old_br);
            SelectObject(hdc, old_pen);
            DeleteObject(cbr);
            DeleteObject(cpen);
        };

        int pad = 20;
        draw_card(pad, 70, cw - pad * 2, 368);
        draw_card(pad, 450, cw - pad * 2, 68);
        draw_card(pad, 528, cw - pad * 2, 52);

        HPEN col_line = CreatePen(PS_SOLID, 1, SUP_BORDER);
        HPEN old_pen = (HPEN)SelectObject(hdc, col_line);
        MoveToEx(hdc, pad + 12, 108, nullptr);
        LineTo(hdc, cw - pad - 12, 108);
        SelectObject(hdc, old_pen);
        DeleteObject(col_line);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id == 9999) { // Save & Close
            g_macros.clear();
            for (int i = 0; i < MAX_MACROS; i++) {
                wchar_t name_buf[256] = {}, tags_buf[1024] = {};
                GetWindowTextW(g_macro_name_edits[i], name_buf, 255);
                GetWindowTextW(g_macro_tags_edits[i], tags_buf, 1023);
                std::string name_u8, tags_u8;
                for (int j = 0; name_buf[j]; j++) name_u8 += (char)name_buf[j];
                for (int j = 0; tags_buf[j]; j++) tags_u8 += (char)tags_buf[j];
                if (!name_u8.empty()) {
                    macro_def md;
                    md.name = name_u8;
                    md.tags = tags_u8;
                    g_macros.push_back(std::move(md));
                }
            }
            save_global_macros();

            // Force all BBCode sources to re-render with updated macros
            obs_enum_sources([](void *, obs_source_t *src) -> bool {
                const char *id = obs_source_get_unversioned_id(src);
                if (id && strcmp(id, "bbcode_text_source") == 0) {
                    obs_data_t *settings = obs_source_get_settings(src);
                    obs_source_update(src, settings);
                    obs_data_release(settings);
                }
                return true;
            }, nullptr);

            DestroyWindow(hwnd);
            return 0;
        }
        if (id == 9998) { DestroyWindow(hwnd); return 0; }
        if (id == 8000) { ShellExecuteW(nullptr, L"open", L"https://docs.streamup.tips", nullptr, nullptr, SW_SHOW); return 0; }
        if (id == 8001) { ShellExecuteW(nullptr, L"open", L"https://discord.streamup.tips", nullptr, nullptr, SW_SHOW); return 0; }
        if (id == 8002) { ShellExecuteW(nullptr, L"open", L"https://streamup.tips", nullptr, nullptr, SW_SHOW); return 0; }
        break;
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
        break;
    case WM_DESTROY:
        if (g_sup_bg_brush)    { DeleteObject(g_sup_bg_brush);    g_sup_bg_brush = nullptr; }
        if (g_sup_field_brush) { DeleteObject(g_sup_field_brush); g_sup_field_brush = nullptr; }
        if (g_sup_btn_brush)   { DeleteObject(g_sup_btn_brush);   g_sup_btn_brush = nullptr; }
        g_macro_hwnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void show_macro_editor(void *)
{
    if (g_macro_hwnd && IsWindow(g_macro_hwnd)) {
        SetForegroundWindow(g_macro_hwnd);
        return;
    }

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = macro_wnd_proc;
        wc.hInstance      = GetModuleHandleW(nullptr);
        wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground  = CreateSolidBrush(SUP_BG_DARKEST);
        wc.lpszClassName  = MACRO_WND_CLASS;
        RegisterClassW(&wc);
        registered = true;
    }

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = 600, wh = 660;
    int wx = (sw - ww) / 2;
    int wy = (sh - wh) / 2;

    g_macro_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        MACRO_WND_CLASS,
        L"StreamUP \u2014 BBCode Macros",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        wx, wy, ww, wh,
        nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);

    HFONT title_font = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT desc_font = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT label_font = CreateFontW(13, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT card_title_font = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT edit_font = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");

    int pad = 20;
    int content_left = pad + 12;
    int content_w = ww - pad * 2 - 24 - 16;

    // Header
    HWND h_title = CreateWindowExW(0, L"STATIC",
        L"\u2728 Global BBCode Macros",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        pad, 12, 400, 28, g_macro_hwnd,
        (HMENU)1, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_title, WM_SETFONT, (WPARAM)title_font, TRUE);

    HWND h_desc = CreateWindowExW(0, L"STATIC",
        L"Define reusable style macros. Use [use=name] in any BBCode source.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        pad, 42, 520, 18, g_macro_hwnd,
        (HMENU)3, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_desc, WM_SETFONT, (WPARAM)desc_font, TRUE);

    // Macro editing card
    int card_y = 78;
    int name_w = 110, tags_w = content_w - name_w - 10;
    int gap = 10;

    HWND h_name_hdr = CreateWindowExW(0, L"STATIC", L"NAME",
        WS_CHILD | WS_VISIBLE, content_left, card_y + 8, name_w, 16,
        g_macro_hwnd, (HMENU)2, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_name_hdr, WM_SETFONT, (WPARAM)label_font, TRUE);

    HWND h_tags_hdr = CreateWindowExW(0, L"STATIC", L"TAGS",
        WS_CHILD | WS_VISIBLE, content_left + name_w + gap, card_y + 8, tags_w, 16,
        g_macro_hwnd, (HMENU)2, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_tags_hdr, WM_SETFONT, (WPARAM)label_font, TRUE);

    int y = card_y + 42;
    for (int i = 0; i < MAX_MACROS; i++) {
        g_macro_name_edits[i] = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER,
            content_left, y, name_w, 24, g_macro_hwnd,
            (HMENU)(LONG_PTR)(100 + i * 2),
            GetModuleHandleW(nullptr), nullptr);
        SendMessage(g_macro_name_edits[i], WM_SETFONT, (WPARAM)edit_font, TRUE);

        g_macro_tags_edits[i] = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER,
            content_left + name_w + gap, y, tags_w, 24, g_macro_hwnd,
            (HMENU)(LONG_PTR)(101 + i * 2),
            GetModuleHandleW(nullptr), nullptr);
        SendMessage(g_macro_tags_edits[i], WM_SETFONT, (WPARAM)edit_font, TRUE);

        if (i < (int)g_macros.size()) {
            std::wstring wn, wt;
            for (auto c : g_macros[i].name) wn += (wchar_t)(unsigned char)c;
            for (auto c : g_macros[i].tags) wt += (wchar_t)(unsigned char)c;
            SetWindowTextW(g_macro_name_edits[i], wn.c_str());
            SetWindowTextW(g_macro_tags_edits[i], wt.c_str());
        }
        y += 30;
    }

    // Save & Close pill
    int btn_w = 140, btn_h = 30;
    y += 8;
    CreateWindowExW(0, L"BUTTON", L"Save & Close",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        (ww - btn_w) / 2, y, btn_w, btn_h, g_macro_hwnd,
        (HMENU)9999, GetModuleHandleW(nullptr), nullptr);

    // Version Information card
    int vi_y = 458;
    HWND h_vi_title = CreateWindowExW(0, L"STATIC",
        L"\xD83D\xDCC1  Version Information",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        content_left, vi_y, 200, 18, g_macro_hwnd,
        (HMENU)5, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_vi_title, WM_SETFONT, (WPARAM)card_title_font, TRUE);

    HWND h_vi_line = CreateWindowExW(0, L"STATIC",
        L"Version: 0.1.0   \u2022   Build: Release   \u2022   Author: TerrierDarts",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        content_left, vi_y + 22, 480, 16, g_macro_hwnd,
        (HMENU)6, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_vi_line, WM_SETFONT, (WPARAM)desc_font, TRUE);

    // Useful Links card
    int ul_y = 536;
    HWND h_ul_title = CreateWindowExW(0, L"STATIC",
        L"\xD83D\xDD17  Useful Links",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        content_left, ul_y, 200, 18, g_macro_hwnd,
        (HMENU)5, GetModuleHandleW(nullptr), nullptr);
    SendMessage(h_ul_title, WM_SETFONT, (WPARAM)card_title_font, TRUE);

    int link_y = ul_y + 22;
    int pill_h = 24;
    int pill_gap = 8;
    int lx = content_left;

    CreateWindowExW(0, L"BUTTON", L"\xD83D\xDCC4 Documentation",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        lx, link_y, 120, pill_h, g_macro_hwnd,
        (HMENU)8000, GetModuleHandleW(nullptr), nullptr);
    lx += 120 + pill_gap;

    CreateWindowExW(0, L"BUTTON", L"\xD83D\xDCAC Discord",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        lx, link_y, 100, pill_h, g_macro_hwnd,
        (HMENU)8001, GetModuleHandleW(nullptr), nullptr);
    lx += 100 + pill_gap;

    CreateWindowExW(0, L"BUTTON", L"\xD83C\xDF10 Website",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        lx, link_y, 95, pill_h, g_macro_hwnd,
        (HMENU)8002, GetModuleHandleW(nullptr), nullptr);

    // Close button
    int close_w = 100, close_h = 28;
    CreateWindowExW(0, L"BUTTON", L"Close",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        (ww - close_w) / 2, wh - 58, close_w, close_h, g_macro_hwnd,
        (HMENU)9998, GetModuleHandleW(nullptr), nullptr);

    ShowWindow(g_macro_hwnd, SW_SHOW);
    UpdateWindow(g_macro_hwnd);
}

#else  // !_WIN32
void show_macro_editor(void *)
{
    blog(LOG_WARNING, "[sup-bbcode] Macro editor is only available on Windows");
}
#endif // _WIN32
