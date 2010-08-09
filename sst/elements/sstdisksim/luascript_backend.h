extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

enum lua_value_types
{
  L_INT=1,
  L_DOUBLE,
  L_LONG,
  L_STRING,
  L_BOOLEAN
};

/* Assuming memory is allocated in the functions,
   with the exception of strings. */
