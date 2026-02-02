#ifndef SPEL_PATH
#define SPEL_PATH
#include <stdbool.h>
#include <stddef.h>
#ifdef _WIN32
#	define PATH_SEP '\\'
#	define PATH_SEP_STR "\\"
#else
#	define PATH_SEP '/'
#	define PATH_SEP_STR "/"
#endif

bool spel_path_exists(const char* path);

bool spel_path_is_dir(const char* path);

bool spel_path_is_file(const char* path);

const char* spel_path_filename(const char* path);

const char* spel_path_extension(const char* path);

char* spel_path_stem(const char* path, char* buf, size_t bufsize);

char* spel_path_dirname(const char* path, char* buf, size_t bufsize);

char* spel_path_join(const char* base, const char* rel, char* buf, size_t bufsize);

char* spel_path_normalize(const char* path, char* buf, size_t bufsize);

bool spel_path_is_absolute(const char* path);

char* spel_path_absolute(const char* path, char* buf, size_t bufsize);
#endif