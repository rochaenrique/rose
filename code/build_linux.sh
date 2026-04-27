if [ ! -d "build/" ]; then
  mkdir build
fi

COMPILER_FLAGS="-Wall"

clang $COMPILER_FLAGS linux_main.c -o ./build/linux_main
