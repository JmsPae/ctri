{ 
  echo "#ifndef SHADERS_H"
  echo "#define SHADERS_H"
  echo
  xxd -i vert.glsl
  echo
  xxd -i frag.glsl
  echo
  echo "#endif // SHADERS_H"
} > shaders.h

gcc -std=c99 -o3 -pthread \
    main.c \
    -lGL -lEGL -ldl -lm -lX11 -lXi -lXcursor \
    -o bin

./bin 
