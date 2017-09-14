
#include "c_cache.h"
#include "c_storage.h"
#include "c_shared_allocator.h"

#ifdef USE_MMAP

#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>  
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

static int create_shmmap(void **p, c_shared_header **shared_header, c_shared_segment **shared_segments, const char *shared_name, unsigned long k_size, unsigned long v_size, char **error_in) {
	unsigned long header_size, alloc_size, segment_size, segments_num = 1024, v_offset, k_num;
	int fd;
	int try = 0, create, segment_body_size, k_len;

	pthread_rwlockattr_t rwattr;
	
	k_len = sizeof(c_kv_key);
	k_num = k_size / k_len;
	k_num = pow(2, floor(log10(k_num)/log10(2)));
	k_size = k_num * k_len;

	header_size = (unsigned long)sizeof(c_shared_header);

	while ((v_size / segments_num) < C_STORAGE_MIN_SEGMENT_SIZE) {
		segments_num >>= 1;
	}

	segment_size = v_size / segments_num;
	segments_num = pow(2, floor(log10(segments_num)/log10(2)));
	v_size = segments_num * segment_size;

	alloc_size = header_size + k_size + v_size;

again:
	if(try > 3) {
		*error_in = "timeout";
		goto error;
	}

	fd = open(shared_name, C_ALLOC_FILE_OPEN_CREAT_FLAG, C_ALLOC_FILE_MODEL);

	if(fd == -1 && errno == EEXIST) {
		goto exist;
	} else if(fd == -1) {
		*error_in = "open";
		goto error;
	} else {
		goto new;
	}

new:
	create = 1;

	if(ftruncate(fd, alloc_size) == -1) {
		*error_in = "ftruncate";
		goto error;
	}

	*p = (void*)mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(*p == MMAP_FAIL) {
		*error_in = "mmap";
		goto error;
	}	

	close(fd);

	// init shared header
	*shared_header = (c_shared_header *)*p;
	
	(*shared_header)->init = 0;

	(*shared_header)->k_size = k_size;
	(*shared_header)->v_size = v_size;

	(*shared_header)->alloc_size = alloc_size;

	(*shared_header)->k_num = k_num;

	(*shared_header)->segment_num = segments_num;
	(*shared_header)->segment_size = segment_size;

	(*shared_header)->k_offset = (unsigned long)sizeof(c_shared_header);
	v_offset = (*shared_header)->v_offset = (*shared_header)->k_offset + (*shared_header)->k_size;

	// init header lock
	if(pthread_rwlockattr_init(&rwattr) != 0) {
		*error_in = "pthread_rwlockattr_init";
		goto error;
	}

	if(pthread_rwlockattr_setpshared(&rwattr, PTHREAD_PROCESS_SHARED) != 0) {
		*error_in = "pthread_rwlockattr_setpshared";
		goto pthreaderr;
	}	

	if(pthread_rwlock_init(&((*shared_header)->rlock), &rwattr) != 0) {
		*error_in = "pthread_rwlock_init";
		goto pthreaderr;
	}

	if(pthread_rwlock_init(&((*shared_header)->wlock), &rwattr) != 0) {
		*error_in = "pthread_rwlock_init";
		pthread_rwlock_destroy(&((*shared_header)->rlock));
		goto pthreaderr;
	}

	pthread_rwlockattr_destroy(&rwattr);

	*shared_segments = (c_shared_segment *)calloc(1, segments_num * sizeof(c_shared_segment));

	if(!*shared_segments) {
		*error_in = "calloc";
		pthread_rwlock_destroy(&((*shared_header)->rlock));
		pthread_rwlock_destroy(&((*shared_header)->wlock));
		goto error;
	}

	// init shared_segments
	for (int i = 0; i < segments_num; ++i) {
		(*shared_segments)[i].p = *p + v_offset + i * segment_size;	
		
		segment_body_size = alloc_size - (v_offset + i * segment_size + segment_size);

		if( segment_body_size >= 0 ) {
			(*shared_segments)[i].seg_header = (c_shared_segment_header*)((*shared_segments)[i].p);
			(*shared_segments)[i].seg_header->size = segment_body_size;
			(*shared_segments)[i].seg_header->pos = 0;
		} else {
			segment_body_size = alloc_size - (v_offset + i * segment_size) - sizeof(c_shared_segment_header);

			if(segment_body_size >= 0) {
				(*shared_segments)[i].seg_header = (c_shared_segment_header*)((*shared_segments)[i].p);
				(*shared_segments)[i].seg_header->size = segment_body_size;
				(*shared_segments)[i].seg_header->pos = 0;
			} else {
				(*shared_segments)[i].seg_header = NULL;
			}
		}
	}

	(*shared_header)->init = 1;
	return C_CACHE_OK;

exist:
	fd = open(shared_name, C_ALLOC_FILE_OPEN_FLAG, C_ALLOC_FILE_MODEL);
	if(fd == -1) {
		if(errno == ENOENT) {
			++try;
			goto again;
		}
	}

	*p = (void*)mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(*p == MMAP_FAIL) {
		*error_in = "mmap";
		goto error;
	}	

	close(fd);	

	create = 0;

	*shared_header = (c_shared_header *)*p;

	int retry = 3;

	while(retry--) {
		if((*shared_header)->init == 1) {
			break;
		} else if(!retry) {
			*error_in = "timeout";
			goto error;
		}
		usleep(2 * 1000);
	}

	*shared_segments = (c_shared_segment *)calloc(1, segments_num * sizeof(c_shared_segment));

	segments_num = (*shared_header)->segment_num;
	segment_size = (*shared_header)->segment_size;

	if(!*shared_segments) {
		*error_in = "calloc";
		goto error;
	}

	// init shared_segments
	for (int i = 0; i < segments_num; ++i) {
		(*shared_segments)[i].p = *p + v_offset + i * segment_size;	

		segment_body_size = alloc_size - (v_offset + i * segment_size + segment_size);

		if( segment_body_size >= 0 ) {
			(*shared_segments)[i].seg_header = (c_shared_segment_header*)((*shared_segments)[i].p);
		} else {
			segment_body_size = alloc_size - (v_offset + i * segment_size) - sizeof(c_shared_segment_header);

			if(segment_body_size >= 0) {
				(*shared_segments)[i].seg_header = (c_shared_segment_header*)((*shared_segments)[i].p);
			} else {
				(*shared_segments)[i].seg_header = NULL;
			}
		}
	}	

	return C_CACHE_OK;

pthreaderr:
	pthread_rwlockattr_destroy(&rwattr);
	
error:
	if(fd > 0) {
		close(fd);
	}

	if(create) {
		unlink(shared_name);
	}

	if(*p != MMAP_FAIL) {
		munmap(*p, alloc_size);
	}

	return C_CACHE_FAIL;
}
/* }}} */

static void detach_shmmap(void **p, c_shared_header **shared_header, c_shared_segment **shared_segments, const char *shared_name) {
	unsigned long alloc_size;
	munmap(*p, (*shared_header)->alloc_size);
	unlink(shared_name);
	free(*shared_segments);
}
/* }}} */


#endif /* USE_MMAP */

