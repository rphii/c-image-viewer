# civ - c image viewer. quite lightweight

the image viewer loads all specified images into memory (and never frees them)
it will also, once viewed, load them into GPU.

---------------------------------------

dependencies:

- pthreads
- cglm
- opengl
- freetype
- (glad - in src)
- (glfw - in src)

clone:

    git clone https://github.com/rphii/c-image-viewer/ && cd c-image-viewer/src

build with ninja:
    
    ninja

build with compiler:

    gcc -o civ.out -I/usr/include/freetype2 *.c -lglfw -lm -lfreetype

launch program:

    ./civ.out IMAGE-FILES-HERE

help: _(defaults shown will be AFTER loading config and AFTER applying arguments, if any are passed)_

    ./civ.out --help

controls:

    K                                 : previous image
    J                                 : next image
    S                                 : scaling
    LEFT_SHIFT + S                    : change filter (nearest, linear)
    MOUSE_LEFT_PAN                    : pan image
    SCROLL                            : zoom image
    D                                 : toggle description
    ARROW_KEY_LEFT  | LEFT_SHIFT + H  : pan left
    ARROW_KEY_RIGHT | LEFT_SHIFT + J  : pan down
    ARROW_KEY_UP    | LEFT_SHIFT + K  : pan up
    ARROW_KEY_DOWN  | LEFT_SHIFT + L  : pan right
    I | LEFT_SHIFT + AROW_KEY_UP      : zoom in
    O | LEFT_SHIFT + AROW_KEY_UP      : zoom out
    Q | ESCAPE                        : quit

clean with ninja:

    ninja -t clean

## Config

following paths get checked for a config file:

    "$XDG_CONFIG_HOME/imv/config",
    "$HOME/.config/civ/config",
    "/etc/civ/config"

the following options can be set as of now:

    font-path = /path/to/font
    font-size = <int>
    show-loaded = <bool>
    show-description = <bool>
    jobs = <int>
    quit-after-full-load = <bool>

## Wayland

Changing from floating to tiling window can be done via:

    windowrule = tile, title:^civ(\.out)?$

## Distributions

- [gentoo ebuild](https://github.com/rphii/gentoo-ebuilds/blob/main/media-gfx/civ/civ-9999.ebuild)

##### TODO so I won't forget

- don't load EVERYTHING, but keep some images around the currently active one loaded
    - CURRENTLY: freeing CPU memory when sent to GPU (as that appears to work?)
- specify directory, check mime types (magic.h)
- config file
    - pan speed
    - zoom speed
    - background color
    - text color
- fix font - fontconfig ...
- load font symbols for filename
- config for key bindings ...
- better error handling
- info / verbose
- make pthreads optional (single core)
- type numbers -> go-to image
- wayland - how to set tiling ? -> use wayland window instead of glfw

