#include <stdint.h>
#include <wchar.h>

typedef struct {
    int32_t x;
    int32_t y;
} Vector2I;

typedef struct {
    wchar_t sprite;
    uint32_t color;
} Texture;

typedef uint64_t EntityComponents;

#define ENTC_CONTROLLED_BY_PLAYER (1<<0)
#define ENTC_INVENTORY (1<<1)
#define ENTC_SOLID (1<<2)
/*
#define ENTC_ (1<< )
*/
typedef struct {
    EntityComponents components_active;
    Vector2I position;
    Texture texture;
} Entity;

void EntityMove(Entity* ent);
void EntityDraw(Entity* ent);
