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


##### TODO so I won't forget

- don't load EVERYTHING, but keep some images around the currently active one loaded
- specify directory, check mime types (magic.h)
- config file / arguments
    - pan speed
    - zoom speed
    - show / hide description
    - filter mode
    - background color
    - text color
    - font size
    - font path
- fix font - fontconfig ...
- load font symbols for filename
- first time pan with kb -> why lag ?
- help
- scroll wheel zooming - not accounted for any delta ...
- config for key bindings ...
- don't crash if no font loaded ...
- better error handling
- info / verbose
- make pthreads optional (single core)

