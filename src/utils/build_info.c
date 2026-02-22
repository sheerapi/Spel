#include "utils/build_info.h"
#include "build_info.generated.h"
#include "build_info_vcs.generated.h"
#include "core/types.h"

spel_hidden void spel_build_info_init()
{
	spel.build_info =
		(spel_build_info){.build_backend = spel_meson_backend,
						  .build_type = spel_build_type,
						  .compiler_args = spel_compiler_args,
						  .compiler_id = spel_compiler_id,
						  .compiler_version = spel_compiler_version,
						  .linker_args = spel_linker_args,
						  .linker_id = spel_linker_id,
						  .target_triple = spel_target_triple,
						  .vcs = (spel_build_vcs_info){.branch = spel_vcs_branch,
													   .commit = spel_vcs_commit,
													   .diff_hash = spel_git_diff_hash,
													   .dirty_count = spel_git_dirty_count,
													   .tag = spel_vcs_describe}};
}