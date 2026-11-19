#include "xbr.h"
#include "utils.h"

// An implementation of Hyllian's xBR level 2 pixel art scaler, working at any
// integer scale.
//
// For each source pixel E and each of its four corners, the neighbourhood is
// tested for an edge running across that corner. If there is one, the output
// subpixels on the far side of the edge are blended towards the neighbouring
// color. Subpixel coverage is precomputed (with supersampling) for the three
// edge slopes xBR knows: 45° (diagonal), 30° (shallow, extending left) and 60°
// (steep, extending up).
//
// The tests are written for the bottom-right corner; the neighbourhood is
// rotated by 90° to evaluate the other three corners.
//
//          A1 B1 C1
//       A0 A  B  C  C4
//       D0 D  E  F  F4
//       G0 G  H  I  I4
//          G5 H5 I5

#define XBR_SUPERSAMPLE 8

typedef enum {
	EDGE_DIAG,
	EDGE_LEFT,
	EDGE_UP,
	EDGE_LEFT_UP,
	NUM_EDGES
} edge_t;

typedef enum {
	N_E, N_F, N_H, N_I, N_B, N_D, N_C, N_G, N_F4, N_I4, N_H5, N_I5,
	NUM_NEIGHBOURS
} neighbour_t;

static const int8_t neighbour_offsets[NUM_NEIGHBOURS][2] = {
	[N_E]  = { 0,  0}, [N_F]  = { 1, 0}, [N_H]  = {0, 1}, [N_I]  = {1, 1},
	[N_B]  = { 0, -1}, [N_D]  = {-1, 0}, [N_C]  = {1, -1}, [N_G]  = {-1, 1},
	[N_F4] = { 2,  0}, [N_I4] = { 2, 1}, [N_H5] = {0, 2}, [N_I5] = {1, 2},
};

typedef struct {
	float y, u, v, a;
} yuva_t;

static uint32_t tables_scale = 0;

// Coverage of each subpixel for the bottom-right corner, per edge type
static float coverage[NUM_EDGES][XBR_MAX_SCALE * XBR_MAX_SCALE];

// Maps an output subpixel to the bottom-right corner's subpixel, per corner
static uint8_t subpixel_map[4][XBR_MAX_SCALE * XBR_MAX_SCALE];

// Index into the 5x5 neighbourhood, per corner and neighbour
static uint8_t neighbour_map[4][NUM_NEIGHBOURS];

static void tables_init(uint32_t scale) {
	if (tables_scale == scale) {
		return;
	}

	for (uint32_t sy = 0; sy < scale; sy++) {
		for (uint32_t sx = 0; sx < scale; sx++) {
			uint32_t diag = 0, left = 0, up = 0, left_up = 0;
			for (uint32_t ssy = 0; ssy < XBR_SUPERSAMPLE; ssy++) {
				for (uint32_t ssx = 0; ssx < XBR_SUPERSAMPLE; ssx++) {
					float fx = (sx + (ssx + 0.5f) / XBR_SUPERSAMPLE) / scale;
					float fy = (sy + (ssy + 0.5f) / XBR_SUPERSAMPLE) / scale;
					bool in_diag = fx + fy > 1.5f;
					bool in_left = fy + 0.5f * fx > 1.0f;
					bool in_up = 2.0f * fx + fy > 2.0f;
					diag += in_diag;
					left += in_diag || in_left;
					up += in_diag || in_up;
					left_up += in_diag || in_left || in_up;
				}
			}
			float samples = XBR_SUPERSAMPLE * XBR_SUPERSAMPLE;
			uint32_t i = sy * scale + sx;
			coverage[EDGE_DIAG][i] = diag / samples;
			coverage[EDGE_LEFT][i] = left / samples;
			coverage[EDGE_UP][i] = up / samples;
			coverage[EDGE_LEFT_UP][i] = left_up / samples;
		}
	}

	// Corner k is the bottom-right corner rotated k * 90° clockwise. Rotating
	// an output subpixel back counter-clockwise k times finds its position
	// relative to the bottom-right corner.
	for (uint32_t k = 0; k < 4; k++) {
		for (uint32_t sy = 0; sy < scale; sy++) {
			for (uint32_t sx = 0; sx < scale; sx++) {
				uint32_t x = sx, y = sy;
				for (uint32_t r = 0; r < k; r++) {
					uint32_t t = x;
					x = y;
					y = scale - 1 - t;
				}
				subpixel_map[k][sy * scale + sx] = y * scale + x;
			}
		}

		for (uint32_t n = 0; n < NUM_NEIGHBOURS; n++) {
			int32_t x = neighbour_offsets[n][0], y = neighbour_offsets[n][1];
			for (uint32_t r = 0; r < k; r++) {
				int32_t t = x;
				x = -y;
				y = t;
			}
			neighbour_map[k][n] = (y + 2) * 5 + (x + 2);
		}
	}

	tables_scale = scale;
}

static inline yuva_t rgba_to_yuva(rgba_t c) {
	return (yuva_t){
		.y =  0.299f * c.r + 0.587f * c.g + 0.114f * c.b,
		.u = -0.169f * c.r - 0.331f * c.g + 0.500f * c.b,
		.v =  0.500f * c.r - 0.419f * c.g - 0.081f * c.b,
		.a = c.a
	};
}

static inline float df(yuva_t a, yuva_t b) {
	return
		48.0f * fabsf(a.y - b.y) +
		 7.0f * fabsf(a.u - b.u) +
		 6.0f * fabsf(a.v - b.v) +
		48.0f * fabsf(a.a - b.a);
}

// Similar colors, using the hqx thresholds
static inline bool eq(yuva_t a, yuva_t b) {
	return
		fabsf(a.y - b.y) <= 48.0f &&
		fabsf(a.u - b.u) <= 7.0f &&
		fabsf(a.v - b.v) <= 6.0f &&
		a.a == b.a;
}

static inline bool same(rgba_t a, rgba_t b) {
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// Blend in linear alpha space, so transparent pixels don't darken the edge
static inline rgba_t blend(rgba_t a, rgba_t b, float t) {
	float wa = a.a * (1.0f - t);
	float wb = b.a * t;
	float alpha = wa + wb;
	if (alpha <= 0) {
		return a;
	}
	return rgba(
		(uint8_t)((a.r * wa + b.r * wb) / alpha + 0.5f),
		(uint8_t)((a.g * wa + b.g * wb) / alpha + 0.5f),
		(uint8_t)((a.b * wa + b.b * wb) / alpha + 0.5f),
		(uint8_t)(alpha + 0.5f)
	);
}

// Tests for an edge across the bottom-right corner of E. Returns the coverage
// table to blend with and sets the neighbour to blend towards, or returns
// NULL if there is no edge.
static const float *corner_edge(const rgba_t *src, const yuva_t *yuv, const uint32_t *n, uint32_t *blend_index) {
	rgba_t re = src[n[N_E]], rf = src[n[N_F]], rh = src[n[N_H]];
	if (same(re, rf) || same(re, rh)) {
		return NULL;
	}

	yuva_t e = yuv[n[N_E]], f = yuv[n[N_F]], h = yuv[n[N_H]], i = yuv[n[N_I]];
	yuva_t b = yuv[n[N_B]], d = yuv[n[N_D]], c = yuv[n[N_C]], g = yuv[n[N_G]];
	yuva_t f4 = yuv[n[N_F4]], i4 = yuv[n[N_I4]], h5 = yuv[n[N_H5]], i5 = yuv[n[N_I5]];

	bool interp_restriction =
		(!eq(f, b) && !eq(h, d)) ||
		(eq(e, i) && !eq(f, i4) && !eq(h, i5)) ||
		eq(e, g) ||
		eq(e, c);
	if (!interp_restriction) {
		return NULL;
	}

	float across = df(e, c) + df(e, g) + df(i, h5) + df(i, f4) + 4.0f * df(h, f);
	float along  = df(h, d) + df(h, i5) + df(f, i4) + df(f, b) + 4.0f * df(e, i);
	if (!(across < along)) {
		return NULL;
	}

	*blend_index = df(e, f) <= df(e, h) ? n[N_F] : n[N_H];

	rgba_t rb = src[n[N_B]], rd = src[n[N_D]], rc = src[n[N_C]], rg = src[n[N_G]];
	float ke = df(f, g);
	float ki = df(h, c);
	bool left = 2.0f * ke <= ki && !same(re, rg) && !same(rd, rg);
	bool up = ke >= 2.0f * ki && !same(re, rc) && !same(rb, rc);

	if (left && up) {
		return coverage[EDGE_LEFT_UP];
	}
	else if (left) {
		return coverage[EDGE_LEFT];
	}
	else if (up) {
		return coverage[EDGE_UP];
	}
	return coverage[EDGE_DIAG];
}

void xbr_scale(const rgba_t *src, uint32_t width, uint32_t height, rgba_t *dst, uint32_t scale) {
	error_if(scale < 1 || scale > XBR_MAX_SCALE, "Invalid xbr scale %d", scale);

	if (scale == 1) {
		memcpy(dst, src, sizeof(rgba_t) * width * height);
		return;
	}

	tables_init(scale);

	yuva_t *yuv = malloc(sizeof(yuva_t) * width * height);
	for (uint32_t i = 0; i < width * height; i++) {
		yuv[i] = rgba_to_yuva(src[i]);
	}

	uint32_t dst_width = width * scale;
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {

			// Source indices of the 5x5 neighbourhood, clamped at the edges
			uint32_t hood[25];
			for (int32_t hy = 0; hy < 5; hy++) {
				for (int32_t hx = 0; hx < 5; hx++) {
					int32_t sx = clamp((int32_t)x + hx - 2, 0, (int32_t)width - 1);
					int32_t sy = clamp((int32_t)y + hy - 2, 0, (int32_t)height - 1);
					hood[hy * 5 + hx] = sy * width + sx;
				}
			}

			const float *corner_coverage[4];
			uint32_t corner_blend[4];
			bool has_edge = false;
			for (uint32_t k = 0; k < 4; k++) {
				uint32_t n[NUM_NEIGHBOURS];
				for (uint32_t i = 0; i < NUM_NEIGHBOURS; i++) {
					n[i] = hood[neighbour_map[k][i]];
				}
				corner_coverage[k] = corner_edge(src, yuv, n, &corner_blend[k]);
				has_edge |= corner_coverage[k] != NULL;
			}

			rgba_t e = src[y * width + x];
			rgba_t *out = dst + (y * scale) * dst_width + x * scale;
			for (uint32_t sy = 0; sy < scale; sy++) {
				for (uint32_t sx = 0; sx < scale; sx++) {
					rgba_t c = e;
					if (has_edge) {
						float best = 0;
						uint32_t best_k = 0;
						for (uint32_t k = 0; k < 4; k++) {
							if (corner_coverage[k]) {
								float cov = corner_coverage[k][subpixel_map[k][sy * scale + sx]];
								if (cov > best) {
									best = cov;
									best_k = k;
								}
							}
						}
						if (best > 0) {
							c = blend(e, src[corner_blend[best_k]], best);
						}
					}
					out[sy * dst_width + sx] = c;
				}
			}
		}
	}

	free(yuv);
}
