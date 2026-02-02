#include "utils/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#	include <direct.h>
#	define getcwd _getcwd
#else
#	include <unistd.h>
#endif

bool spel_path_exists(const char* path)
{
	struct stat st;
	return stat(path, &st) == 0;
}

bool spel_path_is_dir(const char* path)
{
	struct stat st;
	if (stat(path, &st) != 0)
	{
		return false;
	}
	return S_ISDIR(st.st_mode);
}

bool spel_path_is_file(const char* path)
{
	struct stat st;
	if (stat(path, &st) != 0)
	{
		return false;
	}
	return S_ISREG(st.st_mode);
}

const char* spel_path_filename(const char* path)
{
	if (!path)
	{
		return NULL;
	}

	const char* last_sep = strrchr(path, PATH_SEP);
#ifdef _WIN32
	/* On Windows, also check for forward slash */
	const char* last_fwd = strrchr(path, '/');
	if (last_fwd && (!last_sep || last_fwd > last_sep))
	{
		last_sep = last_fwd;
	}
#endif

	return last_sep ? last_sep + 1 : path;
}

const char* spel_path_extension(const char* path)
{
	const char* filename = spel_path_filename(path);
	if (!filename)
	{
		return NULL;
	}

	const char* dot = strrchr(filename, '.');
	/* Don't count leading dot (hidden files on Unix) */
	if (dot && dot != filename)
	{
		return dot;
	}
	return NULL;
}

char* spel_path_stem(const char* path, char* buf, size_t bufsize)
{
	if (!path || !buf || bufsize == 0)
	{
		return NULL;
	}

	const char* filename = spel_path_filename(path);
	const char* ext = spel_path_extension(path);

	size_t len;
	if (ext)
	{
		len = ext - filename;
	}
	else
	{
		len = strlen(filename);
	}

	if (len >= bufsize)
	{
		len = bufsize - 1;
	}
	memcpy(buf, filename, len);
	buf[len] = '\0';

	return buf;
}

char* spel_path_dirname(const char* path, char* buf, size_t bufsize)
{
	if (!path || !buf || bufsize == 0)
	{
		return NULL;
	}

	const char* last_sep = strrchr(path, PATH_SEP);
#ifdef _WIN32
	const char* last_fwd = strrchr(path, '/');
	if (last_fwd && (!last_sep || last_fwd > last_sep))
	{
		last_sep = last_fwd;
	}
#endif

	if (!last_sep)
	{
		snprintf(buf, bufsize, ".");
	}
	else if (last_sep == path)
	{
		snprintf(buf, bufsize, PATH_SEP_STR);
	}
	else
	{
		size_t len = last_sep - path;
		if (len >= bufsize)
		{
			len = bufsize - 1;
		}
		memcpy(buf, path, len);
		buf[len] = '\0';
	}

	return buf;
}

char* spel_path_join(const char* base, const char* rel, char* buf, size_t bufsize)
{
	if (!base || !rel || !buf || bufsize == 0)
	{
		return NULL;
	}

	size_t base_len = strlen(base);

	/* Check if relative path is actually absolute */
	if (spel_path_is_absolute(rel))
	{
		snprintf(buf, bufsize, "%s", rel);
		return buf;
	}

	/* Check if base already ends with separator */
	bool has_sep = (base_len > 0 &&
					(base[base_len - 1] == PATH_SEP || base[base_len - 1] == '/')) != 0;

	if (has_sep)
	{
		snprintf(buf, bufsize, "%s%s", base, rel);
	}
	else
	{
		snprintf(buf, bufsize, "%s%s%s", base, PATH_SEP_STR, rel);
	}

	return buf;
}

char* spel_path_normalize(const char* path, char* buf, size_t bufsize)
{
	if (!path || !buf || bufsize == 0)
	{
		return NULL;
	}

	char temp[4096];
	strncpy(temp, path, sizeof(temp) - 1);
	temp[sizeof(temp) - 1] = '\0';

	char* parts[256];
	int part_count = 0;

	bool is_abs = spel_path_is_absolute(path);

	char* token = strtok(temp, "/\\");
	while (token && part_count < 256)
	{
		if (strcmp(token, ".") == 0)
		{
			/* Skip current directory */
		}
		else if (strcmp(token, "..") == 0)
		{
			/* Go up one level */
			if (part_count > 0)
			{
				part_count--;
			}
		}
		else
		{
			parts[part_count++] = token;
		}
		token = strtok(NULL, "/\\");
	}

	/* Reconstruct path */
	buf[0] = '\0';
	if (is_abs)
	{
		strncat(buf, PATH_SEP_STR, bufsize - 1);
	}

	for (int i = 0; i < part_count; i++)
	{
		if (i > 0 || (int)is_abs)
		{
			strncat(buf, PATH_SEP_STR, bufsize - 1);
		}
		strncat(buf, parts[i], bufsize - 1);
	}

	if (buf[0] == '\0')
	{
		snprintf(buf, bufsize, ".");
	}

	return buf;
}

bool spel_path_is_absolute(const char* path)
{
	if (!path || path[0] == '\0')
	{
		return false;
	}

#ifdef _WIN32
	/* Windows: check for drive letter or UNC path */
	if ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'))
	{
		if (path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
		{
			return true;
		}
	}
	if (path[0] == '\\' && path[1] == '\\')
	{
		return true; /* UNC path */
	}
	return false;
#else
	return path[0] == '/';
#endif
}

char* spel_path_absolute(const char* path, char* buf, size_t bufsize)
{
	if (!path || !buf || bufsize == 0)
	{
		return NULL;
	}

	if (spel_path_is_absolute(path))
	{
		strncpy(buf, path, bufsize - 1);
		buf[bufsize - 1] = '\0';
		return buf;
	}

	char cwd[4096];
	if (!getcwd(cwd, sizeof(cwd)))
	{
		return NULL;
	}

	char joined[4096];
	spel_path_join(cwd, path, joined, sizeof(joined));
	spel_path_normalize(joined, buf, bufsize);

	return buf;
}