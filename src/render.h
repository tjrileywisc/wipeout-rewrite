#ifndef RENDER_H
#define RENDER_H

#include "types.h"

typedef enum {
	RENDER_BLEND_NORMAL,
	RENDER_BLEND_LIGHTER
} render_blend_mode_t;

typedef enum {
	RENDER_RES_NATIVE,
	RENDER_RES_240P,
	RENDER_RES_480P,
} render_resolution_t;

typedef enum {
	RENDER_POST_NONE,
	RENDER_POST_CRT,
	NUM_RENDER_POST_EFFCTS,
} render_post_effect_t;

typedef struct {
	uint32_t num_tris;
	uint32_t num_draw_calls;
} render_stats_t;

typedef uint32_t render_static_buf_t;
#define RENDER_STATIC_BUF_INVALID ((render_static_buf_t)0xFFFFFFFF)

typedef struct {
	uint32_t first_vertex;
	uint32_t vertex_count;
} render_draw_range_t;

#define RENDER_USE_MIPMAPS 1

#define RENDER_FADEOUT_NEAR 48000.0
#define RENDER_FADEOUT_FAR 64000.0

extern uint16_t RENDER_NO_TEXTURE;

void render_init(vec2i_t screen_size);
void render_cleanup(void);

void render_set_screen_size(vec2i_t size);
void render_set_resolution(render_resolution_t res);
void render_set_post_effect(render_post_effect_t post);
vec2i_t render_size(void);

void render_frame_prepare(void);
void render_frame_end(void);
// render_stats_t owned by the renderer
const render_stats_t* render_frame_get_stats(void);

void render_set_viewport(vec2i_t offset, vec2i_t size);
void render_reset_viewport(void);
vec2i_t render_backbuffer_size(void);

void render_set_view(vec3_t pos, vec3_t angles);
void render_set_view_2d(void);
void render_set_model_mat(mat4_t *m);
void render_set_depth_write(bool enabled);
void render_set_depth_test(bool enabled);
void render_set_depth_offset(float offset);
void render_set_screen_position(vec2_t pos);
void render_set_blend_mode(render_blend_mode_t mode);
void render_set_cull_backface(bool enabled);

vec3_t render_transform(vec3_t pos);
void render_push_tris(tris_t tris, uint16_t texture);
void render_push_sprite(vec3_t pos, vec2i_t size, rgba_t color, uint16_t texture);
void render_push_2d(vec2i_t pos, vec2i_t size, rgba_t color, uint16_t texture);
void render_push_2d_tile(vec2i_t pos, vec2i_t uv_offset, vec2i_t uv_size, vec2i_t size, rgba_t color, uint16_t texture_index);

uint16_t render_texture_create(uint32_t width, uint32_t height, rgba_t *pixels);
vec2i_t render_texture_size(uint16_t texture_index);
void render_texture_replace_pixels(uint16_t texture_index, rgba_t *pixels);
uint16_t render_textures_len(void);
void render_textures_reset(uint16_t len);
void render_textures_dump(const char *path);

void render_bake_tris_uv(tris_t *tris, uint32_t count, uint16_t texture_index);
render_static_buf_t render_static_buf_create(const tris_t *tris, uint32_t count);
render_static_buf_t render_static_buf_create_dynamic(const tris_t *tris, uint32_t count);
void render_static_buf_update(render_static_buf_t buf, const tris_t *tris, uint32_t count);
void render_static_buf_destroy(render_static_buf_t buf);
void render_static_buf_draw(render_static_buf_t buf, render_draw_range_t range, mat4_t *model_mat);
uint32_t render_static_bufs_len(void);
void render_static_bufs_reset(uint32_t len);

#endif
