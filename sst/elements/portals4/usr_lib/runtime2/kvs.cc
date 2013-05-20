#include <stdio.h>
#include <map>
#include <string.h>
#include "kvs.h"
#include <string>

#if 1 
static std::map<std::string, std::string> _keyValueMap;

void kvs_put( const char *key, const char *value )
{
    printf("%s() key='%s' value='%s'\n", __func__, key, value );
    _keyValueMap[key] = value;
}

void kvs_get( const char *key, char *value, int length)
{
    strcpy( value, _keyValueMap[key].c_str(), length );

    printf("%s() key='%s' value='%s'\n", __func__, key, value );
}
#endif
