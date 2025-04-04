/*
 * Quantize - GEGL operation for non-destructive palettization of images
 *
 * Written in 2025 by Seth Rader
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>. 
 *
 */

#ifndef COLOR_SPACE_OPS_H_
#define COLOR_SPACE_OPS_H_

struct vec4 {
    // 3 color/weight channels + padding or alpha
    float v[4];
};

typedef float (*distance_sq_t)(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
);

struct color_space_ctx {
    struct vec4 weights;
    distance_sq_t distance_sq;
    struct vec4 *palette;
    int palette_sz;
};

/*
 * Euclidean distance within color cube
 */
float rgb_distance_sq(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
);

/*
 * Combination of methods (approx. length of archimedean spiral and euclidean
 * distance) to find distance in HSL cone
 */
float hsl_distance_sq(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
);

/*
 * Find the index of the nearest palette color to the provided color
 */
int nearest_color_index(
    struct color_space_ctx *ctx,
    struct vec4 in_color
);

#endif
