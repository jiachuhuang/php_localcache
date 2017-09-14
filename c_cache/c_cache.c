#include <stdio.h>

#include "c_cache.h"
#include "c_storage.h"
#include "c_shared_allocator.h"

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

extern c_shared_header *shared_header;

int c_cache_get(char *key, unsigned int len, void **data, unsigned int *size) {
	time_t tv;
	tv = time((time_t *)NULL);

	return c_storage_find(key, len, data, size, tv);
}

int c_cache_set(char *key, unsigned int len, void *data, unsigned int size, unsigned int ttl) {
	time_t tv;
	tv = time((time_t *)NULL);

	if(ttl <= 0) {
		ttl = (unsigned int)tv + 2 * 60 * 60;
	} else {
		ttl += tv;
	}

	return c_storage_update(key, len, data, size, ttl, 0, tv);
}

int c_cache_add(char *key, unsigned int len, void *data, unsigned int size, unsigned int ttl) {
	time_t tv;
	tv = time((time_t *)NULL);

	if(ttl <= 0) {
		ttl = (unsigned int)tv + 2 * 60 * 60;
	} else {
		ttl += tv;
	}

	return c_storage_update(key, len, data, size, ttl, 1, tv);
}

void c_cache_flush() {
	c_storage_flush();
}

int c_cache_delete(char *key, unsigned int len) {
	return c_storage_delete(key, len);
}

int main(int argc, char const *argv[]) {
	char *error;
	unsigned int ks = 256*1024*1024;
	unsigned int vs = 1024*1024*1024;

	c_storage_startup("/tmp/shartest.tmp1", ks, vs, &error);
	
	c_storage_shutdown();
	return 0;
}



















