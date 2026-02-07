#ifndef SPEL_ENTRY
#define SPEL_ENTRY
#include "core/macros.h"
#include "core/types.h"

sp_api spel_app_desc spel_app_desc_default();
sp_api int spel_app_run(spel_app_desc* app);
sp_hidden void spel_app_transform(spel_app_desc* app);

#ifdef SP_WEAK_LINK
int main(int argc, const char** argv);
#endif

sp_hidden void spel_run_frame();

#ifdef SP_WEAK_LINK
// löve style callbacks
sp_weak void spel_conf();
sp_weak void spel_load();
sp_weak void spel_update(double delta);
sp_weak void spel_draw();
sp_weak void spel_quit();
sp_weak void spel_run();

sp_weak void spel_low_memory();
#endif

#if defined(SP_WEAK_LINK) && defined(_MSC_VER)
sp_hidden void spel_fallback();
sp_hidden void spel_update_fallback(double delta);
sp_hidden void spel_run_fallback();

sp_weak_alias(spel_conf, spel_fallback);
sp_weak_alias(spel_load, spel_fallback);
sp_weak_alias(spel_update, spel_update_fallback);
sp_weak_alias(spel_draw, spel_fallback);
sp_weak_alias(spel_quit, spel_fallback);
sp_weak_alias(spel_run, spel_run_fallback);
sp_weak_alias(spel_low_memory, spel_fallback);
#endif

// core functions
sp_api bool spel_args_has(const char* arg);
sp_hidden void spel_assert_fail(const char* assertion, const char* msg, const char* file,
								int line, const char* function);

#endif