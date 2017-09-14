
#ifndef C_CACHE_H
#define C_CACHE_H

#define C_CACHE_FAIL   						0
#define C_CACHE_OK							1

#define C_CACHE_NOTFOUND					0
#define C_CACHE_FOUND						1


int c_cache_get(char *key, unsigned int len, void **data, unsigned int *size);
int c_cache_set(char *key, unsigned int len, void *data, unsigned int size, unsigned int ttl);
// int c_cache_add();
int c_cache_delete();
void c_cache_flush();

#endif /* C_CACHE_H */