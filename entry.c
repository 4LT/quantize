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

// Necessary for enums to compile
#define GETTEXT_PACKAGE "gegl-0.4"
#include <glib/gi18n-lib.h>

#define _QU_STRINGIFY(str) #str
#define QU_STRINGIFY(str) _QU_STRINGIFY(str)
#define _QU_PASTE(a, b) a##b
#define QU_PASTE(a, b) _QU_PASTE(a, b)

#ifdef GEGL_PROPERTIES

enum_start(color_space)
enum_value(COLOR_SPACE_RGB, "rgb", "RGB")
enum_value(COLOR_SPACE_HSL, "hsl", "HSL")
enum_end(color_space_t)

property_enum(
    color_space,
    "Color space",
    color_space_t,
    color_space,
    COLOR_SPACE_RGB
)

property_double(weight_x, "Red/Hue Weight", 1.0)
    description("Scale red or hue impact on color difference")
    value_range(0.0, 2.0)

property_double(weight_y, "Green/Saturation Weight", 1.0)
    description("Scale green or saturation impact on color difference")
    value_range(0.0, 2.0)

property_double(weight_z, "Blue/Lightness Weight", 1.0)
    description("Scale blue or lightness impact on color difference")
    value_range(0.0, 2.0)

#else

#define QU_NAME "4lt:quantize-alpha"

#ifndef QU_VERSION
    #error "No version number"
#endif

#pragma message "Name and version: " QU_NAME "-" QU_STRINGIFY(QU_VERSION)

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME QU_PASTE(quantize_op_alpha_, QU_VERSION)
#define GEGL_OP_C_SOURCE entry.c

#pragma message "Op name: " QU_STRINGIFY(GEGL_OP_NAME)

#include <stdio.h>

#include "gegl-op.h"
#include "quakepal.h"
#include "color_space_ops.h"

// non-fullbright colors only
#define QUAKE_COLOR_COUNT 224

static struct vec4 rgb_palette[QUAKE_COLOR_COUNT];
static struct vec4 hsl_palette[QUAKE_COLOR_COUNT];

static struct {
    const Babl *srgb;
    const Babl *rgb_linear;
    const Babl *hsl;
    const Babl *srgb_to_linear;
    const Babl *srgb_to_hsl;
    const Babl *rgb_linear_to_hsl;
} color_space_info;

static void initialize_color() {
    int offset = 0;
    color_space_info.srgb = babl_format("R'G'B' u8");
    color_space_info.rgb_linear = babl_format("RGB float");
    color_space_info.hsl = babl_format("HSL float");

    color_space_info.srgb_to_linear = babl_fish(
        color_space_info.srgb,
        color_space_info.rgb_linear
    );

    color_space_info.srgb_to_hsl = babl_fish(
        color_space_info.srgb,
        color_space_info.hsl
    );

    color_space_info.rgb_linear_to_hsl = babl_fish(
        color_space_info.rgb_linear,
        color_space_info.hsl
    );

    float *buffer = malloc(QUAKE_COLOR_COUNT * 3 * sizeof(float));

    babl_process(
        color_space_info.srgb_to_linear,
        QUAKEPAL,
        buffer,
        QUAKE_COLOR_COUNT
    );

    for (int idx = 0; idx < QUAKE_COLOR_COUNT; idx++) {
        rgb_palette[idx] = (struct vec4) {
            .v = {
                buffer[offset],
                buffer[offset + 1],
                buffer[offset + 2],
                1.f
            }
        };

        offset+= 3;
    }

    offset = 0;

    babl_process(
        color_space_info.srgb_to_hsl,
        QUAKEPAL,
        buffer,
        QUAKE_COLOR_COUNT
    );

    for (int idx = 0; idx < QUAKE_COLOR_COUNT; idx++) {
        hsl_palette[idx] = (struct vec4) {
            .v = {
                buffer[offset],
                buffer[offset + 1],
                buffer[offset + 2],
                1.f
            }
        };

        offset+= 3;
    }

    free(buffer);
}

static gboolean process(
    GeglOperation       *op,
    void                *in_buf,
    void                *out_buf,
    glong                n_pixels,
    const GeglRectangle *roi,
    gint                 level
) {
    GeglProperties *props = GEGL_PROPERTIES(op);
    struct color_space_ctx *ctx = props->user_data;
    gfloat *GEGL_ALIGNED in_f = in_buf;
    gfloat *GEGL_ALIGNED out_f = out_buf;
    struct vec4 color;
    struct vec4 color_in;
    struct vec4 color_converted;
    int color_index;

    for (long pix = 0; pix < n_pixels; pix++) {
        color_in = *(struct vec4 *)in_f;

        if (props->color_space == COLOR_SPACE_HSL) {
            babl_process(
                color_space_info.rgb_linear_to_hsl,
                &color_in,
                &color_converted,
                1
            );
        } else {
            color_converted = color_in;
        }

        color_index = nearest_color_index(ctx, color_converted);
        color = rgb_palette[color_index];
        color.v[3] = in_f[3];
        *(struct vec4 *)out_f = color;
        
        out_f+= 4;
        in_f+= 4;
    }

    return TRUE;
}

static void prepare(GeglOperation *op) {
    GeglProperties *props = GEGL_PROPERTIES(op);
    struct color_space_ctx *ctx = props->user_data;
    ctx->weights.v[0] = props->weight_x;
    ctx->weights.v[1] = props->weight_y;
    ctx->weights.v[2] = props->weight_z;

    switch (props->color_space) {
        case COLOR_SPACE_HSL:
            ctx->palette = hsl_palette;
            ctx->distance_sq = hsl_distance_sq;
            break;
        default:
            ctx->palette = rgb_palette;
            ctx->distance_sq = rgb_distance_sq;
    }

    ctx->palette_sz = QUAKE_COLOR_COUNT;
}

static void constructed(GObject *obj) {
    GeglOperation *op = GEGL_OPERATION(obj);
    GeglProperties *props = GEGL_PROPERTIES(op);
    props->user_data = malloc(sizeof (struct color_space_ctx));
}

static void dispose(GObject *obj) {
    GeglOperation *op = GEGL_OPERATION(obj);
    GeglProperties *props = GEGL_PROPERTIES(op);
    free(props->user_data);
}

static void gegl_op_class_init (GeglOpClass *cls) {
    GObjectClass *gobj_class = G_OBJECT_CLASS(cls);
    GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (cls);
    GeglOperationPointFilterClass *point_filter_class =
        GEGL_OPERATION_POINT_FILTER_CLASS (cls);

    point_filter_class->process = process;
    operation_class->prepare = prepare;
    gobj_class->constructed = constructed;
    gobj_class->dispose = dispose;

    gegl_operation_class_set_keys(
        operation_class,
        "title",
            "Quantize (alpha version)",
        "name",
            QU_NAME "-" QU_STRINGIFY(QU_VERSION),
        "categories",
            "Colors",
        "description",
            "Reduce colors to (Quake) palette, preserving image properties",
        NULL
    );

    initialize_color();
}

#endif
