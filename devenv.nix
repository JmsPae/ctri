{
  pkgs,
  lib,
  config,
  inputs,
  ...
}: let
  deps = with pkgs; [
    pkg-config
    libGL
    tinyxxd
    python3
    xorg.libX11
    xorg.libXi
    xorg.libXcursor
    xorg.libXrandr
    xorg.libXinerama
  ];
in {
  packages = with pkgs;
    [
      git
      cmake
      emscripten
    ]
    ++ deps;
  languages.c.enable = true;

  scripts = {
    genshad.exec = ''
      {
        echo "#ifndef SHADERS_H"
        echo "#define SHADERS_H"
        echo
        xxd -n shader_vert -i shaders/vert.glsl
        echo
        xxd -n shader_frag -i shaders/frag.glsl
        echo
        echo "#endif"
      } > src/shaders.h
    '';

    genshades.exec = ''
      {
        echo "#ifndef SHADERS_H"
        echo "#define SHADERS_H"
        echo
        xxd -n shader_vert -i shaders/vert_es3.glsl
        echo
        xxd -n shader_frag -i shaders/frag_es3.glsl
        echo
        echo "#endif"
      } > src/shaders.h
    '';

    gen.exec = ''
      cmake -B ./build -DCMAKE_BUILD_TYPE=Debug
      genshad
    '';

    genrel.exec = ''
      cmake -B ./build -DCMAKE_BUILD_TYPE=Release
      genshad
    '';

    genwasm.exec = ''
        export EM_CACHE=${./.}/.cache
      emcmake cmake -B ./build -DCMAKE_TOOLCHAIN_FILE=$EMROOT/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Debug
      genshades
    '';

    genwasmrel.exec = ''
        export EM_CACHE=${./.}/.cache
      emcmake cmake -B ./build -DCMAKE_TOOLCHAIN_FILE=$EMROOT/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Release
      genshades
    '';

    build.exec = ''
      cmake --build ./build -j $(nproc)
    '';

    run.exec = ''
      build
      build/ctri
    '';

    debug.exec = ''
      build
      gdb build/ctri
    '';
  };
  env = {
    LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath deps;
    EMROOT = ''${pkgs.emscripten.outPath}/share/emscripten'';
  };
}
