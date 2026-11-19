#ifndef XBR_H
#define XBR_H

#include "types.h"

#define XBR_MAX_SCALE 8

// Upscale an image by an integer factor using Hyllian's xBR (level 2) edge
// detection. dst must hold (width * scale) * (height * scale) pixels.
void xbr_scale(const rgba_t *src, uint32_t width, uint32_t height, rgba_t *dst, uint32_t scale);

#endif
