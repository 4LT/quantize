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

#include "color_space_ops.h"

float rgb_distance_sq(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
) {
    float d_r = (color1.v[0] - color2.v[0]) * weights.v[0];
    float d_g = (color1.v[1] - color2.v[1]) * weights.v[1];
    float d_b = (color1.v[2] - color2.v[2]) * weights.v[2];

    return d_r*d_r + d_g*d_g + d_b*d_b;
}

float hsl_distance_sq(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
) {
    /* STUB */
    return 1.f;
}

int nearest_color_index(
    struct color_space_ctx *ctx,
    struct vec4 in_color
) {
    int nearest_index = 0;
    float nearest_distance_sq = 1.f/0.f;
    float distance_sq;

    for (int idx = 0; idx < ctx->palette_sz; idx++) {
        distance_sq = ctx->distance_sq(
            in_color,
            ctx->palette[idx],
            ctx->weights
        );

        if (distance_sq < nearest_distance_sq) {
            nearest_index = idx;
            nearest_distance_sq = distance_sq;
        }
    }

    return nearest_index;
}

