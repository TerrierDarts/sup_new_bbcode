// =========================================================================
// SUP BBCode Text — BBCode parser (tags -> segments)
// =========================================================================
#include "bbcode-source.h"

// ---------------------------------------------------------------------------
// Parse key=value parameters from a tag string like "wave amp=50.0 freq=5.0"
// ---------------------------------------------------------------------------
std::map<std::string, std::string> parse_tag_params(const std::string &tag)
{
    std::map<std::string, std::string> result;
    std::regex param_re(R"((\w+)\s*=\s*([^\s\]]+))");
    for (auto it = std::sregex_iterator(tag.begin(), tag.end(), param_re);
         it != std::sregex_iterator(); ++it) {
        std::string key = (*it)[1].str();
        std::string val = (*it)[2].str();
        for (auto &c : key) c = (char)tolower((unsigned char)c);
        result[key] = val;
    }
    return result;
}

float param_float(const std::map<std::string, std::string> &p,
                  const std::string &key, float def)
{
    auto it = p.find(key);
    if (it == p.end()) return def;
    try { return std::stof(it->second); } catch (...) { return def; }
}

// ---------------------------------------------------------------------------
// Parse BBCode, build segments, render textures.
// Must be called INSIDE obs_enter_graphics().
// ---------------------------------------------------------------------------
void parse_bbcode(bbcode_text_source *s, const char *raw)
{
    free_segments(s);
    if (!raw || !*raw) return;

    // Replace literal newlines with [/n] markers
    std::string input(raw);
    {
        std::string tmp;
        tmp.reserve(input.size());
        for (size_t i = 0; i < input.size(); i++) {
            if (input[i] == '\r') {
                if (i + 1 < input.size() && input[i + 1] == '\n') i++;
                tmp += "[/n]";
            } else if (input[i] == '\n') {
                tmp += "[/n]";
            } else {
                tmp += input[i];
            }
        }
        input = std::move(tmp);
    }

    try {
        std::regex tag_re(
            R"(\[([/]?)(b|i|u|s|dropcap|color(?:=[^\]]+)?|font(?:=[^\]]+)?|size(?:=[^\]]+)?|opacity(?:=[^\]]+)?|bg(?:=[^\]]+)?|use(?:=[^\]]+)?|reset|(?:wave|rainbow|pulse|tornado|shake|fade|outline|shadow|gradient|bounce|typewriter|glitch|scroll|curve|hacker)(?:\s[^\]]*)?|/n|/all)\])",
            std::regex::icase);

        size_t     pos = 0;
        bb_segment style;
        style.color  = s->default_color;
        style.bold   = s->base_bold;
        style.italic = s->base_italic;

        auto reset_style = [&]() {
            style.color         = s->default_color;
            style.bold          = s->base_bold;
            style.italic        = s->base_italic;
            style.underline     = false;
            style.strikethrough = false;
            style.fx            = FX_NONE;
            style.params        = fx_params{};
            style.has_outline   = false;
            style.outline_color = 0x000000;
            style.outline_size  = 2;
            style.has_shadow    = false;
            style.shadow_color  = 0x000000;
            style.shadow_x      = 2;
            style.shadow_y      = 2;
            style.has_bg        = false;
            style.bg_color      = 0x000000;
            style.opacity       = 1.0f;
            style.font_override.clear();
            style.size_override = 0;
            style.dropcap       = false;
            style.has_gradient  = false;
            style.gradient_from = 0xFFFFFF;
            style.gradient_to   = 0x000000;
        };

        auto flush_text = [&](const std::string &txt) {
            if (txt.empty()) return;
            if (style.dropcap && txt.size() > 0) {
                bb_segment dc_seg = style;
                dc_seg.text = txt.substr(0, 1);
                dc_seg.dropcap = false;
                int base_sz = dc_seg.size_override > 0 ? dc_seg.size_override : s->font_size;
                dc_seg.size_override = base_sz * 3;
                dc_seg.bold = true;
                render_seg_textures(s, dc_seg);
                s->segments.push_back(std::move(dc_seg));
                if (txt.size() > 1) {
                    bb_segment rest = style;
                    rest.text    = txt.substr(1);
                    rest.dropcap = false;
                    render_seg_textures(s, rest);
                    s->segments.push_back(std::move(rest));
                }
            } else {
                bb_segment seg = style;
                seg.text       = txt;
                render_seg_textures(s, seg);
                s->segments.push_back(std::move(seg));
            }
        };

        auto flush_newline = [&]() {
            bb_segment seg;
            seg.is_newline = true;
            s->segments.push_back(std::move(seg));
        };

        for (auto it = std::sregex_iterator(input.begin(), input.end(), tag_re);
             it != std::sregex_iterator(); ++it)
        {
            auto &m = *it;
            size_t mpos = (size_t)m.position();
            if (mpos > pos)
                flush_text(input.substr(pos, mpos - pos));
            pos = mpos + (size_t)m.length();

            bool        closing = !m[1].str().empty();
            std::string tag     = m[2].str();
            std::string tag_low = tag;
            for (auto &c : tag_low) c = (char)tolower((unsigned char)c);

            std::string tag_base = tag_low;
            {
                auto sp = tag_base.find(' ');
                auto eq = tag_base.find('=');
                size_t cut = (sp < eq) ? sp : eq;
                if (cut != std::string::npos) tag_base = tag_base.substr(0, cut);
            }

            if (tag_base == "/n") {
                flush_newline();
            }
            else if (tag_base == "b")  style.bold   = !closing ? true : s->base_bold;
            else if (tag_base == "i")  style.italic = !closing ? true : s->base_italic;
            else if (tag_base == "color") {
                if (closing) {
                    style.color = s->default_color;
                } else {
                    auto eq = tag.find('=');
                    if (eq != std::string::npos) {
                        std::string val = tag.substr(eq + 1);
                        if (!val.empty() && val[0] == '#') {
                            val = val.substr(1);
                            try { style.color = std::stoul(val, nullptr, 16) & 0xFFFFFF; }
                            catch (...) { style.color = s->default_color; }
                        } else {
                            std::string low_val = val;
                            for (auto &c : low_val) c = (char)tolower((unsigned char)c);
                            style.color = lookup_named_color(low_val);
                        }
                    }
                }
            }
            else if (tag_base == "wave") {
                if (closing) { style.fx &= ~FX_WAVE; }
                else {
                    style.fx |= FX_WAVE;
                    auto p = parse_tag_params(tag);
                    style.params.wave_amp  = param_float(p, "amp",  50.0f);
                    style.params.wave_freq = param_float(p, "freq",  5.0f);
                }
            }
            else if (tag_base == "rainbow") {
                if (closing) { style.fx &= ~FX_RAINBOW; }
                else {
                    style.fx |= FX_RAINBOW;
                    auto p = parse_tag_params(tag);
                    style.params.rainbow_freq  = param_float(p, "freq",  1.0f);
                    style.params.rainbow_sat   = param_float(p, "sat",   0.8f);
                    style.params.rainbow_val   = param_float(p, "val",   0.8f);
                    style.params.rainbow_speed = param_float(p, "speed", 1.0f);
                }
            }
            else if (tag_base == "pulse") {
                if (closing) { style.fx &= ~FX_PULSE; }
                else {
                    style.fx |= FX_PULSE;
                    auto p = parse_tag_params(tag);
                    style.params.pulse_freq = param_float(p, "freq", 1.0f);
                    style.params.pulse_ease = param_float(p, "ease", -2.0f);
                }
            }
            else if (tag_base == "tornado") {
                if (closing) { style.fx &= ~FX_TORNADO; }
                else {
                    style.fx |= FX_TORNADO;
                    auto p = parse_tag_params(tag);
                    style.params.tornado_radius = param_float(p, "radius", 10.0f);
                    style.params.tornado_freq   = param_float(p, "freq",    1.0f);
                }
            }
            else if (tag_base == "shake") {
                if (closing) { style.fx &= ~FX_SHAKE; }
                else {
                    style.fx |= FX_SHAKE;
                    auto p = parse_tag_params(tag);
                    style.params.shake_rate  = param_float(p, "rate",  20.0f);
                    style.params.shake_level = param_float(p, "level",  5.0f);
                }
            }
            else if (tag_base == "fade") {
                if (closing) { style.fx &= ~FX_FADE; }
                else {
                    style.fx |= FX_FADE;
                    auto p = parse_tag_params(tag);
                    style.params.fade_start  = param_float(p, "start",   4.0f);
                    style.params.fade_length = param_float(p, "length", 14.0f);
                }
            }
            else if (tag_base == "outline") {
                if (closing) {
                    style.has_outline   = false;
                    style.outline_color = 0x000000;
                    style.outline_size  = 2;
                } else {
                    style.has_outline = true;
                    auto p = parse_tag_params(tag);
                    if (p.count("color")) {
                        std::string cv = p["color"];
                        if (!cv.empty() && cv[0] == '#') {
                            cv = cv.substr(1);
                            try { style.outline_color = std::stoul(cv, nullptr, 16) & 0xFFFFFF; }
                            catch (...) { style.outline_color = 0x000000; }
                        } else {
                            std::string low_cv = cv;
                            for (auto &c : low_cv) c = (char)tolower((unsigned char)c);
                            style.outline_color = lookup_named_color(low_cv);
                        }
                    }
                    style.outline_size = (int)param_float(p, "size", 2.0f);
                    if (style.outline_size < 1) style.outline_size = 1;
                    if (style.outline_size > 20) style.outline_size = 20;
                }
            }
            else if (tag_base == "u") {
                style.underline = !closing;
            }
            else if (tag_base == "s") {
                style.strikethrough = !closing;
            }
            else if (tag_base == "font") {
                if (closing) {
                    style.font_override.clear();
                } else {
                    auto eq = tag.find('=');
                    if (eq != std::string::npos) {
                        style.font_override = tag.substr(eq + 1);
                    }
                }
            }
            else if (tag_base == "size") {
                if (closing) {
                    style.size_override = 0;
                } else {
                    auto eq = tag.find('=');
                    if (eq != std::string::npos) {
                        try { style.size_override = std::stoi(tag.substr(eq + 1)); }
                        catch (...) { style.size_override = 0; }
                        if (style.size_override < 1) style.size_override = 0;
                        if (style.size_override > 500) style.size_override = 500;
                    }
                }
            }
            else if (tag_base == "shadow") {
                if (closing) {
                    style.has_shadow   = false;
                    style.shadow_color = 0x000000;
                    style.shadow_x     = 2;
                    style.shadow_y     = 2;
                } else {
                    style.has_shadow = true;
                    auto p = parse_tag_params(tag);
                    if (p.count("color")) {
                        std::string cv = p["color"];
                        if (!cv.empty() && cv[0] == '#') {
                            cv = cv.substr(1);
                            try { style.shadow_color = std::stoul(cv, nullptr, 16) & 0xFFFFFF; }
                            catch (...) { style.shadow_color = 0x000000; }
                        } else {
                            std::string low_cv = cv;
                            for (auto &c : low_cv) c = (char)tolower((unsigned char)c);
                            style.shadow_color = lookup_named_color(low_cv);
                        }
                    }
                    style.shadow_x = (int)param_float(p, "x", 2.0f);
                    style.shadow_y = (int)param_float(p, "y", 2.0f);
                }
            }
            else if (tag_base == "bg") {
                if (closing) {
                    style.has_bg   = false;
                    style.bg_color = 0x000000;
                } else {
                    style.has_bg = true;
                    auto eq = tag.find('=');
                    if (eq != std::string::npos) {
                        std::string val = tag.substr(eq + 1);
                        if (!val.empty() && val[0] == '#') {
                            val = val.substr(1);
                            try { style.bg_color = std::stoul(val, nullptr, 16) & 0xFFFFFF; }
                            catch (...) { style.bg_color = 0x000000; }
                        } else {
                            std::string low_val = val;
                            for (auto &c : low_val) c = (char)tolower((unsigned char)c);
                            style.bg_color = lookup_named_color(low_val);
                        }
                    }
                }
            }
            else if (tag_base == "opacity") {
                if (closing) {
                    style.opacity = 1.0f;
                } else {
                    auto eq = tag.find('=');
                    if (eq != std::string::npos) {
                        try { style.opacity = std::stof(tag.substr(eq + 1)); }
                        catch (...) { style.opacity = 1.0f; }
                        if (style.opacity < 0.0f) style.opacity = 0.0f;
                        if (style.opacity > 1.0f) style.opacity = 1.0f;
                    }
                }
            }
            else if (tag_base == "gradient") {
                if (closing) {
                    style.has_gradient  = false;
                    style.gradient_from = 0xFFFFFF;
                    style.gradient_to   = 0x000000;
                } else {
                    style.has_gradient = true;
                    auto p = parse_tag_params(tag);

                    auto parse_color_val = [](const std::string &val, uint32_t fallback) -> uint32_t {
                        if (val.empty()) return fallback;
                        if (val[0] == '#') {
                            std::string hex = val.substr(1);
                            try { return std::stoul(hex, nullptr, 16) & 0xFFFFFF; }
                            catch (...) { return fallback; }
                        }
                        std::string low = val;
                        for (auto &c : low) c = (char)tolower((unsigned char)c);
                        return lookup_named_color(low);
                    };

                    if (p.count("from"))
                        style.gradient_from = parse_color_val(p["from"], 0xFFFFFF);
                    if (p.count("to"))
                        style.gradient_to = parse_color_val(p["to"], 0x000000);
                }
            }
            else if (tag_base == "dropcap") {
                style.dropcap = !closing;
            }
            else if (tag_base == "bounce") {
                if (closing) { style.fx &= ~FX_BOUNCE; }
                else {
                    style.fx |= FX_BOUNCE;
                    auto p = parse_tag_params(tag);
                    style.params.bounce_amp  = param_float(p, "amp",  20.0f);
                    style.params.bounce_freq = param_float(p, "freq",  3.0f);
                }
            }
            else if (tag_base == "typewriter") {
                if (closing) { style.fx &= ~FX_TYPEWRITER; }
                else {
                    style.fx |= FX_TYPEWRITER;
                    auto p = parse_tag_params(tag);
                    style.params.typewriter_speed = param_float(p, "speed", 10.0f);
                    style.params.typewriter_loop  = param_float(p, "loop",   0.0f);
                }
            }
            else if (tag_base == "glitch") {
                if (closing) { style.fx &= ~FX_GLITCH; }
                else {
                    style.fx |= FX_GLITCH;
                    auto p = parse_tag_params(tag);
                    style.params.glitch_rate      = param_float(p, "rate",      5.0f);
                    style.params.glitch_intensity = param_float(p, "intensity", 1.0f);
                }
            }
            else if (tag_base == "scroll") {
                if (closing) { style.fx &= ~FX_SCROLL; }
                else {
                    style.fx |= FX_SCROLL;
                    auto p = parse_tag_params(tag);
                    style.params.scroll_speed = param_float(p, "speed", 50.0f);
                }
            }
            else if (tag_base == "curve") {
                if (closing) { style.fx &= ~FX_CURVE; }
                else {
                    style.fx |= FX_CURVE;
                    auto p = parse_tag_params(tag);
                    style.params.curve_radius = param_float(p, "radius", 200.0f);
                    style.params.curve_angle  = param_float(p, "angle",  180.0f);
                }
            }
            else if (tag_base == "hacker") {
                if (closing) { style.fx &= ~FX_HACKER; }
                else {
                    style.fx |= FX_HACKER;
                    auto p = parse_tag_params(tag);
                    style.params.hacker_speed = param_float(p, "speed", 5.0f);
                    style.params.hacker_loop  = param_float(p, "loop",  2.0f);
                }
            }
            // --- [use=macro_name] / [/use] ---
            else if (tag_base == "use") {
                if (closing) {
                    reset_style();
                } else {
                    auto eq = tag.find('=');
                    if (eq != std::string::npos) {
                        std::string macro_name = tag.substr(eq + 1);
                        for (auto &c : macro_name) c = (char)tolower((unsigned char)c);
                        for (auto &m : g_macros) {
                            std::string mname = m.name;
                            for (auto &c : mname) c = (char)tolower((unsigned char)c);
                            if (mname == macro_name && !m.tags.empty()) {
                                std::string mtags = m.tags;
                                std::regex mini_re(R"(\[([/]?)(b|i|u|s|dropcap|color(?:=[^\]]+)?|font(?:=[^\]]+)?|size(?:=[^\]]+)?|opacity(?:=[^\]]+)?|bg(?:=[^\]]+)?|(?:wave|rainbow|pulse|tornado|shake|fade|outline|shadow|gradient|bounce|typewriter|glitch|scroll|curve|hacker)(?:\s[^\]]*)?)\])", std::regex::icase);
                                for (auto mit = std::sregex_iterator(mtags.begin(), mtags.end(), mini_re);
                                     mit != std::sregex_iterator(); ++mit) {
                                    auto &mm = *mit;
                                    bool mc = !mm[1].str().empty();
                                    std::string mtag = mm[2].str();
                                    std::string mtag_low = mtag;
                                    for (auto &c : mtag_low) c = (char)tolower((unsigned char)c);
                                    std::string mtag_base = mtag_low;
                                    { auto sp = mtag_base.find(' '); auto meq = mtag_base.find('=');
                                      size_t cut = (sp < meq) ? sp : meq;
                                      if (cut != std::string::npos) mtag_base = mtag_base.substr(0, cut); }

                                    if (mtag_base == "b") style.bold = !mc ? true : s->base_bold;
                                    else if (mtag_base == "i") style.italic = !mc ? true : s->base_italic;
                                    else if (mtag_base == "u") style.underline = !mc ? false : false;
                                    else if (mtag_base == "s") style.strikethrough = !mc ? true : false;
                                    else if (mtag_base == "color" && !mc) {
                                        auto ceq = mtag.find('=');
                                        if (ceq != std::string::npos) {
                                            std::string val = mtag.substr(ceq + 1);
                                            if (!val.empty() && val[0] == '#') {
                                                val = val.substr(1);
                                                try { style.color = std::stoul(val, nullptr, 16) & 0xFFFFFF; } catch (...) {}
                                            } else {
                                                std::string lv = val; for (auto &c : lv) c = (char)tolower((unsigned char)c);
                                                style.color = lookup_named_color(lv);
                                            }
                                        }
                                    }
                                    else if (mtag_base == "font" && !mc) {
                                        auto feq = mtag.find('=');
                                        if (feq != std::string::npos) style.font_override = mtag.substr(feq + 1);
                                    }
                                    else if (mtag_base == "size" && !mc) {
                                        auto seq = mtag.find('=');
                                        if (seq != std::string::npos) {
                                            try { style.size_override = std::stoi(mtag.substr(seq + 1)); } catch (...) {}
                                        }
                                    }
                                    else if (mtag_base == "opacity" && !mc) {
                                        auto oeq = mtag.find('=');
                                        if (oeq != std::string::npos) {
                                            try { style.opacity = std::stof(mtag.substr(oeq + 1)); } catch (...) {}
                                        }
                                    }
                                    else if (mtag_base == "bg" && !mc) {
                                        style.has_bg = true;
                                        auto beq = mtag.find('=');
                                        if (beq != std::string::npos) {
                                            std::string val = mtag.substr(beq + 1);
                                            if (!val.empty() && val[0] == '#') {
                                                val = val.substr(1);
                                                try { style.bg_color = std::stoul(val, nullptr, 16) & 0xFFFFFF; } catch (...) {}
                                            } else {
                                                std::string lv = val; for (auto &c : lv) c = (char)tolower((unsigned char)c);
                                                style.bg_color = lookup_named_color(lv);
                                            }
                                        }
                                    }
                                    else if (mtag_base == "outline" && !mc) {
                                        style.has_outline = true;
                                        auto mp = parse_tag_params(mtag);
                                        if (mp.count("color")) {
                                            std::string cv = mp["color"];
                                            if (!cv.empty() && cv[0] == '#') { cv = cv.substr(1);
                                                try { style.outline_color = std::stoul(cv, nullptr, 16) & 0xFFFFFF; } catch (...) {}
                                            } else { std::string lv = cv; for (auto &c : lv) c = (char)tolower((unsigned char)c); style.outline_color = lookup_named_color(lv); }
                                        }
                                        style.outline_size = (int)param_float(mp, "size", 2.0f);
                                    }
                                    else if (mtag_base == "shadow" && !mc) {
                                        style.has_shadow = true;
                                        auto mp = parse_tag_params(mtag);
                                        if (mp.count("color")) {
                                            std::string cv = mp["color"];
                                            if (!cv.empty() && cv[0] == '#') { cv = cv.substr(1);
                                                try { style.shadow_color = std::stoul(cv, nullptr, 16) & 0xFFFFFF; } catch (...) {}
                                            } else { std::string lv = cv; for (auto &c : lv) c = (char)tolower((unsigned char)c); style.shadow_color = lookup_named_color(lv); }
                                        }
                                        style.shadow_x = (int)param_float(mp, "x", 2.0f);
                                        style.shadow_y = (int)param_float(mp, "y", 2.0f);
                                    }
                                    else if (mtag_base == "wave" && !mc) { style.fx |= FX_WAVE; auto mp = parse_tag_params(mtag); style.params.wave_amp = param_float(mp,"amp",50.0f); style.params.wave_freq = param_float(mp,"freq",5.0f); }
                                    else if (mtag_base == "rainbow" && !mc) { style.fx |= FX_RAINBOW; auto mp = parse_tag_params(mtag); style.params.rainbow_freq = param_float(mp,"freq",1.0f); style.params.rainbow_sat = param_float(mp,"sat",0.8f); style.params.rainbow_val = param_float(mp,"val",0.8f); style.params.rainbow_speed = param_float(mp,"speed",1.0f); }
                                    else if (mtag_base == "pulse" && !mc) { style.fx |= FX_PULSE; auto mp = parse_tag_params(mtag); style.params.pulse_freq = param_float(mp,"freq",1.0f); style.params.pulse_ease = param_float(mp,"ease",-2.0f); }
                                    else if (mtag_base == "tornado" && !mc) { style.fx |= FX_TORNADO; auto mp = parse_tag_params(mtag); style.params.tornado_radius = param_float(mp,"radius",10.0f); style.params.tornado_freq = param_float(mp,"freq",1.0f); }
                                    else if (mtag_base == "shake" && !mc) { style.fx |= FX_SHAKE; auto mp = parse_tag_params(mtag); style.params.shake_rate = param_float(mp,"rate",20.0f); style.params.shake_level = param_float(mp,"level",5.0f); }
                                    else if (mtag_base == "fade" && !mc) { style.fx |= FX_FADE; auto mp = parse_tag_params(mtag); style.params.fade_start = param_float(mp,"start",4.0f); style.params.fade_length = param_float(mp,"length",14.0f); }
                                    else if (mtag_base == "bounce" && !mc) { style.fx |= FX_BOUNCE; auto mp = parse_tag_params(mtag); style.params.bounce_amp = param_float(mp,"amp",20.0f); style.params.bounce_freq = param_float(mp,"freq",3.0f); }
                                    else if (mtag_base == "scroll" && !mc) { style.fx |= FX_SCROLL; auto mp = parse_tag_params(mtag); style.params.scroll_speed = param_float(mp,"speed",50.0f); }
                                    else if (mtag_base == "curve" && !mc) { style.fx |= FX_CURVE; auto mp = parse_tag_params(mtag); style.params.curve_radius = param_float(mp,"radius",200.0f); style.params.curve_angle = param_float(mp,"angle",180.0f); }
                                    else if (mtag_base == "hacker" && !mc) { style.fx |= FX_HACKER; auto mp = parse_tag_params(mtag); style.params.hacker_speed = param_float(mp,"speed",5.0f); style.params.hacker_loop = param_float(mp,"loop",2.0f); }
                                    else if (mtag_base == "dropcap" && !mc) { style.dropcap = true; }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            // --- [reset] or [/all] ---
            else if (tag_base == "reset" || tag_base == "/all") {
                reset_style();
            }
        }
        if (pos < input.size())
            flush_text(input.substr(pos));
    } catch (...) {}
}
