#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static uint64_t hash_str(const char* s)
{
	uint64_t h = 1469598103934665603ULL;
	while (*s)
	{
		h ^= (unsigned char)*s++;
		h *= 1099511628211ULL;
	}
	return h;
}

int main(int argc, const char** argv)
{
	for (size_t i = 1; i < argc; i++)
    {
        printf("%s -> 0x%lx\n", argv[i], hash_str(argv[i]));
	}

	return 0;
}