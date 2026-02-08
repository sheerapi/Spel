#ifndef SPEL_BUILD_INFO
#define SPEL_BUILD_INFO
#include "core/macros.h"
#	include <stddef.h>

typedef struct
{
	const char* tag;
	const char* commit;
	const char* branch;
	const char* diff_hash;
	size_t dirty_count;
} spel_build_vcs_info;

typedef struct
{
	const char* compiler_id;
	const char* compiler_version;
	const char* compiler_args;

	const char* linker_id;
	const char* linker_args;

	const char* target_triple;
	const char* build_type;
	const char* build_backend;

	spel_build_vcs_info vcs;
} spel_build_info;

sp_hidden void spel_build_info_init();

#endif