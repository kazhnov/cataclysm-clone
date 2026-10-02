#define _XOPEN_SOURCE

#include <math.h>
#include <locale.h>
#include <notcurses/notcurses.h>
#include <time.h>
#include <game.h>
#include <space.h>
#include <stdint.h>

#define PI 3.1415926535

struct notcurses *nc;
struct ncplane *plane;
struct ncplane *planet_info_plane;
struct ncpile *panel_pile;
int should_quit = 0;

FILE* game_log;

void handle_input();
void game_draw();
void game_tick();

Entity entities[1];

#define ORB_COUNT 5
Orb orbs[5];
uint32_t orb_selected = 2;

int32_t skip_frames = 0;

void OrbSelectedSwitch(int32_t delta);

clock_t current_time;
clock_t previous_time;
double GetDeltaTime();

typedef struct {
    double scale;
    double inclination;
    Vector2F offset;
    Vector2F going_from;
    double offset_animation_progress;
} SpaceView;

SpaceView space_view = {.scale = 1, .inclination = -PI/2};

void SpaceInclinationChange(double delta);
void SpaceScaleChange(double factor);
void SpaceViewUpdate();

void SkipFrames(uint32_t frames);

float cell_ratio = 1;

Vector2I window_size_pixels = {0};
Vector2I window_size_cells = {0};
void WindowSizeUpdate();
void OrbsRandomize();

int main() {
    setlocale(LC_ALL, "");
    game_log = fopen("info.log", "w");
    nc = notcurses_init(NULL, NULL);
    
    OrbsRandomize();

    current_time = clock();
    previous_time = current_time;
    
    orbs[0].position = (Polar){0};
    orbs[0].angular_velocity = 0;
    plane = notcurses_stdplane(nc);
    
    struct ncplane_options planet_info_plane_opts = {
	.cols = 1,
	.rows = 2,
	.flags = NCPLANE_OPTION_AUTOGROW,
    };
    planet_info_plane = ncplane_create(plane, &planet_info_plane_opts);
        
    while (!should_quit) {
	previous_time = current_time;
	current_time = clock();
	ncplane_erase(notcurses_stdplane(nc));
	WindowSizeUpdate();
	SpaceViewUpdate();
	game_draw();
	if (skip_frames <= 0) {
	    ncplane_printf_yx(plane, 0, 1, "Input");	    
	    skip_frames = 0;
	    handle_input();
	} else {
	    ncplane_printf_yx(plane, 0, 1, "Animation");
	    skip_frames--;
	}
	notcurses_render(nc);

    }
    notcurses_stop(nc);
    fclose(game_log);
    return 0;
}

void WindowSizeUpdate() {
    notcurses_stddim_yx(nc, &window_size_cells.y, &window_size_cells.x);
    fprintf(game_log, "Rows Columns: %d %d\n", window_size_cells.x, window_size_cells.y);
    Vector2I cell_size = {0};
    ncplane_pixel_geom(notcurses_stdplane(nc), &window_size_pixels.y, &window_size_pixels.x, &cell_size.y, &cell_size.x, NULL, NULL);
    cell_ratio = cell_size.x/(float)cell_size.y;
    fprintf(game_log, "window size: %d %d\n", window_size_pixels.x, window_size_pixels.y);
    fprintf(game_log, "cell size: %d %d\n", cell_size.x, cell_size.y);
}

void OrbsRandomize() {
    for (size_t i = 0; i < ORB_COUNT; i++) {
	orbs[i].position.angle = rand()%6;
	orbs[i].position.radius = rand()%30;
	orbs[i].angular_velocity = 0.02+(rand()%10)*0.001f;
    }
}

void handle_input() {
    uint32_t id;
    struct ncinput ni;

    id = notcurses_get(nc, NULL, &ni);

    switch (id) {
    case 27:
	should_quit = 1;
	break;
    }
    if (ni.evtype == NCTYPE_PRESS) {
	switch (id) {
	case NCKEY_TAB:
	    OrbSelectedSwitch(1);
	    break;
	}
    }
    if (ni.evtype == NCTYPE_PRESS || ni.evtype == NCTYPE_REPEAT) {
	switch (id) {
	case '.':
	    game_tick();
	    break;
	case NCKEY_DOWN:
	    SpaceInclinationChange(0.1);
	    break;
	case NCKEY_UP:
	    SpaceInclinationChange(-0.1);
	    break;
	case '-':
	    SpaceScaleChange(1.1);
	    break;
	case '=':
	    SpaceScaleChange(1/1.1);
	    break;
	}
    }
    /*
    if (ni.evtype == NCTYPE_REPEAT) {
	switch (id) {
	case '7':
	    player.x -= 1;
	case '8':
	    player.y -= 1;
	    break;

	case '9':
	    player.y -= 1;
	case '6':
	    player.x += 1;
	    break;

	case '3':
	    player.x += 1;
	case '2':
	    player.y += 1;
	    break;

	case '1':
	    player.y += 1;
	case '4':
	    player.x -= 1;
	    break;
	}
    }
    */
}

void game_draw() {
    if (space_view.offset_animation_progress >= 1) {
	OrbitDraw(orb_selected);
    }

    
    for (size_t i = 0; i < ORB_COUNT; i++) {
	fprintf(game_log, "Orb #%ld\n", i);
	OrbDraw(i);
    }
}

void game_tick() {
    for (size_t i = 0; i < ORB_COUNT; i++) {
	OrbMoveTicks(orbs + i, 1);
    }
}

void OrbMoveTicks(Orb* orb, uint32_t ticks) {
    orb->position.angle += orb->angular_velocity*ticks;
}

void OrbitDraw(uint32_t id) {
    Orb* orb = orbs + id;
    const size_t steps = 128;
    double angle;
    char orbit_char = '.';
    struct ncplane* plane = notcurses_stdplane(nc);
    
    for (size_t i = 0; i < steps; i++) {
	angle = i*2*PI/steps;
	Polar polar = (Polar){.angle = angle, .radius = orb->position.radius};
	Vector2I position = SpaceGetScreenPosition(Polar2Vector2F(polar));
	ncplane_putchar_yx(plane, position.y, position.x, orbit_char);
    }
}

void OrbDraw(uint32_t id) {    
    Orb* orb = orbs + id;
    struct ncplane *plane = notcurses_stdplane(nc);
    fprintf(game_log, "Polar: %lf %lf\n", orb->position.radius, orb->position.angle);        
    Vector2F positionf = Polar2Vector2F(orb->position);
    fprintf(game_log, "Vector2F: %lf %lf\n", positionf.x, positionf.y);    
    Vector2I pos = SpaceGetScreenPosition(positionf);
    fprintf(game_log, "Screen position: %d %d\n", pos.x, pos.y);
    ncplane_putegc_yx(plane, pos.y, pos.x, "🪐", NULL);

    // draw selection
    if (orb_selected == id) {
	ncplane_putegc_yx(plane, pos.y, pos.x - 1, "[", NULL);
	ncplane_putegc_yx(plane, pos.y, pos.x + 2, "]", NULL);
	ncplane_erase(planet_info_plane);
	ncplane_move_yx(planet_info_plane, pos.y, pos.x + 4);
	ncplane_printf(planet_info_plane, "radius: %2.3lf", orb->position.radius);
	ncplane_printf_yx(planet_info_plane, 1, 0, "angle:  %2.3lf", orb->position.angle);
    }
}

Vector2I SpaceGetScreenPosition(Vector2F unoffset_position) {
    Vector2F position;
    position.x = unoffset_position.x - space_view.offset.x;
    position.y = unoffset_position.y - space_view.offset.y;    
    Vector2I screen_position = {0};
    screen_position.x = position.x/(space_view.scale*cell_ratio);
    screen_position.y = (position.y/space_view.scale)*sin(space_view.inclination);
    screen_position.x += window_size_cells.x/2;
    screen_position.y += window_size_cells.y/2;

    return screen_position;
}


Vector2F Polar2Vector2F(Polar polar) {
    Vector2F pos = {0};
    pos.x = polar.radius*cos(polar.angle);
    pos.y = polar.radius*sin(polar.angle);

    return pos;
}


void SpaceInclinationChange(double delta) {
    space_view.inclination += delta;
    if (space_view.inclination > PI/2) {
	space_view.inclination = PI/2;
    } else if (space_view.inclination < -PI/2) {
	space_view.inclination = -PI/2;
    }
}
void SpaceScaleChange(double factor) {
    space_view.scale *= factor;
}

void OrbSelectedSwitch(int32_t delta) {
    orb_selected += delta;
    if (orb_selected >= ORB_COUNT) {
	orb_selected = 0;
    }
    space_view.going_from.x = space_view.offset.x;
    space_view.going_from.y = space_view.offset.y;
    space_view.offset_animation_progress = 0;
}

double Lerp(double a, double b, double step) {
    return a + step*(b - a);
}

void SpaceViewUpdate() {
    Vector2F offset = Polar2Vector2F(orbs[orb_selected].position);
    if (space_view.offset_animation_progress < 1.0) {
	space_view.offset.x = Lerp(space_view.going_from.x, offset.x, space_view.offset_animation_progress);
	space_view.offset.y = Lerp(space_view.going_from.y, offset.y, space_view.offset_animation_progress);
	space_view.offset_animation_progress += 5*GetDeltaTime();
	SkipFrames(1);
    } else {
	space_view.offset.x = offset.x;
	space_view.offset.y = offset.y;
    }
}

void SkipFrames(uint32_t frames) {
    if (skip_frames < frames) {
	skip_frames = frames;
    }
}

double GetDeltaTime(void) {
    return ((double)(current_time - previous_time))/CLOCKS_PER_SEC;
}

