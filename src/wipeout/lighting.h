#ifndef LIGHTING_H
#define LIGHTING_H

#include "../types.h"
#include "object.h"

// Dynamic lighting for models (currently the ships) and the track.
//
// The game's art has all its lighting baked into vertex colors. This adds a
// directional "sun", an ambient term and a set of point lights on top of that,
// evaluated per vertex on the CPU with a flat (per face) normal. The result is
// handed to the renderer as ordinary vertex colors, so it works the same with
// every renderer.
//
// Each frame the point lights are gathered into one list (lights_reset, then
// lights_add from the systems that emit light), and each lit model picks the
// lights that reach it when it is drawn.

#define LIGHTS_MAX 64

typedef struct light_t {
	vec3_t pos;
	vec3_t color;       // 1.0 = full strength; may go above
	float radius;       // no light beyond this distance
	const void *owner;  // a model owned by this is not lit by it (e.g. a ship's own engine)
} light_t;

void lights_reset(void);
void lights_add(vec3_t pos, vec3_t color, float radius, const void *owner);
int lights_len(void);
const light_t *lights_get(void);

// Copies the lights (from the frame's list and `extra`, minus those owned by
// `owner`) that reach a sphere at center/radius into out. Returns the count.
int lights_gather(
	vec3_t center, float radius, const void *owner,
	const light_t *extra, int extra_len, light_t *out, int out_max
);

// Per face normals of an object in model space, pointing out of the model.
// Allocated with mem_bump; computed once at load time.
vec3_t *lighting_face_normals(Object *object);

// Draws the object lit by the sun, the ambient light, the frame's light list
// (minus lights owned by `owner`) and the given extra lights. Primitives
// flagged with `unlit_flags` keep their own colors.
void lighting_draw_object(
	Object *object, mat4_t *mat, const vec3_t *face_normals,
	const void *owner, const light_t *extra, int extra_len, int unlit_flags
);

// Adds the given point lights to the (baked) vertex colors of a track triangle
// in world space, with normal as the face normal.
void lighting_track_tris(tris_t *tris, vec3_t normal, const light_t *local, int local_len);

// Track lighting modes (save.track_lighting)
#define TRACK_LIGHTING_OFF    0
#define TRACK_LIGHTING_VERTEX 1
#define TRACK_LIGHTING_PIXEL  2

// Has the renderer light the track per pixel with the frame's lights plus the
// given extra ones, from here until lighting_track_pixel_end. Returns false if
// the renderer can't; the track then has to be lit with lighting_track_tris.
bool lighting_track_pixel_begin(const light_t *extra, int extra_len);
void lighting_track_pixel_end(void);

#endif
