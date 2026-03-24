// =========================================================================
// SUP BBCode Text — OBS source callbacks (create, update, tick, render, etc.)
// =========================================================================
#include "bbcode-source.h"

// ---------------------------------------------------------------------------
// Forward declaration
// ---------------------------------------------------------------------------
static void bbcode_text_update(void *data, obs_data_t *settings);

// ---------------------------------------------------------------------------
// OBS callbacks
// ---------------------------------------------------------------------------
static const char *bbcode_text_get_name(void *) { return "BBCode Text"; }

static void bbcode_text_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "text", "");
    obs_data_set_default_string(settings, "global_tags", "");

    obs_data_t *font_obj = obs_data_create();
    obs_data_set_default_string(font_obj, "face", "Arial");
    obs_data_set_default_int(font_obj, "size", 48);
    obs_data_set_default_int(font_obj, "flags", 0);
    obs_data_set_default_obj(settings, "font", font_obj);
    obs_data_release(font_obj);

    obs_data_set_default_int(settings, "default_color", 0xFFFFFFFF);
    obs_data_set_default_int(settings, "antialias_mode", 0);

    obs_data_set_default_int(settings, "pad_left",   0);
    obs_data_set_default_int(settings, "pad_right",  0);
    obs_data_set_default_int(settings, "pad_top",    15);
    obs_data_set_default_int(settings, "pad_bottom", 0);
    obs_data_set_default_int(settings, "custom_width",  0);
    obs_data_set_default_int(settings, "custom_height", 0);

    obs_data_set_default_bool(settings, "auto_close", false);
    obs_data_set_default_bool(settings, "refresh_on_active", false);
}

// ---------------------------------------------------------------------------
// Proc handlers — allow external control via obs-websocket / scripts
// ---------------------------------------------------------------------------
static void proc_set_text(void *data, calldata_t *cd)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    if (!s || !s->source) return;
    const char *text = calldata_string(cd, "text");
    if (!text) return;
    obs_data_t *settings = obs_source_get_settings(s->source);
    obs_data_set_string(settings, "text", text);
    bbcode_text_update(s, settings);
    obs_data_release(settings);
}

static void proc_get_text(void *data, calldata_t *cd)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    if (!s) return;
    calldata_set_string(cd, "text", s->raw ? s->raw : "");
}

static void proc_set_global_tags(void *data, calldata_t *cd)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    if (!s || !s->source) return;
    const char *tags = calldata_string(cd, "tags");
    if (!tags) tags = "";
    obs_data_t *settings = obs_source_get_settings(s->source);
    obs_data_set_string(settings, "global_tags", tags);
    bbcode_text_update(s, settings);
    obs_data_release(settings);
}

static void proc_get_global_tags(void *data, calldata_t *cd)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    if (!s) return;
    calldata_set_string(cd, "tags", s->global_tags ? s->global_tags : "");
}

// ---------------------------------------------------------------------------
// Create / Destroy / Update
// ---------------------------------------------------------------------------
static void *bbcode_text_create(obs_data_t *settings, obs_source_t *source)
{
    bbcode_text_source *s = new bbcode_text_source();
    s->source = source;

    // Load custom color-multiply effect for rainbow tinting
    char *effect_path = obs_module_file("color_multiply.effect");
    if (effect_path) {
        obs_enter_graphics();
        s->color_effect = gs_effect_create_from_file(effect_path, nullptr);
        obs_leave_graphics();
        bfree(effect_path);
    }

    bbcode_text_update(s, settings);

    proc_handler_t *ph = obs_source_get_proc_handler(source);
    proc_handler_add(ph, "void set_text(in string text)",        proc_set_text,        s);
    proc_handler_add(ph, "void get_text(out string text)",       proc_get_text,        s);
    proc_handler_add(ph, "void set_global_tags(in string tags)", proc_set_global_tags, s);
    proc_handler_add(ph, "void get_global_tags(out string tags)",proc_get_global_tags, s);

    return s;
}

static void bbcode_text_destroy(void *data)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    if (!s) return;
    obs_enter_graphics();
    free_segments(s);
    if (s->color_effect) {
        gs_effect_destroy(s->color_effect);
        s->color_effect = nullptr;
    }
    obs_leave_graphics();
    bfree(s->raw);
    bfree(s->font_face);
    bfree(s->global_tags);
    delete s;
}

static void bbcode_text_update(void *data, obs_data_t *settings)
{
    bbcode_text_source *s = (bbcode_text_source *)data;

    const char *text = obs_data_get_string(settings, "text");
    const char *effective = (text && *text) ? text : "Hello [b]BBCode[/b]!";

    bfree(s->raw);
    s->raw = bstrdup(effective);

    const char *gtags = obs_data_get_string(settings, "global_tags");
    bfree(s->global_tags);
    s->global_tags = bstrdup(gtags ? gtags : "");

    std::string wrapped;
    std::string open_tags;
    std::string close_tags;
    if (s->global_tags && s->global_tags[0]) {
        open_tags = s->global_tags;
        std::vector<std::string> tag_names;
        std::regex tag_re(R"(\[([a-zA-Z]+)[^\]]*\])");
        auto it = std::sregex_iterator(open_tags.begin(), open_tags.end(), tag_re);
        auto end_it = std::sregex_iterator();
        for (; it != end_it; ++it) {
            tag_names.push_back((*it)[1].str());
        }
        for (int i = (int)tag_names.size() - 1; i >= 0; i--) {
            close_tags += "[/" + tag_names[i] + "]";
        }
        wrapped = open_tags + effective + close_tags;
    } else {
        wrapped = effective;
    }

    obs_data_t *font_obj = obs_data_get_obj(settings, "font");
    if (font_obj) {
        const char *face = obs_data_get_string(font_obj, "face");
        int size         = (int)obs_data_get_int(font_obj, "size");
        int64_t flags    = obs_data_get_int(font_obj, "flags");

        bfree(s->font_face);
        s->font_face   = bstrdup((face && *face) ? face : "Arial");
        s->font_size   = (size > 4) ? size : 48;
        s->base_bold   = (flags & OBS_FONT_BOLD) != 0;
        s->base_italic = (flags & OBS_FONT_ITALIC) != 0;
        obs_data_release(font_obj);
    }

    uint32_t abgr = (uint32_t)obs_data_get_int(settings, "default_color");
    uint8_t cr = (abgr)       & 0xFF;
    uint8_t cg = (abgr >> 8)  & 0xFF;
    uint8_t cb = (abgr >> 16) & 0xFF;
    s->default_color = ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | (uint32_t)cb;

    s->antialias_mode = (int)obs_data_get_int(settings, "antialias_mode");

    s->pad_left   = (int)obs_data_get_int(settings, "pad_left");
    s->pad_right  = (int)obs_data_get_int(settings, "pad_right");
    s->pad_top    = (int)obs_data_get_int(settings, "pad_top");
    s->pad_bottom = (int)obs_data_get_int(settings, "pad_bottom");
    s->custom_width  = (int)obs_data_get_int(settings, "custom_width");
    s->custom_height = (int)obs_data_get_int(settings, "custom_height");

    s->auto_close = obs_data_get_bool(settings, "auto_close");
    
    // Check if refresh_on_active setting has changed
    bool refresh_on_active = obs_data_get_bool(settings, "refresh_on_active");
    
    // Reset effects if refresh is enabled and this is an update trigger
    if (refresh_on_active) {
        s->effect_progress = 0.0f;
        s->last_time = os_gettime_ns();
    }

    if (s->auto_close) {
        wrapped += "[reset]";
    }

    obs_enter_graphics();
    parse_bbcode(s, wrapped.c_str());

    // Calculate multi-line dimensions
    uint32_t max_line_w = 0, cur_line_w = 0;
    uint32_t cur_line_h = 0;
    uint32_t total_h = 0;
    for (auto &seg : s->segments) {
        if (seg.is_newline) {
            if (cur_line_w > max_line_w) max_line_w = cur_line_w;
            total_h += (cur_line_h > 0 ? cur_line_h : (uint32_t)s->font_size);
            cur_line_w = 0;
            cur_line_h = 0;
            continue;
        }
        cur_line_w += (uint32_t)seg.tex_width;
        if ((uint32_t)seg.tex_height > cur_line_h) cur_line_h = (uint32_t)seg.tex_height;
    }
    if (cur_line_w > max_line_w) max_line_w = cur_line_w;
    total_h += (cur_line_h > 0 ? cur_line_h : (uint32_t)s->font_size);

    uint32_t extra_h = 60;
    uint32_t extra_w = 0;
    for (auto &seg : s->segments) {
        if (seg.fx & FX_CURVE) {
            uint32_t curve_extra = (uint32_t)(seg.params.curve_radius * 2.0f);
            if (curve_extra > extra_h) extra_h = curve_extra;
        }
        if (seg.fx & FX_BOUNCE) {
            uint32_t bounce_extra = (uint32_t)(seg.params.bounce_amp + 10.0f);
            if (bounce_extra > extra_h) extra_h = bounce_extra;
        }
    }

    uint32_t content_w = (max_line_w > 0 ? max_line_w : 200) + extra_w;
    uint32_t content_h = total_h + extra_h;

    uint32_t final_w = content_w + (uint32_t)s->pad_left + (uint32_t)s->pad_right;
    uint32_t final_h = content_h + (uint32_t)s->pad_top  + (uint32_t)s->pad_bottom;

    s->width  = (s->custom_width  > 0) ? (uint32_t)s->custom_width  : final_w;
    s->height = (s->custom_height > 0) ? (uint32_t)s->custom_height : final_h;

    obs_leave_graphics();
}

// ---------------------------------------------------------------------------
// Tick — animation timer + glitch re-render
//
// RAINBOW FIX: Rainbow no longer re-renders textures here!
// Textures are rendered once in WHITE at parse time.
// Color tint is applied at draw time in bbcode_text_render() via
// the GPU "color" parameter — essentially free.
// ---------------------------------------------------------------------------
static void bbcode_text_tick(void *data, float seconds)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    s->elapsed += seconds;

    // NOTE: Rainbow segments NO LONGER re-render here.
    // The color is computed and applied per-frame in bbcode_text_render().

    // Re-render glitch segments: randomly swap chars to random ASCII
    bool has_glitch = false;
    for (auto &seg : s->segments)
        if (seg.fx & FX_GLITCH) { has_glitch = true; break; }

    if (has_glitch) {
        obs_enter_graphics();
        for (auto &seg : s->segments) {
            if (!(seg.fx & FX_GLITCH)) continue;
            float rate      = seg.params.glitch_rate;
            float intensity = seg.params.glitch_intensity;
            for (int i = 0; i < (int)seg.chars.size(); i++) {
                auto &c = seg.chars[i];
                float chance = rate * seconds * intensity;
                float r = (float)(rand() % 10000) / 10000.0f;

                // For glitch re-render: use white base if rainbow, else segment color
                bb_segment render_seg = seg;
                if (seg.fx & FX_RAINBOW) {
                    render_seg.color = 0xFFFFFF; // white base — tinted at draw time
                }

                if (r < chance) {
                    char glitch_ch = (char)(33 + (rand() % 94));
                    std::string gch(1, glitch_ch);
                    if (c.tex) { gs_texture_destroy(c.tex); c.tex = nullptr; }
                    c.tex = render_string_to_tex(s, gch, render_seg, c.w, c.h);
                } else if (!(seg.fx & FX_RAINBOW)) {
                    // Restore original char (rainbow chars stay white — no need to re-render)
                    std::string orig(1, seg.text[i]);
                    if (c.tex) { gs_texture_destroy(c.tex); c.tex = nullptr; }
                    c.tex = render_string_to_tex(s, orig, render_seg, c.w, c.h);
                }
            }
        }
        obs_leave_graphics();
    }

    // Re-render hacker segments: scramble unrevealed chars, restore revealed ones
    bool has_hacker = false;
    for (auto &seg : s->segments)
        if (seg.fx & FX_HACKER) { has_hacker = true; break; }

    if (has_hacker) {
        obs_enter_graphics();
        for (auto &seg : s->segments) {
            if (!(seg.fx & FX_HACKER)) continue;
            float spd   = seg.params.hacker_speed;
            float loop  = seg.params.hacker_loop;
            int   total = (int)seg.chars.size();

            // Compute reveal index
            float cycle_time = (float)total / fmaxf(spd, 0.1f);
            float anim_t = s->elapsed;
            if (loop > 0.0f) {
                float full_cycle = cycle_time + loop;
                anim_t = fmodf(s->elapsed, full_cycle);
            }
            int revealed = (int)(anim_t * spd);
            if (revealed > total) revealed = total;

            bb_segment render_seg = seg;
            if (seg.fx & FX_RAINBOW) {
                render_seg.color = 0xFFFFFF;
            }

            for (int i = 0; i < total; i++) {
                auto &c = seg.chars[i];
                if (i < revealed) {
                    // Revealed — restore the real character
                    std::string orig(1, seg.text[i]);
                    if (c.tex) { gs_texture_destroy(c.tex); c.tex = nullptr; }
                    c.tex = render_string_to_tex(s, orig, render_seg, c.w, c.h);
                } else {
                    // Not yet revealed — scramble to random ASCII
                    char scramble_ch = (char)(33 + (rand() % 94));
                    std::string sch(1, scramble_ch);
                    if (c.tex) { gs_texture_destroy(c.tex); c.tex = nullptr; }
                    c.tex = render_string_to_tex(s, sch, render_seg, c.w, c.h);
                }
            }
        }
        obs_leave_graphics();
    }
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------
static uint32_t bbcode_text_getwidth (void *data) { return ((bbcode_text_source *)data)->width;  }
static uint32_t bbcode_text_getheight(void *data) { return ((bbcode_text_source *)data)->height; }

static obs_properties_t *bbcode_text_properties(void *)
{
    obs_properties_t *p = obs_properties_create();
    obs_properties_add_text(p, "text", "BBCode Text", OBS_TEXT_MULTILINE);
    obs_properties_add_text(p, "global_tags", "Global Tags (e.g. [rainbow][wave])", OBS_TEXT_DEFAULT);
    obs_properties_add_font(p, "font", "Font");
    obs_properties_add_color(p, "default_color", "Default Color");
    obs_properties_add_bool(p, "auto_close", "Auto-Close Tags (close all open tags at end)");

    obs_property_t *aa = obs_properties_add_list(p, "antialias_mode", "Antialiasing",
                                                  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(aa, "ClearType (sharpest)", 0);
    obs_property_list_add_int(aa, "Antialiased (smooth)", 1);
    obs_property_list_add_int(aa, "None (pixel-perfect)", 2);

    obs_properties_t *layout_group = obs_properties_create();
    obs_properties_add_int(layout_group, "pad_left",   "Padding Left",   0, 500, 1);
    obs_properties_add_int(layout_group, "pad_right",  "Padding Right",  0, 500, 1);
    obs_properties_add_int(layout_group, "pad_top",    "Padding Top",    0, 500, 1);
    obs_properties_add_int(layout_group, "pad_bottom", "Padding Bottom", 0, 500, 1);
    obs_properties_add_int(layout_group, "custom_width",  "Custom Width (0=auto)",  0, 7680, 1);
    obs_properties_add_int(layout_group, "custom_height", "Custom Height (0=auto)", 0, 4320, 1);
    obs_properties_add_group(p, "layout_settings", "Layout & Sizing", OBS_GROUP_NORMAL, layout_group);

    obs_properties_add_button(p, "help_button", "BBCode Help / Tag Reference", help_button_clicked);

    obs_properties_add_text(p, "plugin_info",
        "<a href=\"https://docs.streamup.tips\">StreamUP BBCode</a> (0.1.0) made with \xe2\x9d\xa4 by "
        "<a href=\"https://streamup.tips\">StreamUP</a>",
        OBS_TEXT_INFO);

    return p;
}

// ---------------------------------------------------------------------------
// Render — per-segment/per-char GPU drawing with all effects
//
// RAINBOW: Uses a custom color_multiply effect to tint white textures
// with the per-char rainbow color on the GPU — zero GDI overhead.
// ---------------------------------------------------------------------------
static void bbcode_text_render(void *data, gs_effect_t *effect)
{
    bbcode_text_source *s = (bbcode_text_source *)data;
    if (!s || s->segments.empty()) return;

    // Default effect params
    gs_eparam_t *image_param = gs_effect_get_param_by_name(effect, "image");

    // Custom color effect params (for rainbow)
    gs_effect_t *ceff        = s->color_effect;
    gs_eparam_t *ceff_image  = ceff ? gs_effect_get_param_by_name(ceff, "image")      : nullptr;
    gs_eparam_t *ceff_tint   = ceff ? gs_effect_get_param_by_name(ceff, "tint_color")  : nullptr;

    gs_blend_state_push();
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    float t = s->elapsed;
    float x = (float)s->pad_left;
    float y = (float)s->pad_top;
    float line_h = 0.0f;
    int global_char_idx = 0;

    for (auto &seg : s->segments) {
        if (seg.is_newline) {
            if (line_h < 1.0f) line_h = (float)s->font_size;
            y += line_h;
            x = (float)s->pad_left;
            line_h = 0.0f;
            continue;
        }

        if ((float)seg.tex_height > line_h) line_h = (float)seg.tex_height;

        uint32_t fx_needing_per_char = seg.fx & ~FX_SCROLL;
        bool per_char = (fx_needing_per_char != FX_NONE) || seg.has_gradient;

        if (!per_char) {
            // Non-animated (or scroll-only): draw whole segment texture
            if (!seg.tex) { x += (float)seg.tex_width; global_char_idx += (int)seg.text.size(); continue; }

            if (seg.fx & FX_SCROLL) {
                float seg_w = (float)seg.tex_width;
                float seg_h = (float)seg.tex_height;
                float spd   = seg.params.scroll_speed;
                float scroll_offset = fmodf(t * spd, seg_w * 2.0f);

                struct gs_rect clip = {};
                clip.x  = (int)x;
                clip.y  = (int)y;
                clip.cx = (int)seg_w;
                clip.cy = (int)seg_h;
                gs_set_scissor_rect(&clip);

                float draw_x1 = x + seg_w - scroll_offset;
                float draw_x2 = draw_x1 + seg_w;

                gs_matrix_push();
                gs_matrix_translate3f(draw_x1, y, 0.0f);
                gs_effect_set_texture(image_param, seg.tex);
                gs_draw_sprite(seg.tex, 0, (uint32_t)seg.tex_width, (uint32_t)seg.tex_height);
                gs_matrix_pop();

                gs_matrix_push();
                gs_matrix_translate3f(draw_x2, y, 0.0f);
                gs_effect_set_texture(image_param, seg.tex);
                gs_draw_sprite(seg.tex, 0, (uint32_t)seg.tex_width, (uint32_t)seg.tex_height);
                gs_matrix_pop();

                gs_set_scissor_rect(nullptr);
                x += seg_w;
            } else {
                gs_matrix_push();
                gs_matrix_translate3f(x, y, 0.0f);
                gs_effect_set_texture(image_param, seg.tex);
                gs_draw_sprite(seg.tex, 0, (uint32_t)seg.tex_width, (uint32_t)seg.tex_height);
                gs_matrix_pop();
                x += (float)seg.tex_width;
            }
            global_char_idx += (int)seg.text.size();
        } else {
            // Animated: draw per-character with transforms
            for (int ci = 0; ci < (int)seg.chars.size(); ci++) {
                auto &c = seg.chars[ci];
                if (!c.tex) { x += 2.0f; global_char_idx++; continue; }

                float dx = 0.0f, dy = 0.0f;
                float scale_x = 1.0f, scale_y = 1.0f;
                float phase = (float)ci * 0.3f;

                if (seg.fx & FX_WAVE) {
                    float amp  = seg.params.wave_amp * 0.2f;
                    float freq = seg.params.wave_freq;
                    dy += sinf(t * freq + phase) * amp;
                }

                // Compute rainbow tint color for this char (if applicable)
                bool use_rainbow_effect = (seg.fx & FX_RAINBOW) && ceff && ceff_image && ceff_tint;
                struct vec4 rainbow_col;
                if (seg.fx & FX_RAINBOW) {
                    float freq  = seg.params.rainbow_freq;
                    float speed = seg.params.rainbow_speed;
                    float sat   = seg.params.rainbow_sat;
                    float val   = seg.params.rainbow_val;
                    float hue = fmodf((t * speed * 360.0f) + (float)ci * (360.0f / fmaxf(freq * 10.0f, 1.0f)), 360.0f);
                    uint32_t rgb = hsv_to_rgb(hue, sat, val);
                    float rr = ((rgb >> 16) & 0xFF) / 255.0f;
                    float rg = ((rgb >>  8) & 0xFF) / 255.0f;
                    float rb = ( rgb        & 0xFF) / 255.0f;
                    vec4_set(&rainbow_col, rr, rg, rb, 1.0f);
                }

                if (seg.fx & FX_PULSE) {
                    float freq = seg.params.pulse_freq;
                    float ease = seg.params.pulse_ease;
                    float raw_t = 0.5f + 0.5f * sinf(t * freq * (float)M_PI * 2.0f + phase);
                    float eased = (ease < 0.0f)
                        ? powf(raw_t, fabsf(ease))
                        : powf(raw_t, ease);
                    float s_val = 0.5f + 0.5f * eased;
                    scale_x *= s_val;
                    scale_y *= s_val;
                }
                if (seg.fx & FX_TORNADO) {
                    float radius = seg.params.tornado_radius;
                    float freq   = seg.params.tornado_freq;
                    dx += sinf(t * freq * (float)M_PI * 2.0f + phase) * radius;
                    dy += cosf(t * freq * (float)M_PI * 2.0f + phase) * radius;
                }
                if (seg.fx & FX_SHAKE) {
                    float rate  = seg.params.shake_rate;
                    float level = seg.params.shake_level;
                    dx += sinf(t * rate + phase * 17.0f) * level;
                    dy += cosf(t * rate * 0.83f + phase * 13.0f) * level;
                }
                if (seg.fx & FX_FADE) {
                    // Alpha baked into texture
                }
                if (seg.fx & FX_BOUNCE) {
                    float amp  = seg.params.bounce_amp;
                    float freq = seg.params.bounce_freq;
                    float raw = sinf(t * freq * (float)M_PI * 2.0f + phase);
                    dy -= fabsf(raw) * amp;
                }
                if (seg.fx & FX_TYPEWRITER) {
                    float spd  = seg.params.typewriter_speed;
                    float loop = seg.params.typewriter_loop;
                    int total  = (int)seg.chars.size();
                    float cycle_time = (float)total / fmaxf(spd, 0.1f);
                    float anim_t = t;
                    if (loop > 0.0f) {
                        float full_cycle = cycle_time + loop;
                        anim_t = fmodf(t, full_cycle);
                    }
                    int visible = (int)(anim_t * spd);
                    if (ci >= visible) {
                        x += (float)c.w;
                        global_char_idx++;
                        continue;
                    }
                }
                if (seg.fx & FX_GLITCH) {
                    // Texture swap handled in tick
                }

                bool use_curve = (seg.fx & FX_CURVE) != 0;

                if (use_curve) {
                    float radius    = seg.params.curve_radius;
                    float total_deg = seg.params.curve_angle;
                    int total       = (int)seg.chars.size();
                    float total_rad = total_deg * (float)M_PI / 180.0f;
                    float step      = (total > 1) ? total_rad / (float)(total - 1) : 0.0f;
                    float start_ang = -total_rad * 0.5f;
                    float ang       = start_ang + step * (float)ci;

                    float arc_cx = x + (float)seg.tex_width * 0.5f;
                    float arc_cy = y + radius;

                    float char_x = arc_cx + sinf(ang) * radius - (float)c.w * 0.5f;
                    float char_y = arc_cy - cosf(ang) * radius - (float)c.h * 0.5f;

                    gs_matrix_push();
                    gs_matrix_translate3f(char_x + dx, char_y + dy, 0.0f);
                    gs_matrix_translate3f((float)c.w * 0.5f, (float)c.h * 0.5f, 0.0f);
                    gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, ang);
                    gs_matrix_scale3f(scale_x, scale_y, 1.0f);
                    gs_matrix_translate3f(-(float)c.w * 0.5f, -(float)c.h * 0.5f, 0.0f);

                    if (use_rainbow_effect) {
                        gs_effect_set_texture(ceff_image, c.tex);
                        gs_effect_set_vec4(ceff_tint, &rainbow_col);
                        gs_technique_t *tech = gs_effect_get_technique(ceff, "Draw");
                        gs_technique_begin(tech);
                        gs_technique_begin_pass(tech, 0);
                        gs_draw_sprite(c.tex, 0, (uint32_t)c.w, (uint32_t)c.h);
                        gs_technique_end_pass(tech);
                        gs_technique_end(tech);
                    } else {
                        gs_effect_set_texture(image_param, c.tex);
                        gs_draw_sprite(c.tex, 0, (uint32_t)c.w, (uint32_t)c.h);
                    }
                    gs_matrix_pop();

                    global_char_idx++;
                } else {
                    float cx_center = x + (float)c.w * 0.5f;
                    float cy_center = y + (float)c.h * 0.5f;

                    gs_matrix_push();
                    gs_matrix_translate3f(cx_center + dx, cy_center + dy, 0.0f);
                    gs_matrix_scale3f(scale_x, scale_y, 1.0f);
                    gs_matrix_translate3f(-(float)c.w * 0.5f, -(float)c.h * 0.5f, 0.0f);

                    if (use_rainbow_effect) {
                        gs_effect_set_texture(ceff_image, c.tex);
                        gs_effect_set_vec4(ceff_tint, &rainbow_col);
                        gs_technique_t *tech = gs_effect_get_technique(ceff, "Draw");
                        gs_technique_begin(tech);
                        gs_technique_begin_pass(tech, 0);
                        gs_draw_sprite(c.tex, 0, (uint32_t)c.w, (uint32_t)c.h);
                        gs_technique_end_pass(tech);
                        gs_technique_end(tech);
                    } else {
                        gs_effect_set_texture(image_param, c.tex);
                        gs_draw_sprite(c.tex, 0, (uint32_t)c.w, (uint32_t)c.h);
                    }

                    gs_matrix_pop();

                    x += (float)c.w;
                    global_char_idx++;
                }
            }
            // For curve, advance x by whole segment width
            if (seg.fx & FX_CURVE)
                x += (float)seg.tex_width;
        }
    }

    gs_blend_state_pop();
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------
struct obs_source_info bbcode_text_source_info = {
    .id             = "bbcode_text_source",
    .type           = OBS_SOURCE_TYPE_INPUT,
    .output_flags   = OBS_SOURCE_VIDEO,
    .get_name       = bbcode_text_get_name,
    .create         = bbcode_text_create,
    .destroy        = bbcode_text_destroy,
    .get_width      = bbcode_text_getwidth,
    .get_height     = bbcode_text_getheight,
    .get_defaults   = bbcode_text_defaults,
    .get_properties = bbcode_text_properties,
    .update         = bbcode_text_update,
    .video_tick     = bbcode_text_tick,
    .video_render   = bbcode_text_render,
    .icon_type      = OBS_ICON_TYPE_TEXT,
};
