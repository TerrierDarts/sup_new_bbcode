#pragma once
// =========================================================================
// SUP BBCode Text — Shared header
// =========================================================================

#include <obs-module.h>
#include <graphics/graphics.h>
#include <util/platform.h>
#include <callback/proc.h>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =========================================================================
// Animation effects (bit flags — combinable)
// =========================================================================
static const uint32_t FX_NONE       = 0;
static const uint32_t FX_WAVE       = 1 << 0;
static const uint32_t FX_RAINBOW    = 1 << 1;
static const uint32_t FX_PULSE      = 1 << 2;
static const uint32_t FX_TORNADO    = 1 << 3;
static const uint32_t FX_SHAKE      = 1 << 4;
static const uint32_t FX_FADE       = 1 << 5;
static const uint32_t FX_BOUNCE     = 1 << 6;
static const uint32_t FX_TYPEWRITER = 1 << 7;
static const uint32_t FX_GLITCH     = 1 << 8;
static const uint32_t FX_SCROLL     = 1 << 9;
static const uint32_t FX_CURVE      = 1 << 10;
static const uint32_t FX_HACKER     = 1 << 11;

// =========================================================================
// Per-effect parameters (Godot-style)
// =========================================================================
struct fx_params {
    // Wave
    float wave_amp   = 50.0f;
    float wave_freq  = 5.0f;

    // Rainbow
    float rainbow_freq  = 1.0f;
    float rainbow_sat   = 0.8f;
    float rainbow_val   = 0.8f;
    float rainbow_speed = 1.0f;

    // Pulse
    float pulse_freq = 1.0f;
    float pulse_ease = -2.0f;

    // Tornado
    float tornado_radius = 10.0f;
    float tornado_freq   = 1.0f;

    // Shake
    float shake_rate  = 20.0f;
    float shake_level = 5.0f;

    // Fade
    float fade_start  = 4.0f;
    float fade_length = 14.0f;

    // Bounce
    float bounce_amp  = 20.0f;
    float bounce_freq = 3.0f;

    // Typewriter
    float typewriter_speed = 10.0f;
    float typewriter_loop  = 0.0f;

    // Glitch
    float glitch_rate      = 5.0f;
    float glitch_intensity = 1.0f;

    // Scroll
    float scroll_speed = 50.0f;

    // Curve
    float curve_radius = 200.0f;
    float curve_angle  = 180.0f;

    // Hacker
    float hacker_speed = 5.0f;
    float hacker_loop  = 2.0f;
};

// =========================================================================
// Data types
// =========================================================================

struct bb_char {
    gs_texture_t *tex   = nullptr;
    int           w     = 0;
    int           h     = 0;
    int           index = 0;
};

struct bb_segment {
    std::string          text;
    uint32_t             color         = 0xFFFFFF;
    bool                 bold          = false;
    bool                 italic        = false;
    bool                 underline     = false;
    bool                 strikethrough = false;
    uint32_t             fx            = FX_NONE;
    fx_params            params;
    bool                 is_newline    = false;
    bool                 has_outline   = false;
    uint32_t             outline_color = 0x000000;
    int                  outline_size  = 2;
    bool                 has_shadow    = false;
    uint32_t             shadow_color  = 0x000000;
    int                  shadow_x      = 2;
    int                  shadow_y      = 2;
    bool                 has_bg        = false;
    uint32_t             bg_color      = 0x000000;
    float                opacity       = 1.0f;
    std::string          font_override;
    int                  size_override = 0;
    bool                 dropcap       = false;
    bool                 has_gradient  = false;
    uint32_t             gradient_from = 0xFFFFFF;
    uint32_t             gradient_to   = 0x000000;
    std::vector<bb_char> chars;
    gs_texture_t        *tex           = nullptr;
    int                  tex_width     = 0;
    int                  tex_height    = 0;
};

struct bbcode_text_source {
    obs_source_t            *source        = nullptr;
    char                    *raw           = nullptr;
    std::vector<bb_segment>  segments;
    uint32_t                 width         = 1;
    uint32_t                 height        = 1;
    float                    elapsed       = 0.0f;

    gs_effect_t             *color_effect  = nullptr;

    char                    *font_face     = nullptr;
    char                    *global_tags   = nullptr;
    int                      font_size     = 48;
    uint32_t                 default_color = 0xFFFFFF;
    bool                     base_bold     = false;
    bool                     base_italic   = false;

    int                      antialias_mode = 0; // 0=ClearType, 1=AA, 2=None

    int                      pad_left      = 0;
    int                      pad_right     = 0;
    int                      pad_top       = 15;
    int                      pad_bottom    = 0;
    int                      custom_width  = 0;
    int                      custom_height = 0;

    bool                     auto_close    = false;

    // Animation progress
    float                    effect_progress = 0.0f;
    uint64_t                 last_time      = 0;
};

// =========================================================================
// Global macro system
// =========================================================================
struct macro_def {
    std::string name;
    std::string tags;
};
extern std::vector<macro_def> g_macros;
extern const int MAX_MACROS;

void load_global_macros();
void save_global_macros();
void show_macro_editor(void *);

// =========================================================================
// Shared utility functions
// =========================================================================
uint32_t lookup_named_color(const std::string &name);
uint32_t hsv_to_rgb(float h, float s_val, float v);

// =========================================================================
// Renderer
// =========================================================================
gs_texture_t *render_string_to_tex(bbcode_text_source *s, const std::string &str,
                                    const bb_segment &seg,
                                    int &out_w, int &out_h,
                                    float alpha_mult = 1.0f);
void render_seg_textures(bbcode_text_source *s, bb_segment &seg);
void free_segments(bbcode_text_source *s);

// =========================================================================
// Parser helpers
// =========================================================================
std::map<std::string, std::string> parse_tag_params(const std::string &tag);
float param_float(const std::map<std::string, std::string> &p,
                  const std::string &key, float def);
void parse_bbcode(bbcode_text_source *s, const char *input);

// =========================================================================
// Help popup
// =========================================================================
bool help_button_clicked(obs_properties_t *, obs_property_t *, void *);

// =========================================================================
// StreamUP palette (Win32)
// =========================================================================
#ifdef _WIN32
#define SUP_BG_DARKEST    RGB(0x09, 0x09, 0x09)
#define SUP_BG_PRIMARY    RGB(0x16, 0x16, 0x17)
#define SUP_BG_SECONDARY  RGB(0x11, 0x11, 0x11)
#define SUP_BG_TERTIARY   RGB(0x2C, 0x2C, 0x2E)
#define SUP_ACCENT        RGB(0x00, 0x76, 0xDF)
#define SUP_ACCENT_HOVER  RGB(0x00, 0x71, 0xE3)
#define SUP_TEXT_PRIMARY   RGB(0xFF, 0xFF, 0xFF)
#define SUP_TEXT_SECONDARY RGB(0xC7, 0xC7, 0xCC)
#define SUP_TEXT_MUTED     RGB(0x8E, 0x8E, 0x93)
#define SUP_BORDER         RGB(0x30, 0x30, 0x34)
#endif
