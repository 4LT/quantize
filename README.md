# Quantize

Palettization GEGL operation.  Allows restricting colors to the colors from
~~a palette~~ the Quake palette without destructively impacting the image.
(If editing a full-color image, the image will remain in full-color mode.)

## GIMP Usage

With an image already loaded and layer selected, select
`Tools > GEGL Operation...` from the menu bar.

From the drop-down menu select `Quantize (beta version)`.

An additional drop-down is should now be visible to select color model for
weights, as well as sliders to adjust the weights.  The available color models
are RGB and LCH (lightness, chroma, and hue).  The weight sliders impact how
much each channel affects the color distance calculated for each pixel color and
the palette colors.

The palettization process then attempts to select colors from the palette with 
a minimal distance from the pixel color.

The current (beta) release only supports converting to a built-in Quake palette
limited to the first 224 colors (all non-fullbright colors).

## Build and Installation

### Linux

Requires the necessary build tools and libraries (`pkg-config` and `libgegl-dev`
on Ubuntu).

Run `make` from the project root to build. 

Run `make install` to copy the built artifact into
`$HOME/.local/share/gegl-0.4/plug-ins`

Run `make install-flatpak` to copy the built artifact into
`$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins`

otherwise install manually by copying `quantize.so` into the appropriate
directory.

### Windows Build

Requires MSYS2, with the following packages installed:

* `base-devel`
* `mingw-w64-x86_64-toolchain`
* `mingw-w64-x86_64-gegl`

Run `make EXT=dll` from the project root under the MSYS2 environment to build.

### Windows Installation

Copy quantize.dll from your build OR the provided ZIP archive into
`C:\Users\<YOU>\AppData\Local\gegl-0.4\plug-ins`.  `AppData` is a hidden folder,
so you'll need to type the name manually if browsing through Windows Explorer.

## Troubleshooting

### GIMP crashes on start-up or when attempting to use the filter

Remove the filter from the plug-ins folder.  If the problem persists after
re-installing the filter, open an issue in the issue tracker.

### The filter behaves in some other surprising or unexpected manner

Make sure there aren't multiple versions installed simultaneously.  Delete any
file named `quantize-alpha*` in your plug-ins folder.  Note that this will break
any XCF file that depends on the filter.

### GIMP issues warnings, behaves unexpectedly, etc. after oading an XCF that uses the filter

Likely due to an older version of the filter being used when saving the XCF.
Remove the the filter in the chain of operations from the XCF and add it back.
This may require temporarily removing the filter from the plug-ins folder in
case the version mismatch causes instability when trying to remove the
filter operation.
