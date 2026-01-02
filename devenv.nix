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
        xxd -i shaders/vert.glsl
        echo
        xxd -i shaders/frag.glsl
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

  env.LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath deps;
}
