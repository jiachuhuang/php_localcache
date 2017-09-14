
#ifndef C_SHARED_ALLOCATOR_H
#define C_SHARED_ALLOCATOR_H

#define USE_MMAP 1

#define C_ALLOC_FILE_MODEL					(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define C_ALLOC_FILE_OPEN_CREAT_FLAG		(O_CREAT | O_EXCL | O_RDWR)
#define C_ALLOC_FILE_OPEN_FLAG				(O_RDWR)

#ifdef USE_MMAP
	#define MMAP_FAIL						((void*)-1)	
#else 
	#error(no builtin shared memory supported)	
#endif /* USE_MMAP */				

int c_cache_allocator_startup(void **p, c_shared_header **shared_header, c_shared_segment **shared_segments, const char *shared_name, unsigned long k_size, unsigned long v_size, char **error_in);
void c_cache_allocator_shutdown(void **p, c_shared_header **shared_header, c_shared_segment **shared_segments, const char *shared_name);
void *c_cache_allocator_raw_alloc(c_shared_header **shared_header, c_shared_segment **shared_segments, const unsigned int real_size, const unsigned int hash, unsigned int *seg, unsigned long *offset);
/* int c_cache_allocator_free(void *p); */

#endif /* C_SHARED_ALLOCATOR_H */