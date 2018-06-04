typedef void *ApiHashMap;

void *malloc(size_t size);
void    free(void *ptr);

void hash_map_insert(ApiHashMap *hash_map, ApiCString key, void *val);

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
} ApiLogLevel;

void log_msg(ApiLogLevel level, ApiCString msg);

typedef struct
{
    ApiCString id;
    ApiCString name;
    enum
    {
        EMPTY,
        FLOOR,
        WALL
    } shape;

    struct
    {
        ApiU8 x;
        ApiU8 y;
    } tex_pos;
} ApiTileType;

typedef struct
{
    ApiCString id;
    ApiCString name;
} ApiEntityType;

typedef struct
{
    ApiHashMap tile_types;
    ApiHashMap entity_types;
} ApiDatabase;

typedef struct
{
    ApiDatabase   *db;
    ApiEntityType *player_type;
} ApiConfig;
