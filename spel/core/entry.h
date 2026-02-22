#ifndef SPEL_ENTRY
#define SPEL_ENTRY
#include "core/macros.h"
#include "core/types.h"

spel_api spel_app_desc spel_app_desc_default();
spel_api int spel_app_run(spel_app_desc* app);
spel_hidden void spel_app_transform(spel_app_desc* app);

#ifdef SP_WEAK_LINK
int main(int argc, const char** argv);
#endif

spel_hidden void spel_run_frame();

#if defined(SP_WEAK_LINK) && (defined(__GNUC__) || defined(__clang__))
// löve style callbacks
spel_weak void spel_conf();
spel_weak void spel_load();
spel_weak void spel_update(double delta);
spel_weak void spel_draw();
spel_weak void spel_quit();
spel_weak void spel_run();

spel_weak void spel_low_memory();
#endif

#if defined(SP_WEAK_LINK) && defined(_MSC_VER)
spel_hidden void spel_fallback();
spel_hidden void spel_update_fallback(double delta);
spel_hidden void spel_run_fallback();

spel_weak_alias(spel_conf, spel_fallback);
spel_weak_alias(spel_load, spel_fallback);
spel_weak_alias(spel_update, spel_update_fallback);
spel_weak_alias(spel_draw, spel_fallback);
spel_weak_alias(spel_quit, spel_fallback);
spel_weak_alias(spel_run, spel_run_fallback);
spel_weak_alias(spel_low_memory, spel_fallback);
#endif

// core functions
spel_api bool spel_args_has(const char* arg);
spel_hidden void spel_assert_fail(const char* assertion, const char* msg, const char* file,
								int line, const char* function);

#endif