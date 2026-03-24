#include <obs-module.h>
#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("sup-bbcode", "en-US")

extern struct obs_source_info bbcode_text_source_info;

// Defined in bbcode-source.cpp
extern void load_global_macros();
extern void show_macro_editor(void *);

bool obs_module_load(void)
{
    obs_register_source(&bbcode_text_source_info);
    load_global_macros();
#ifdef _WIN32
    obs_frontend_add_tools_menu_item("StreamUP - BBCode Macros", show_macro_editor, nullptr);
#endif
    return true;
}

const char *obs_module_name(void)
{
    return "SUP BBCode Text";
}

const char *obs_module_description(void)
{
    return "A simple BBCode-enabled text source for OBS";
}

void obs_module_unload(void)
{
}
