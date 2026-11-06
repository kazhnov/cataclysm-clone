#define _XOPEN_SOURCE

#include <math.h>
#include <locale.h>
#include <notcurses/notcurses.h>
#include <time.h>
#include <game.h>
#include <space.h>
#include <stdint.h>

#define PI 3.1415926535

typedef struct {
    struct ncplane *stdplane;
    struct ncplane *space_plane;
    struct ncplane *info_plane;
    struct ncplane *planet_info_plane;
    Vector2I space_plane_dimensions;
} Screen;

Screen screen = {0};

struct notcurses *nc;
uint64_t menus_outline_channels;
int should_quit = 0;

FILE* game_log;
//#define DEBUG
#ifdef DEBUG
#define LOG(a, ...) fprintf(game_log,  "[%s : %s : %d] "a, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(a, ...)
#endif

void handle_input();
void game_draw();
void game_tick();

Entity entities[1];

#define ORB_COUNT 5
Orb orbs[5];
uint32_t orb_selected = 2;
uint32_t orb_following = 0;

int32_t skip_frames = 0;

void OrbSelectedSwitch(int32_t delta);
void OrbSelectedGoto();

clock_t current_time;
clock_t previous_time;
double GetDeltaTime();

typedef struct {
    double scale;
    double inclination;
    Vector2F offset;
    Vector2F going_from;
    Vector2F going_to;
    double offset_animation_progress;
} SpaceView;

SpaceView space_view = {.scale = 1, .inclination = -PI/6};

void SpaceInclinationChange(double delta);
void SpaceScaleChange(double factor);
void SpaceViewUpdate();

void SkipFrames(uint32_t frames);

float cell_ratio = 1;

Vector2I window_size_pixels = {0};
Vector2I window_size_cells = {0};
void ScreenUpdate();
void ScreenInit();
void WindowSizeUpdate();
void OrbsRandomize();

int main() {
    setlocale(LC_ALL, "");
    game_log = fopen("info.log", "w");
    struct notcurses_options opts = {
    };
    nc = notcurses_init(&opts, NULL);

    menus_outline_channels = NCCHANNELS_INITIALIZER(0, 127, 255, 0, 0, 0);
    ScreenInit();
    OrbsRandomize();

    current_time = clock();
    previous_time = current_time;
    
    orbs[0].position = (Polar){0};
    orbs[0].angular_velocity = 0;

    Vector2I screen_size;
    
    
    while (!should_quit) {
	previous_time = current_time;
	current_time = clock();
	ncplane_erase(notcurses_stdplane(nc));
	
	WindowSizeUpdate();
	ScreenUpdate();
	SpaceViewUpdate();
	
	game_draw();
	
	if (skip_frames <= 0) {
//	    ncplane_printf_yx(plane, 0, 1, "Input");	    
	    skip_frames = 0;
	    handle_input();
	} else {
//	    ncplane_printf_yx(plane, 0, 1, "Animation");
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
    LOG( "Rows Columns: %d %d\n", window_size_cells.x, window_size_cells.y);
    Vector2I cell_size = {0};
    ncplane_pixel_geom(notcurses_stdplane(nc), &window_size_pixels.y, &window_size_pixels.x, &cell_size.y, &cell_size.x, NULL, NULL);
    cell_ratio = cell_size.x/(float)cell_size.y;
    LOG( "window size: %d %d\n", window_size_pixels.x, window_size_pixels.y);
    LOG( "cell size: %d %d\n", cell_size.x, cell_size.y);
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
	case NCKEY_ENTER:
	    OrbSelectedGoto();
	    break;
	}
    }
    if (ni.evtype == NCTYPE_PRESS || ni.evtype == NCTYPE_REPEAT) {
	switch (id) {
	case '.':
	    game_tick();
	    break;
	case NCKEY_DOWN:
	    SpaceInclinationChange(PI/12);
	    break;
	case NCKEY_UP:
	    SpaceInclinationChange(-PI/12);
	    break;
	case '-':
	    SpaceScaleChange(1.1);
	    break;
	case '=':
	    SpaceScaleChange(1/1.1);
	    break;
	}
    }
}

void game_draw() {
    if (space_view.offset_animation_progress >= 1) {
	OrbitDraw(orb_selected);
    }

    
    for (size_t i = 0; i < ORB_COUNT; i++) {
	LOG( "Orb #%ld\n", i);
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

    for (size_t i = 0; i < steps; i++) {
	angle = i*2*PI/steps;
	Polar polar = (Polar){.angle = angle, .radius = orb->position.radius};
	Vector2I position = SpaceGetScreenPosition(Polar2Vector2F(polar));
	ncplane_putchar_yx(screen.space_plane, position.y, position.x, orbit_char);
    }
}

void OrbDraw(uint32_t id) {    
    Orb* orb = orbs + id;
    LOG( "Polar: %lf %lf\n", orb->position.radius, orb->position.angle);        
    Vector2F positionf = Polar2Vector2F(orb->position);
    LOG( "Vector2F: %lf %lf\n", positionf.x, positionf.y);    
    Vector2I pos = SpaceGetScreenPosition(positionf);
    LOG( "Screen position: %d %d\n", pos.x, pos.y);
    ncplane_putegc_yx(screen.space_plane, pos.y, pos.x, "🪐", NULL);

    // draw selection
    if (orb_selected == id) {
	ncplane_putegc_yx(screen.space_plane, pos.y, pos.x - 1, "[", NULL);
	ncplane_putegc_yx(screen.space_plane, pos.y, pos.x + 2, "]", NULL);
	ncplane_erase(screen.planet_info_plane);

	uint32_t y = 0;
	ncplane_printf_yx(screen.planet_info_plane, y++, 1, "planet:");		
	ncplane_printf_yx(screen.planet_info_plane, y++, 1, "radius: %2.3lf", orb->position.radius);
	ncplane_printf_yx(screen.planet_info_plane, y++, 1, "angle:  %2.3lf", orb->position.angle);

	ncplane_move_yx(screen.planet_info_plane, pos.y-y/2, pos.x + 4);

    }
}

Vector2I SpaceGetScreenPosition(Vector2F unoffset_position) {
    Vector2F position;
    position.x = unoffset_position.x - space_view.offset.x;
    position.y = unoffset_position.y - space_view.offset.y;    
    Vector2I screen_position = {0};
    screen_position.x = position.x/(space_view.scale*cell_ratio);
    screen_position.y = (position.y/space_view.scale)*sin(space_view.inclination);
    screen_position.x += screen.space_plane_dimensions.x/2;
    screen_position.y += screen.space_plane_dimensions.y/2;

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
}

void OrbSelectedGoto() {
    if (orb_following == orb_selected) return;
    orb_following = orb_selected;
    space_view.going_from.x = space_view.offset.x;
    space_view.going_from.y = space_view.offset.y;
    space_view.offset_animation_progress = 0;
    
    space_view.going_to.x = Polar2Vector2F(orbs[orb_following].position).x;
    space_view.going_to.y = Polar2Vector2F(orbs[orb_following].position).y;
}

double Lerp(double a, double b, double step) {
    return a + step*(b - a);
}

double Sinlerp(double a, double b, double step) {
    return Lerp(a, b, sin((PI/2.)*step));
}

void SpaceViewUpdate() {
    Vector2F offset = Polar2Vector2F(orbs[orb_following].position);
    if (space_view.offset_animation_progress < 1.0) {
	space_view.offset.x = Sinlerp(space_view.going_from.x, space_view.going_to.x, space_view.offset_animation_progress);
	space_view.offset.y = Sinlerp(space_view.going_from.y, space_view.going_to.y, space_view.offset_animation_progress);
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



void ScreenInit() {
    screen.stdplane = notcurses_stdplane(nc);
    
    struct ncplane_options info_plane_opts = {
	.cols = 1,
	.rows = 1,
    };
    screen.info_plane = ncplane_create(screen.stdplane, &info_plane_opts);


    struct ncplane_options space_plane_opts = {
	.cols = 1,
	.rows = 1,
    };
    screen.space_plane = ncplane_create(screen.stdplane, &space_plane_opts);

    struct ncplane_options planet_info_plane_opts = {
	.cols = 1,
	.rows = 4,
	.flags = NCPLANE_OPTION_AUTOGROW,
    };
    screen.planet_info_plane = ncplane_create(screen.stdplane, &planet_info_plane_opts);
    
    uint64_t channels1 = 0;
    ncchannels_set_bchannel(&channels1, 0xFF0000);
    ncplane_set_base(screen.space_plane, " ", 0, channels1);
    
    uint64_t channels2 = 0;
    ncchannels_set_bchannel(&channels2, 0x0000FF);
    ncplane_set_base(screen.info_plane, " ", 0, channels2);


    
}

void ScreenUpdate() {
    Vector2I screen_size;
    ncplane_dim_yx(screen.stdplane, &screen_size.y, &screen_size.x);

    Vector2I space_plane_size;
    space_plane_size.x = screen_size.y/cell_ratio;
    space_plane_size.y = screen_size.y;
    
    LOG("space_plane_size: %d %d\n", space_plane_size.x, space_plane_size.y);
    ncplane_resize(screen.space_plane, 0, 0, 0, 0,
		   0, 0, space_plane_size.y, space_plane_size.x);
    

    ncplane_resize(screen.info_plane, 0, 0, 0, 0,
		   0, space_plane_size.x, space_plane_size.y, screen_size.x - space_plane_size.x);

    ncplane_dim_yx(screen.space_plane, &screen.space_plane_dimensions.y, &screen.space_plane_dimensions.x);
	
	
    ncplane_erase(screen.space_plane);
    ncplane_erase(screen.info_plane);
    ncplane_erase(screen.planet_info_plane);
    for (size_t i = 0; i < space_plane_size.y; i++) {
	ncplane_putchar_yx(screen.info_plane, i, 0, '|');
    }	
    
}
