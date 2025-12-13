#ifndef SPEL_ENTRY
#define SPEL_ENTRY

#include "core/macros.h"
#include "core/types.h"

int main(int argc, const char** argv);

sp_hidden void spel_fallback();
sp_hidden void spel_update_fallback(double delta);
sp_hidden void spel_error_fallback(const char* msg);
sp_hidden void spel_run_fallback();

// löve style callbacks
sp_weak void spel_conf();
sp_weak void spel_load();
sp_weak void spel_update(double delta);
sp_weak void spel_error(const char* msg);
sp_weak void spel_draw();
sp_weak void spel_quit();
sp_weak void spel_run();

sp_weak void spel_low_memory();

sp_weak_alias(spel_conf, spel_fallback);
sp_weak_alias(spel_load, spel_fallback);
sp_weak_alias(spel_update, spel_update_fallback);
sp_weak_alias(spel_error, spel_error_fallback);
sp_weak_alias(spel_draw, spel_fallback);
sp_weak_alias(spel_quit, spel_fallback);
sp_weak_alias(spel_run, spel_run_fallback);
sp_weak_alias(spel_low_memory, spel_fallback);

// core functions
sp_api bool spel_args_has(const char* arg);

#endif