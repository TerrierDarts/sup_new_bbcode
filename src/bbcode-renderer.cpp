// =========================================================================
// SUP BBCode Text — Text rendering (GDI -> OBS texture)
// =========================================================================
#include "bbcode-source.h"

// ---------------------------------------------------------------------------
// Render a string to a GDI DIB -> OBS texture.  Returns texture + size.
// Must be called INSIDE obs_enter_graphics().
// Takes the segment for all style info + optional alpha multiplier for fade.
// ---------------------------------------------------------------------------
#ifdef _WIN32
gs_texture_t *render_string_to_tex(bbcode_text_source *s, const std::string &str,
                                    const bb_segment &seg,
                                    int &out_w, int &out_h,
                                    float alpha_mult)
{
    if (str.empty()) return nullptr;

    const char *face = (!seg.font_override.empty()) ? seg.font_override.c_str()
                     : (s->font_face && *s->font_face) ? s->font_face : "Arial";
    int fsize = (seg.size_override > 0) ? seg.size_override
              : (s->font_size > 0)      ? s->font_size : 48;

    float final_alpha = seg.opacity * alpha_mult;
    if (final_alpha <= 0.0f) { out_w = 0; out_h = 0; return nullptr; }

    uint32_t color         = seg.color;
    bool     bold          = seg.bold;
    bool     italic        = seg.italic;
    bool     has_outline   = seg.has_outline;
    uint32_t outline_color = seg.outline_color;
    int      outline_size  = seg.outline_size;
    bool     has_shadow    = seg.has_shadow;
    uint32_t shadow_color  = seg.shadow_color;
    int      shadow_x      = seg.shadow_x;
    int      shadow_y      = seg.shadow_y;

    wchar_t wface[64] = {};
    MultiByteToWideChar(CP_UTF8, 0, face, -1, wface, 63);

    HDC screen = GetDC(nullptr);
    HDC dc     = CreateCompatibleDC(screen);

    DWORD aa_quality = CLEARTYPE_QUALITY;
    if (s->antialias_mode == 1) aa_quality = ANTIALIASED_QUALITY;
    else if (s->antialias_mode == 2) aa_quality = NONANTIALIASED_QUALITY;

    HFONT font = CreateFontW(
        fsize, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        italic ? TRUE : FALSE,
        FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS, aa_quality,
        DEFAULT_PITCH | FF_DONTCARE, wface);

    HGDIOBJ old_font = SelectObject(dc, font);

    std::wstring wtext(str.begin(), str.end());
    SIZE sz = {};
    GetTextExtentPoint32W(dc, wtext.c_str(), (int)wtext.size(), &sz);

    int pad = has_outline ? outline_size : 0;
    int shadow_pad_r = (has_shadow && shadow_x > 0) ? shadow_x : 0;
    int shadow_pad_b = (has_shadow && shadow_y > 0) ? shadow_y : 0;
    int shadow_pad_l = (has_shadow && shadow_x < 0) ? -shadow_x : 0;
    int shadow_pad_t = (has_shadow && shadow_y < 0) ? -shadow_y : 0;
    int extra_l = pad + shadow_pad_l;
    int extra_r = pad + shadow_pad_r;
    int extra_t = pad + shadow_pad_t;
    int extra_b = pad + shadow_pad_b;

    int W = (sz.cx > 0 ? sz.cx : 8) + 8 + extra_l + extra_r;
    int H = (sz.cy > 0 ? sz.cy : fsize) + 4 + extra_t + extra_b;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       =  W;
    bi.bmiHeader.biHeight      = -H;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void   *bits = nullptr;
    HBITMAP bmp  = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp || !bits) {
        SelectObject(dc, old_font); DeleteObject(font);
        DeleteDC(dc); ReleaseDC(nullptr, screen);
        return nullptr;
    }

    HGDIOBJ old_bmp = SelectObject(dc, bmp);
    SelectObject(dc, font);

    memset(bits, 0, W * H * 4);
    SetBkMode(dc, TRANSPARENT);

    int text_x = 4 + extra_l;
    int text_y = 2 + extra_t;

    auto draw_decorated = [&](int dx, int dy) {
        TextOutW(dc, dx, dy, wtext.c_str(), (int)wtext.size());
        if (seg.underline) {
            TEXTMETRICW tm;
            GetTextMetricsW(dc, &tm);
            int line_y = dy + tm.tmAscent + 1;
            HPEN pen = CreatePen(PS_SOLID, 1, GetTextColor(dc));
            HGDIOBJ old_pen = SelectObject(dc, pen);
            MoveToEx(dc, dx, line_y, nullptr);
            LineTo(dc, dx + sz.cx, line_y);
            SelectObject(dc, old_pen);
            DeleteObject(pen);
        }
        if (seg.strikethrough) {
            TEXTMETRICW tm;
            GetTextMetricsW(dc, &tm);
            int line_y = dy + tm.tmAscent / 2;
            HPEN pen = CreatePen(PS_SOLID, 1, GetTextColor(dc));
            HGDIOBJ old_pen = SelectObject(dc, pen);
            MoveToEx(dc, dx, line_y, nullptr);
            LineTo(dc, dx + sz.cx, line_y);
            SelectObject(dc, old_pen);
            DeleteObject(pen);
        }
    };

    if (has_shadow || has_outline) {
        // Multi-layer: shadow -> outline -> main text

        // Pass 1: Shadow
        std::vector<uint8_t> shadow_alpha_map;
        if (has_shadow) {
            SetTextColor(dc, RGB(255, 255, 255));
            draw_decorated(text_x + shadow_x, text_y + shadow_y);
            GdiFlush();

            shadow_alpha_map.resize(W * H, 0);
            uint32_t *px = (uint32_t *)bits;
            for (int i = 0; i < W * H; i++) {
                uint8_t pb = (px[i])       & 0xFF;
                uint8_t pg = (px[i] >> 8)  & 0xFF;
                uint8_t pr = (px[i] >> 16) & 0xFF;
                uint8_t a = pr; if (pg > a) a = pg; if (pb > a) a = pb;
                shadow_alpha_map[i] = a;
            }
            memset(bits, 0, W * H * 4);
        }

        // Pass 2: Outline
        std::vector<uint8_t> outline_alpha_map;
        if (has_outline && outline_size > 0) {
            SetTextColor(dc, RGB(255, 255, 255));
            for (int oy = -outline_size; oy <= outline_size; oy++) {
                for (int ox = -outline_size; ox <= outline_size; ox++) {
                    if (ox == 0 && oy == 0) continue;
                    if (ox * ox + oy * oy > outline_size * outline_size + outline_size) continue;
                    draw_decorated(text_x + ox, text_y + oy);
                }
            }
            GdiFlush();

            outline_alpha_map.resize(W * H, 0);
            uint32_t *px = (uint32_t *)bits;
            for (int i = 0; i < W * H; i++) {
                uint8_t pb = (px[i])       & 0xFF;
                uint8_t pg = (px[i] >> 8)  & 0xFF;
                uint8_t pr = (px[i] >> 16) & 0xFF;
                uint8_t a = pr; if (pg > a) a = pg; if (pb > a) a = pb;
                outline_alpha_map[i] = a;
            }
            memset(bits, 0, W * H * 4);
        }

        // Pass 3: Main text
        SetTextColor(dc, RGB(255, 255, 255));
        draw_decorated(text_x, text_y);
        GdiFlush();

        // Composite
        uint8_t cr = (color >> 16) & 0xFF;
        uint8_t cg = (color >>  8) & 0xFF;
        uint8_t cb =  color        & 0xFF;
        uint8_t or_ = (outline_color >> 16) & 0xFF;
        uint8_t og  = (outline_color >>  8) & 0xFF;
        uint8_t ob  =  outline_color        & 0xFF;
        uint8_t sr  = (shadow_color >> 16) & 0xFF;
        uint8_t sg  = (shadow_color >>  8) & 0xFF;
        uint8_t sb  =  shadow_color        & 0xFF;

        uint32_t *px = (uint32_t *)bits;
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                int i = y * W + x;
                uint8_t pb = (px[i])       & 0xFF;
                uint8_t pg = (px[i] >> 8)  & 0xFF;
                uint8_t pr = (px[i] >> 16) & 0xFF;
                uint8_t text_a = pr; if (pg > text_a) text_a = pg; if (pb > text_a) text_a = pb;
                uint8_t out_a  = (!outline_alpha_map.empty()) ? outline_alpha_map[i] : 0;
                uint8_t shd_a  = (!shadow_alpha_map.empty())  ? shadow_alpha_map[i]  : 0;

                bool in_bg = seg.has_bg && x >= text_x && x < text_x + sz.cx
                                        && y >= text_y && y < text_y + sz.cy;

                if (text_a > 0) {
                    uint8_t fa = (uint8_t)(text_a * final_alpha);
                    px[i] = ((uint32_t)fa << 24) | ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | cb;
                } else if (out_a > 0) {
                    uint8_t fa = (uint8_t)(out_a * final_alpha);
                    px[i] = ((uint32_t)fa << 24) | ((uint32_t)or_ << 16) | ((uint32_t)og << 8) | ob;
                } else if (shd_a > 0) {
                    uint8_t fa = (uint8_t)(shd_a * final_alpha);
                    px[i] = ((uint32_t)fa << 24) | ((uint32_t)sr << 16) | ((uint32_t)sg << 8) | sb;
                } else if (in_bg) {
                    uint8_t br_ = (seg.bg_color >> 16) & 0xFF;
                    uint8_t bg_ = (seg.bg_color >>  8) & 0xFF;
                    uint8_t bb_ =  seg.bg_color        & 0xFF;
                    uint8_t fa  = (uint8_t)(255 * final_alpha);
                    px[i] = ((uint32_t)fa << 24) | ((uint32_t)br_ << 16) | ((uint32_t)bg_ << 8) | bb_;
                } else {
                    px[i] = 0;
                }
            }
        }
    } else {
        // Simple: no outline/shadow
        SetTextColor(dc, RGB(255, 255, 255));
        draw_decorated(text_x, text_y);
        GdiFlush();

        uint8_t cr = (color >> 16) & 0xFF;
        uint8_t cg = (color >>  8) & 0xFF;
        uint8_t cb =  color        & 0xFF;

        uint32_t *px = (uint32_t *)bits;
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                int i = y * W + x;
                uint8_t pb = (px[i])       & 0xFF;
                uint8_t pg = (px[i] >> 8)  & 0xFF;
                uint8_t pr = (px[i] >> 16) & 0xFF;
                uint8_t a = pr; if (pg > a) a = pg; if (pb > a) a = pb;

                bool in_bg = seg.has_bg && x >= text_x && x < text_x + (int)sz.cx
                                        && y >= text_y && y < text_y + (int)sz.cy;

                if (a > 0) {
                    uint8_t fa = (uint8_t)(a * final_alpha);
                    px[i] = ((uint32_t)fa << 24) | ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | cb;
                } else if (in_bg) {
                    uint8_t br_ = (seg.bg_color >> 16) & 0xFF;
                    uint8_t bg_ = (seg.bg_color >>  8) & 0xFF;
                    uint8_t bb_ =  seg.bg_color        & 0xFF;
                    uint8_t fa  = (uint8_t)(255 * final_alpha);
                    px[i] = ((uint32_t)fa << 24) | ((uint32_t)br_ << 16) | ((uint32_t)bg_ << 8) | bb_;
                } else {
                    px[i] = 0;
                }
            }
        }
    }

    std::vector<uint8_t> pix(W * H * 4);
    memcpy(pix.data(), bits, pix.size());

    SelectObject(dc, old_bmp); SelectObject(dc, old_font);
    DeleteObject(font); DeleteObject(bmp);
    DeleteDC(dc); ReleaseDC(nullptr, screen);

    const uint8_t *row0 = pix.data();
    gs_texture_t *tex = gs_texture_create((uint32_t)W, (uint32_t)H, GS_BGRA, 1, &row0, 0);
    out_w = W; out_h = H;
    return tex;
}
#else  // !_WIN32
gs_texture_t *render_string_to_tex(bbcode_text_source *s, const std::string &str,
                                    const bb_segment &seg,
                                    int &out_w, int &out_h,
                                    float alpha_mult)
{
    (void)s; (void)str; (void)seg; (void)alpha_mult;
    out_w = 0; out_h = 0;
    return nullptr;
}
#endif // _WIN32

// ---------------------------------------------------------------------------
// Build textures for a segment.
// For animated segments, render each character separately.
// Must be called INSIDE obs_enter_graphics().
//
// RAINBOW FIX: Rainbow chars are rendered in WHITE here (once).
// The actual rainbow color is applied at draw time via GPU color multiply
// in bbcode_text_render(), making it essentially free.
// ---------------------------------------------------------------------------
void render_seg_textures(bbcode_text_source *s, bb_segment &seg)
{
    if (seg.text.empty()) return;

    uint32_t fx_needing_per_char = seg.fx & ~FX_SCROLL;
    bool needs_per_char = (fx_needing_per_char != FX_NONE) || seg.has_gradient;

    if (needs_per_char) {
        int total_chars = (int)seg.text.size();
        for (int i = 0; i < total_chars; i++) {
            std::string ch(1, seg.text[i]);
            bb_char bc;
            bc.index = i;

            bb_segment char_seg = seg;

            // RAINBOW FIX: render in WHITE so we can tint at draw time
            if (seg.fx & FX_RAINBOW) {
                char_seg.color = 0xFFFFFF;
            }

            // Gradient: interpolate color from->to across the text length
            if (seg.has_gradient && total_chars > 1) {
                float t = (float)i / (float)(total_chars - 1);
                uint8_t fr = (seg.gradient_from >> 16) & 0xFF;
                uint8_t fg = (seg.gradient_from >>  8) & 0xFF;
                uint8_t fb =  seg.gradient_from        & 0xFF;
                uint8_t tr = (seg.gradient_to >> 16) & 0xFF;
                uint8_t tg = (seg.gradient_to >>  8) & 0xFF;
                uint8_t tb =  seg.gradient_to        & 0xFF;
                uint8_t mr = (uint8_t)(fr + t * (tr - fr));
                uint8_t mg = (uint8_t)(fg + t * (tg - fg));
                uint8_t mb = (uint8_t)(fb + t * (tb - fb));
                char_seg.color = ((uint32_t)mr << 16) | ((uint32_t)mg << 8) | mb;
            }

            // Per-character fade alpha
            float char_alpha = 1.0f;
            if (seg.fx & FX_FADE) {
                float start  = seg.params.fade_start;
                float length = seg.params.fade_length;
                float cp     = (float)i;
                if (cp < start)
                    char_alpha = 1.0f;
                else if (cp < start + length)
                    char_alpha = 1.0f - (cp - start) / fmaxf(length, 1.0f);
                else
                    char_alpha = 0.0f;
            }

            if (char_alpha <= 0.0f) {
                bc.tex = nullptr;
                bc.w = 0; bc.h = 0;
                int tmp_w = 0, tmp_h = 0;
                gs_texture_t *tmp = render_string_to_tex(s, ch, char_seg, tmp_w, tmp_h, 0.0f);
                bc.w = tmp_w; bc.h = tmp_h;
                if (tmp) gs_texture_destroy(tmp);
            } else {
                bc.tex = render_string_to_tex(s, ch, char_seg, bc.w, bc.h, char_alpha);
            }
            seg.chars.push_back(bc);
        }
        int tw = 0, th = 0;
        for (auto &c : seg.chars) { tw += c.w; if (c.h > th) th = c.h; }
        seg.tex_width  = tw;
        seg.tex_height = th;
    } else {
        seg.tex = render_string_to_tex(s, seg.text, seg, seg.tex_width, seg.tex_height);
    }
}

// ---------------------------------------------------------------------------
// Free segment resources
// ---------------------------------------------------------------------------
void free_segments(bbcode_text_source *s)
{
    for (auto &seg : s->segments) {
        if (seg.tex) { gs_texture_destroy(seg.tex); seg.tex = nullptr; }
        for (auto &c : seg.chars)
            if (c.tex) { gs_texture_destroy(c.tex); c.tex = nullptr; }
        seg.chars.clear();
    }
    s->segments.clear();
}
