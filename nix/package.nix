{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
  pkg-config,
  makeWrapper,
  glew,
  libGL,
  sdl3,
  imgui,
  libcpr,
  cpptrace,
  libvorbis,
  libogg,
  zlib-ng,
  freetype,
  libpng,
  opus,
  zstd,
  jemalloc,
  re2,
  abseil-cpp,
  openssl,
  curl,
  wayland,
  libxkbcommon,
  rev ? "unknown",
}:

let
  # upstream's vcpkg manifest asks for imgui with the sdl3/opengl3/freetype bindings
  imgui' =
    (imgui.override {
      IMGUI_BUILD_GLFW_BINDING = false;
      IMGUI_BUILD_SDL3_BINDING = true;
      IMGUI_FREETYPE = true;
    }).overrideAttrs
      (old: rec {
        # the Lua bindings need ImGuiCol_InputTextCursor (>= 1.92.0)
        version = "1.92.0";
        src = fetchFromGitHub {
          owner = "ocornut";
          repo = "imgui";
          tag = "v${version}";
          hash = "sha256-qiESah4j/yHqkOuL5BYANvZfrSqM04CqKONz1jVnkJ0=";
        };
        # nixpkgs marks the freetype option as untested
        propagatedBuildInputs = old.propagatedBuildInputs ++ [ freetype ];
        # vcpkg's freetype is a lowercase CONFIG package, nixpkgs ships the
        # stock (FindFreetype / FreetypeConfig) one
        postPatch = old.postPatch + ''
          substituteInPlace CMakeLists.txt \
            --replace-fail 'find_package(freetype CONFIG REQUIRED)' 'find_package(Freetype REQUIRED)' \
            --replace-fail 'PUBLIC freetype)' 'PUBLIC Freetype::Freetype)'
          substituteInPlace imgui-config.cmake.in \
            --replace-fail 'find_dependency(freetype CONFIG)' 'find_dependency(Freetype)'
        '';
        meta = old.meta // {
          broken = false;
        };
      });
in
stdenv.mkDerivation {
  pname = "openstarbound";
  version = "unstable-${rev}";

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../source
      ../cmake
      ../assets
      ../scripts/packing.config
      ../scripts/linux
    ];
  };

  sourceRoot = "source/source";

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    makeWrapper
  ];

  buildInputs = [
    glew
    libGL
    sdl3
    imgui'
    libcpr
    cpptrace
    libvorbis
    libogg
    zlib-ng
    freetype
    libpng
    opus
    zstd
    jemalloc
    re2
    abseil-cpp
    openssl
    curl
    wayland
    libxkbcommon
  ];

  # the imgui lua bindings pass non-literal format strings
  hardeningDisable = [ "format" ];

  cmakeBuildType = "Release";

  cmakeFlags = [
    (lib.cmakeFeature "STAR_SOURCE_IDENTIFIER" rev)
    (lib.cmakeBool "STAR_USE_JEMALLOC" true)
    (lib.cmakeBool "STAR_ENABLE_STEAM_INTEGRATION" false)
    (lib.cmakeBool "STAR_ENABLE_DISCORD_INTEGRATION" false)
    (lib.cmakeBool "BUILD_TESTING" false)
  ];

  # the build writes binaries to ../dist next to the (read-only) source tree
  preConfigure = "chmod u+w .. && mkdir ../dist";

  enableParallelBuilding = true;

  # the build drops binaries in ../dist relative to the cmake source dir
  installPhase = ''
    runHook preInstall

    dist=$NIX_BUILD_TOP/source/dist
    libexec=$out/libexec/openstarbound
    mkdir -p $libexec $out/bin $out/share/openstarbound/assets

    cp $dist/starbound $dist/starbound_server \
       $dist/asset_packer $dist/asset_unpacker $dist/btree_repacker \
       $dist/dump_versioned_json $dist/make_versioned_json $libexec/

    # opensb.pak is loaded alongside the user's packed.pak
    $dist/asset_packer -c $NIX_BUILD_TOP/source/scripts/packing.config $NIX_BUILD_TOP/source/assets/opensb \
      $out/share/openstarbound/assets/opensb.pak

    for pair in openstarbound:starbound openstarbound-server:starbound_server; do
      substitute ${./launcher.sh} $out/bin/''${pair%%:*} \
        --subst-var out --subst-var-by binary ''${pair##*:}
      chmod +x $out/bin/''${pair%%:*}
    done

    install -Dm644 $NIX_BUILD_TOP/source/source/client/openstarbound.png \
      $out/share/icons/hicolor/256x256/apps/openstarbound.png

    runHook postInstall
  '';

  meta = {
    description = "OpenStarbound client and server";
    homepage = "https://github.com/OpenStarbound/OpenStarbound";
    # unlicensed
    platforms = [
      "x86_64-linux"
      "aarch64-linux"
    ];
    mainProgram = "openstarbound";
  };
}
