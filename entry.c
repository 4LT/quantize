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

#define _QU_DBG_STRING(str) #str
#define QU_DBG_STRING(str) _QU_DBG_STRING(str)
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
#define QU_ZERO 0
#define QU_NUMERIC_VERSION QU_PASTE(QU_ZERO, QU_VERSION)

/* Macro debugging

#ifdef QU_VERSION
    #pragma message "Version: " QU_DBG_STRING(QU_VERSION)
    #pragma message "Version Numeric: " QU_DBG_STRING(QU_NUMERIC_VERSION)
#endif

*/

// #if instead of #ifdef in case VERSION is defined but empty
#if QU_NUMERIC_VERSION
    #define _QU_BUILD_NAME_VERSION(name, version) \
        (name "-" #version)
    #define QU_BUILD_NAME_VERSION(name, version) \
        _QU_BUILD_NAME_VERSION(name, version)
    #define QU_NAME_VERSION QU_BUILD_NAME_VERSION(QU_NAME, QU_VERSION)
#else
    #define QU_NAME_VERSION (QU_NAME)
    #warning "No version number"
#endif

#pragma message "Name and version: " QU_DBG_STRING(QU_NAME_VERSION)

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME quantize_op
#define GEGL_OP_C_SOURCE entry.c

#include "gegl-op.h"
#include "quakepal.h"

// non-fullbright colors only
#define QUAKE_COLOR_COUNT 224

struct quantize_color {
    float c[4];
};

struct quantize_ctx {
    float weights[3];
};

static struct quantize_color colors[QUAKE_COLOR_COUNT];

static float color_distance_sq(
    struct quantize_ctx *ctx,
    struct quantize_color c1,
    struct quantize_color c2
) {
    float d_r = (c1.c[0] - c2.c[0]) * ctx->weights[0];
    float d_g = (c1.c[1] - c2.c[1]) * ctx->weights[1];
    float d_b = (c1.c[2] - c2.c[2]) * ctx->weights[2];

    return d_r * d_r + d_g * d_g + d_b * d_b;
}

static struct quantize_color nearest_color(
    struct quantize_ctx *ctx,
    struct quantize_color in_color
) {
    struct quantize_color *nearest = colors;
    struct quantize_color *selected;
    float nearest_distance_sq = 1.f/0.f;
    float distance_sq;

    for (int idx = 0; idx < QUAKE_COLOR_COUNT; idx++) {
        selected = colors + idx;
        distance_sq = color_distance_sq(ctx, in_color, *selected);
        if (distance_sq < nearest_distance_sq) {
            nearest = selected;
            nearest_distance_sq = distance_sq;
        }
    }

    return *nearest;
}

static void populate_colors() {
    int offset = 0;
    const Babl *srgb = babl_format("R'G'B' u8");
    const Babl *linear = babl_format("RGB float");
    float *linear_buffer = malloc(QUAKE_COLOR_COUNT * 3 * sizeof(float));

    babl_process(
        babl_fish(srgb, linear),
        QUAKEPAL,
        linear_buffer,
        QUAKE_COLOR_COUNT
    );

    for (int idx = 0; idx < QUAKE_COLOR_COUNT; idx++) {
        colors[idx] = (struct quantize_color) {
            .c = {
                linear_buffer[offset],
                linear_buffer[offset + 1],
                linear_buffer[offset + 2],
                1.f
            }
        };

        offset+= 3;
    }

    free(linear_buffer);
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
    struct quantize_ctx *ctx = props->user_data;
    gfloat *GEGL_ALIGNED in_f = in_buf;
    gfloat *GEGL_ALIGNED out_f = out_buf;

    for (long pix = 0; pix < n_pixels; pix++) {
        struct quantize_color col =
            nearest_color(ctx, *(struct quantize_color *)in_f);
        col.c[3] = in_f[3];
        *(struct quantize_color *)out_f = col;
        
        out_f+= 4;
        in_f+= 4;
    }

    return TRUE;
}

static void prepare(GeglOperation *op) {
    GeglProperties *props = GEGL_PROPERTIES(op);
    struct quantize_ctx *ctx = props->user_data;
    ctx->weights[0] = props->weight_r;
    ctx->weights[1] = props->weight_g;
    ctx->weights[2] = props->weight_b;
}

static void constructed(GObject *obj) {
    GeglOperation *op = GEGL_OPERATION(obj);
    GeglProperties *props = GEGL_PROPERTIES(op);
    props->user_data = malloc(sizeof (struct quantize_ctx));
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
            QU_NAME_VERSION,
        "categories",
            "Colors",
        "description",
            "Reduce colors to (Quake) palette, preserving image properties",
        NULL
    );

    populate_colors();
}

#endif
