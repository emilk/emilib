
# What
This is a loose collection of libraries I tend to reuse between different project.

They are meant mostly for me (Emil Ernerfeldt), but if you find them useful, have at it.

Their only dependencies are either system dependencies (pthread etc) and [Loguru](https://github.com/emilk/loguru). gl_lib also depends on OpenGL and GLEW. The libraries does not depend on each other.

They all work on OSX and iOS.

### gl_lib
Code for doing simple things in OpenGL.

### mem_map
Simple wrapper around mmap with RAII.

### text_paint
Nice text rendering for OSX and iOS.
