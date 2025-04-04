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

#define _QU_STRINGIFY(str) #str
#define QU_STRINGIFY(str) _QU_STRINGIFY(str)
#define _QU_PASTE(a, b) a##b
#define QU_PASTE(a, b) _QU_PASTE(a, b)

#ifdef GEGL_PROPERTIES

property_double(weight_r, "Red Weight", 1.0)
    description("Scale red impact on color difference")
    value_range(0.0, 2.0)

property_double(weight_g, "Green Weight", 1.0)
    description("Scale green impact on color difference")
    value_range(0.0, 2.0)

property_double(weight_b, "Blue Weight", 1.0)
    description("Scale blue impact on color difference")
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

#include "gegl-op.h"
#include "quakepal.h"
#include "color_space_ops.h"

// non-fullbright colors only
#define QUAKE_COLOR_COUNT 224

static struct vec4 rgb_palette[QUAKE_COLOR_COUNT];
static struct vec4 hsl_palette[QUAKE_COLOR_COUNT];

static void initialize_colors() {
    int offset = 0;
    const Babl *srgb = babl_format("R'G'B' u8");
    const Babl *linear = babl_format("RGB float");
    const Babl *hsl = babl_format("HSL float");
    float *buffer = malloc(QUAKE_COLOR_COUNT * 3 * sizeof(float));

    babl_process(
        babl_fish(srgb, linear),
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

    babl_process(
        babl_fish(srgb, hsl),
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
    int color_index;

    for (long pix = 0; pix < n_pixels; pix++) {
        color_index = nearest_color_index(ctx, *(struct vec4 *)in_f);
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
    ctx->weights.v[0] = props->weight_r;
    ctx->weights.v[1] = props->weight_g;
    ctx->weights.v[2] = props->weight_b;
    ctx->palette = rgb_palette;
    ctx->palette_sz = QUAKE_COLOR_COUNT;
    ctx->distance_sq = rgb_distance_sq;
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

    initialize_colors();
}

#endif
