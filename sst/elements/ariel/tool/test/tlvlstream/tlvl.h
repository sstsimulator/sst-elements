
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

typedef int tlvl_Tag;

void* tlvl_malloc(size_t size);
void  tlvl_free(void* ptr);
tlvl_Tag tlvl_memcpy(void* dest, void* src, size_t length);
void tlvl_waitComplete(tlvl_Tag in);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
