#ifndef SPEL_ENV_INFO
#define SPEL_ENV_INFO
#include "core/macros.h"
#include <stdint.h>

typedef struct
{
	uint32_t pid;
	uint32_t ppid;

	const char* exe_path;
	const char* working_dir;

	const char* name;

	uint32_t argc;
	const char** argv;

	uint64_t start_time_ns;
	long start_time;
} spel_runtime_process_info;

typedef struct
{
	bool debug;

	const char* display_server;
	const char* session_type;

	const char* locale;

	long timezone; // difference between utc and local, in seconds

	const char* os_name;
	const char* os_release;
	const char* arch;
	const char* distro;
	const char* distro_color; // ansi sequence. use it like "\e[<sequence>m"

	uint32_t envc;
	const char** envars;
} spel_runtime_env_info;

typedef struct
{
	char* cpu_model;
	uint32_t cpu_cores;
	uint32_t cpu_threads;

	uint64_t ram_total;
	uint64_t ram_available;

	const char* gpu_name;
	const char* gpu_driver;
	const char* gpu_api;
	const char* gpu_api_version;

	bool has_sse;
	bool has_avx;
	bool has_avx2;
	bool has_neon;
} spel_runtime_hardware_info;

typedef struct
{
	int physical_id;
	int core_id;
} spel_cpu_core_key;

sp_api void spel_runtime_info_setup();

#endif