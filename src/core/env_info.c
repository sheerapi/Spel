#define _DEFAULT_SOURCE 1
#include "core/env_info.h"
#include "core/types.h"
#include "utils/path.h"
#include "utils/time.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

extern char** environ;

char* get_os_release_value(const char* key)
{
	FILE* file = fopen("/etc/os-release", "r");
	if (file == NULL)
	{
		file = fopen("/usr/lib/os-release", "r");
		if (file == NULL)
		{
			perror("Error opening os-release file");
			return nullptr;
		}
	}

	char line[256];
	while (fgets(line, sizeof(line), file))
	{
		line[strcspn(line, "\n")] = 0;

		char* equals_sign = strchr(line, '=');
		if (equals_sign != NULL)
		{
			*equals_sign = '\0';
			char* current_key = line;
			char* value = equals_sign + 1;

			if (value[0] == '"' || value[0] == '\'')
			{
				int len = (int)strlen(value);
				if (len >= 2 && value[len - 1] == value[0])
				{
					value[len - 1] = '\0';
					value++;
				}
			}

			if (strcmp(current_key, key) == 0)
			{
				fclose(file);
				return strdup(value);
			}
		}
	}

	fclose(file);
	return nullptr;
}

void spel_fill_process_info(spel_runtime_process_info* out)
{
	static char exe_path[PATH_MAX];
	static char cwd[PATH_MAX];

	out->pid = (uint32_t)getpid();
	out->ppid = (uint32_t)getppid();

	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (len > 0)
	{
		exe_path[len] = '\0';
		out->exe_path = exe_path;
	}
	else
	{
		out->exe_path = "<unknown>";
	}

	if (getcwd(cwd, sizeof(cwd)))
	{
		out->working_dir = cwd;
	}
	else
	{
		out->working_dir = "<unknown>";
	}
}

static const char* detect_display_server(void)
{
	if (getenv("WAYLAND_DISPLAY"))
	{
		return "wayland";
	}
	if (getenv("DISPLAY"))
	{
		return "x11";
	}
	if (getenv("TERM"))
	{
		return "tty";
	}
	return "unknown";
}

static const char* detect_session_type(void)
{
	if (getenv("SSH_CONNECTION"))
	{
		return "remote";
	}
	if (getenv("WAYLAND_DISPLAY") || getenv("DISPLAY"))
	{
		return "graphical";
	}
	return "headless";
}

void spel_fill_env_info(spel_runtime_env_info* out)
{
	static struct utsname uts;
	if (uname(&uts) == 0)
	{
		out->os_name = uts.sysname;
		out->arch = uts.machine;
		out->os_release = uts.release;
	}

	out->distro = get_os_release_value("PRETTY_NAME");
	if (!out->distro)
	{
		out->distro = get_os_release_value("NAME");
		if (!out->distro)
		{
			out->distro = "<none>";
		}
	}

	out->distro_color = get_os_release_value("ANSI_COLOR");
	if (!out->distro)
	{
		out->distro = "0";
	}

	size_t count = 0;
	while (environ[count])
	{
		count++;
	}

	out->envc = count;
	out->envars = (const char**)environ;

	out->display_server = detect_display_server();
	out->session_type = detect_session_type();

	out->locale = getenv("LANG") ?: "<unset>";

	time_t t = spel_time_now_sec();

	struct tm local;

	localtime_r(&t, &local);
	localtime_r(&t, &local);

	out->timezone = local.tm_gmtoff;
}

static void read_cpu_model(char* out, size_t outSize)
{
	FILE* f = fopen("/proc/cpuinfo", "r");
	if (!f)
	{
		strncpy(out, "<unknown>", outSize);
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), f))
	{
		if (strncmp(line, "model name", 10) == 0)
		{
			char* colon = strchr(line, ':');
			if (colon)
			{
				strncpy(out, colon + 2, outSize);
				out[strcspn(out, "\n")] = 0;
				break;
			}
		}
	}

	fclose(f);
}

static void read_meminfo(uint64_t* total, uint64_t* available)
{
	FILE* f = fopen("/proc/meminfo", "r");
	if (!f)
	{
		return;
	}

	char key[64];
	uint64_t value;
	char unit[16];

	while (fscanf(f, "%63s %lu %15s\n", key, &value, unit) == 3)
	{
		if (strcmp(key, "MemTotal:") == 0)
		{
			*total = value * 1024;
		}
		else if (strcmp(key, "MemAvailable:") == 0)
		{
			*available = value * 1024;
		}
	}

	fclose(f);
}

static int core_key_exists(spel_cpu_core_key* keys, size_t count, int phys, int core)
{
	for (size_t i = 0; i < count; i++)
	{
		if (keys[i].physical_id == phys && keys[i].core_id == core)
		{
			return 1;
		}
	}
	return 0;
}

uint32_t spel_detect_physical_cores()
{
	FILE* f = fopen("/proc/cpuinfo", "r");
	if (!f)
	{
		return 0;
	}

	spel_cpu_core_key keys[256];
	size_t key_count = 0;

	int physical_id = -1;
	int core_id = -1;

	char line[256];

	while (fgets(line, sizeof(line), f))
	{
		if (line[0] == '\n')
		{
			if (physical_id >= 0 && core_id >= 0)
			{
				if (!core_key_exists(keys, key_count, physical_id, core_id))
				{
					keys[key_count++] = (spel_cpu_core_key){.physical_id = physical_id,
															.core_id = core_id};
				}
			}

			physical_id = -1;
			core_id = -1;
			continue;
		}

		if (sscanf(line, "physical id : %d", &physical_id) == 1)
		{
			continue;
		}

		if (sscanf(line, "core id : %d", &core_id) == 1)
		{
			continue;
		}
	}

	fclose(f);

	return key_count > 0 ? (uint32_t)key_count : 0;
}

void spel_runtime_info_setup()
{
	spel.process.start_time = spel_time_now_sec();
	spel.process.start_time_ns = spel_time_now_ns();

	spel_fill_env_info(&spel.env);
	spel_fill_process_info(&spel.process);

	spel.hardware.cpu_model = spel_memory_malloc(128, SPEL_MEM_TAG_CORE);
	spel.hardware.cpu_cores = spel_detect_physical_cores();
	read_cpu_model(spel.hardware.cpu_model, 128);

	spel.hardware.cpu_threads = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
	read_meminfo(&spel.hardware.ram_total, &spel.hardware.ram_available);

#if defined(__x86_64__) || defined(__i386__)
	spel.hardware.has_sse = __builtin_cpu_supports("sse");
	spel.hardware.has_avx2 = __builtin_cpu_supports("avx2");
	spel.hardware.has_avx = __builtin_cpu_supports("avx");
#elif defined(__aarch64__) || defined(__arm__)
	unsigned long hwcap = getauxval(AT_HWCAP);
	spel.hardware.has_neon = hwcap & HWCAP_NEON;
#endif
}