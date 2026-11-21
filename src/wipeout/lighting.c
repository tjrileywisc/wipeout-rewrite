#include "../mem.h"
#include "../utils.h"

#include "lighting.h"

// The sun: a directional light, the same everywhere on the track. Model space
// has y pointing down, so "up" is -y. The ambient term lights every face, the
// sun adds to faces turned towards it. Together they keep roughly the baked
// brightness of the models on their upper faces and darken their undersides.
#define LIGHTING_AMBIENT     0.62
#define LIGHTING_SUN         0.55

// Point lights "wrap" around a model by this much: faces turned slightly away
// from a light still catch some of it. With strict N.L, a pad under a ship
// would only light its underside, which the camera hardly ever sees.
#define LIGHTING_POINT_WRAP  0.5
// The track keeps its baked lighting (no sun or ambient term) and only gets the
// point lights on top. Its faces are large, so the lights wrap a little further
// around them to make the pools wider and softer.
#define LIGHTING_TRACK_WRAP     0.3
#define LIGHTING_TRACK_STRENGTH 1.0

static const vec3_t lighting_sun_dir = {0.30, -0.90, 0.32}; // towards the sun, normalized below

// Limits for the per draw scratch buffers. Models above these are drawn unlit.
#define LIGHTING_VERTICES_MAX   512
#define LIGHTING_PRIMITIVES_MAX 512
#define LIGHTING_LOCAL_MAX      16

static light_t lights[LIGHTS_MAX];
static int lights_count;

void lights_reset(void) {
	lights_count = 0;
}

void lights_add(vec3_t pos, vec3_t color, float radius, const void *owner) {
	if (lights_count >= LIGHTS_MAX || radius <= 0) {
		return;
	}
	lights[lights_count++] = (light_t){.pos = pos, .color = color, .radius = radius, .owner = owner};
}

int lights_len(void) {
	return lights_count;
}

const light_t *lights_get(void) {
	return lights;
}

vec3_t *lighting_face_normals(Object *object) {
	vec3_t *normals = mem_bump(sizeof(vec3_t) * object->primitives_len);
	for (int i = 0; i < object->primitives_len; i++) {
		primitive_t *prm = &object->primitives[i];
		normals[i] = vec3(0, 0, 0);

		primitive_vertex_t *v;
		if (prm->type == PRM_TYPE_TRI) {
			v = prm->u.tri.v;
		}
		else if (prm->type == PRM_TYPE_QUAD) {
			v = prm->u.quad.v;
		}
		else {
			continue;
		}

		// Faces are drawn in the order v2, v1, v0, so this points out of the
		// model (checked against the ship models: all of them agree).
		vec3_t p0 = object->vertices[v[0].coord];
		vec3_t p1 = object->vertices[v[1].coord];
		vec3_t p2 = object->vertices[v[2].coord];
		vec3_t n = vec3_cross(vec3_sub(p2, p0), vec3_sub(p1, p0));
		if (vec3_len(n) > 0.0001) {
			normals[i] = vec3_normalize(n);
		}
	}
	return normals;
}

static uint8_t lighting_apply(uint8_t base, float factor) {
	float c = base * factor;
	return c > 255 ? 255 : (c < 0 ? 0 : (uint8_t)c);
}

int lights_gather(
	vec3_t center, float radius, const void *owner,
	const light_t *extra, int extra_len, light_t *out, int out_max
) {
	int out_len = 0;
	for (int src = 0; src < 2; src++) {
		const light_t *list = src == 0 ? lights : extra;
		int len = src == 0 ? lights_count : extra_len;
		for (int i = 0; i < len && out_len < out_max; i++) {
			if (owner && list[i].owner == owner) {
				continue;
			}
			float reach = list[i].radius + radius;
			if (vec3_len_sq(vec3_sub(list[i].pos, center)) < reach * reach) {
				out[out_len++] = list[i];
			}
		}
	}
	return out_len;
}

// Sum of the point lights at p, for a surface with the (unit) normal n
static vec3_t lighting_point_lights(const light_t *local, int local_len, vec3_t p, vec3_t n, float wrap) {
	vec3_t f = vec3(0, 0, 0);
	for (int l = 0; l < local_len; l++) {
		vec3_t to_light = vec3_sub(local[l].pos, p);
		float dist_sq = vec3_len_sq(to_light);
		float r = local[l].radius;
		if (dist_sq >= r * r) {
			continue;
		}
		float dist = sqrtf(dist_sq);
		float ndl = dist > 0.0001 ? vec3_dot(n, to_light) / dist : 1;
		ndl = (ndl + wrap) / (1.0 + wrap);
		if (ndl <= 0) {
			continue;
		}
		float atten = 1.0 - dist / r;
		atten *= atten;
		f = vec3_add(f, vec3_mulf(local[l].color, atten * ndl));
	}
	return f;
}

void lighting_draw_object(
	Object *object, mat4_t *mat, const vec3_t *face_normals,
	const void *owner, const light_t *extra, int extra_len, int unlit_flags
) {
	if (
		object->vertices_len > LIGHTING_VERTICES_MAX ||
		object->primitives_len > LIGHTING_PRIMITIVES_MAX
	) {
		object_draw(object, mat);
		return;
	}

	static vec3_t world_pos[LIGHTING_VERTICES_MAX];
	static rgba_t colors[LIGHTING_PRIMITIVES_MAX * 4];

	// object->radius is measured from the model space origin
	vec3_t translation = vec3_transform(vec3(0, 0, 0), mat);
	vec3_t center = translation;

	// Lights that can reach this object at all
	light_t local[LIGHTING_LOCAL_MAX];
	int local_len = lights_gather(center, object->radius, owner, extra, extra_len, local, LIGHTING_LOCAL_MAX);

	for (int i = 0; i < object->vertices_len; i++) {
		world_pos[i] = vec3_transform(object->vertices[i], mat);
	}

	vec3_t sun = vec3_normalize(lighting_sun_dir);

	for (int i = 0; i < object->primitives_len; i++) {
		primitive_t *prm = &object->primitives[i];

		primitive_vertex_t *v;
		int v_len;
		if (prm->type == PRM_TYPE_TRI) {
			v = prm->u.tri.v;
			v_len = 3;
		}
		else if (prm->type == PRM_TYPE_QUAD) {
			v = prm->u.quad.v;
			v_len = 4;
		}
		else {
			continue;
		}

		if (flags_any(prm->flag, unlit_flags)) {
			for (int j = 0; j < v_len; j++) {
				colors[i * 4 + j] = v[j].color;
			}
			continue;
		}

		// Model to world rotation of the face normal. The model matrices are
		// rotation + translation only, but normalize anyway in case of scale.
		vec3_t n = vec3_sub(vec3_transform(face_normals[i], mat), translation);
		float n_len = vec3_len(n);
		if (n_len > 0.0001) {
			n = vec3_mulf(n, 1.0 / n_len);
		}

		float sun_dot = vec3_dot(n, sun);
		float flat = LIGHTING_AMBIENT + LIGHTING_SUN * (sun_dot > 0 ? sun_dot : 0);

		for (int j = 0; j < v_len; j++) {
			vec3_t f = vec3_add(
				vec3(flat, flat, flat),
				lighting_point_lights(local, local_len, world_pos[v[j].coord], n, LIGHTING_POINT_WRAP)
			);

			rgba_t base = v[j].color;
			colors[i * 4 + j] = rgba(
				lighting_apply(base.r, f.x),
				lighting_apply(base.g, f.y),
				lighting_apply(base.b, f.z),
				base.a
			);
		}
	}

	object_draw_colored(object, mat, colors);
}

void lighting_track_tris(tris_t *tris, vec3_t normal, const light_t *local, int local_len) {
	for (int j = 0; j < 3; j++) {
		vertex_t *v = &tris->vertices[j];
		vec3_t f = vec3_add(
			vec3(1, 1, 1),
			vec3_mulf(lighting_point_lights(local, local_len, v->pos, normal, LIGHTING_TRACK_WRAP), LIGHTING_TRACK_STRENGTH)
		);
		v->color = rgba(
			lighting_apply(v->color.r, f.x),
			lighting_apply(v->color.g, f.y),
			lighting_apply(v->color.b, f.z),
			v->color.a
		);
	}
}
