LIBS="-lGLESv2 -lEGL -lm -lX11 -lwayland-client -lwayland-server -lwayland-cursor -lwayland-egl"
COMPILER_FLAGS="-fsanitize=address,undefined -g -Wall -Wextra -I../"

WAYLAND_DEBUG=client
echo ./build/linux_gfx         && clang $COMPILER_FLAGS -o ./build/linux_gfx `realpath examples/linux_gfx.c`          $LIBS
echo ./build/linux_gl          && clang $COMPILER_FLAGS -o ./build/linux_gl  `realpath examples/linux_gl.c`           $LIBS
echo ./build/external_linux_gl && clang -g -o ./build/external_linux_gl      `realpath examples/external_linux_gl.c`  $LIBS

