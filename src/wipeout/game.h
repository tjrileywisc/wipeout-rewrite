#ifndef GAME_H
#define GAME_H

#include "../types.h"

#include "droid.h"
#include "ship.h"
#include "camera.h"
#include "track.h"

#define NUM_AI_OPPONENTS 7
#define NUM_PILOTS_PER_TEAM 2
#define NUM_NON_BONUS_CIRCUITS 6
#define NUM_MUSIC_TRACKS 11
#define NUM_HIGHSCORES 5

#define NUM_LAPS 3
#define NUM_LIVES 3
#define QUALIFYING_RANK 3
#define SAVE_DATA_MAGIC 0x64736f77
#define NUM_PROFILES 8
#define PROFILE_NONE -1
#define MAX_LOCAL_PLAYERS 4

typedef enum {
	GAME_WIPEOUT, 
	GAME_WIPEOUT_2097, 
	GAME_WIPEOUT_64
} game_release_t;

typedef enum {
	A_UP,
	A_DOWN,
	A_LEFT,
	A_RIGHT,
	A_BRAKE_LEFT,
	A_BRAKE_RIGHT,
	A_THRUST,
	A_FIRE,
	A_CHANGE_VIEW,
	NUM_GAME_ACTIONS,

	A_MENU_UP,
	A_MENU_DOWN,
	A_MENU_LEFT,
	A_MENU_RIGHT,
	A_MENU_BACK,
	A_MENU_SELECT,
	A_MENU_START,
	A_MENU_QUIT,

	A_TOGGLE_TEXTURE_UPSCALE,
} action_t;


typedef enum {
	GAME_SCENE_INTRO,
	GAME_SCENE_TITLE,
	GAME_SCENE_MAIN_MENU,
	GAME_SCENE_HIGHSCORES,
	GAME_SCENE_RACE,
	GAME_SCENE_NONE,
	NUM_GAME_SCENES
} game_scene_t;

enum race_class {
	RACE_CLASS_VENOM,
	RACE_CLASS_RAPIER,
	NUM_RACE_CLASSES
};

enum race_type {
	RACE_TYPE_CHAMPIONSHIP,
	RACE_TYPE_SINGLE,
	RACE_TYPE_NETWORK,
	RACE_TYPE_TIME_TRIAL,
	NUM_RACE_TYPES,
};

enum highscore_tab {
	HIGHSCORE_TAB_TIME_TRIAL,
	HIGHSCORE_TAB_RACE,
	NUM_HIGHSCORE_TABS
};

enum pilot {
	PILOT_JOHN_DEKKA,
	PILOT_DANIEL_CHANG,
	PILOT_ARIAL_TETSUO,
	PILOT_ANASTASIA_CHEROVOSKI,
	PILOT_KEL_SOLAAR,
	PILOT_ARIAN_TETSUO,
	PILOT_SOFIA_DE_LA_RENTE,
	PILOT_PAUL_JACKSON,
	NUM_PILOTS
};

enum team {
	TEAM_AG_SYSTEMS,
	TEAM_AURICOM,
	TEAM_QIREX,
	TEAM_FEISAR,
	NUM_TEAMS
};

enum circuit {
	CIRCUIT_ALTIMA_VII,
	CIRCUIT_KARBONIS_V,
	CIRCUIT_TERRAMAX,
	CIRCUIT_KORODERA,
	CIRCUIT_ARRIDOS_IV,
	CIRCUIT_SILVERSTREAM,
	CIRCUIT_FIRESTAR,
	NUM_WIPEOUT_CIRCUITS,
	//wipeout2
	CIRCUIT_TALONS_REACH = NUM_WIPEOUT_CIRCUITS,
	CIRCUIT_SAGARMATHA,
	CIRCUIT_VALPARAISO,
	CIRCUIT_PHENITIA_PARK,
	CIRCUIT_GARE_DEUROPA,
	CIRCUIT_ODESSA_KEYS,
	CIRCUIT_VOSTOK_ISLAND,
	CIRCUIT_SPILSKINANKE,
	CIRCUIT_UNFINISHED_TRACK,
	//wipeout64
	CIRCUIT_KLIES_BRIDGE,
	CIRCUIT_QORON_IV,
	CIRCUIT_SOKANA,
	CIRCUIT_DYRONESS,
	CIRCUIT_MACHAON_II,
	CIRCUIT_TERAFUMOS,
	CIRCUIT_VELOCITAR,
	NUM_CIRCUITS
};
// How detailed are the stats? Just the FPS or draw internals
typedef enum {
	DRAW_STATS_OFF,
	DRAW_STATS_FPS,
	DRAW_STATS_DEBUG
} draw_stats_t;

// Game definitions

typedef struct {
	char *name;
} race_class_t;

typedef struct {
	char *name;
} race_type_t;

typedef struct {
	char *name;
	char *portrait;
	int logo_model;
	int team;
} pilot_t;

typedef struct {
	float thrust_max;
	float thrust_magnitude;
	bool fight_back;
} ai_setting_t;

typedef struct {
	float mass;
	float thrust_max;
	float resistance;
	float turn_rate;
	float turn_rate_max;
	float skid;
} team_attributes_t;

typedef struct {
	char *name;
	int logo_model;
	int pilots[NUM_PILOTS_PER_TEAM];
	team_attributes_t attributes[NUM_RACE_CLASSES];
} team_t;

typedef struct {
	char *path;
	float start_line_pos;
	float behind_speed;
	float spread_base;
	float spread_factor;
	float sky_y_offset;
} circuit_settings_t;

typedef struct {
	char *name;
	bool is_bonus_circuit;
	game_release_t release;
	circuit_settings_t settings[NUM_RACE_CLASSES];
} circuit_t;

typedef struct {
	char *path;
	char *name;
} music_track_t;

typedef struct {
	race_class_t race_classes[NUM_RACE_CLASSES];
	race_type_t race_types[NUM_RACE_TYPES];
	pilot_t pilots[NUM_PILOTS];
	team_t teams[NUM_TEAMS];
	ai_setting_t ai_settings[NUM_RACE_CLASSES][NUM_AI_OPPONENTS];
	circuit_t circuits[NUM_CIRCUITS];
	int ship_model_to_pilot[NUM_PILOTS];
	int race_points_for_rank[NUM_PILOTS];
	music_track_t music[NUM_MUSIC_TRACKS];
	char *credits[104];
	struct {
		char *venom[15];
		char *venom_all_circuits[19];
		char *rapier[26];
		char *rapier_all_circuits[24];
	} congratulations;
} game_def_t;



// Running game data

typedef struct {
	uint16_t pilot;
	uint16_t points;
} pilot_points_t;

typedef struct {
	float frame_time;
	float frame_rate;
	
	int race_class;
	int race_type;
	int highscore_tab;
	int team;
	int pilot;
	int circuit;
	bool is_attract_mode;
	bool show_credits;

	bool is_new_lap_record;
	bool is_new_race_record;
	float best_lap;
	float race_time;
	int lives;
	int race_position;
	
	float lap_times[NUM_PILOTS][NUM_LAPS];
	pilot_points_t race_ranks[NUM_PILOTS];
	pilot_points_t championship_ranks[NUM_PILOTS];

	int local_player_count; // 1 = solo, 2/3/4 = split-screen
	int pilot2;
	int pilot3;
	int pilot4;

	camera_t camera;
	camera_t camera2;
	camera_t camera3;
	camera_t camera4;
	camera_t spectator_camera; // fixed start-line view used in 3-player mode
	droid_t droid;
	droid_t droid2;
	droid_t droid3;
	droid_t droid4;
	ship_t ships[NUM_PILOTS];
	track_t track;

	bool additional_circuits;
	bool installed_circuits[NUM_CIRCUITS];

	// Profile slot per local player (index into save.profiles), or PROFILE_NONE
	int profile[MAX_LOCAL_PLAYERS];
	int race_player; // local player who set g.race_time

	// Grand prix championships: the finishing order of the previous race,
	// used as the starting grid of the next one (index 0 = pole position)
	int grand_prix_grid[NUM_PILOTS];
	bool has_grand_prix_grid;
} game_t;



// Save Data

typedef struct {
	bool used;
	char code[4];     // 1-3 chars from the hall of fame charset, NUL terminated
	uint8_t color[3]; // engine color: r, g, b
} profile_t;

typedef struct {
	char name[4];
	float time;
} highscores_entry_t;

typedef struct {
	highscores_entry_t entries[NUM_HIGHSCORES];
	float lap_record;
} highscores_t;

typedef struct {
	uint32_t magic;
	bool is_dirty;

	float sfx_volume;
	float music_volume;
	float internal_roll;
	uint8_t ui_scale;
	draw_stats_t draw_stats;
	bool fullscreen;
	int screen_res;
	int post_effect;
	float screen_shake;
	bool enable_force_feedback;
	int network_interface;

	uint32_t has_rapier_class;
	uint32_t has_bonus_circuits;
	
	uint8_t buttons[NUM_GAME_ACTIONS][2];
	float analog_response;

	char highscores_name[4];
	highscores_t highscores[NUM_RACE_CLASSES][NUM_CIRCUITS][NUM_HIGHSCORE_TABS];

	// Added after the first release of the save format; must stay last so
	// older (shorter) save files can still be loaded
	int texture_upscale;
	bool ship_markers;
	int ship_lighting;
	int track_lighting;
	profile_t profiles[NUM_PROFILES];
	int last_profile; // last profile used in single player, or PROFILE_NONE
	// Per profile slot: use the stock engine colors instead of profile_t.color.
	// Kept out of profile_t so saves written before it was added still load.
	bool profile_default_color[NUM_PROFILES];
	// Single player championships run as a grand prix (see game_is_grand_prix)
	bool grand_prix;
} save_t;




extern const game_def_t def;
extern game_t g;
extern save_t save;

void game_init(void);
void game_set_scene(game_scene_t scene);
void game_reset_championship(void);
// A grand prix is a championship where every race starts in the finishing
// order of the previous one and there's no need to qualify (top 3) to go on.
// Split-screen championships always are; single player ones by option.
bool game_is_grand_prix(void);
void game_update(void);

// The profile in the given save slot, or NULL if the slot is unused/invalid
profile_t *game_profile(int slot);
// The profile's engine color for ship_set_engine_color(); NULL = the default
// colors (no profile, or the profile uses the defaults)
const uint8_t *game_profile_engine_color(const profile_t *profile);
// Restore all best times and lap records to the built-in ones
void game_reset_highscores(void);
// The profile of local player 0..3 in the current game, or NULL
profile_t *game_player_profile(int player);
// Forget a deleted profile everywhere it is selected
void game_profile_deleted(int slot);

#endif
