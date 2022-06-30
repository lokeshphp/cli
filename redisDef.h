#ifndef REDISDEF_H
#define REDISDEF_H

#ifdef WIN32
//#include <F:\\TNM\\redis\\redis-plus-plus-master\\src\\sw\\redis++\\redis++.h>
#include <D:\\projects\\TNM\\redis\\redis-plus-plus-master\\src\\sw\\redis++\\redis++.h>
using namespace sw::redis;
#endif
#ifndef WIN32
#include </usr/local/include/sw/redis++/redis++.h>
using namespace sw::redis;
#endif

#endif //REDISDEF_H
