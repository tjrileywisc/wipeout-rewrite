#include "../utils.h"
#include "../system.h"
#include "../mem.h"
#include "../platform.h"
#include "../input.h"

#include "menu.h"
#include "main_menu.h"
#include "game.h"
#include "image.h"
#include "server_com.h"
#include "ui.h"

static void page_main_init(menu_t *menu);
static void page_options_init(menu_t *menu);
static void page_race_class_init(menu_t *menu);
static void page_race_type_init(menu_t *menu);
static void page_team_init(menu_t *menu);
static void page_pilot_init(menu_t *menu);
static void page_split_screen_init(menu_t *menu);
static void page_circuit_init(menu_t *menu);
static void page_network_init(menu_t *menu);
static void page_network_connected_init(menu_t *menu);
static void page_options_controls_init(menu_t *menu);
static void page_options_video_init(menu_t *menu);
static void page_options_audio_init(menu_t *menu);
static void page_options_game_init(menu_t *menu);
static void page_options_highscores_init(menu_t *menu);

static uint16_t background;
static texture_list_t track_images;
static menu_t *main_menu;
static int split_screen_preview_pilots[2];

static struct {
	Object *race_classes[2];
	Object *teams[4];
	Object *pilots[8];
	struct { Object *stopwatch, *save, *load, *headphones, *cd; } options;
	struct { Object *championship, *msdos, *single_race, *options; } misc;
	Object *rescue;
	Object *controller;
} models;

static void draw_model(Object *model, vec2_t offset, vec3_t pos, float rotation) {
	render_set_view(vec3(0,0,0), vec3(0, -M_PI, -M_PI));
	render_set_screen_position(offset);
	mat4_t mat = mat4_identity();
	mat4_set_translation(&mat, pos);
	mat4_set_yaw_pitch_roll(&mat, vec3(0, rotation, M_PI));
	object_draw(model, &mat);
	render_set_screen_position(vec2(0, 0));
}

// -----------------------------------------------------------------------------
// Commonly used settings choices
static const char *opts_off_on[] = {"OFF", "ON"};
static const char *opts_track_lighting[] = {"OFF", "VERTEX", "PIXEL"}; // TRACK_LIGHTING_*

// -----------------------------------------------------------------------------
// Main Menu

static void button_start_game(menu_t *menu, int) {
	page_race_type_init(menu);
}

static void button_split_screen(menu_t *menu, int) {
	page_split_screen_init(menu);
}

static void button_options(menu_t *menu, int) {
	page_options_init(menu);
}

static void button_quit_confirm(menu_t *menu, int data) {
	if (data) {
		system_exit();
	}
	else {
		menu_pop(menu);
	}
}

static void button_quit(menu_t *menu, int) {
	menu_confirm(menu, "ARE YOU SURE YOU", "WANT TO QUIT", "YES", "NO", button_quit_confirm);
}

static void page_main_draw(menu_t*, int data) {
	switch (data) {
		case 0: draw_model(g.ships[0].model, vec2(0, -0.1), vec3(0, 0, -700), system_cycle_time()); break;
		case 1:
			draw_model(g.ships[split_screen_preview_pilots[0]].model, vec2(0, -0.3), vec3(-700, -200, -1300), system_cycle_time()*1.1);
			draw_model(g.ships[split_screen_preview_pilots[1]].model, vec2(0, -0.3), vec3( 700, -200, -1300), system_cycle_time()*1.2);
			break;
		case 2: draw_model(models.misc.options, vec2(0, -0.2), vec3(0, 0, -700), system_cycle_time()); break;
		case 3: draw_model(models.misc.msdos, vec2(0, -0.2), vec3(0, 0, -700), system_cycle_time()); break;
	}
}

static void page_main_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "OPTIONS", page_main_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;

	menu_page_add_button(page, 0, "START GAME", button_start_game);
	menu_page_add_button(page, 1, "MULTIPLAYER", button_split_screen);
	menu_page_add_button(page, 2, "OPTIONS", button_options);

	#ifndef __EMSCRIPTEN__
		menu_page_add_button(page, 3, "QUIT", button_quit);
	#endif
}

// -----------------------------------------------------------------------------
// Network

static void page_network_draw(menu_t *menu, int) {
	menu_page_t *page = &menu->pages[menu->index];

	int name_col = page->items_pos.x + page->block_width - 200;
	int server_ip_col = page->items_pos.x + page->block_width - 100;
	int server_ping_col = page->items_pos.x + page->block_width;

	int line_y = page->items_pos.y - 20;

	vec2i_t col0_pos = vec2i(name_col - ui_text_width("NAME", UI_SIZE_8), line_y);
	ui_draw_text("NAME", ui_scaled_pos(page->items_anchor, col0_pos), UI_SIZE_8, UI_COLOR_DEFAULT);

	vec2i_t col1_pos = vec2i(server_ip_col - ui_text_width("IP", UI_SIZE_8), line_y);
	ui_draw_text("IP", ui_scaled_pos(page->items_anchor, col1_pos), UI_SIZE_8, UI_COLOR_DEFAULT);

	vec2i_t col2_pos = vec2i(server_ping_col - ui_text_width("PING", UI_SIZE_8), line_y);
	ui_draw_text("PING", ui_scaled_pos(page->items_anchor, col2_pos), UI_SIZE_8, UI_COLOR_DEFAULT);
	line_y += 40;

	for (unsigned int i = 0; i < server_com_get_n_servers(); i++) {
		//server_info_t *server = &server_com_get_servers()[i];

		// TODO:
		//pos.x = server_ip_col - ui_text_width(server->ip, UI_SIZE_8);
		//ui_draw_text(server->ip, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_8, text_color);

		//pos.x = server_ping_col - ui_text_width(server->ping, UI_SIZE_8);
		//char ping_str[16];
		//snprintf(ping_str, sizeof(ping_str), "%d ms", server->ping);
		///ui_draw_text(ping_str, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_8, text_color);

		line_y += 12;
	}
}

static void toggle_network_interface(menu_t*, int data) {
	save.network_interface = data;
	save.is_dirty = true;

	// TODO
	// additionally, should restart the network discovery thread
	server_com_init_network_discovery();
}

static void page_network_connected_draw(menu_t *menu, int) {
	menu_page_t *page = &menu->pages[menu->index];

	int name_col = page->items_pos.x + page->block_width - 200;
	int ip_col   = page->items_pos.x + page->block_width;
	int line_y   = page->items_pos.y - 20;

	vec2i_t hdr_name_pos = vec2i(name_col - ui_text_width("NAME", UI_SIZE_8), line_y);
	ui_draw_text("NAME", ui_scaled_pos(page->items_anchor, hdr_name_pos), UI_SIZE_8, UI_COLOR_DEFAULT);

	vec2i_t hdr_ip_pos = vec2i(ip_col - ui_text_width("IP", UI_SIZE_8), line_y);
	ui_draw_text("IP", ui_scaled_pos(page->items_anchor, hdr_ip_pos), UI_SIZE_8, UI_COLOR_DEFAULT);
	line_y += 20;

	for (unsigned int i = 0; i < server_com_get_n_connected_clients(); i++) {
		vec2i_t name_pos = vec2i(name_col - ui_text_width(server_com_get_connected_client_name(i), UI_SIZE_8), line_y);
		ui_draw_text(server_com_get_connected_client_name(i), ui_scaled_pos(page->items_anchor, name_pos), UI_SIZE_8, UI_COLOR_DEFAULT);

		// Replace '.' with 'f' — the font encodes '.' at glyph index 37 ('f' - 'A')
		char display_ip[16];
		snprintf(display_ip, sizeof(display_ip), "%s", server_com_get_connected_client_ip(i));
		for (int j = 0; display_ip[j]; j++) {
			if (display_ip[j] == '.') display_ip[j] = 'f';
		}

		vec2i_t ip_pos = vec2i(ip_col - ui_text_width(display_ip, UI_SIZE_8), line_y);
		ui_draw_text(display_ip, ui_scaled_pos(page->items_anchor, ip_pos), UI_SIZE_8, UI_COLOR_DEFAULT);
		line_y += 12;
	}
}

static void page_network_connected_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "CONNECTED", page_network_connected_draw, NULL, server_com_disconnect);

	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -50);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
}

static void page_network_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "NETWORK", page_network_draw, server_com_init_network_discovery, server_com_halt_network_discovery);
	server_com_set_menu_page(page);
	server_com_set_connect_callback(page_network_connected_init);

	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -50);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	static const char *opts_network_interfaces[] = { "LAN", "INTERNET" };

	menu_page_add_toggle(page, save.network_interface, "NETWORK INTERFACE", opts_network_interfaces, len(opts_network_interfaces), toggle_network_interface);
}


// -----------------------------------------------------------------------------
// Options

static void button_controls(menu_t *menu, int) {
	page_options_controls_init(menu);
}

static void button_video(menu_t *menu, int) {
	page_options_video_init(menu);
}

static void button_audio(menu_t *menu, int) {
	page_options_audio_init(menu);
}

static void button_game(menu_t *menu, int) {
	page_options_game_init(menu);
}

static void button_highscores(menu_t *menu, int) {
	page_options_highscores_init(menu);
}

static void page_options_draw(menu_t*, int data) {
	switch (data) {
		case 0: draw_model(models.controller, vec2(0, -0.1), vec3(0, 0, -6000), system_cycle_time()); break;
		case 1: draw_model(models.rescue, vec2(0, -0.2), vec3(0, 0, -700), system_cycle_time()); break; // TODO: needs better model
		case 2: draw_model(models.options.headphones, vec2(0, -0.2), vec3(0, 0, -300), system_cycle_time()); break;
		case 3: draw_model(models.options.stopwatch, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time()); break;
		case 4: draw_model(models.misc.single_race, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time()); break;
	}
}

static void page_options_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "OPTIONS", page_options_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	menu_page_add_button(page, 4, "GAME", button_game);
	menu_page_add_button(page, 0, "CONTROLS", button_controls);
	menu_page_add_button(page, 1, "VIDEO", button_video);
	menu_page_add_button(page, 2, "AUDIO", button_audio);
	menu_page_add_button(page, 3, "BEST TIMES", button_highscores);
}


// -----------------------------------------------------------------------------
// Options Controls

static int control_current_action;
static float await_input_deadline;

void button_capture(void *user, button_t button, int32_t) {
	if (button == INPUT_INVALID) {
		return;
	}

	menu_t *menu = (menu_t *)user;
	if (button == INPUT_KEY_ESCAPE) {
		input_capture(NULL, NULL);
		menu_pop(menu);
		return;
	}

	int index = button < INPUT_KEY_MAX ? 0 : 1; // joypad or keyboard

	// unbind this button if it's bound anywhere
	for (unsigned int i = 0; i < len(save.buttons); i++) {
		if (save.buttons[i][index] == button) {
			save.buttons[i][index] = INPUT_INVALID;
		}
	}
	input_capture(NULL, NULL);
	input_bind(INPUT_LAYER_USER, button, control_current_action);
	save.buttons[control_current_action][index] = button;
	save.is_dirty = true;
	menu_pop(menu);
}

static void page_options_control_set_draw(menu_t *menu, int) {
	float remaining = await_input_deadline - platform_now();

	menu_page_t *page = &menu->pages[menu->index];
	char remaining_text[2] = { '0' + (uint8_t)clamp(remaining + 1, 0, 3), '\0'};
	vec2i_t pos = vec2i(page->items_pos.x, page->items_pos.y + 24);
	ui_draw_text_centered(remaining_text, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_16, UI_COLOR_DEFAULT);

	if (remaining <= 0) {
		input_capture(NULL, NULL);
		menu_pop(menu);
		return;
	}
}

static void page_options_controls_set_init(menu_t *menu, int data) {
	control_current_action = data;
	await_input_deadline = platform_now() + 3;

	menu_push(menu, "AWAITING INPUT", page_options_control_set_draw, NULL, NULL);
	input_capture(button_capture, menu);
}


static void page_options_control_draw(menu_t *menu, int) {
	menu_page_t *page = &menu->pages[menu->index];

	int left = page->items_pos.x + page->block_width - 100;
	int right = page->items_pos.x + page->block_width;
	int line_y = page->items_pos.y - 20;

	vec2i_t left_head_pos = vec2i(left - ui_text_width("KEYBOARD", UI_SIZE_8), line_y);
	ui_draw_text("KEYBOARD", ui_scaled_pos(page->items_anchor, left_head_pos), UI_SIZE_8, UI_COLOR_DEFAULT);

	vec2i_t right_head_pos = vec2i(right - ui_text_width("JOYSTICK", UI_SIZE_8), line_y);
	ui_draw_text("JOYSTICK", ui_scaled_pos(page->items_anchor, right_head_pos), UI_SIZE_8, UI_COLOR_DEFAULT);
	line_y += 20;

	for (int action = 0; action < NUM_GAME_ACTIONS; action++) {
		rgba_t text_color = UI_COLOR_DEFAULT;
		if (action == page->index) {
			text_color = UI_COLOR_ACCENT;
		}

		if (save.buttons[action][0] != INPUT_INVALID) {
			const char *name = input_button_to_name(save.buttons[action][0]);
			if (!name) {
				name = "UNKNWN";
			}
			vec2i_t pos = vec2i(left - ui_text_width(name, UI_SIZE_8), line_y);
			ui_draw_text(name, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_8, text_color);
		}
		if (save.buttons[action][1] != INPUT_INVALID) {
			const char *name = input_button_to_name(save.buttons[action][1]);
			if (!name) {
				name = "UNKNWN";
			}
			vec2i_t pos = vec2i(right - ui_text_width(name, UI_SIZE_8), line_y);
			ui_draw_text(name, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_8, text_color);
		}
		line_y += 12;
	}
}

static void toggle_analog_response(menu_t *menu, int data) {
	save.analog_response = (float)data + 1;
	save.is_dirty = true;
}

static const char *analog_response[] = {"LINEAR", "MODERATE", "HEAVY"};

static void toggle_enable_force_feedback(menu_t*, int data) {
	save.enable_force_feedback = data;
	save.is_dirty = true;
	platform_set_force_feedback(save.enable_force_feedback);
}

static void page_options_controls_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "CONTROLS", page_options_control_draw, NULL, NULL);
	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -50);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	// const char *thrust_name = button_name(A_THRUST);
	// printf("thrust: %s\n", thrust_name);
	menu_page_add_button(page, A_UP, "UP", page_options_controls_set_init);
	menu_page_add_button(page, A_DOWN, "DOWN", page_options_controls_set_init);
	menu_page_add_button(page, A_LEFT, "LEFT", page_options_controls_set_init);
	menu_page_add_button(page, A_RIGHT, "RIGHT", page_options_controls_set_init);
	menu_page_add_button(page, A_BRAKE_LEFT, "BRAKE L", page_options_controls_set_init);
	menu_page_add_button(page, A_BRAKE_RIGHT, "BRAKE R", page_options_controls_set_init);
	menu_page_add_button(page, A_THRUST, "THRUST", page_options_controls_set_init);
	menu_page_add_button(page, A_FIRE, "FIRE", page_options_controls_set_init);
	menu_page_add_button(page, A_CHANGE_VIEW, "VIEW", page_options_controls_set_init);

	menu_page_add_toggle(page, save.analog_response - 1, "ANALOG RESPONSE", analog_response, len(analog_response), toggle_analog_response);
	menu_page_add_toggle(page, save.enable_force_feedback, "ENABLE FORCE FEEDBACK", opts_off_on, len(opts_off_on), toggle_enable_force_feedback);
}

// -----------------------------------------------------------------------------
// Options Video

static void toggle_fullscreen(menu_t*, int data) {
	save.fullscreen = data;
	save.is_dirty = true;
	platform_set_fullscreen(save.fullscreen);
}

static void toggle_internal_roll(menu_t*, int data) {
	save.internal_roll = (float)data * 0.1;
	save.is_dirty = true;
}

static void toggle_draw_stats(menu_t *menu, int data) {
	save.draw_stats = data;
	save.is_dirty = true;
}

static void toggle_ui_scale(menu_t*, int data) {
	save.ui_scale = data;
	save.is_dirty = true;
}

static void toggle_res(menu_t*, int data) {
	render_set_resolution(data);
	save.screen_res = data;
	save.is_dirty = true;
}

static void toggle_post(menu_t*, int data) {
	render_set_post_effect(data);
	save.post_effect = data;
	save.is_dirty = true;
}

static void toggle_screen_shake(menu_t *menu, int data) {
	save.screen_shake = (float)data * 0.5;
	save.is_dirty = true;
}

static void toggle_texture_upscale(menu_t*, int data) {
	render_set_texture_upscale(data);
	save.texture_upscale = data;
	save.is_dirty = true;
}

static const char *opts_roll[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"};
static const char *opts_ui_sizes[] = {"AUTO", "1X", "2X", "3X", "4X"};
static const char *opts_draw_stats[] = {"OFF", "FPS", "DEBUG"};
static const char *opts_res[] = {"NATIVE", "240P", "480P"};
static const char *opts_post[] = {"NONE", "CRT EFFECT"};
static const char *opts_screen_shake[] = {"DISABLED", "REDUCED", "FULL"};
static const char *opts_texture_upscale[] = {"OFF", "XBR"};

static void page_options_video_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "VIDEO OPTIONS", NULL, NULL, NULL);
	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -60);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	#ifndef __EMSCRIPTEN__
		menu_page_add_toggle(page, save.fullscreen, "FULLSCREEN", opts_off_on, len(opts_off_on), toggle_fullscreen);
	#endif
	menu_page_add_toggle(page, save.internal_roll * 10, "INTERNAL VIEW ROLL", opts_roll, len(opts_roll), toggle_internal_roll);
	menu_page_add_toggle(page, save.screen_shake * 2, "SCREEN SHAKE", opts_screen_shake, len(opts_screen_shake), toggle_screen_shake);
	menu_page_add_toggle(page, save.ui_scale, "UI SCALE", opts_ui_sizes, len(opts_ui_sizes), toggle_ui_scale);
	menu_page_add_toggle(page, save.draw_stats, "DRAW STATS", opts_draw_stats, len(opts_draw_stats), toggle_draw_stats);
	menu_page_add_toggle(page, save.screen_res, "SCREEN RESOLUTION", opts_res, len(opts_res), toggle_res);
	menu_page_add_toggle(page, save.post_effect, "POST PROCESSING", opts_post, len(opts_post), toggle_post);
	menu_page_add_toggle(page, save.texture_upscale, "TEXTURE UPSCALING", opts_texture_upscale, len(opts_texture_upscale), toggle_texture_upscale);
}

// -----------------------------------------------------------------------------
// Options Audio

static void toggle_music_volume(menu_t*, int data) {
	save.music_volume = (float)data * 0.1;
	save.is_dirty = true;
}

static void toggle_sfx_volume(menu_t*, int data) {
	save.sfx_volume = (float)data * 0.1;	
	save.is_dirty = true;
}

static const char *opts_volume[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"};

static void page_options_audio_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "AUDIO OPTIONS", NULL, NULL, NULL);

	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -80);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	menu_page_add_toggle(page, save.music_volume * 10, "MUSIC VOLUME", opts_volume, len(opts_volume), toggle_music_volume);
	menu_page_add_toggle(page, save.sfx_volume * 10, "SOUND EFFECTS VOLUME", opts_volume, len(opts_volume), toggle_sfx_volume);
}

// -----------------------------------------------------------------------------
// Options Game

static void toggle_ship_markers(menu_t*, int data) {
	save.ship_markers = data;
	save.is_dirty = true;
}

static void toggle_ship_lighting(menu_t*, int data) {
	save.ship_lighting = data;
	save.is_dirty = true;
}

static void toggle_track_lighting(menu_t*, int data) {
	save.track_lighting = data;
	save.is_dirty = true;
}

static void page_options_game_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "GAME OPTIONS", NULL, NULL, NULL);

	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -80);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	menu_page_add_toggle(page, save.ship_markers, "SHIP MARKERS", opts_off_on, len(opts_off_on), toggle_ship_markers);
	menu_page_add_toggle(page, save.ship_lighting, "SHIP LIGHTING", opts_off_on, len(opts_off_on), toggle_ship_lighting);
	menu_page_add_toggle(page, save.track_lighting, "TRACK LIGHTING", opts_track_lighting, len(opts_track_lighting), toggle_track_lighting);
}

// -----------------------------------------------------------------------------
// Options Best Times

static int options_highscores_race_class;
static int options_highscores_circuit;
static int options_highscores_tab;

static void page_options_highscores_viewer_input_handler() {
	int last_race_class_index = options_highscores_race_class;
	int last_circuit_index = options_highscores_circuit;

	if (input_pressed(A_MENU_UP)) {
		options_highscores_race_class--;
	}
	else if (input_pressed(A_MENU_DOWN)) {
		options_highscores_race_class++;
	}
	options_highscores_race_class = wrap_around(options_highscores_race_class, 0, NUM_RACE_CLASSES);

	if (input_pressed(A_MENU_LEFT)) {
		do {
			options_highscores_circuit = wrap_around(options_highscores_circuit - 1, 0, NUM_CIRCUITS);
		} while (!g.installed_circuits[options_highscores_circuit]);
	}
	else if (input_pressed(A_MENU_RIGHT)) {
		do {
			options_highscores_circuit = wrap_around(options_highscores_circuit + 1, 0, NUM_CIRCUITS);
		} while (!g.installed_circuits[options_highscores_circuit]);
	}

	if ((last_race_class_index != options_highscores_race_class) ||
		(last_circuit_index != options_highscores_circuit)) {
		sfx_play(SFX_MENU_MOVE);
	}
}

static void page_options_highscores_viewer_draw(menu_t*, int) {
	ui_pos_t anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	vec2i_t pos = vec2i(0, -70);
	ui_draw_text_centered(def.race_classes[options_highscores_race_class].name, ui_scaled_pos(anchor, pos), UI_SIZE_12, UI_COLOR_DEFAULT);
	pos.y += 16;
	ui_draw_text_centered(def.circuits[options_highscores_circuit].name, ui_scaled_pos(anchor, pos), UI_SIZE_12, UI_COLOR_ACCENT);
	
	vec2i_t entry_pos = vec2i(pos.x - 110, pos.y + 24);
	highscores_t *hs = &save.highscores[options_highscores_race_class][options_highscores_circuit][options_highscores_tab];
	for (int i = 0; i < NUM_HIGHSCORES; i++) {
		ui_draw_text(hs->entries[i].name, ui_scaled_pos(anchor, entry_pos), UI_SIZE_16, UI_COLOR_DEFAULT);
		ui_draw_time(hs->entries[i].time, ui_scaled_pos(anchor, vec2i(entry_pos.x + 110, entry_pos.y)), UI_SIZE_16, UI_COLOR_DEFAULT);
		entry_pos.y += 24;
	}

	vec2i_t lap_pos = vec2i(entry_pos.x - 40, entry_pos.y + 8);
	ui_draw_text("LAP RECORD", ui_scaled_pos(anchor, lap_pos), UI_SIZE_12, UI_COLOR_ACCENT);
	ui_draw_time(hs->lap_record, ui_scaled_pos(anchor, vec2i(lap_pos.x + 180, lap_pos.y - 4)), UI_SIZE_16, UI_COLOR_DEFAULT);

	page_options_highscores_viewer_input_handler();
}

static void page_options_highscores_viewer_init(menu_t *menu) {
	menu_page_t *page;
	if (options_highscores_tab == HIGHSCORE_TAB_TIME_TRIAL) {
		page = menu_push(menu, "BEST TIME TRIAL TIMES", page_options_highscores_viewer_draw, NULL, NULL);
	}
	else /*options_highscores_tab == HIGHSCORE_TAB_RACE)*/ {
		page = menu_push(menu, "BEST RACE TIMES", page_options_highscores_viewer_draw, NULL, NULL);
	}

	flags_add(page->layout_flags, MENU_FIXED);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->title_pos = vec2i(0, 30);
}

static void button_highscores_viewer(menu_t *menu, int data) {
	options_highscores_tab = data;
	page_options_highscores_viewer_init(menu);
}

static void page_options_highscores_draw(menu_t*, int) {
	draw_model(models.options.stopwatch, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time());
}

static void page_options_highscores_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "VIEW BEST TIMES", page_options_highscores_draw, NULL, NULL);

	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;

	options_highscores_race_class = RACE_CLASS_VENOM;
	options_highscores_circuit = CIRCUIT_ALTIMA_VII;

	menu_page_add_button(page, HIGHSCORE_TAB_TIME_TRIAL, "TIME TRIAL TIMES", button_highscores_viewer);
	menu_page_add_button(page, HIGHSCORE_TAB_RACE, "RACE TIMES", button_highscores_viewer);
}



// -----------------------------------------------------------------------------
// Racing class

static void button_race_class_select(menu_t *menu, int data) {
	if (!save.has_rapier_class && data == RACE_CLASS_RAPIER) {
		return;
	}
	g.race_class = data;
	page_team_init(menu);
}

static void page_race_class_draw(menu_t *menu, int data) {
	menu_page_t *page = &menu->pages[menu->index];
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	draw_model(models.race_classes[data], vec2(0, -0.2), vec3(0, 0, -350), system_cycle_time());

	if (!save.has_rapier_class && data == RACE_CLASS_RAPIER) {
		render_set_view_2d();
		vec2i_t pos = vec2i(page->items_pos.x, page->items_pos.y + 32);
		ui_draw_text_centered("NOT AVAILABLE", ui_scaled_pos(page->items_anchor, pos), UI_SIZE_12, UI_COLOR_ACCENT);
	}
}

static void page_race_class_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT RACING CLASS", page_race_class_draw, NULL, NULL);
	for (unsigned int i = 0; i < len(def.race_classes); i++) {
		menu_page_add_button(page, i, def.race_classes[i].name, button_race_class_select);
	}
}



// -----------------------------------------------------------------------------
// Race Type

static void button_race_type_select(menu_t *menu, int data) {
	g.local_player_count = 1;
	g.race_type = data;
	g.highscore_tab = g.race_type == RACE_TYPE_TIME_TRIAL ? HIGHSCORE_TAB_TIME_TRIAL : HIGHSCORE_TAB_RACE;

	if (g.race_type != RACE_TYPE_NETWORK) {
		page_race_class_init(menu);
	} else {
		page_network_init(menu);
	}
}

static void page_race_type_draw(menu_t*, int data) {
	switch (data) {
		case RACE_TYPE_CHAMPIONSHIP: draw_model(models.misc.championship, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time()); break;
		case RACE_TYPE_SINGLE: draw_model(models.misc.single_race, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time()); break;
		case RACE_TYPE_TIME_TRIAL: draw_model(models.options.stopwatch, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time()); break;
	}
}

static void page_race_type_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT RACE TYPE", page_race_type_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (unsigned int i = 0; i < len(def.race_types); i++) {
		menu_page_add_button(page, i, def.race_types[i].name, button_race_type_select);
	}
}



// -----------------------------------------------------------------------------
// Team

static void button_team_select(menu_t *menu, int data) {
	g.team = data;
	page_pilot_init(menu);
}

static void page_team_draw(menu_t*, int data) {
	int team_model_index = (data + 3) % 4; // models in the prm are shifted by -1
	draw_model(models.teams[team_model_index], vec2(0, -0.2), vec3(0, 0, -10000), system_cycle_time());
	draw_model(g.ships[def.teams[data].pilots[0]].model, vec2(0, -0.3), vec3(-700, -800, -1300), system_cycle_time()*1.1);
	draw_model(g.ships[def.teams[data].pilots[1]].model, vec2(0, -0.3), vec3( 700, -800, -1300), system_cycle_time()*1.2);
}

static void page_team_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT YOUR TEAM", page_team_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (unsigned int i = 0; i < len(def.teams); i++) {
		menu_page_add_button(page, i, def.teams[i].name, button_team_select);
	}
}



// -----------------------------------------------------------------------------
// Pilot

static void button_pilot_select(menu_t *menu, int data) {
	g.pilot = data;
	if (g.race_type != RACE_TYPE_CHAMPIONSHIP) {
		page_circuit_init(menu);
	}
	else {
		g.circuit = 0;
		game_reset_championship();
		game_set_scene(GAME_SCENE_RACE);
	}
}

static void page_pilot_draw(menu_t*, int data) {
	draw_model(models.pilots[def.pilots[data].logo_model], vec2(0, -0.2), vec3(0, 0, -10000), system_cycle_time());
}

static void page_pilot_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "CHOOSE YOUR PILOT", page_pilot_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (unsigned int i = 0; i < len(def.teams[g.team].pilots); i++) {
		menu_page_add_button(page, def.teams[g.team].pilots[i], def.pilots[def.teams[g.team].pilots[i]].name, button_pilot_select);
	}
}



// -----------------------------------------------------------------------------
// Split Screen
//
// A single lobby page. Player 1 moves up/down between the race settings and
// their own racer column, and changes the selected one with left/right.
// Players 2-4 join with their gamepad and cycle through the free racers in
// their column. Player 1 starts the race once at least two players are in.

enum {
	LOBBY_ROW_CIRCUIT,
	LOBBY_ROW_CLASS,
	LOBBY_ROW_TYPE,
	LOBBY_ROW_RACER,
	NUM_LOBBY_ROWS
};

static const char *opts_split_screen_types[] = {"SINGLE RACE", "CHAMPIONSHIP"};
static const int split_screen_types[] = {RACE_TYPE_SINGLE, RACE_TYPE_CHAMPIONSHIP};

static struct {
	bool joined[INPUT_MAX_PLAYERS];
	int pilot[INPUT_MAX_PLAYERS];
	int circuits[NUM_CIRCUITS];
	int circuits_len;
	int circuit_index;
	int race_classes_len;
	int race_type_index;
	int row;
} lobby;

static bool lobby_pilot_taken(int pilot, int player) {
	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		if (p != player && lobby.joined[p] && lobby.pilot[p] == pilot) {
			return true;
		}
	}
	return false;
}

// Step from pilot in direction dir to the next racer no other player has
static int lobby_next_free_pilot(int player, int pilot, int dir) {
	do {
		pilot = wrap_around(pilot + dir, 0, NUM_PILOTS);
	} while (lobby_pilot_taken(pilot, player));
	return pilot;
}

static int lobby_player_count(void) {
	int count = 0;
	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		count += lobby.joined[p];
	}
	return count;
}

static void split_screen_start(void) {
	int pilots[INPUT_MAX_PLAYERS];
	int count = 0;
	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		if (lobby.joined[p]) {
			pilots[count++] = lobby.pilot[p];
		}
	}

	g.local_player_count = count;
	g.pilot = pilots[0];
	if (count >= 2) { g.pilot2 = pilots[1]; }
	if (count >= 3) { g.pilot3 = pilots[2]; }
	if (count >= 4) { g.pilot4 = pilots[3]; }
	g.team = def.pilots[g.pilot].team;
	g.highscore_tab = HIGHSCORE_TAB_RACE;

	if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		g.circuit = 0;
		game_reset_championship();
	}
	else {
		g.circuit = lobby.circuits[lobby.circuit_index];
	}
	game_set_scene(GAME_SCENE_RACE);
}

static void page_split_screen_handle_p1_input(void) {
	if (input_pressed(A_MENU_START) && lobby_player_count() >= 2) {
		sfx_play(SFX_MENU_SELECT);
		split_screen_start();
		return;
	}

	if (input_pressed(A_MENU_UP)) {
		lobby.row = wrap_around(lobby.row - 1, 0, NUM_LOBBY_ROWS);
		sfx_play(SFX_MENU_MOVE);
	}
	else if (input_pressed(A_MENU_DOWN)) {
		lobby.row = wrap_around(lobby.row + 1, 0, NUM_LOBBY_ROWS);
		sfx_play(SFX_MENU_MOVE);
	}

	int dir = 0;
	if (input_pressed(A_MENU_LEFT)) {
		dir = -1;
	}
	else if (input_pressed(A_MENU_RIGHT) || input_pressed(A_MENU_SELECT)) {
		dir = 1;
	}
	if (!dir) {
		return;
	}

	switch (lobby.row) {
		case LOBBY_ROW_CIRCUIT:
			if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
				return; // championship always runs every circuit
			}
			lobby.circuit_index = wrap_around(lobby.circuit_index + dir, 0, lobby.circuits_len);
			break;
		case LOBBY_ROW_CLASS:
			g.race_class = wrap_around(g.race_class + dir, 0, lobby.race_classes_len);
			break;
		case LOBBY_ROW_TYPE:
			lobby.race_type_index = wrap_around(lobby.race_type_index + dir, 0, len(opts_split_screen_types));
			g.race_type = split_screen_types[lobby.race_type_index];
			break;
		case LOBBY_ROW_RACER:
			lobby.pilot[0] = lobby_next_free_pilot(0, lobby.pilot[0], dir);
			break;
	}
	sfx_play(SFX_MENU_SELECT);
}

static void page_split_screen_handle_join_input(void) {
	// Players 2-4; player 1 is always in
	for (int p = 1; p < INPUT_MAX_PLAYERS; p++) {
		if (!lobby.joined[p]) {
			if (input_pressed_p(A_MENU_SELECT, p) || input_pressed_p(A_MENU_START, p)) {
				lobby.pilot[p] = lobby_next_free_pilot(p, NUM_PILOTS - 1, 1);
				lobby.joined[p] = true;
				sfx_play(SFX_MENU_SELECT);
			}
		}
		else if (input_pressed_p(A_MENU_BACK, p)) {
			lobby.joined[p] = false;
			sfx_play(SFX_MENU_SELECT);
		}
		else if (input_pressed_p(A_MENU_LEFT, p)) {
			lobby.pilot[p] = lobby_next_free_pilot(p, lobby.pilot[p], -1);
			sfx_play(SFX_MENU_MOVE);
		}
		else if (input_pressed_p(A_MENU_RIGHT, p)) {
			lobby.pilot[p] = lobby_next_free_pilot(p, lobby.pilot[p], 1);
			sfx_play(SFX_MENU_MOVE);
		}
	}
}

static void page_split_screen_draw_setting(const char *label, const char *value, bool selected, vec2i_t pos) {
	ui_pos_t anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	ui_draw_text(label, ui_scaled_pos(anchor, pos), UI_SIZE_12, UI_COLOR_ACCENT);
	pos.x += 8;
	pos.y += 14;
	rgba_t color = (selected && blink()) ? UI_COLOR_ACCENT : UI_COLOR_DEFAULT;
	ui_draw_text(value, ui_scaled_pos(anchor, pos), UI_SIZE_8, color);
}

static void page_split_screen_draw_player(int player, vec2i_t pos) {
	ui_pos_t anchor = UI_POS_BOTTOM | UI_POS_CENTER;

	char label[16];
	snprintf(label, sizeof(label), "PLAYER %d", player + 1);
	ui_draw_text_centered(label, ui_scaled_pos(anchor, pos), UI_SIZE_8, UI_COLOR_DEFAULT);
	pos.y += 16;

	if (!lobby.joined[player]) {
		ui_draw_text_centered("PRESS START", ui_scaled_pos(anchor, pos), UI_SIZE_8, UI_COLOR_ACCENT);
		return;
	}

	// Split the pilot name into first name and surname so it fits the column
	const pilot_t *pilot = &def.pilots[lobby.pilot[player]];
	char first[32];
	snprintf(first, sizeof(first), "%s", pilot->name);
	char *rest = strchr(first, ' ');
	if (rest) {
		*rest++ = '\0';
	}

	bool selected = player == 0 && lobby.row == LOBBY_ROW_RACER;
	rgba_t color = (selected && blink()) ? UI_COLOR_ACCENT : UI_COLOR_DEFAULT;
	ui_draw_text_centered(first, ui_scaled_pos(anchor, pos), UI_SIZE_8, color);
	pos.y += 10;
	if (rest) {
		ui_draw_text_centered(rest, ui_scaled_pos(anchor, pos), UI_SIZE_8, color);
	}
	pos.y += 14;
	ui_draw_text_centered(def.teams[pilot->team].name, ui_scaled_pos(anchor, pos), UI_SIZE_8, UI_COLOR_ACCENT);
}

// Small spinning ship under the player's column. pos is the column center in
// ui units, relative to the bottom center of the screen.
static void page_split_screen_draw_ship(int player, vec2i_t pos) {
	vec2i_t screen = render_size();
	float half_w = (float)screen.x / ui_get_scale() / 2.0f;
	float half_h = (float)screen.y / ui_get_scale() / 2.0f;
	vec2_t offset = vec2(pos.x / half_w, (half_h + pos.y) / half_h);
	draw_model(g.ships[lobby.pilot[player]].model, offset, vec3(0, 0, -2000), system_cycle_time());
}

static void page_split_screen_draw(menu_t*, int) {
	page_split_screen_handle_p1_input();
	page_split_screen_handle_join_input();

	// Spread the player columns across the screen width
	int screen_width = render_size().x / ui_get_scale();
	int spacing = clamp(screen_width / INPUT_MAX_PLAYERS, 80, 150);
	vec2i_t columns[INPUT_MAX_PLAYERS];
	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		columns[p] = vec2i((p * 2 - (INPUT_MAX_PLAYERS - 1)) * spacing / 2, -116);
	}

	// 3d ships first, then switch back to 2d for everything else
	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		if (lobby.joined[p]) {
			page_split_screen_draw_ship(p, vec2i(columns[p].x, -34));
		}
	}
	render_set_view_2d();

	bool is_championship = g.race_type == RACE_TYPE_CHAMPIONSHIP;
	int circuit = is_championship ? 0 : lobby.circuits[lobby.circuit_index];

	page_split_screen_draw_setting("CIRCUIT", is_championship ? "ALL CIRCUITS" : def.circuits[circuit].name, lobby.row == LOBBY_ROW_CIRCUIT, vec2i(-150, -90));
	page_split_screen_draw_setting("CLASS", def.race_classes[g.race_class].name, lobby.row == LOBBY_ROW_CLASS, vec2i(-150, -62));
	page_split_screen_draw_setting("TYPE", opts_split_screen_types[lobby.race_type_index], lobby.row == LOBBY_ROW_TYPE, vec2i(-150, -34));

	if (circuit < track_images.len) {
		vec2i_t pos = vec2i(90, -60);
		vec2i_t size = vec2i(128, 74);
		vec2i_t scaled_pos = ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(pos.x - size.x/2, pos.y - size.y/2));
		render_push_2d(scaled_pos, ui_scaled(size), rgba(128, 128, 128, 255), texture_from_list(track_images, circuit));
	}

	vec2i_t hint_pos = ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(0, 4));
	if (lobby_player_count() >= 2) {
		ui_draw_text_centered("PRESS START TO RACE", hint_pos, UI_SIZE_8, blink() ? UI_COLOR_ACCENT : UI_COLOR_DEFAULT);
	}
	else {
		ui_draw_text_centered("WAITING FOR PLAYERS", hint_pos, UI_SIZE_8, UI_COLOR_DEFAULT);
	}

	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		page_split_screen_draw_player(p, columns[p]);
	}
}

static void page_split_screen_init(menu_t *menu) {
	for (int p = 0; p < INPUT_MAX_PLAYERS; p++) {
		lobby.joined[p] = false;
	}
	lobby.joined[0] = true;
	lobby.pilot[0] = g.pilot;
	lobby.row = LOBBY_ROW_CIRCUIT;

	lobby.circuits_len = 0;
	lobby.circuit_index = 0;
	for (int i = 0; i < NUM_CIRCUITS; i++) {
		if (
			(i < CIRCUIT_TALONS_REACH || g.installed_circuits[i]) &&
			(!def.circuits[i].is_bonus_circuit || save.has_bonus_circuits)
		) {
			if (i == g.circuit) {
				lobby.circuit_index = lobby.circuits_len;
			}
			lobby.circuits[lobby.circuits_len++] = i;
		}
	}

	lobby.race_classes_len = save.has_rapier_class ? NUM_RACE_CLASSES : 1;
	if (g.race_class >= lobby.race_classes_len) {
		g.race_class = RACE_CLASS_VENOM;
	}

	lobby.race_type_index = g.race_type == RACE_TYPE_CHAMPIONSHIP ? 1 : 0;
	g.race_type = split_screen_types[lobby.race_type_index];

	// No menu entries: all input is handled in the draw function. The menu
	// still draws the title and handles player 1's back button.
	menu_page_t *page = menu_push(menu, "SPLIT SCREEN", page_split_screen_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 20);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
}


// -----------------------------------------------------------------------------
// Circuit

static void button_circuit_select(menu_t *menu, int data) {
	g.circuit = data;
	game_set_scene(GAME_SCENE_RACE);
}

static void page_circuit_additional_draw(menu_t *menu, int data) {}

static void page_circuit_additional_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "ADDITIONAL CIRCUITS", page_circuit_additional_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, 50);
	page->items_anchor = UI_POS_TOP | UI_POS_CENTER;

	for (int i = CIRCUIT_TALONS_REACH; i < len(def.circuits); i++) {
		if (g.installed_circuits[i] &&
			(!def.circuits[i].is_bonus_circuit || save.has_bonus_circuits)) {
			menu_page_add_button(page, i, def.circuits[i].name, button_circuit_select);
		}
	}
}

static void button_circuit_additional_select(menu_t *menu, int data) {
	page_circuit_additional_init(menu);
}

static void page_circuit_draw(menu_t *menu, int data) {
	vec2i_t pos = vec2i(0, -25);
	vec2i_t size = vec2i(128, 74);
	vec2i_t scaled_size = ui_scaled(size);
	vec2i_t scaled_pos = ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(pos.x - size.x/2, pos.y - size.y/2));
	render_push_2d(scaled_pos, scaled_size, rgba(128, 128, 128, 255), texture_from_list(track_images, data));
}

static void page_circuit_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT RACING CIRCUIT", page_circuit_draw, NULL, NULL);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -100);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (int i = 0; i < CIRCUIT_TALONS_REACH; i++) {
		if (!def.circuits[i].is_bonus_circuit || save.has_bonus_circuits) {
			menu_page_add_button(page, i, def.circuits[i].name, button_circuit_select);
		}
	}
	if (g.additional_circuits) {
		menu_page_add_button(page, 0, "ADDITIONAL CIRCUITS", button_circuit_additional_select); 
	}
}

#define objects_unpack(DEST, SRC) \
	objects_unpack_imp((Object **)&DEST, sizeof(DEST)/sizeof(Object*), SRC)

static void objects_unpack_imp(Object **dest_array, int len, Object *src) {
	int i;
	for (i = 0; src && i < len; i++) {
		dest_array[i] = src;
		src = src->next;
	}
	error_if(i != len, "expected %d models got %d", len, i)
}


void main_menu_init(void) {
	g.is_attract_mode = false;
	g.local_player_count = 1;
	input_set_menu_player(0); // ensure P1 controls menus on (re)entry

	ships_reset_exhaust_plumes();

	split_screen_preview_pilots[0] = rand_int(0, NUM_PILOTS);
	split_screen_preview_pilots[1] = rand_int(0, NUM_PILOTS - 1);
	if (split_screen_preview_pilots[1] >= split_screen_preview_pilots[0]) {
		split_screen_preview_pilots[1]++; // never the same ship twice
	}

	main_menu = mem_bump(sizeof(menu_t));

	background = image_get_texture("wipeout/textures/wipeout1.tim");
	track_images = image_get_compressed_textures("wipeout/textures/track.cmp");

	objects_unpack(models.race_classes, objects_load("wipeout/common/leeg.prm", image_get_compressed_textures("wipeout/common/leeg.cmp")));
	objects_unpack(models.teams, objects_load("wipeout/common/teams.prm", texture_list_empty()));
	objects_unpack(models.pilots, objects_load("wipeout/common/pilot.prm", image_get_compressed_textures("wipeout/common/pilot.cmp")));
	objects_unpack(models.options, objects_load("wipeout/common/alopt.prm", image_get_compressed_textures("wipeout/common/alopt.cmp")));
	objects_unpack(models.rescue, objects_load("wipeout/common/rescu.prm", image_get_compressed_textures("wipeout/common/rescu.cmp")));
	objects_unpack(models.controller, objects_load("wipeout/common/pad1.prm", image_get_compressed_textures("wipeout/common/pad1.cmp")));
	objects_unpack(models.misc, objects_load("wipeout/common/msdos.prm", image_get_compressed_textures("wipeout/common/msdos.cmp")));

	menu_reset(main_menu);
	page_main_init(main_menu);
}

void main_menu_update(void) {
	render_set_view_2d();
	render_push_2d(vec2i(0, 0), render_size(), rgba(128, 128, 128, 255), background);

	menu_update(main_menu);
}

