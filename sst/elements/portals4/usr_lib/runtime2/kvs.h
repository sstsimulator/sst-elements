

#if defined(__cplusplus)
extern "C" {
#endif

void kvs_put( const char *key, const char *value );

void kvs_get( const char *key, char *value, int length);


#if defined(__cplusplus)
}
#endif
