# Quantize

Palettization GEGL operation.  Allows restricting colors to the colors from
~~a palette~~ the Quake palette without destructively impacting the image.
(If editing a full-color image, the image will remain in full-color mode.)

## GIMP Usage

With an image already loaded and layer selected, select
`Tools > GEGL Operation...` from the menu bar.

From the drop-down menu select `Quantize (alpha version)`.

Optional: Adjust the red, green, and blue weights with sliders.  These affect
how each channel impacts the palette color selected for each input color.

The current (alpha) release only supports converting to a built-in Quake palette
limited to the first 224 colors (all non-fullbright colors).

## Build and Installation

### Linux

_Requires that GEGL be installed.  This ought to be the case if GIMP is already
installed, though development packages may be needed dependding on distro_

Run `make` from the project root to build. 

Run `make install` to copy the built artifact into
`$HOME/.local/share/gegl-0.4/plug-ins`; otherwise install manually by copying
`quantize-alpha1.so` into the appropriate directory.

### Windows

Sorry, couldn't get cross-compilation to work for this release.
