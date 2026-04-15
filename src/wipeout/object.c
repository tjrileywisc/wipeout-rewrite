#include "../types.h"
#include "../mem.h"
#include "../render.h"
#include "../utils.h"
#include "../platform.h"

#include "object.h"
#include "track.h"
#include "ship.h"
#include "weapon.h"
#include "droid.h"
#include "camera.h"
#include "object.h"
#include "scene.h"
#include "hud.h"
#include "object.h"

Object *objects_load(char *name, texture_list_t tl) {
	uint32_t length = 0;
	uint8_t *bytes = platform_load_asset(name, &length);
	if (!bytes) {
		die("Failed to load file %s\n", name);
	}
	printf("load: %s\n", name);

	Object *objectList = mem_mark();
	Object *prevObject = NULL;
	uint32_t p = 0;

	while (p < length) {
		Object *object = mem_bump(sizeof(Object));
		if (prevObject) {
			prevObject->next = object;
		}
		prevObject = object;

		for (int i = 0; i < 16; i++) {
			object->name[i] = get_i8(bytes, &p);
		}
		
		object->mat = mat4_identity();
		object->static_vbo = RENDER_STATIC_BUF_INVALID;
		object->static_vbo_count = 0;
		object->has_sprites = false;
		object->skip_static_bake = false;
		object->vertices_len = get_i16(bytes, &p); p += 2;
		object->vertices = NULL; get_i32(bytes, &p);
		object->normals_len = get_i16(bytes, &p); p += 2;
		object->normals = NULL; get_i32(bytes, &p);
		object->primitives_len = get_i16(bytes, &p); p += 2;
		object->primitives = NULL; get_i32(bytes, &p);
		get_i32(bytes, &p);
		get_i32(bytes, &p);
		get_i32(bytes, &p); // Skeleton ref
		object->extent = get_i32(bytes, &p);
		object->flags = get_i16(bytes, &p); p += 2;
		object->next = NULL; get_i32(bytes, &p);

		p += 3 * 3 * 2; // relative rot matrix
		p += 2; // padding

		object->origin.x = get_i32(bytes, &p);
		object->origin.y = get_i32(bytes, &p);
		object->origin.z = get_i32(bytes, &p);

		p += 3 * 3 * 2; // absolute rot matrix
		p += 2; // padding
		p += 3 * 4; // absolute translation matrix
		p += 2; // skeleton update flag
		p += 2; // padding
		p += 4; // skeleton super
		p += 4; // skeleton sub
		p += 4; // skeleton next

		object->radius = 0;
		object->vertices = mem_bump(object->vertices_len * sizeof(vec3_t));
		for (int i = 0; i < object->vertices_len; i++) {
			object->vertices[i].x = get_i16(bytes, &p);
			object->vertices[i].y = get_i16(bytes, &p);
			object->vertices[i].z = get_i16(bytes, &p);
			p += 2; // padding

			if (fabsf(object->vertices[i].x) > object->radius) {
				object->radius = fabsf(object->vertices[i].x);
			}
			if (fabsf(object->vertices[i].y) > object->radius) {
				object->radius = fabsf(object->vertices[i].y);
			}
			if (fabsf(object->vertices[i].z) > object->radius) {
				object->radius = fabsf(object->vertices[i].z);
			}
		}



		object->normals = mem_bump(object->normals_len * sizeof(vec3_t));
		for (int i = 0; i < object->normals_len; i++) {
			object->normals[i].x = get_i16(bytes, &p);
			object->normals[i].y = get_i16(bytes, &p);
			object->normals[i].z = get_i16(bytes, &p);
			p += 2; // padding
		}

		object->primitives = mem_mark();
		for (int i = 0; i < object->primitives_len; i++) {
			Prm prm;
			int16_t prm_type = get_i16(bytes, &p);
			int16_t prm_flag = get_i16(bytes, &p);

			switch (prm_type) {
			case PRM_TYPE_F3:
				prm.ptr = mem_bump(sizeof(F3));
				prm.f3->coords[0] = get_i16(bytes, &p);
				prm.f3->coords[1] = get_i16(bytes, &p);
				prm.f3->coords[2] = get_i16(bytes, &p);
				prm.f3->pad1 = get_i16(bytes, &p);
				prm.f3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_F4:
				prm.ptr = mem_bump(sizeof(F4));
				prm.f4->coords[0] = get_i16(bytes, &p);
				prm.f4->coords[1] = get_i16(bytes, &p);
				prm.f4->coords[2] = get_i16(bytes, &p);
				prm.f4->coords[3] = get_i16(bytes, &p);
				prm.f4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_FT3:
				prm.ptr = mem_bump(sizeof(FT3));
				prm.ft3->coords[0] = get_i16(bytes, &p);
				prm.ft3->coords[1] = get_i16(bytes, &p);
				prm.ft3->coords[2] = get_i16(bytes, &p);

				prm.ft3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.ft3->cba = get_i16(bytes, &p);
				prm.ft3->tsb = get_i16(bytes, &p);
				prm.ft3->u0 = get_i8(bytes, &p);
				prm.ft3->v0 = get_i8(bytes, &p);
				prm.ft3->u1 = get_i8(bytes, &p);
				prm.ft3->v1 = get_i8(bytes, &p);
				prm.ft3->u2 = get_i8(bytes, &p);
				prm.ft3->v2 = get_i8(bytes, &p);

				prm.ft3->pad1 = get_i16(bytes, &p);
				prm.ft3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_FT4:
				prm.ptr = mem_bump(sizeof(FT4));
				prm.ft4->coords[0] = get_i16(bytes, &p);
				prm.ft4->coords[1] = get_i16(bytes, &p);
				prm.ft4->coords[2] = get_i16(bytes, &p);
				prm.ft4->coords[3] = get_i16(bytes, &p);

				prm.ft4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.ft4->cba = get_i16(bytes, &p);
				prm.ft4->tsb = get_i16(bytes, &p);
				prm.ft4->u0 = get_i8(bytes, &p);
				prm.ft4->v0 = get_i8(bytes, &p);
				prm.ft4->u1 = get_i8(bytes, &p);
				prm.ft4->v1 = get_i8(bytes, &p);
				prm.ft4->u2 = get_i8(bytes, &p);
				prm.ft4->v2 = get_i8(bytes, &p);
				prm.ft4->u3 = get_i8(bytes, &p);
				prm.ft4->v3 = get_i8(bytes, &p);
				prm.ft4->pad1 = get_i16(bytes, &p);
				prm.ft4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_G3:
				prm.ptr = mem_bump(sizeof(G3));
				prm.g3->coords[0] = get_i16(bytes, &p);
				prm.g3->coords[1] = get_i16(bytes, &p);
				prm.g3->coords[2] = get_i16(bytes, &p);
				prm.g3->pad1 = get_i16(bytes, &p);
				prm.g3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.g3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.g3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_G4:
				prm.ptr = mem_bump(sizeof(G4));
				prm.g4->coords[0] = get_i16(bytes, &p);
				prm.g4->coords[1] = get_i16(bytes, &p);
				prm.g4->coords[2] = get_i16(bytes, &p);
				prm.g4->coords[3] = get_i16(bytes, &p);
				prm.g4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.g4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.g4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.g4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_GT3:
				prm.ptr = mem_bump(sizeof(GT3));
				prm.gt3->coords[0] = get_i16(bytes, &p);
				prm.gt3->coords[1] = get_i16(bytes, &p);
				prm.gt3->coords[2] = get_i16(bytes, &p);

				prm.gt3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.gt3->cba = get_i16(bytes, &p);
				prm.gt3->tsb = get_i16(bytes, &p);
				prm.gt3->u0 = get_i8(bytes, &p);
				prm.gt3->v0 = get_i8(bytes, &p);
				prm.gt3->u1 = get_i8(bytes, &p);
				prm.gt3->v1 = get_i8(bytes, &p);
				prm.gt3->u2 = get_i8(bytes, &p);
				prm.gt3->v2 = get_i8(bytes, &p);
				prm.gt3->pad1 = get_i16(bytes, &p);
				prm.gt3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_GT4:
				prm.ptr = mem_bump(sizeof(GT4));
				prm.gt4->coords[0] = get_i16(bytes, &p);
				prm.gt4->coords[1] = get_i16(bytes, &p);
				prm.gt4->coords[2] = get_i16(bytes, &p);
				prm.gt4->coords[3] = get_i16(bytes, &p);

				prm.gt4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.gt4->cba = get_i16(bytes, &p);
				prm.gt4->tsb = get_i16(bytes, &p);
				prm.gt4->u0 = get_i8(bytes, &p);
				prm.gt4->v0 = get_i8(bytes, &p);
				prm.gt4->u1 = get_i8(bytes, &p);
				prm.gt4->v1 = get_i8(bytes, &p);
				prm.gt4->u2 = get_i8(bytes, &p);
				prm.gt4->v2 = get_i8(bytes, &p);
				prm.gt4->u3 = get_i8(bytes, &p);
				prm.gt4->v3 = get_i8(bytes, &p);
				prm.gt4->pad1 = get_i16(bytes, &p);
				prm.gt4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;


			case PRM_TYPE_LSF3:
				prm.ptr = mem_bump(sizeof(LSF3));
				prm.lsf3->coords[0] = get_i16(bytes, &p);
				prm.lsf3->coords[1] = get_i16(bytes, &p);
				prm.lsf3->coords[2] = get_i16(bytes, &p);
				prm.lsf3->normal = get_i16(bytes, &p);
				prm.lsf3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSF4:
				prm.ptr = mem_bump(sizeof(LSF4));
				prm.lsf4->coords[0] = get_i16(bytes, &p);
				prm.lsf4->coords[1] = get_i16(bytes, &p);
				prm.lsf4->coords[2] = get_i16(bytes, &p);
				prm.lsf4->coords[3] = get_i16(bytes, &p);
				prm.lsf4->normal = get_i16(bytes, &p);
				prm.lsf4->pad1 = get_i16(bytes, &p);
				prm.lsf4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSFT3:
				prm.ptr = mem_bump(sizeof(LSFT3));
				prm.lsft3->coords[0] = get_i16(bytes, &p);
				prm.lsft3->coords[1] = get_i16(bytes, &p);
				prm.lsft3->coords[2] = get_i16(bytes, &p);
				prm.lsft3->normal = get_i16(bytes, &p);

				prm.lsft3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsft3->cba = get_i16(bytes, &p);
				prm.lsft3->tsb = get_i16(bytes, &p);
				prm.lsft3->u0 = get_i8(bytes, &p);
				prm.lsft3->v0 = get_i8(bytes, &p);
				prm.lsft3->u1 = get_i8(bytes, &p);
				prm.lsft3->v1 = get_i8(bytes, &p);
				prm.lsft3->u2 = get_i8(bytes, &p);
				prm.lsft3->v2 = get_i8(bytes, &p);
				prm.lsft3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSFT4:
				prm.ptr = mem_bump(sizeof(LSFT4));
				prm.lsft4->coords[0] = get_i16(bytes, &p);
				prm.lsft4->coords[1] = get_i16(bytes, &p);
				prm.lsft4->coords[2] = get_i16(bytes, &p);
				prm.lsft4->coords[3] = get_i16(bytes, &p);
				prm.lsft4->normal = get_i16(bytes, &p);

				prm.lsft4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsft4->cba = get_i16(bytes, &p);
				prm.lsft4->tsb = get_i16(bytes, &p);
				prm.lsft4->u0 = get_i8(bytes, &p);
				prm.lsft4->v0 = get_i8(bytes, &p);
				prm.lsft4->u1 = get_i8(bytes, &p);
				prm.lsft4->v1 = get_i8(bytes, &p);
				prm.lsft4->u2 = get_i8(bytes, &p);
				prm.lsft4->v2 = get_i8(bytes, &p);
				prm.lsft4->u3 = get_i8(bytes, &p);
				prm.lsft4->v3 = get_i8(bytes, &p);
				prm.lsft4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSG3:
				prm.ptr = mem_bump(sizeof(LSG3));
				prm.lsg3->coords[0] = get_i16(bytes, &p);
				prm.lsg3->coords[1] = get_i16(bytes, &p);
				prm.lsg3->coords[2] = get_i16(bytes, &p);
				prm.lsg3->normals[0] = get_i16(bytes, &p);
				prm.lsg3->normals[1] = get_i16(bytes, &p);
				prm.lsg3->normals[2] = get_i16(bytes, &p);
				prm.lsg3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSG4:
				prm.ptr = mem_bump(sizeof(LSG4));
				prm.lsg4->coords[0] = get_i16(bytes, &p);
				prm.lsg4->coords[1] = get_i16(bytes, &p);
				prm.lsg4->coords[2] = get_i16(bytes, &p);
				prm.lsg4->coords[3] = get_i16(bytes, &p);
				prm.lsg4->normals[0] = get_i16(bytes, &p);
				prm.lsg4->normals[1] = get_i16(bytes, &p);
				prm.lsg4->normals[2] = get_i16(bytes, &p);
				prm.lsg4->normals[3] = get_i16(bytes, &p);
				prm.lsg4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSGT3:
				prm.ptr = mem_bump(sizeof(LSGT3));
				prm.lsgt3->coords[0] = get_i16(bytes, &p);
				prm.lsgt3->coords[1] = get_i16(bytes, &p);
				prm.lsgt3->coords[2] = get_i16(bytes, &p);
				prm.lsgt3->normals[0] = get_i16(bytes, &p);
				prm.lsgt3->normals[1] = get_i16(bytes, &p);
				prm.lsgt3->normals[2] = get_i16(bytes, &p);

				prm.lsgt3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsgt3->cba = get_i16(bytes, &p);
				prm.lsgt3->tsb = get_i16(bytes, &p);
				prm.lsgt3->u0 = get_i8(bytes, &p);
				prm.lsgt3->v0 = get_i8(bytes, &p);
				prm.lsgt3->u1 = get_i8(bytes, &p);
				prm.lsgt3->v1 = get_i8(bytes, &p);
				prm.lsgt3->u2 = get_i8(bytes, &p);
				prm.lsgt3->v2 = get_i8(bytes, &p);
				prm.lsgt3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSGT4:
				prm.ptr = mem_bump(sizeof(LSGT4));
				prm.lsgt4->coords[0] = get_i16(bytes, &p);
				prm.lsgt4->coords[1] = get_i16(bytes, &p);
				prm.lsgt4->coords[2] = get_i16(bytes, &p);
				prm.lsgt4->coords[3] = get_i16(bytes, &p);
				prm.lsgt4->normals[0] = get_i16(bytes, &p);
				prm.lsgt4->normals[1] = get_i16(bytes, &p);
				prm.lsgt4->normals[2] = get_i16(bytes, &p);
				prm.lsgt4->normals[3] = get_i16(bytes, &p);

				prm.lsgt4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsgt4->cba = get_i16(bytes, &p);
				prm.lsgt4->tsb = get_i16(bytes, &p);
				prm.lsgt4->u0 = get_i8(bytes, &p);
				prm.lsgt4->v0 = get_i8(bytes, &p);
				prm.lsgt4->u1 = get_i8(bytes, &p);
				prm.lsgt4->v1 = get_i8(bytes, &p);
				prm.lsgt4->u2 = get_i8(bytes, &p);
				prm.lsgt4->v2 = get_i8(bytes, &p);
				prm.lsgt4->pad1 = get_i16(bytes, &p);
				prm.lsgt4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;


			case PRM_TYPE_TSPR:
			case PRM_TYPE_BSPR:
				prm.ptr = mem_bump(sizeof(SPR));
				prm.spr->coord = get_i16(bytes, &p);
				prm.spr->width = get_i16(bytes, &p);
				prm.spr->height = get_i16(bytes, &p);
				prm.spr->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.spr->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_SPLINE:
				prm.ptr = mem_bump(sizeof(Spline));
				prm.spline->control1.x = get_i32(bytes, &p);
				prm.spline->control1.y = get_i32(bytes, &p);
				prm.spline->control1.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spline->position.x = get_i32(bytes, &p);
				prm.spline->position.y = get_i32(bytes, &p);
				prm.spline->position.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spline->control2.x = get_i32(bytes, &p);
				prm.spline->control2.y = get_i32(bytes, &p);
				prm.spline->control2.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spline->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_POINT_LIGHT:
				prm.ptr = mem_bump(sizeof(PointLight));
				prm.pointLight->position.x = get_i32(bytes, &p);
				prm.pointLight->position.y = get_i32(bytes, &p);
				prm.pointLight->position.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.pointLight->color = rgba_from_u32(get_u32(bytes, &p));
				prm.pointLight->startFalloff = get_i16(bytes, &p);
				prm.pointLight->endFalloff = get_i16(bytes, &p);
				break;

			case PRM_TYPE_SPOT_LIGHT:
				prm.ptr = mem_bump(sizeof(SpotLight));
				prm.spotLight->position.x = get_i32(bytes, &p);
				prm.spotLight->position.y = get_i32(bytes, &p);
				prm.spotLight->position.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spotLight->direction.x = get_i16(bytes, &p);
				prm.spotLight->direction.y = get_i16(bytes, &p);
				prm.spotLight->direction.z = get_i16(bytes, &p);
				p += 2; // padding
				prm.spotLight->color = rgba_from_u32(get_u32(bytes, &p));
				prm.spotLight->startFalloff = get_i16(bytes, &p);
				prm.spotLight->endFalloff = get_i16(bytes, &p);
				prm.spotLight->coneAngle = get_i16(bytes, &p);
				prm.spotLight->spreadAngle = get_i16(bytes, &p);
				break;

			case PRM_TYPE_INFINITE_LIGHT:
				prm.ptr = mem_bump(sizeof(InfiniteLight));
				prm.infiniteLight->direction.x = get_i16(bytes, &p);
				prm.infiniteLight->direction.y = get_i16(bytes, &p);
				prm.infiniteLight->direction.z = get_i16(bytes, &p);
				p += 2; // padding
				prm.infiniteLight->color = rgba_from_u32(get_u32(bytes, &p));
				break;


			default:
				die("bad primitive type %x \n", prm_type);
			} // switch

			prm.f3->type = prm_type;
			prm.f3->flag = prm_flag;
		} // each prim
	} // each object

	mem_temp_free(bytes);
	return objectList;
}


void object_bake_vbo(Object *object) {
	if (object->skip_static_bake || object->static_vbo != RENDER_STATIC_BUF_INVALID) {
		return;
	}

	// Count how many tris this object produces (excluding sprites)
	int tris_count = 0;
	Prm poly = {.primitive = object->primitives};
	for (int i = 0; i < object->primitives_len; i++) {
		switch (poly.primitive->type) {
		case PRM_TYPE_GT3: case PRM_TYPE_FT3: case PRM_TYPE_G3:
		case PRM_TYPE_F3:  case PRM_TYPE_LSF3: case PRM_TYPE_LSFT3:
		case PRM_TYPE_LSG3: case PRM_TYPE_LSGT3:
			tris_count += 1;
			break;
		case PRM_TYPE_GT4: case PRM_TYPE_FT4: case PRM_TYPE_G4:
		case PRM_TYPE_F4:  case PRM_TYPE_LSF4: case PRM_TYPE_LSFT4:
		case PRM_TYPE_LSG4: case PRM_TYPE_LSGT4:
			tris_count += 2;
			break;
		case PRM_TYPE_TSPR: case PRM_TYPE_BSPR:
			object->has_sprites = true;
			break;
		default:
			break;
		}
		// Advance pointer using the same logic as object_draw
		switch (poly.primitive->type) {
		case PRM_TYPE_GT3:  poly.gt3++;  break;
		case PRM_TYPE_GT4:  poly.gt4++;  break;
		case PRM_TYPE_FT3:  poly.ft3++;  break;
		case PRM_TYPE_FT4:  poly.ft4++;  break;
		case PRM_TYPE_G3:   poly.g3++;   break;
		case PRM_TYPE_G4:   poly.g4++;   break;
		case PRM_TYPE_F3:   poly.f3++;   break;
		case PRM_TYPE_F4:   poly.f4++;   break;
		case PRM_TYPE_TSPR: case PRM_TYPE_BSPR: poly.spr++; break;
		case PRM_TYPE_LSF3:  poly.lsf3++;  break;
		case PRM_TYPE_LSF4:  poly.lsf4++;  break;
		case PRM_TYPE_LSFT3: poly.lsft3++; break;
		case PRM_TYPE_LSFT4: poly.lsft4++; break;
		case PRM_TYPE_LSG3:  poly.lsg3++;  break;
		case PRM_TYPE_LSG4:  poly.lsg4++;  break;
		case PRM_TYPE_LSGT3: poly.lsgt3++; break;
		case PRM_TYPE_LSGT4: poly.lsgt4++; break;
		default: break;
		}
	}

	if (tris_count == 0) {
		return;
	}

	vec3_t *vertex = object->vertices;
	tris_t *buf = mem_temp_alloc(sizeof(tris_t) * tris_count);
	int bi = 0;

	poly.primitive = object->primitives;
	for (int i = 0; i < object->primitives_len; i++) {
		int coord0, coord1, coord2, coord3;
		uint16_t tex;
		switch (poly.primitive->type) {
		case PRM_TYPE_GT3:
			coord0 = poly.gt3->coords[0];
			coord1 = poly.gt3->coords[1];
			coord2 = poly.gt3->coords[2];
			tex = poly.gt3->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.gt3->u2, poly.gt3->v2}, .color = poly.gt3->color[2]},
				{.pos = vertex[coord1], .uv = {poly.gt3->u1, poly.gt3->v1}, .color = poly.gt3->color[1]},
				{.pos = vertex[coord0], .uv = {poly.gt3->u0, poly.gt3->v0}, .color = poly.gt3->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex);
			bi++;
			poly.gt3++;
			break;

		case PRM_TYPE_GT4:
			coord0 = poly.gt4->coords[0];
			coord1 = poly.gt4->coords[1];
			coord2 = poly.gt4->coords[2];
			coord3 = poly.gt4->coords[3];
			tex = poly.gt4->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.gt4->u2, poly.gt4->v2}, .color = poly.gt4->color[2]},
				{.pos = vertex[coord1], .uv = {poly.gt4->u1, poly.gt4->v1}, .color = poly.gt4->color[1]},
				{.pos = vertex[coord0], .uv = {poly.gt4->u0, poly.gt4->v0}, .color = poly.gt4->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex);
			bi++;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.gt4->u2, poly.gt4->v2}, .color = poly.gt4->color[2]},
				{.pos = vertex[coord3], .uv = {poly.gt4->u3, poly.gt4->v3}, .color = poly.gt4->color[3]},
				{.pos = vertex[coord1], .uv = {poly.gt4->u1, poly.gt4->v1}, .color = poly.gt4->color[1]},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex);
			bi++;
			poly.gt4++;
			break;

		case PRM_TYPE_FT3:
			coord0 = poly.ft3->coords[0];
			coord1 = poly.ft3->coords[1];
			coord2 = poly.ft3->coords[2];
			tex = poly.ft3->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.ft3->u2, poly.ft3->v2}, .color = poly.ft3->color},
				{.pos = vertex[coord1], .uv = {poly.ft3->u1, poly.ft3->v1}, .color = poly.ft3->color},
				{.pos = vertex[coord0], .uv = {poly.ft3->u0, poly.ft3->v0}, .color = poly.ft3->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex);
			bi++;
			poly.ft3++;
			break;

		case PRM_TYPE_FT4:
			coord0 = poly.ft4->coords[0];
			coord1 = poly.ft4->coords[1];
			coord2 = poly.ft4->coords[2];
			coord3 = poly.ft4->coords[3];
			tex = poly.ft4->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.ft4->u2, poly.ft4->v2}, .color = poly.ft4->color},
				{.pos = vertex[coord1], .uv = {poly.ft4->u1, poly.ft4->v1}, .color = poly.ft4->color},
				{.pos = vertex[coord0], .uv = {poly.ft4->u0, poly.ft4->v0}, .color = poly.ft4->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex);
			bi++;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.ft4->u2, poly.ft4->v2}, .color = poly.ft4->color},
				{.pos = vertex[coord3], .uv = {poly.ft4->u3, poly.ft4->v3}, .color = poly.ft4->color},
				{.pos = vertex[coord1], .uv = {poly.ft4->u1, poly.ft4->v1}, .color = poly.ft4->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex);
			bi++;
			poly.ft4++;
			break;

		case PRM_TYPE_G3:
			coord0 = poly.g3->coords[0];
			coord1 = poly.g3->coords[1];
			coord2 = poly.g3->coords[2];
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.g3->color[2]},
				{.pos = vertex[coord1], .color = poly.g3->color[1]},
				{.pos = vertex[coord0], .color = poly.g3->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE);
			bi++;
			poly.g3++;
			break;

		case PRM_TYPE_G4:
			coord0 = poly.g4->coords[0];
			coord1 = poly.g4->coords[1];
			coord2 = poly.g4->coords[2];
			coord3 = poly.g4->coords[3];
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.g4->color[2]},
				{.pos = vertex[coord1], .color = poly.g4->color[1]},
				{.pos = vertex[coord0], .color = poly.g4->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE);
			bi++;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.g4->color[2]},
				{.pos = vertex[coord3], .color = poly.g4->color[3]},
				{.pos = vertex[coord1], .color = poly.g4->color[1]},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE);
			bi++;
			poly.g4++;
			break;

		case PRM_TYPE_F3:
			coord0 = poly.f3->coords[0];
			coord1 = poly.f3->coords[1];
			coord2 = poly.f3->coords[2];
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.f3->color},
				{.pos = vertex[coord1], .color = poly.f3->color},
				{.pos = vertex[coord0], .color = poly.f3->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE);
			bi++;
			poly.f3++;
			break;

		case PRM_TYPE_F4:
			coord0 = poly.f4->coords[0];
			coord1 = poly.f4->coords[1];
			coord2 = poly.f4->coords[2];
			coord3 = poly.f4->coords[3];
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.f4->color},
				{.pos = vertex[coord1], .color = poly.f4->color},
				{.pos = vertex[coord0], .color = poly.f4->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE);
			bi++;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.f4->color},
				{.pos = vertex[coord3], .color = poly.f4->color},
				{.pos = vertex[coord1], .color = poly.f4->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE);
			bi++;
			poly.f4++;
			break;

		case PRM_TYPE_LSF3:
			coord0 = poly.lsf3->coords[0]; coord1 = poly.lsf3->coords[1]; coord2 = poly.lsf3->coords[2];
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.lsf3->color},
				{.pos = vertex[coord1], .color = poly.lsf3->color},
				{.pos = vertex[coord0], .color = poly.lsf3->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE); bi++;
			poly.lsf3++; break;

		case PRM_TYPE_LSF4:
			coord0 = poly.lsf4->coords[0]; coord1 = poly.lsf4->coords[1];
			coord2 = poly.lsf4->coords[2]; coord3 = poly.lsf4->coords[3];
			buf[bi] = (tris_t){.vertices = {{.pos = vertex[coord2], .color = poly.lsf4->color}, {.pos = vertex[coord1], .color = poly.lsf4->color}, {.pos = vertex[coord0], .color = poly.lsf4->color}}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE); bi++;
			buf[bi] = (tris_t){.vertices = {{.pos = vertex[coord2], .color = poly.lsf4->color}, {.pos = vertex[coord3], .color = poly.lsf4->color}, {.pos = vertex[coord1], .color = poly.lsf4->color}}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE); bi++;
			poly.lsf4++; break;

		case PRM_TYPE_LSFT3:
			coord0 = poly.lsft3->coords[0]; coord1 = poly.lsft3->coords[1]; coord2 = poly.lsft3->coords[2];
			tex = poly.lsft3->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.lsft3->u2, poly.lsft3->v2}, .color = poly.lsft3->color},
				{.pos = vertex[coord1], .uv = {poly.lsft3->u1, poly.lsft3->v1}, .color = poly.lsft3->color},
				{.pos = vertex[coord0], .uv = {poly.lsft3->u0, poly.lsft3->v0}, .color = poly.lsft3->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex); bi++;
			poly.lsft3++; break;

		case PRM_TYPE_LSFT4:
			coord0 = poly.lsft4->coords[0]; coord1 = poly.lsft4->coords[1];
			coord2 = poly.lsft4->coords[2]; coord3 = poly.lsft4->coords[3];
			tex = poly.lsft4->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.lsft4->u2, poly.lsft4->v2}, .color = poly.lsft4->color},
				{.pos = vertex[coord1], .uv = {poly.lsft4->u1, poly.lsft4->v1}, .color = poly.lsft4->color},
				{.pos = vertex[coord0], .uv = {poly.lsft4->u0, poly.lsft4->v0}, .color = poly.lsft4->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex); bi++;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.lsft4->u2, poly.lsft4->v2}, .color = poly.lsft4->color},
				{.pos = vertex[coord3], .uv = {poly.lsft4->u3, poly.lsft4->v3}, .color = poly.lsft4->color},
				{.pos = vertex[coord1], .uv = {poly.lsft4->u1, poly.lsft4->v1}, .color = poly.lsft4->color},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex); bi++;
			poly.lsft4++; break;

		case PRM_TYPE_LSG3:
			coord0 = poly.lsg3->coords[0]; coord1 = poly.lsg3->coords[1]; coord2 = poly.lsg3->coords[2];
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .color = poly.lsg3->color[2]},
				{.pos = vertex[coord1], .color = poly.lsg3->color[1]},
				{.pos = vertex[coord0], .color = poly.lsg3->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE); bi++;
			poly.lsg3++; break;

		case PRM_TYPE_LSG4:
			coord0 = poly.lsg4->coords[0]; coord1 = poly.lsg4->coords[1];
			coord2 = poly.lsg4->coords[2]; coord3 = poly.lsg4->coords[3];
			buf[bi] = (tris_t){.vertices = {{.pos = vertex[coord2], .color = poly.lsg4->color[2]}, {.pos = vertex[coord1], .color = poly.lsg4->color[1]}, {.pos = vertex[coord0], .color = poly.lsg4->color[0]}}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE); bi++;
			buf[bi] = (tris_t){.vertices = {{.pos = vertex[coord2], .color = poly.lsg4->color[2]}, {.pos = vertex[coord3], .color = poly.lsg4->color[3]}, {.pos = vertex[coord1], .color = poly.lsg4->color[1]}}};
			render_bake_tris_uv(&buf[bi], 1, RENDER_NO_TEXTURE); bi++;
			poly.lsg4++; break;

		case PRM_TYPE_LSGT3:
			coord0 = poly.lsgt3->coords[0]; coord1 = poly.lsgt3->coords[1]; coord2 = poly.lsgt3->coords[2];
			tex = poly.lsgt3->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.lsgt3->u2, poly.lsgt3->v2}, .color = poly.lsgt3->color[2]},
				{.pos = vertex[coord1], .uv = {poly.lsgt3->u1, poly.lsgt3->v1}, .color = poly.lsgt3->color[1]},
				{.pos = vertex[coord0], .uv = {poly.lsgt3->u0, poly.lsgt3->v0}, .color = poly.lsgt3->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex); bi++;
			poly.lsgt3++; break;

		case PRM_TYPE_LSGT4:
			coord0 = poly.lsgt4->coords[0]; coord1 = poly.lsgt4->coords[1];
			coord2 = poly.lsgt4->coords[2]; coord3 = poly.lsgt4->coords[3];
			tex = poly.lsgt4->texture;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.lsgt4->u2, poly.lsgt4->v2}, .color = poly.lsgt4->color[2]},
				{.pos = vertex[coord1], .uv = {poly.lsgt4->u1, poly.lsgt4->v1}, .color = poly.lsgt4->color[1]},
				{.pos = vertex[coord0], .uv = {poly.lsgt4->u0, poly.lsgt4->v0}, .color = poly.lsgt4->color[0]},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex); bi++;
			buf[bi] = (tris_t){.vertices = {
				{.pos = vertex[coord2], .uv = {poly.lsgt4->u2, poly.lsgt4->v2}, .color = poly.lsgt4->color[2]},
				{.pos = vertex[coord3], .uv = {poly.lsgt4->u3, poly.lsgt4->v3}, .color = poly.lsgt4->color[3]},
				{.pos = vertex[coord1], .uv = {poly.lsgt4->u1, poly.lsgt4->v1}, .color = poly.lsgt4->color[1]},
			}};
			render_bake_tris_uv(&buf[bi], 1, tex); bi++;
			poly.lsgt4++; break;

		case PRM_TYPE_TSPR:
		case PRM_TYPE_BSPR:
			poly.spr++;
			break;

		default:
			break;
		}
	}

	object->static_vbo = render_static_buf_create(buf, bi);
	object->static_vbo_count = (uint32_t)bi * 3;
	mem_temp_free(buf);
}

void object_draw(Object *object, mat4_t *mat) {
	// Fast path: draw from pre-baked GPU buffer
	if (object->static_vbo != RENDER_STATIC_BUF_INVALID) {
		render_draw_range_t range = {0, object->static_vbo_count};
		render_static_buf_draw(object->static_vbo, range, mat);
		if (object->has_sprites) {
			// Sprites are view-dependent; fall through to push them dynamically
			render_set_model_mat(mat);
			Prm poly = {.primitive = object->primitives};
			vec3_t *vertex = object->vertices;
			for (int i = 0; i < object->primitives_len; i++) {
				switch (poly.primitive->type) {
				case PRM_TYPE_TSPR:
				case PRM_TYPE_BSPR:
					render_push_sprite(
						vec3(
							vertex[poly.spr->coord].x,
							vertex[poly.spr->coord].y + ((poly.primitive->type == PRM_TYPE_TSPR ? poly.spr->height : -poly.spr->height) >> 1),
							vertex[poly.spr->coord].z
						),
						vec2i(poly.spr->width, poly.spr->height),
						poly.spr->color,
						poly.spr->texture
					);
					poly.spr++;
					break;
				default:
					// Skip non-sprite primitives; advance pointer
					switch (poly.primitive->type) {
					case PRM_TYPE_GT3:  poly.gt3++;  break;
					case PRM_TYPE_GT4:  poly.gt4++;  break;
					case PRM_TYPE_FT3:  poly.ft3++;  break;
					case PRM_TYPE_FT4:  poly.ft4++;  break;
					case PRM_TYPE_G3:   poly.g3++;   break;
					case PRM_TYPE_G4:   poly.g4++;   break;
					case PRM_TYPE_F3:   poly.f3++;   break;
					case PRM_TYPE_F4:   poly.f4++;   break;
					case PRM_TYPE_LSF3:  poly.lsf3++;  break;
					case PRM_TYPE_LSF4:  poly.lsf4++;  break;
					case PRM_TYPE_LSFT3: poly.lsft3++; break;
					case PRM_TYPE_LSFT4: poly.lsft4++; break;
					case PRM_TYPE_LSG3:  poly.lsg3++;  break;
					case PRM_TYPE_LSG4:  poly.lsg4++;  break;
					case PRM_TYPE_LSGT3: poly.lsgt3++; break;
					case PRM_TYPE_LSGT4: poly.lsgt4++; break;
					default: break;
					}
					break;
				}
			}
		}
		return;
	}

	vec3_t *vertex = object->vertices;

	Prm poly = {.primitive = object->primitives};
	int primitives_len = object->primitives_len;

	render_set_model_mat(mat);

	// TODO: check for PRM_SINGLE_SIDED

	for (int i = 0; i < primitives_len; i++) {
		int coord0;
		int coord1;
		int coord2;
		int coord3;
		switch (poly.primitive->type) {
		case PRM_TYPE_GT3:
			coord0 = poly.gt3->coords[0];
			coord1 = poly.gt3->coords[1];
			coord2 = poly.gt3->coords[2];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.gt3->u2, poly.gt3->v2},
						.color = poly.gt3->color[2]
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.gt3->u1, poly.gt3->v1},
						.color = poly.gt3->color[1]
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.gt3->u0, poly.gt3->v0},
						.color = poly.gt3->color[0]
					},
				}
			}, poly.gt3->texture);

			poly.gt3 += 1;
			break;

		case PRM_TYPE_GT4:
			coord0 = poly.gt4->coords[0];
			coord1 = poly.gt4->coords[1];
			coord2 = poly.gt4->coords[2];
			coord3 = poly.gt4->coords[3];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.gt4->u2, poly.gt4->v2},
						.color = poly.gt4->color[2]
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.gt4->u1, poly.gt4->v1},
						.color = poly.gt4->color[1]
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.gt4->u0, poly.gt4->v0},
						.color = poly.gt4->color[0]
					},
				}
			}, poly.gt4->texture);
			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.gt4->u2, poly.gt4->v2},
						.color = poly.gt4->color[2]
					},
					{
						.pos = vertex[coord3],
						.uv = {poly.gt4->u3, poly.gt4->v3},
						.color = poly.gt4->color[3]
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.gt4->u1, poly.gt4->v1},
						.color = poly.gt4->color[1]
					},
				}
			}, poly.gt4->texture);

			poly.gt4 += 1;
			break;

		case PRM_TYPE_FT3:
			coord0 = poly.ft3->coords[0];
			coord1 = poly.ft3->coords[1];
			coord2 = poly.ft3->coords[2];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.ft3->u2, poly.ft3->v2},
						.color = poly.ft3->color
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.ft3->u1, poly.ft3->v1},
						.color = poly.ft3->color
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.ft3->u0, poly.ft3->v0},
						.color = poly.ft3->color
					},
				}
			}, poly.ft3->texture);

			poly.ft3 += 1;
			break;

		case PRM_TYPE_FT4:
			coord0 = poly.ft4->coords[0];
			coord1 = poly.ft4->coords[1];
			coord2 = poly.ft4->coords[2];
			coord3 = poly.ft4->coords[3];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.ft4->u2, poly.ft4->v2},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.ft4->u1, poly.ft4->v1},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.ft4->u0, poly.ft4->v0},
						.color = poly.ft4->color
					},
				}
			}, poly.ft4->texture);
			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.ft4->u2, poly.ft4->v2},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord3],
						.uv = {poly.ft4->u3, poly.ft4->v3},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.ft4->u1, poly.ft4->v1},
						.color = poly.ft4->color
					},
				}
			}, poly.ft4->texture);

			poly.ft4 += 1;
			break;

		case PRM_TYPE_G3:
			coord0 = poly.g3->coords[0];
			coord1 = poly.g3->coords[1];
			coord2 = poly.g3->coords[2];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.color = poly.g3->color[2]
					},
					{
						.pos = vertex[coord1],
						.color = poly.g3->color[1]
					},
					{
						.pos = vertex[coord0],
						.color = poly.g3->color[0]
					},
				}
			}, RENDER_NO_TEXTURE);

			poly.g3 += 1;
			break;

		case PRM_TYPE_G4:
			coord0 = poly.g4->coords[0];
			coord1 = poly.g4->coords[1];
			coord2 = poly.g4->coords[2];
			coord3 = poly.g4->coords[3];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.color = poly.g4->color[2]
					},
					{
						.pos = vertex[coord1],
						.color = poly.g4->color[1]
					},
					{
						.pos = vertex[coord0],
						.color = poly.g4->color[0]
					},
				}
			}, RENDER_NO_TEXTURE);
			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.color = poly.g4->color[2]
					},
					{
						.pos = vertex[coord3],
						.color = poly.g4->color[3]
					},
					{
						.pos = vertex[coord1],
						.color = poly.g4->color[1]
					},
				}
			}, RENDER_NO_TEXTURE);

			poly.g4 += 1;
			break;

		case PRM_TYPE_F3:
			coord0 = poly.f3->coords[0];
			coord1 = poly.f3->coords[1];
			coord2 = poly.f3->coords[2];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.color = poly.f3->color
					},
					{
						.pos = vertex[coord1],
						.color = poly.f3->color
					},
					{
						.pos = vertex[coord0],
						.color = poly.f3->color
					},
				}
			}, RENDER_NO_TEXTURE);

			poly.f3 += 1;
			break;

		case PRM_TYPE_F4:
			coord0 = poly.f4->coords[0];
			coord1 = poly.f4->coords[1];
			coord2 = poly.f4->coords[2];
			coord3 = poly.f4->coords[3];

			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.color = poly.f4->color
					},
					{
						.pos = vertex[coord1],
						.color = poly.f4->color
					},
					{
						.pos = vertex[coord0],
						.color = poly.f4->color
					},
				}
			}, RENDER_NO_TEXTURE);
			render_push_tris((tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.color = poly.f4->color
					},
					{
						.pos = vertex[coord3],
						.color = poly.f4->color
					},
					{
						.pos = vertex[coord1],
						.color = poly.f4->color
					},
				}
			}, RENDER_NO_TEXTURE);

			poly.f4 += 1;
			break;

		case PRM_TYPE_TSPR:
		case PRM_TYPE_BSPR:
			coord0 = poly.spr->coord;

			render_push_sprite(
				vec3(
					vertex[coord0].x,
					vertex[coord0].y + ((poly.primitive->type == PRM_TYPE_TSPR ? poly.spr->height : -poly.spr->height) >> 1),
					vertex[coord0].z
				),
				vec2i(poly.spr->width, poly.spr->height),
				poly.spr->color,
				poly.spr->texture
			);

			poly.spr += 1;
			break;

		default:
			break;

		}
	}
}
