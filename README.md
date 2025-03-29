# civ - c image viewer. quite lightweight

the image viewer loads all specified images into memory (and never frees them)
it will also, once viewed, load them into GPU.

---------------------------------------

dependencies:

- [rphii/rphiic](https://github.com/rphii/rphiic)
- stb\_image
- pthreads
- cglm
- opengl
- freetype
- glfw
- (glad - in src)
- meson (build)

clone:

    git clone https://github.com/rphii/c-image-viewer/

build with meson:

    meson setup build   # only need to call once
    meson compile -C build

install with meson:

    meson install -C build

usage:

    civ IMAGE-FILES-HERE

help: _(defaults shown will be AFTER loading config and AFTER applying arguments, if any are passed)_

    civ --help

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
    P                                 : print the filename of the active image to stdout

## Config

following paths get checked for a config file:

    $XDG_CONFIG_HOME/imv/civ.conf
    $HOME/.config/civ/civ.conf
    /etc/civ/civ.conf

the following options can be set as of now, see also `--help`:

    font-path = /path/to/font
    font-size = <int>
    show-loaded = <bool>
    show-description = <bool>
    jobs = <int>
    quit-after-full-load = <bool>
    shuffle = <bool>
    image-cap = <int>

an example config:

    font-path = /usr/share/fonts/lato/Lato-Regular.ttf
    font-size = 12
    show-loaded = false
    show-description = yes
    jobs = 4

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

