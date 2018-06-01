void *malloc(size_t size);
void    free(void *ptr);

typedef enum
{
    OFF,
    CRITICAL,
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE,
    ALL = TRACE
} LogLevel;

void log_msg(LogLevel level, const char *msg);

typedef struct {      int x, y; } Point2i;
typedef struct { unsigned x, y; } Point2u;
typedef struct {    float x, y; } Point2f;
typedef struct {   double x, y; } Point2d;

typedef struct {      int x, y, z; } Point3i;
typedef struct { unsigned x, y, z; } Point3u;
typedef struct {    float x, y, z; } Point3f;
typedef struct {   double x, y, z; } Point3d;

typedef struct
{
    void *player_type;
} Config;

void       *entity_type_ctor(const char *name);
const char *entity_type_name(void *ptr);
void        entity_type_dtor(void *ptr);
