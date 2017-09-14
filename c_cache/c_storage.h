#include <pthread.h>
#include <stdlib.h>

#ifndef C_STORAGE_H
#define C_STORAGE_H

#define C_STORAGE_MAX_KEY_LEN (48)
#define C_STORAGE_MIN_SEGMENT_SIZE (8*1024*1024)
#define C_STORAGE_MAX_SEGMENT_SIZE (128*1024*1024)

#define C_STORAGE_MISS(x) (++((x)->miss))
#define C_STORAGE_HITS(x) (++((x)->hits))
#define C_STORAGE_KEY_NUM(x) ((x)->k_num)
#define C_STORAGE_SEGMENT_NUM(x) ((x)->segment_num)
#define C_STORAGE_SEGMENT_SIZE(x) ((x)->segment_size)

#define C_STORAGE_CRC_THRESHOLD 256

#define USE_MALLOC malloc
#define USE_FREE free

typedef struct {
	unsigned int init;
	pthread_rwlock_t rlock;
	pthread_rwlock_t wlock;
	unsigned long k_size;
	unsigned long v_size;
	unsigned long alloc_size;
	unsigned int k_num;
	unsigned int segment_num;
	unsigned int segment_size;
	unsigned int miss; 
	unsigned int fails; 
	unsigned long hits; 
	unsigned long k_offset;
	unsigned long v_offset;
} c_shared_header;

typedef struct {
	unsigned int pos;
	unsigned int size;
} c_shared_segment_header;

typedef struct {
	c_shared_segment_header *seg_header;
	void *p;
} c_shared_segment;

typedef struct {
	unsigned long offset;
	unsigned int segment_index;
	unsigned int len;
	unsigned long atime;
} c_kv_val;

typedef struct {
	unsigned long h;
	unsigned long crc;
	unsigned long ttl;
	unsigned int len;
	/* unsigned int flag; */
	unsigned char key[C_STORAGE_MAX_KEY_LEN];
	c_kv_val val;
} c_kv_key;

int c_storage_startup(char *shm_filename, unsigned int k_size, unsigned int v_size, char **msg);
void c_storage_shutdown();
int c_storage_find(char *key, unsigned int len, void **data, unsigned int *size,/* unsigned int *flag,*/ unsigned long tv);
int c_storage_update(char *key, unsigned int len, void *data, unsigned int size, unsigned int ttl,/* unsigned int *flag,*/ unsigned int add, unsigned long tv);
int c_storage_delete(char *key, unsigned int len);
void c_storage_flush();
/* int c_storage_get_info(); */
/* int c_storage_exists(char *key, unsigned int len); */

#endif /* C_STORAGE_H */

