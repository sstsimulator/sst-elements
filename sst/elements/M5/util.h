#ifndef _util_h
#define _util_h

class SimObject;

class M5;

typedef std::map< std::string, SimObject* > objectMap_t;

extern objectMap_t buildConfig( M5*, std::string, std::string );

#endif
