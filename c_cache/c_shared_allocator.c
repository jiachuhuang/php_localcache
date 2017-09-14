#include "c_cache.h"
#include "c_storage.h"
#include "c_shared_allocator.h"

#include <pthread.h>
#include <stdio.h>

#ifdef USE_MMAP
	#include "c_shared_mmap.c"
#else 
	#error(no builtin shared memory supported)	
#endif

int c_cache_allocator_startup(void **p, c_shared_header **shared_header, c_shared_segment **shared_segments, const char *shared_name, unsigned long k_size, unsigned long v_size, char **error_in) {
	int ret;

	ret = create_shmmap(p, shared_header, shared_segments, shared_name, k_size, v_size, error_in);

	if(ret == C_CACHE_FAIL) {
		return C_CACHE_FAIL;
	}

	return C_CACHE_OK;
}

void c_cache_allocator_shutdown(void **p, c_shared_header **shared_header, c_shared_segment **shared_segments, const char *shared_name) {

	detach_shmmap(p, shared_header, shared_segments, shared_name);
}

void *c_cache_allocator_raw_alloc(c_shared_header **shared_header, c_shared_segment **shared_segments, const unsigned int real_size, const unsigned int hash, unsigned int *seg, unsigned long *offset) {
	unsigned int segment_num, size, pos;
	int i, current, max_try;

	segment_num = C_STORAGE_SEGMENT_NUM(*shared_header);
	current = hash & (segment_num - 1);

	max_try = segment_num > 4? 4: segment_num;

	do {
		if(!(*shared_segments)[current].seg_header) {
			goto newcur;
		}
		size = (*shared_segments)[current].seg_header->size;
		pos = (*shared_segments)[current].seg_header->pos;

		if(real_size >= (size - pos)) {
			goto found;
		}
newcur:
		current = (current + 1) & (segment_num - 1);	
	} while(--max_try);

	(*shared_segments)[current].seg_header->pos = 0;
	size = (*shared_segments)[current].seg_header->size;
	pos = 0;

found:
	(*shared_segments)[current].seg_header->pos = pos + real_size;
	*seg = current;
	*offset = pos;
	return (void*) ((*shared_segments)[current].p + pos);
}


