# TODO

## Win32 & Platform Layers
 - Graphics Layer
   - Vulkan graphics example
   - Graphics independent initialization/rendering layer
     - Fix error handling, should use message box (notion of a terminal error)

 - Multiple windows
   - Multiple windows GL example
   - Multiple windows Vulkan example
   - Graphics independent mulitple windows

 - OS Layer 
   - Linux wayland or X11 & vulkan example
   - OS independent layer (multiple windows)

 - Input handling
   - Windows input handling
   - Linux input handling
   - OS independent input handling
   - (bonus) Windows live input replay
   - (bonus) Fixed position memory on debug builds

 - Research SRGB
 - Restore UI layer

 - OS utilities/pendencies
   - Clipboard
   - Minimize, maximize
   - Cursor image
   - On resize draw

 - Profile & Test
   - Build Profiler
   - Profile & Memory leaks check

## Higher lever layers
 - Rendering Layer
   - Fix rectangle madness

 - UI improvements
   - Draw rects with gradient
   - Build "set next thing" macros
   - Dragging stuff (position and size)
   - Set cursor image/type on hover/on drag etc.

 - Text input
   - Finish / FIX single line input
     - Clip text
     - Scroll
   - First version multiline input
   - Text truncation

 - Handle UTF8
   - rendering
   - text size calculation

 - Profile/Measure/Debug Utils
   - Memory management
     - Review all arena usages and actually group lifetimes
   - Simple timer profile 
     - Handle recursive functions
     - Emit to flame graph viewer
     - Render flame graph viewer in debug UI
   - Check possible paralalizing

 - Memory/Work improvements
   - Worker thread queue
   - Asset manager
   - Growing/Dynamic arenas
   - UI string store
   - UI text size cache

 - Docking/Windows handling
   - Panel/docking system
   - Cross window support for panels/tabs (like chrome)

## App design ideas
 - Default Every Day times (Wake/Work/Commute/Lunch/Dinner)
   - To estimate correctly how many hours in a day
   - Flexible settings like weekly, weekends or not