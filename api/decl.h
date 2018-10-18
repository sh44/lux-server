typedef unsigned char      ApiU8;
typedef unsigned short     ApiU16;
typedef unsigned int       ApiU32;
typedef unsigned long long ApiU64;

typedef signed char      ApiI8;
typedef signed short     ApiI16;
typedef signed int       ApiI32;
typedef signed long long ApiI64;

typedef float ApiF32;

void quit();
void make_admin(char const* name);
void kick(char const* name, char const* reason);
void broadcast(char const* str);
void place_light(ApiI64 x, ApiI64 y, ApiI64 z, ApiU8 r, ApiU8 g, ApiU8 b);
void new_entity(ApiF32 x, ApiF32 y, ApiF32 z);
