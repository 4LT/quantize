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
#include <math.h>

float rgb_distance_sq(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
) {
    float d_r = (color2.v[0] - color1.v[0]) * weights.v[0];
    float d_g = (color2.v[1] - color1.v[1]) * weights.v[1];
    float d_b = (color2.v[2] - color1.v[2]) * weights.v[2];

    return d_r*d_r + d_g*d_g + d_b*d_b;
}

static float hue_difference(float hue2, float hue1) {
    float d = hue2 - hue1;
    d = d - floor(d);
    return d < 0.5f ? d : 1.f - d;
}

float lch_distance_sq(
    struct vec4 color1,
    struct vec4 color2,
    struct vec4 weights
) {
    float pi = acos(-1.0);
    float height_0 = color1.v[0]; // Lightness 1
    float height_f = color2.v[0]; // Lightness 2
    float r_0 = color1.v[1]; // Chroma 1
    float r_f = color2.v[1]; // Chroma 2
    float d_hue = hue_difference(color2.v[2], color1.v[2]);
    float r_min = r_0 < r_f ? r_0 : r_f;
    float r_max = r_0 > r_f ? r_0 : r_f;
    float d_height = height_f - height_0;
    float d_r = r_max - r_min;
    float r_mid = (r_min + r_max)/2.f;
    float d_theta = d_hue * pi * 2; // Min. diff. in hue
    
    // Note: weights are in reverse order from the colors b/c the color model
    // is LCH, but the UI matches the Hue-Chroma control in GIMP which is in
    // hue-chroma-lightness order
    float d_height_weighted = d_height * weights.v[2];
    float d_r_weighted = d_r * weights.v[1];
    float d_theta_weighted = d_theta * weights.v[0];

    // d_theta_weighted tends towards 0 as r_min/r_max approaches 0
    // for cases where perceptual difference is more similar to euclidean
    // distance

    if (r_max != 0.f) {
        d_theta_weighted *= r_min / r_max;
    }

    // sqrt(d_r^2 + (d_theta * r_mid)^2) approx. arc length of archimedean
    // spiral segment
    
    return d_r_weighted*d_r_weighted +
        (r_mid*d_theta_weighted) * (r_mid*d_theta_weighted) +
        d_height_weighted*d_height_weighted;
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

