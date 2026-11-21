#include "../mem.h"
#include "../utils.h"
#include "../render.h"
#include "../system.h"
#include "../platform.h"

#include "track.h"
#include "lighting.h"
#include "camera.h"
#include "game.h"

void track_load(const char *base_path) {
	// Load and assemble high res track tiles

	bool wipeout64_mode = def.circuits[g.circuit].release == GAME_WIPEOUT_64;
	g.track.textures.start = render_textures_len();
	g.track.textures.len = 0;

	ttf_t *ttf = track_load_tile_format(get_path(base_path, "library.ttf"));
	cmp_t *cmp = image_load_compressed(get_path(base_path, "library.cmp"));
	int temp_tile_size = wipeout64_mode ? 64 : 128;
	int sub_tile_size  = wipeout64_mode ? 64 : 32;
	int tiles          = wipeout64_mode ? 1  : 4;
	
	image_t *temp_tile = image_alloc(temp_tile_size, temp_tile_size);
	int len = wipeout64_mode ? cmp->len : ttf->len;
	for (int i = 0; i < len; i++) {
		for (int tx = 0; tx < tiles; tx++) {
			for (int ty = 0; ty < tiles; ty++) {
				uint32_t sub_tile_index = ttf->tiles[i].near[ty * tiles + tx];
				image_t *sub_tile = image_load_from_bytes(cmp->entries[wipeout64_mode ? i : sub_tile_index], false);
				image_copy(sub_tile, temp_tile, 0, 0, sub_tile_size, sub_tile_size, tx * sub_tile_size, ty * sub_tile_size);
				mem_temp_free(sub_tile);
			}
		}
		render_texture_create(temp_tile->width, temp_tile->height, temp_tile->pixels);
		g.track.textures.len++;
	}

	mem_temp_free(temp_tile);
	mem_temp_free(cmp);
	mem_temp_free(ttf);

	vec3_t *vertices = track_load_vertices(get_path(base_path, "track.trv"));
	track_load_faces(get_path(base_path, "track.trf"), vertices);
	mem_temp_free(vertices);

	// Wipeout 2097 .tex loading
	char *tex_path = get_path(base_path, "track.tex");
	if (file_exists(tex_path)) track_load_texture_file(tex_path);

	track_load_sections(get_path(base_path, "track.trs"));

	g.track.pickups_len = 0;
	section_t *s = g.track.sections;
	section_t *j = NULL;

	// Enumerate all sections; take care to give both stretches at a junction
	// the same numbers.
	int num = 0;
	do {
		s->num = num++;
		if (s->junction) { // start junction
			j = s->junction;
			do {
				j->num = num++;
				j = j->next;
			} while (!j->junction); // end junction
			num = s->num;
		}
		s = s->next;
	} while (s != g.track.sections);
	g.track.total_section_nums = num;
	g.track.pickups = mem_mark();
	for (int i = 0; i < g.track.section_count; i++) {
		track_face_t *face = track_section_get_base_face(&g.track.sections[i]);
		
		for (int f = 0; f < 2; f++) {
			if (flags_any(face->flags, FACE_PICKUP_RIGHT | FACE_PICKUP_LEFT)) {
				mem_bump(sizeof(track_pickup_t));
				g.track.pickups[g.track.pickups_len].face = face;
				g.track.pickups[g.track.pickups_len].cooldown_timer = 0;
				g.track.pickups_len++;
			}
			
			if (flags_is(face->flags, FACE_BOOST)) {
				track_face_set_color(face, rgba(0, 0, 255, 255));
			}
			face++;
		}
	}
}

ttf_t *track_load_tile_format(char *ttf_name) {
	uint32_t ttf_size;
	uint8_t *ttf_bytes = platform_load_asset(ttf_name, &ttf_size);

	uint32_t p = 0;
	uint32_t num_tiles = ttf_size / 42;

	ttf_t *ttf = mem_temp_alloc(sizeof(ttf_t) + sizeof(ttf_tile_t) * num_tiles);
	ttf->len = num_tiles;

	for (unsigned int t = 0; t < num_tiles; t++) {
		for (int i = 0; i < 16; i++) {
			ttf->tiles[t].near[i] = get_i16(ttf_bytes, &p);
		}
		for (int i = 0; i < 4; i++) {
			ttf->tiles[t].med[i] = get_i16(ttf_bytes, &p);
		}
		ttf->tiles[t].far = get_i16(ttf_bytes, &p);
	}
	mem_temp_free(ttf_bytes);

	return ttf;
}

bool track_collect_pickups(track_face_t *face) {
	if (flags_is(face->flags, FACE_PICKUP_ACTIVE)) {
		flags_rm(face->flags, FACE_PICKUP_ACTIVE);
		flags_add(face->flags, FACE_PICKUP_COLLECTED);
		track_face_set_color(face, rgba(255, 255, 255, 255));
		return true;
	}
	else {
		return false;
	}
}

vec3_t *track_load_vertices(char *file_name) {
	uint32_t size;
	uint8_t *bytes = platform_load_asset(file_name, &size);

	g.track.vertex_count = size / 16; // VECTOR_SIZE
	vec3_t *vertices = mem_temp_alloc(sizeof(vec3_t) * g.track.vertex_count);
	
	uint32_t p = 0;
	for (int i = 0; i < g.track.vertex_count; i++) {
		vertices[i].x = get_i32(bytes, &p);
		vertices[i].y = get_i32(bytes, &p);
		vertices[i].z = get_i32(bytes, &p);
		p += 4; // padding
	}

	mem_temp_free(bytes);
	return vertices;
}

static const vec2_t track_uv[2][4] = {
	{{128, 0}, {  0, 0}, {  0, 128}, {128, 128}},
	{{  0, 0}, {128, 0}, {128, 128}, {  0, 128}}
};

static const vec2_t wipeout64_track_uv[2][4] = {
	{{64, 0}, {  0, 0}, {  0, 64}, {64, 64}},
	{{  0, 0}, {64, 0}, {64, 64}, {  0, 64}}
};

void track_load_faces(char *file_name, vec3_t *vertices) {
	uint32_t size;
	uint8_t *bytes = platform_load_asset(file_name, &size);

	g.track.face_count = size / 20; // TRACK_FACE_DATA_SIZE
	g.track.faces = mem_bump(sizeof(track_face_t) * g.track.face_count);

	uint32_t p = 0;
	track_face_t *tf = g.track.faces;

	
	for (int i = 0; i < g.track.face_count; i++) {

		vec3_t v0 = vertices[get_i16(bytes, &p)];
		vec3_t v1 = vertices[get_i16(bytes, &p)];
		vec3_t v2 = vertices[get_i16(bytes, &p)];
		vec3_t v3 = vertices[get_i16(bytes, &p)];
		tf->normal.x = (float)get_i16(bytes, &p) / 4096.0;
		tf->normal.y = (float)get_i16(bytes, &p) / 4096.0;
		tf->normal.z = (float)get_i16(bytes, &p) / 4096.0;

		tf->texture = get_i8(bytes, &p);
		tf->flags = get_i8(bytes, &p);

		const vec2_t *uv;
		rgba_t color = rgba_from_u32(get_u32(bytes, &p));
		if (def.circuits[g.circuit].release == GAME_WIPEOUT_64) {
			uv = wipeout64_track_uv[flags_is(tf->flags, FACE_FLIP_TEXTURE) ? 1 : 0];
		} else {
			uv = track_uv[flags_is(tf->flags, FACE_FLIP_TEXTURE) ? 1 : 0];
		}
		tf->tris[0] = (tris_t){
			.vertices = {
				{.pos = v0, .uv = uv[0], .color = color},
				{.pos = v1, .uv = uv[1], .color = color},
				{.pos = v2, .uv = uv[2], .color = color},
			}
		};
		tf->tris[1] = (tris_t){
			.vertices = {
				{.pos = v3, .uv = uv[3], .color = color},
				{.pos = v0, .uv = uv[0], .color = color},
				{.pos = v2, .uv = uv[2], .color = color},
			}
		};

		tf++;
	}

	mem_temp_free(bytes);
}

void track_load_texture_file(char *tex_path) {
	int size;
	uint8_t *bytes = platform_load_asset(tex_path, &size);
	uint32_t p = 0;
	for (int i = 0; i < g.track.face_count; i++) {
		g.track.faces[i].texture = get_u8(bytes, &p);
		g.track.faces[i].flags = get_u8(bytes, &p);
	}
	mem_temp_free(bytes);
}

void track_load_sections(char *file_name) {
	uint32_t size;
	uint8_t *bytes = platform_load_asset(file_name, &size);

	g.track.section_count = size / 156; // SECTION_DATA_SIZE
	g.track.sections = mem_bump(sizeof(section_t) * g.track.section_count);

	uint32_t p = 0;
	section_t *ts = g.track.sections;
	for (int i = 0; i < g.track.section_count; i++) {
		int32_t junction_index = get_i32(bytes, &p);
		if (junction_index != -1) {
			ts->junction = g.track.sections + junction_index;
		}
		else {
			ts->junction = NULL;
		}

		ts->prev = g.track.sections + get_i32(bytes, &p);
		ts->next = g.track.sections + get_i32(bytes, &p);

		ts->center.x = get_i32(bytes, &p);
		ts->center.y = get_i32(bytes, &p);
		ts->center.z = get_i32(bytes, &p);

		int16_t version = get_i16(bytes, &p);
		error_if(version != TRACK_VERSION, "Convert track with track10: section: %d Track: %d\n", version, TRACK_VERSION);
		p += 2; // padding

		p += 4 + 4; // objects pointer, objectCount
		p += 5 * 3 * 4; // view section pointers
		p += 5 * 3 * 2; // view section counts

		p += 4 * 2; // high list
		p += 4 * 2; // med list

		ts->face_start = get_i16(bytes, &p);
		ts->face_count = get_i16(bytes, &p);

		// global/local radius
		// Seems to consistently be larger than the section radius calculated below,
		// but by a varying factor. Includes props?
		p += 2 * 2;

		ts->flags = get_i16(bytes, &p);
		ts->num = get_i16(bytes, &p);
		p += 2; // padding

		// Get bounding sphere of section
		track_face_t *face = g.track.faces + ts->face_start;
		for (uint32_t j = 0; j < ts->face_count; j++) {
			for (uint32_t k = 0; k < 2; k++) {
				for (uint32_t o = 0; o < 3; o++) {
					ts->radius = max(ts->radius, vec3_len_sq(vec3_sub(face->tris[k].vertices[o].pos, ts->center)));
				}
			}
			face++;
		}
		ts->radius = sqrt(ts->radius);

		ts++;
	}

	mem_temp_free(bytes);
}




void track_draw_section(section_t *section) {
	track_face_t *face = g.track.faces + section->face_start;
	uint16_t face_count = section->face_count;
	
	for (uint32_t j = 0; j < face_count; j++) {
		uint16_t tex_index = texture_from_list(g.track.textures, face->texture);
		render_push_tris(face->tris[0], tex_index);
		render_push_tris(face->tris[1], tex_index);
		face++;
	}
}

// Whether a section is (roughly) in front of the camera. Ideally we'd want to
// do a full frustum culling here. FIXME.
static bool track_section_is_visible(section_t *s, vec3_t cam_pos, vec3_t cam_dir) {
	vec3_t diff = vec3_sub(s->center, cam_pos);
	float cam_dot = vec3_dot(diff, cam_dir);
	return
		cam_dot > -s->radius &&
		cam_dot < (RENDER_FADEOUT_FAR * RENDER_FADEOUT_FAR) + s->radius;
}

// Dynamic lighting (see lighting.h): sections that a light reaches are drawn
// with the lights added to their baked vertex colors.
// Boost and weapon pads light what's around them, the track and ships alike.
#define TRACK_SECTION_LIGHTS_MAX 24

static const vec3_t boost_pad_light_color = {0.35f, 0.75f, 2.20f};
#define PAD_LIGHT_INTENSITY 1.8
#define PAD_LIGHT_RADIUS    1600.0
#define PAD_LIGHT_HEIGHT    100.0 // above the pad, so it's not in a ship's plane when on the pad

int track_pad_lights(section_t *section, int back, int ahead, light_t *out, int out_max) {
	int out_len = 0;
	section_t *s = section;
	int count = 1 + ahead;
	for (int i = 0; i < back && s->prev; i++, count++) {
		s = s->prev;
	}
	for (int i = 0; i < count && s; i++, s = s->next) {
		// Pads are only ever on the two base faces of a section
		track_face_t *face = track_section_get_base_face(s);
		for (int f = 0; f < 2; f++, face++) {
			vec3_t color;
			if (flags_is(face->flags, FACE_BOOST)) {
				color = boost_pad_light_color;
			}
			else if (flags_is(face->flags, FACE_PICKUP_ACTIVE)) {
				// Weapon pads cycle through colors; light in the current one
				rgba_t c = face->tris[0].vertices[0].color;
				color = vec3_mulf(vec3(c.r, c.g, c.b), PAD_LIGHT_INTENSITY / 255.0);
			}
			else {
				continue;
			}
			if (out_len >= out_max) {
				return out_len;
			}
			vec3_t center = vec3_mulf(vec3_add(
				vec3_add(face->tris[0].vertices[0].pos, face->tris[0].vertices[1].pos),
				vec3_add(face->tris[0].vertices[2].pos, face->tris[1].vertices[0].pos)
			), 0.25);
			out[out_len++] = (light_t){
				.pos = vec3_add(center, vec3_mulf(face->normal, PAD_LIGHT_HEIGHT)),
				.color = color,
				.radius = PAD_LIGHT_RADIUS,
				.owner = NULL
			};
		}
	}
	return out_len;
}

static void track_draw_section_lit(section_t *section, const light_t *local, int local_len) {
	track_face_t *face = g.track.faces + section->face_start;
	for (uint32_t j = 0; j < section->face_count; j++, face++) {
		uint16_t tex_index = texture_from_list(g.track.textures, face->texture);
		// The pads glow by themselves; more light would only wash them out
		if (flags_any(face->flags, FACE_BOOST | FACE_PICKUP_ACTIVE)) {
			render_push_tris(face->tris[0], tex_index);
			render_push_tris(face->tris[1], tex_index);
			continue;
		}
		for (int t = 0; t < 2; t++) {
			tris_t tris = face->tris[t];
			lighting_track_tris(&tris, face->normal, local, local_len);
			render_push_tris(tris, tex_index);
		}
	}
}

void track_draw(camera_t *camera) {
	render_set_model_mat(&mat4_identity());

	vec3_t cam_pos = camera->position;
	vec3_t cam_dir = camera_forward(camera);
	bool lit = save.track_lighting;

	for(int32_t i = 0; i < g.track.section_count; i++) {
		section_t *s = &g.track.sections[i];
		if (!track_section_is_visible(s, cam_pos, cam_dir)) {
			continue;
		}
		light_t local[TRACK_SECTION_LIGHTS_MAX];
		int local_len = 0;
		if (lit) {
			light_t pads[6];
			int pads_len = track_pad_lights(s, 1, 1, pads, len(pads));
			local_len = lights_gather(s->center, s->radius, NULL, pads, pads_len, local, TRACK_SECTION_LIGHTS_MAX);
		}
		if (local_len > 0) {
			track_draw_section_lit(s, local, local_len);
		}
		else {
			track_draw_section(s);
		}
	}
}

// Pad glow: boost pads and active weapon pads are drawn again, faintly, into
// the renderer's glow target, in their own texture and color. The pads lie on
// the track surface, so they are moved towards the camera by a small fixed
// distance to pass the depth test against it.
#define TRACK_PAD_GLOW_ALPHA      70
#define TRACK_PAD_GLOW_DEPTH_BIAS 4.0

static void track_draw_face_glow(track_face_t *face, vec3_t cam_pos) {
	uint16_t tex_index = texture_from_list(g.track.textures, face->texture);
	for (int t = 0; t < 2; t++) {
		tris_t tris = face->tris[t];
		for (int v = 0; v < 3; v++) {
			vec3_t to_cam = vec3_sub(cam_pos, tris.vertices[v].pos);
			float dist = vec3_len(to_cam);
			if (dist > 1) {
				tris.vertices[v].pos = vec3_add(tris.vertices[v].pos, vec3_mulf(to_cam, TRACK_PAD_GLOW_DEPTH_BIAS / dist));
			}
			tris.vertices[v].color.a = TRACK_PAD_GLOW_ALPHA;
		}
		render_push_tris(tris, tex_index);
	}
}

void track_draw_pad_glow(camera_t *camera) {
	vec3_t cam_pos = camera->position;
	vec3_t cam_dir = camera_forward(camera);

	render_set_model_mat(&mat4_identity());
	if (!render_set_glow_target(true)) {
		return;
	}
	render_set_depth_write(false);
	render_set_cull_backface(false);
	render_set_blend_mode(RENDER_BLEND_LIGHTER);

	for (int32_t i = 0; i < g.track.section_count; i++) {
		section_t *s = &g.track.sections[i];
		if (!track_section_is_visible(s, cam_pos, cam_dir)) {
			continue;
		}
		// Pads are only ever on the two base faces of a section
		track_face_t *face = track_section_get_base_face(s);
		for (int f = 0; f < 2; f++, face++) {
			if (flags_any(face->flags, FACE_BOOST | FACE_PICKUP_ACTIVE)) {
				track_draw_face_glow(face, cam_pos);
			}
		}
	}

	render_set_blend_mode(RENDER_BLEND_NORMAL);
	render_set_cull_backface(true);
	render_set_depth_write(true);
	render_set_glow_target(false);
}

void track_cycle_pickups(void) {
	float pickup_cycle_time = 1.5 * system_cycle_time();

	for (int i = 0; i < g.track.pickups_len; i++) {
		if (flags_is(g.track.pickups[i].face->flags, FACE_PICKUP_COLLECTED)) {
			flags_rm(g.track.pickups[i].face->flags, FACE_PICKUP_COLLECTED);
			g.track.pickups[i].cooldown_timer = TRACK_PICKUP_COOLDOWN_TIME;
		}
		else if (g.track.pickups[i].cooldown_timer <= 0) {
			flags_add(g.track.pickups[i].face->flags, FACE_PICKUP_ACTIVE);
			track_face_set_color(g.track.pickups[i].face, rgba(
				sinf( pickup_cycle_time + i) * 127 + 128,
				cosf( pickup_cycle_time + i) * 127 + 128,
				sinf(-pickup_cycle_time - i) * 127 + 128,
				255
			));
		}
		else{
			g.track.pickups[i].cooldown_timer -= system_tick();
		}
	}
}

void track_face_set_color(track_face_t *face, rgba_t color) {
	face->tris[0].vertices[0].color = color;
	face->tris[0].vertices[1].color = color;
	face->tris[0].vertices[2].color = color;

	face->tris[1].vertices[0].color = color;
	face->tris[1].vertices[1].color = color;
	face->tris[1].vertices[2].color = color;
}

track_face_t *track_section_get_base_face(section_t *section) {
	track_face_t *face = g.track.faces +section->face_start;
	while(flags_not(face->flags, FACE_TRACK_BASE)) {
		face++;
	}
	return face;
}

section_t *track_nearest_section(vec3_t pos, vec3_t bias, section_t *section, float *distance) {
	// Start search several sections before current section

	for (int i = 0; i < TRACK_SEARCH_LOOK_BACK; i++) {
		section = section->prev;
	}

	// Find vector from ship center to track section under
	// consideration
	float shortest_distance = 1000000000.0;
	section_t *nearest_section = section;
	section_t *junction = NULL;
	for (int i = 0; i < TRACK_SEARCH_LOOK_AHEAD; i++) {
		if (section->junction) {
			junction = section->junction;
		}

		// Some callers of this function want to de-emphasize the .y component
		// of the difference, hence the multiplication with the bias vector.
		// For the real, exact difference bias should be vec3(1,1,1)
		float d = vec3_len(vec3_mul(vec3_sub(pos, section->center), bias));
		if (d < shortest_distance) {
			shortest_distance = d;
			nearest_section = section;
		}

		section = section->next;
	}

	if (junction) {
		section = junction;
		for (int i = 0; i < TRACK_SEARCH_LOOK_AHEAD; i++) {
			float d = vec3_len(vec3_mul(vec3_sub(pos, section->center), bias));
			if (d < shortest_distance) {
				shortest_distance = d;
				nearest_section = section;
			}

			if (flags_is(junction->flags, SECTION_JUNCTION_START)) {
				section = section->next;
			}
			else {
				section = section->prev;
			}
		}
	}

	if (distance != NULL) {
		*distance = shortest_distance;
	}
	return nearest_section;
}
