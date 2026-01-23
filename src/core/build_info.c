#include "core/build_info.h"
#include "build_info.generated.h"
#include "build_info_vcs.generated.h"
#include "core/types.h"

void spel_build_info_init()
{
	spel.build_info =
		(spel_build_info){.build_backend = sp_meson_backend,
						  .build_type = sp_build_type,
						  .compiler_args = sp_compiler_args,
						  .compiler_id = sp_compiler_id,
						  .compiler_version = sp_compiler_version,
						  .linker_args = sp_linker_args,
						  .linker_id = sp_linker_id,
						  .target_triple = sp_target_triple,
						  .vcs = (spel_build_vcs_info){.branch = sp_vcs_branch,
													   .commit = sp_vcs_commit,
													   .diff_hash = sp_git_diff_hash,
													   .dirty_count = sp_git_dirty_count,
													   .tag = sp_vcs_describe}};
}