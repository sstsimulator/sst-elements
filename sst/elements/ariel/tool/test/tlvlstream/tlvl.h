
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

typedef int tlvl_Tag;

void* tlvl_malloc(size_t size, int level);
void  tlvl_free(void* ptr);
tlvl_Tag tlvl_memcpy(void* dest, void* src, size_t length);
void tlvl_waitComplete(tlvl_Tag in);
void tlvl_set_pool(int pool);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
