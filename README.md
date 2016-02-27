
# What
This is a loose collection of C++11 libraries I tend to reuse between different, mostly game-related, projects.

They are meant mostly for me (Emil Ernerfeldt), but if you find them useful, have at it.

Their only dependencies are either system dependencies (pthread etc) and [Loguru](https://github.com/emilk/loguru). gl_lib also depends on OpenGL and GLEW. The libraries does not depend on each other.

They all work on OSX and iOS and probably Linux. Some may work on Windows.

### coroutine
This is a "fake coroutine" class which implements a cooperative thread and methods for passing execution between the outer and inner thread.

This is really nice for handling things that you would normally use a state machine for. Perfect for games where you might want to have a scripted event, a dialouge or something else running in its own thread, but not at the same time as the main game logic thread.

### dir_watcher
This library allows you to watch for changes in a directory, e.g. new files, deleted files or changes in existing files. This is great for doing automatic hot-reloading of things like textures, shaders, etc.

### gl_lib
Code for doing simple things in OpenGL.

### list_map / list_set
Simple O(N) map/set with small overhead and great performance for small N.

### mem_map
Simple wrapper around mmap with RAII.

### text_paint
Nice text rendering for OSX and iOS.

### utf8
Really basic utf8 operations.
