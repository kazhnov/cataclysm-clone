#include <stdint.h>

typedef struct {
    double x;
    double y;
} Vector2F;

typedef struct {
    double radius;
    double angle;
} Polar;

typedef struct {
    Polar position;    
    double angular_velocity;
} Orb;

void OrbMoveTicks(Orb* orb, uint32_t ticks);
void OrbDraw(uint32_t id);
void OrbitDraw(uint32_t id);

Vector2I SpaceGetScreenPosition(Vector2F position);
Vector2F Polar2Vector2F(Polar polar);
