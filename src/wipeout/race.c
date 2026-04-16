#include "../mem.h"
#include "../input.h"
#include "../platform.h"
#include "../system.h"
#include "../utils.h"
#include "../render.h"

#include "object.h"
#include "track.h"
#include "ship.h"
#include "weapon.h"
#include "droid.h"
#include "camera.h"
#include "object.h"
#include "scene.h"
#include "game.h"
#include "hud.h"
#include "sfx.h"
#include "race.h"
#include "particle.h"
#include "menu.h"
#include "ship_ai.h"
#include "ingame_menus.h"

#define ATTRACT_DURATION 60.0

static bool is_paused = false;
static bool menu_is_scroll_text = false;
static bool has_show_credits = false;
static float attract_start_time;
static menu_t *active_menu = NULL;

void race_init(void) {
	ingame_menus_load();
	menu_is_scroll_text = false;

	const circut_settings_t *cs = &def.circuts[g.circut].settings[g.race_class];
	track_load(cs->path);
	scene_load(cs->path, cs->sky_y_offset);
	
	if (g.circut == CIRCUT_SILVERSTREAM && g.race_class == RACE_CLASS_RAPIER) {
		scene_init_aurora_borealis();	
	} 

	if (g.is_attract_mode) {
		g.pilot = rand_int(0, len(def.pilots));
	}
	race_start();
	// render_textures_dump("texture_atlas.png");

	if (g.is_attract_mode) {
		attract_start_time = system_time();

		for (unsigned int i = 0; i < len(g.ships); i++) {
			flags_rm(g.ships[i].flags, SHIP_VIEW_INTERNAL);
			flags_rm(g.ships[i].flags, SHIP_RACING);
		}

		g.camera.update_func = camera_update_attract_random;
		if (!has_show_credits || rand_int(0, 10) == 0) {
			active_menu = text_scroll_menu_init(def.credits, len(def.credits));
			menu_is_scroll_text = true;
			has_show_credits = true;
		}
	}

	is_paused = false;
}

static void race_draw_view(int pilot_idx, camera_t *camera, droid_t *droid) {
	// 3D
	render_set_view(camera->position, camera->angle);
	render_set_screen_position(camera->shake);

	render_set_cull_backface(false);
	scene_draw(camera);
	track_draw(camera);
	render_set_cull_backface(true);

	ships_draw(pilot_idx);  // -1 = spectator: no ship suppressed
	droid_draw(droid);
	weapons_draw();
	particles_draw();

	// 2D HUD — skipped for spectator view (pilot_idx == -1)
	render_set_screen_position(vec2(0, 0));
	render_set_view_2d();

	if (pilot_idx >= 0 && flags_is(g.ships[pilot_idx].flags, SHIP_RACING)) {
		hud_draw(&g.ships[pilot_idx]);
	}
}

void race_update(void) {
	if (is_paused) {
		if (!active_menu) {
			active_menu = pause_menu_init();
		}
		if (input_pressed(A_MENU_QUIT)) {
			race_unpause();
		}
	}
	else {
		ships_update();
		droid_update(&g.droid, &g.ships[g.pilot]);
		camera_update(&g.camera, &g.ships[g.pilot], &g.droid);
		if (g.local_player_count >= 2) {
			droid_update(&g.droid2, &g.ships[g.pilot2]);
			camera_update(&g.camera2, &g.ships[g.pilot2], &g.droid2);
		}
		if (g.local_player_count >= 3) {
			droid_update(&g.droid3, &g.ships[g.pilot3]);
			camera_update(&g.camera3, &g.ships[g.pilot3], &g.droid3);
			// Spectator camera tracks P1 from a fixed start-line vantage point
			camera_update_spectator(&g.spectator_camera, &g.ships[g.pilot], NULL);
		}
		if (g.local_player_count >= 4) {
			droid_update(&g.droid4, &g.ships[g.pilot4]);
			camera_update(&g.camera4, &g.ships[g.pilot4], &g.droid4);
		}
		weapons_update();
		particles_update();
		scene_update();
		if (g.race_type != RACE_TYPE_TIME_TRIAL) {
			track_cycle_pickups();
		}

		if (g.is_attract_mode) {
			if (input_pressed(A_MENU_START) || input_pressed(A_MENU_SELECT)) {
				game_set_scene(GAME_SCENE_MAIN_MENU);
			}
			float duration = system_time() - attract_start_time;
			if ((!active_menu && duration > 30) || duration > 120) {
				game_set_scene(GAME_SCENE_TITLE);
			}
		}
		else if (active_menu == NULL && (input_pressed(A_MENU_START) || input_pressed(A_MENU_QUIT))) {
			race_pause();
		}
	}

	// Draw views
	if (g.local_player_count >= 3) {
		// 2x2 quadrant layout (3P uses bottom-right for spectator)
		vec2i_t full = render_backbuffer_size();
		vec2i_t q = vec2i(full.x / 2, full.y / 2); // quadrant size
		// Top-left: P1
		render_set_viewport(vec2i(0, full.y / 2), q);
		race_draw_view(g.pilot, &g.camera, &g.droid);
		// Top-right: P2
		render_set_viewport(vec2i(full.x / 2, full.y / 2), q);
		race_draw_view(g.pilot2, &g.camera2, &g.droid2);
		// Bottom-left: P3
		render_set_viewport(vec2i(0, 0), q);
		race_draw_view(g.pilot3, &g.camera3, &g.droid3);
		// Bottom-right: P4 or spectator camera pointed at start line
		render_set_viewport(vec2i(full.x / 2, 0), q);
		if (g.local_player_count >= 4) {
			race_draw_view(g.pilot4, &g.camera4, &g.droid4);
		} else {
			race_draw_view(-1, &g.spectator_camera, &g.droid);
		}
		render_reset_viewport();
	}
	else if (g.local_player_count == 2) {
		vec2i_t full = render_backbuffer_size();
		vec2i_t half = vec2i(full.x, full.y / 2);
		// P1: top half (GL origin is bottom-left, so top half starts at y = full.y/2)
		render_set_viewport(vec2i(0, full.y / 2), half);
		race_draw_view(g.pilot, &g.camera, &g.droid);
		// P2: bottom half
		render_set_viewport(vec2i(0, 0), half);
		race_draw_view(g.pilot2, &g.camera2, &g.droid2);
		render_reset_viewport();
	}
	else {
		race_draw_view(g.pilot, &g.camera, &g.droid);
	}

	// 2D attract mode label (single player only)
	if (g.local_player_count == 1 && g.is_attract_mode && !active_menu) {
		render_set_screen_position(vec2(0, 0));
		render_set_view_2d();
		ui_draw_text("DEMO MODE", ui_scaled_pos(UI_POS_TOP | UI_POS_CENTER, vec2i(-56, 24)), UI_SIZE_8, UI_COLOR_ACCENT);
	}

	// Menu overlay (full screen)
	if (active_menu) {
		render_set_screen_position(vec2(0, 0));
		render_set_view_2d();
		if (!menu_is_scroll_text) {
			vec2i_t size = render_size();
			render_push_2d(vec2i(0, 0), size, rgba(0, 0, 0, 128), RENDER_NO_TEXTURE);
		}
		menu_update(active_menu);
	}
}

void race_start(void) {
	active_menu = NULL;
	sfx_reset();
	scene_init();
	camera_init(&g.camera, g.track.sections);
	g.camera.update_func = camera_update_race_intro;
	if (g.local_player_count >= 2) {
		camera_init(&g.camera2, g.track.sections);
		g.camera2.update_func = camera_update_race_intro;
	}
	if (g.local_player_count >= 3) {
		camera_init(&g.camera3, g.track.sections);
		g.camera3.update_func = camera_update_race_intro;

		// Position spectator camera at the start/finish line, slightly elevated
		int start_pos = def.circuts[g.circut].settings[g.race_class].start_line_pos;
		section_t *sec = g.track.sections;
		for (int i = 0; i < start_pos; i++) { sec = sec->next; }
		g.spectator_camera.position = vec3(sec->center.x, sec->center.y - 800, sec->center.z);
		g.spectator_camera.shake = vec2(0, 0);
		g.spectator_camera.shake_timer = 0;
		g.spectator_camera.update_func = camera_update_spectator;
		// Initial angle: face back down the track toward the grid
		section_t *prev = sec->prev ? sec->prev : sec;
		vec3_t back = vec3_sub(prev->center, sec->center);
		g.spectator_camera.angle = vec3(0, -atan2(back.x, back.z), 0);
	}
	if (g.local_player_count >= 4) {
		camera_init(&g.camera4, g.track.sections);
		g.camera4.update_func = camera_update_race_intro;
	}
	ships_init(g.track.sections);
	droid_init(&g.droid, &g.ships[g.pilot]);
	if (g.local_player_count >= 2) {
		droid_init(&g.droid2, &g.ships[g.pilot2]);
	}
	if (g.local_player_count >= 3) {
		droid_init(&g.droid3, &g.ships[g.pilot3]);
	}
	if (g.local_player_count >= 4) {
		droid_init(&g.droid4, &g.ships[g.pilot4]);
	}
	particles_init();
	weapons_init();

	for (unsigned int i = 0; i < len(g.race_ranks); i++) {
		g.race_ranks[i].points = 0;
		g.race_ranks[i].pilot = i;
	}
	for (unsigned int i = 0; i < len(g.lap_times); i++) {
		for (unsigned int j = 0; j < len(g.lap_times[i]); j++) {
			g.lap_times[i][j] = 0;
		}
	}
	g.is_new_race_record = false;
	g.is_new_lap_record = false;
	g.best_lap = 0;
	g.race_time = 0;
}

void race_restart(void) {
	race_unpause();

	if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		g.lives--;
		if (g.lives == 0) {
			race_release_control();
			active_menu = game_over_menu_init();
			return;
		}
	}

	race_start();
}

static bool sort_points_compare(pilot_points_t *pa, pilot_points_t *pb) {
	return (pa->points < pb->points);
}

void race_end(int finishing_pilot) {
	race_release_control();

	g.race_position = g.ships[finishing_pilot].position_rank;

	g.race_time = 0;
	g.best_lap = g.lap_times[finishing_pilot][0];
	for (int i = 0; i < NUM_LAPS; i++) {
		g.race_time += g.lap_times[finishing_pilot][i];
		if (g.lap_times[finishing_pilot][i] < g.best_lap) {
			g.best_lap = g.lap_times[finishing_pilot][i];
		}
	}

	highscores_t *hs = &save.highscores[g.race_class][g.circut][g.highscore_tab];
	if (g.best_lap < hs->lap_record) {
		hs->lap_record = g.best_lap;
		g.is_new_lap_record = true;
		save.is_dirty = true;
	}

	for (int i = 0; i < NUM_HIGHSCORES; i++) {
		if (g.race_time < hs->entries[i].time) {
			g.is_new_race_record = true;
			break;
		}
	}

	if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		for (unsigned int i = 0; i < len(def.race_points_for_rank); i++) {
			g.race_ranks[i].points = def.race_points_for_rank[i];

			// Find the pilot for this race rank in the championship table
			for (unsigned int j = 0; j < len(g.championship_ranks); j++) {
				if (g.race_ranks[i].pilot == g.championship_ranks[j].pilot) {
					g.championship_ranks[j].points += def.race_points_for_rank[i];
					break;
				}
			}
		}
		sort(g.championship_ranks, len(g.championship_ranks), sort_points_compare);
	}

	active_menu = race_stats_menu_init();
}

void race_next(void) {
	int next_circut = g.circut + 1;

	// Championship complete
	if (
		(save.has_bonus_circuts && next_circut >= NUM_CIRCUTS) ||
		(!save.has_bonus_circuts && next_circut >= NUM_NON_BONUS_CIRCUTS)
	) {
		if (g.race_class == RACE_CLASS_RAPIER) {
			if (save.has_bonus_circuts) {
				active_menu = text_scroll_menu_init(def.congratulations.rapier_all_circuts, len(def.congratulations.rapier_all_circuts));
			}
			else {
				save.has_bonus_circuts = true;
				active_menu = text_scroll_menu_init(def.congratulations.rapier, len(def.congratulations.rapier));
			}
		}
		else {
			save.has_rapier_class = true;
			if (save.has_bonus_circuts) {
				active_menu = text_scroll_menu_init(def.congratulations.venom_all_circuts, len(def.congratulations.venom_all_circuts));
			}
			else {
				active_menu = text_scroll_menu_init(def.congratulations.venom, len(def.congratulations.venom));
			}
		}
		save.is_dirty = true;
		menu_is_scroll_text = true;
	}

	// Next track
	else {
		g.circut = next_circut;
		game_set_scene(GAME_SCENE_RACE);
	}
}

static void race_release_player(int pilot_idx, camera_t *cam) {
	flags_rm(g.ships[pilot_idx].flags, SHIP_RACING);
	g.ships[pilot_idx].remote_thrust_max = 3160;
	g.ships[pilot_idx].remote_thrust_mag = 32;
	g.ships[pilot_idx].speed = 3160;
	cam->update_func = camera_update_attract_random;
}

void race_release_control(void) {
	race_release_player(g.pilot, &g.camera);
	if (g.local_player_count >= 2) { race_release_player(g.pilot2, &g.camera2); }
	if (g.local_player_count >= 3) { race_release_player(g.pilot3, &g.camera3); }
	if (g.local_player_count >= 4) { race_release_player(g.pilot4, &g.camera4); }
}

void race_pause(void) {
	sfx_pause();
	is_paused = true;
}

void race_unpause(void) {
	sfx_unpause();
	is_paused = false;
	active_menu = NULL;
}
