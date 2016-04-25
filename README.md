# What
This is a loose collection of C++14 libraries I tend to reuse between different, mostly game-related, projects.

They are meant mostly for me (Emil Ernerfeldt), but if you find them useful, have at it.

Their only dependencies are either system dependencies (pthread etc) and [Loguru](https://github.com/emilk/loguru). gl_lib also depends on OpenGL and GLEW. The libraries does not depend on each other.

They all work on OSX and iOS and probably Linux. Some may work elsewhere.

### configuru
This is not part of emilib, but a copy of https://github.com/emilk/configuru here for convenience.

### coroutine
This is a "fake coroutine" class which implements a cooperative thread and methods for passing execution between the outer and inner thread.

This is really nice for handling things that you would normally use a state machine for. Perfect for games where you might want to have a scripted event, a dialouge or something else running in its own thread, but not at the same time as the main game logic thread.

### dir_watcher
This library allows you to watch for changes in a directory, e.g. new files, deleted files or changes in existing files. This is great for doing automatic hot-reloading of things like textures, shaders, etc.

### gl_lib
Code for doing simple things in OpenGL.

### hash_map / hash_set
Cache-firendly hash map/set with open adressing, linear probing and power-of-two capacity.

### irange
Simple integer range, allowing things like:

``` C++
for (const auto ix : irange(end)) { CHECK_F(0 <= ix && ix < end); }
for (const auto ix : irange(begin, end)) { CHECK_F(begin <= ix && ix < end); }
for (const auto ix : indices(some_vector)) { CHECK_F(0 <= ix && ix < some_vector.size(); }
for (const char ch : emilib::cstr_range("hello world!"))
```

### list_map / list_set
Simple O(N) map/set with small overhead and great performance for small N.

### loguru
This is not part of emilib, but a copy of https://github.com/emilk/loguru here for convenience, and as a dependency for most other librarties.

### mem_map
Simple wrapper around mmap with RAII.

### music
Stream mp3 music on OSX and iOS.

### movement_tracker
Track movement of some data, e.g. to estimate velocity from recent movement.

### string_interning
Stupid simple thread-safe string interning.

### strprintf
Minimalistic string formating library. Provdides two functions: `strprintf` and `vstrprintf` that act like `printf` and `vprintf` respectively, but return the formated string as a `std::string` instead of printing it.

### text_paint
Nice text rendering for OSX and iOS.

### timer
Monotonic wall time chronometer.

### tuple_util
Adds `for_each_tuple` for iterating over a `std::tuple` and also overloads `std::hash` for `std::tuple`.

### utf8
Really basic utf8 operations.

### wav
Parse WAVE (.wav) sound files into a nice format.
