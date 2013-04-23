#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x94, 0x35, 0x9F, 0xB5, 0x58, 0x18, 0x49, 0x42, 0xA4, 0x2E, 0x92, 0x27, 0xB6, 0x09, 0x16, 0xE6 }
PBL_APP_INFO(MY_UUID,
             "Game of Life", "Dalton Claybrook",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

Layer gameLayer;
TextLayer timeLayer;

long randSeed;

#define TIME_ZONE_OFFSET -5
#define DOTS_WIDTH 24
#define DOTS_HEIGHT 28
#define MAX_NUMBER 999

int dots[DOTS_HEIGHT][DOTS_WIDTH];
int tempDots[DOTS_HEIGHT * DOTS_WIDTH];

int get_unix_time_from_current_time(PblTm *current_time)
{
	unsigned int unix_time;
    
	/* Convert time to seconds since epoch. */
    unix_time = ((0-TIME_ZONE_OFFSET)*3600) + /* time zone offset */
    + current_time->tm_sec /* start with seconds */
    + current_time->tm_min*60 /* add minutes */
    + current_time->tm_hour*3600 /* add hours */
    + current_time->tm_yday*86400 /* add days */
    + (current_time->tm_year-70)*31536000 /* add years since 1970 */
    + ((current_time->tm_year-69)/4)*86400 /* add a day after leap years, starting in 1973 */
    - ((current_time->tm_year-1)/100)*86400 /* remove a leap day every 100 years, starting in 2001 */
    + ((current_time->tm_year+299)/400)*86400; /* add a leap day back every 400 years, starting in 2001*/
    
	return unix_time;
}

int get_unix_time()
{
	PblTm current_time;
	get_time(&current_time);
    
	return get_unix_time_from_current_time(&current_time);
}

int random(int max)
{
	randSeed = (((randSeed * 214013L + 2531011L) >> 16) & 32767);
	return (randSeed % max);
}

void draw_cell(GContext* ctx, int row, int column) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    GPoint circlePoint = GPoint(column*6+3, row*6+3);
    graphics_fill_circle(ctx, circlePoint, 2);
}

void gameLayer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    int i;
    int j;
    for (i=0; i<DOTS_HEIGHT; i++) {
        for (j=0; j<DOTS_WIDTH; j++) {
            if (dots[i][j] == 1) {
                draw_cell(ctx, i, j);
            }
        }
    }
}

void setup_dots_array() {
    int i;
    int j;
    int n = DOTS_WIDTH * DOTS_HEIGHT;
    int half = n / 2;
    
    //Populate with half alive
    for (i=0; i<n; i++) {
        if (i<half) {
            tempDots[i] = 0;
        } else {
            tempDots[i] = 1;
        }
    }
    
    //Shuffle
    for (i=0; i<n-1; i++)
    {
        j = i + random(MAX_NUMBER) / (MAX_NUMBER / (n - i) + 1);
        int t = tempDots[j];
        tempDots[j] = tempDots[i];
        tempDots[i] = t;
    }
    
    for (i=0; i<DOTS_HEIGHT; i++) {
        for (j=0; j<DOTS_WIDTH; j++) {
            dots[i][j] = tempDots[i * DOTS_WIDTH + j];
        }
    }
}

GPoint corrected_point(GPoint point) {
    int x = point.x;
    int y = point.y;
    
    if (x<0) x = DOTS_WIDTH-1;
    else if (x>=DOTS_WIDTH) x = 0;
    if (y<0) y = DOTS_HEIGHT-1;
    else if (y>=DOTS_HEIGHT) y = 0;
    
    return GPoint(x, y);
}

void process_time_step() {
    int i = 0;
    int x;
    int y;
    
    for (y=0; y<DOTS_HEIGHT; y++) {
        for (x=0; x<DOTS_WIDTH; x++) {
            int neighbors = 0;
            GPoint currentPoint;
            //left
            currentPoint = corrected_point(GPoint(x-1, y));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //top left
            currentPoint = corrected_point(GPoint(x-1, y-1));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //top
            currentPoint = corrected_point(GPoint(x, y-1));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //top right
            currentPoint = corrected_point(GPoint(x+1, y-1));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //right
            currentPoint = corrected_point(GPoint(x+1, y));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //bottom right
            currentPoint = corrected_point(GPoint(x+1, y+1));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //bottom
            currentPoint = corrected_point(GPoint(x, y+1));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            //bottom left
            currentPoint = corrected_point(GPoint(x-1, y+1));
            if (dots[currentPoint.y][currentPoint.x] == 1) neighbors++;
            
            if (dots[y][x] == 1) {
                if ((neighbors < 2) || (neighbors > 3)) {
                    tempDots[i] = 0;
                } else {
                    tempDots[i] = 1;
                }
            } else {
                if (neighbors == 3) {
                    tempDots[i] = 1;
                } else {
                    tempDots[i] = 0;
                }
            }
            i++;
        }
    }
    
    for (y=0; y<DOTS_HEIGHT; y++) {
        for (x=0; x<DOTS_WIDTH; x++) {
            dots[y][x] = tempDots[y * DOTS_WIDTH + x];
        }
    }
}

void set_time(PblTm *tm) {
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00 PM";
    char *time_format;
    
    if (clock_is_24h_style()) {
        time_format = "%R";
    } else {
        time_format = "%I:%M %p";
    }
    
    string_format_time(time_text, sizeof(time_text), time_format, tm);
    
    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    
    text_layer_set_text(&timeLayer, time_text);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)t;
    (void)ctx;
    
    if (t->tick_time->tm_sec == 0) {
        setup_dots_array();
        set_time(t->tick_time);
    }
    
    process_time_step();
    layer_mark_dirty(&gameLayer);
}

void handle_init(AppContextRef ctx) {
    (void)ctx;
    
    window_init(&window, "Game of Life watch");
    window_stack_push(&window, true /* Animated */);
    
    window_set_background_color(&window, GColorBlack);
    
    // Init the game layer
    layer_init(&gameLayer, window.layer.frame);
    gameLayer.update_proc = &gameLayer_update_callback;
    layer_add_child(&window.layer, &gameLayer);
    
    // Init the time layer
    text_layer_init(&timeLayer, window.layer.frame);
    text_layer_set_text_color(&timeLayer, GColorBlack);
    text_layer_set_background_color(&timeLayer, GColorWhite);
    text_layer_set_text_alignment(&timeLayer, GTextAlignmentCenter);
    text_layer_set_font(&timeLayer, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
    layer_set_frame(&timeLayer.layer, GRect(0, 116, 144, 40));
    layer_add_child(&window.layer, &timeLayer.layer);
    
    //Only set the seed once
	randSeed = get_unix_time();
    setup_dots_array();
    
    PblTm t;
    get_time(&t);
    set_time(&t);
}


void pbl_main(void *params) {
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        
        .tick_info = {
            .tick_handler = &handle_second_tick,
            .tick_units = SECOND_UNIT
        }
    };
    app_event_loop(params, &handlers);
}