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

#ifndef QU_VERSION
    #error "No version number"
#endif

#define QU_NAME "4lt:quantize"

#ifdef GEGL_PROPERTIES

enum_start(color_space)
enum_value(COLOR_SPACE_RGB, "rgb", "Linear RGB")
enum_value(COLOR_SPACE_LCH, "lch", "Hue-Chroma-Lightness")
enum_end(color_space_t_a3)

property_enum(
    color_space,
    "Color space",
    color_space_t_a3,
    color_space,
    COLOR_SPACE_RGB
)

property_double(weight_r, "Red Weight", 1.0)
    description("Scale red impact on color difference")
    value_range(0.0, 2.0)
    ui_meta("visible", "color_space {rgb}")

property_double(weight_g, "Green Weight", 1.0)
    description("Scale green impact on color difference")
    value_range(0.0, 2.0)
    ui_meta("visible", "color_space {rgb}")

property_double(weight_b, "Blue Weight", 1.0)
    description("Scale blue impact on color difference")
    value_range(0.0, 2.0)
    ui_meta("visible", "color_space {rgb}")

property_double(weight_hue, "Hue Weight", 1.0)
    description("Scale hue impact on color difference")
    value_range(0.0, 2.0)
    ui_meta("visible", "color_space {lch}")

property_double(weight_chroma, "Chroma Weight", 1.0)
    description("Scale chroma impact on color difference")
    value_range(0.0, 2.0)
    ui_meta("visible", "color_space {lch}")

property_double(weight_lightness, "Lightness Weight", 1.0)
    description("Scale lightness impact on color difference")
    value_range(0.0, 2.0)
    ui_meta("visible", "color_space {lch}")

#else

#pragma message "Name and version: " QU_NAME "-" QU_STRINGIFY(QU_VERSION)

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME quantize_beta
#define GEGL_OP_C_SOURCE entry.c

#include "gegl-op.h"
#include "quakepal.h"
#include "color_space_ops.h"

// non-fullbright colors only
#define QUAKE_COLOR_COUNT 224

static struct vec4 rgb_palette[QUAKE_COLOR_COUNT];
static struct vec4 lch_palette[QUAKE_COLOR_COUNT];

static struct {
    const Babl *srgb;
    const Babl *rgb_linear;
    const Babl *lch;
    const Babl *srgb_to_linear;
    const Babl *srgb_to_lch;
    const Babl *rgb_linear_to_lch;
} color_space_info;

static struct vec4 normalize_lch(struct vec4 lch) {
    lch.v[0]/= 100.f;
    lch.v[1]/= 180.f;
    lch.v[2]/= 360.f;
    return lch;
}

static void initialize_color() {
    int offset = 0;
    color_space_info.srgb = babl_format("R'G'B' u8");
    color_space_info.rgb_linear = babl_format("RGB float");
    color_space_info.lch = babl_format("CIE LCH(ab) float");

    color_space_info.srgb_to_linear = babl_fish(
        color_space_info.srgb,
        color_space_info.rgb_linear
    );

    color_space_info.srgb_to_lch = babl_fish(
        color_space_info.srgb,
        color_space_info.lch
    );

    color_space_info.rgb_linear_to_lch = babl_fish(
        color_space_info.rgb_linear,
        color_space_info.lch
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
        color_space_info.srgb_to_lch,
        QUAKEPAL,
        buffer,
        QUAKE_COLOR_COUNT
    );

    for (int idx = 0; idx < QUAKE_COLOR_COUNT; idx++) {
        lch_palette[idx] = normalize_lch((struct vec4) {
            .v = {
                buffer[offset],
                buffer[offset + 1],
                buffer[offset + 2],
                1.f
            }
        });

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

        if (props->color_space == COLOR_SPACE_LCH) {
            babl_process(
                color_space_info.rgb_linear_to_lch,
                &color_in,
                &color_converted,
                1
            );

            color_converted = normalize_lch(color_converted);
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

    switch (props->color_space) {
        case COLOR_SPACE_LCH:
            ctx->palette = lch_palette;
            ctx->distance_sq = lch_distance_sq;
            ctx->weights.v[0] = props->weight_lightness;
            ctx->weights.v[1] = props->weight_chroma;
            ctx->weights.v[2] = props->weight_hue;
            break;
        default:
            ctx->palette = rgb_palette;
            ctx->distance_sq = rgb_distance_sq;
            ctx->weights.v[0] = props->weight_r;
            ctx->weights.v[1] = props->weight_g;
            ctx->weights.v[2] = props->weight_b;
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
            "Quantize (beta version)",
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
