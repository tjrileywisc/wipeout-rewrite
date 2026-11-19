#include <xbr.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#define W 8
#define H 8
#define S 4

static const rgba_t black = {0, 0, 0, 255};
static const rgba_t white = {255, 255, 255, 255};
static const rgba_t red = {255, 0, 0, 255};
static const rgba_t clear_px = {0, 0, 0, 0};

static bool same(rgba_t a, rgba_t b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static rgba_t nearest(const rgba_t *src, int x, int y) {
    return src[(y / S) * W + (x / S)];
}

void xbr_scale_1_copies(void **state) {
    (void)state;
    rgba_t src[W * H], dst[W * H];
    for (int i = 0; i < W * H; i++) {
        src[i] = (rgba_t){i, i * 2, i * 3, 255};
    }
    xbr_scale(src, W, H, dst, 1);
    assert_memory_equal(src, dst, sizeof(src));
}

void xbr_flat_image_stays_flat(void **state) {
    (void)state;
    rgba_t src[W * H], dst[W * H * S * S];
    for (int i = 0; i < W * H; i++) {
        src[i] = red;
    }
    xbr_scale(src, W, H, dst, S);
    for (int i = 0; i < W * H * S * S; i++) {
        assert_true(same(dst[i], red));
    }
}

void xbr_straight_edge_is_untouched(void **state) {
    (void)state;
    rgba_t src[W * H], dst[W * H * S * S];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            src[y * W + x] = y < H / 2 ? black : white;
        }
    }
    xbr_scale(src, W, H, dst, S);
    for (int y = 0; y < H * S; y++) {
        for (int x = 0; x < W * S; x++) {
            assert_true(same(dst[y * W * S + x], nearest(src, x, y)));
        }
    }
}

void xbr_diagonal_is_smoothed(void **state) {
    (void)state;
    rgba_t src[W * H], dst[W * H * S * S];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            src[y * W + x] = x > y ? white : black;
        }
    }
    xbr_scale(src, W, H, dst, S);

    int changed = 0;
    for (int y = 0; y < H * S; y++) {
        for (int x = 0; x < W * S; x++) {
            rgba_t c = dst[y * W * S + x];
            // Only blends of black and white
            assert_true(c.r == c.g && c.g == c.b && c.a == 255);
            changed += !same(c, nearest(src, x, y));
        }
    }
    assert_true(changed > 0);
}

void xbr_is_transpose_symmetric(void **state) {
    (void)state;
    rgba_t src[W * H], dst[W * H * S * S];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            src[y * W + x] = x + y >= W ? white : black;
        }
    }
    xbr_scale(src, W, H, dst, S);

    // The source is symmetric along its diagonal, so all four corner
    // rotations must produce a symmetric result
    for (int y = 0; y < H * S; y++) {
        for (int x = 0; x < W * S; x++) {
            assert_true(same(dst[y * W * S + x], dst[x * W * S + y]));
        }
    }
}

void xbr_transparent_edge_keeps_color(void **state) {
    (void)state;
    rgba_t src[W * H], dst[W * H * S * S];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            src[y * W + x] = x > y ? red : clear_px;
        }
    }
    xbr_scale(src, W, H, dst, S);

    int partial = 0;
    for (int i = 0; i < W * H * S * S; i++) {
        if (dst[i].a > 0) {
            assert_int_equal(dst[i].r, 255);
            assert_int_equal(dst[i].g, 0);
            assert_int_equal(dst[i].b, 0);
        }
        partial += dst[i].a > 0 && dst[i].a < 255;
    }
    assert_true(partial > 0);
}
