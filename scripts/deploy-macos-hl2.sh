#!/bin/bash
set -euo pipefail

# Deploy the freshly-built arm64 Source Engine into the Half-Life 2 Steam install.
# Replaces the old i386 binaries with our 64-bit arm64 build, fixing install names
# so the dylibs resolve against @loader_path (portable).

BUILD_DIR="/Users/jesus/Desktop/Source-Engine-macos-port/SOURCE-ENGINE-ORIGINAL/source/build"
HL2_DIR="/Users/jesus/Library/Application Support/Steam/steamapps/common/Half-Life 2"
BIN_DIR="$HL2_DIR/bin"
HOMEBREW="/opt/homebrew/opt"

# Module pairs "staged_name:build_relpath" (lib-prefixed is what foundLibraryWithPrefix expects)
MODULES=(
  "liblauncher.dylib:launcher/liblauncher.dylib"
  "libengine.dylib:engine/libengine.dylib"
  "libfilesystem_stdio.dylib:filesystem/libfilesystem_stdio.dylib"
  "libtier0.dylib:tier0/libtier0.dylib"
  "libvstdlib.dylib:vstdlib/libvstdlib.dylib"
  "libtogl.dylib:togl/libtogl.dylib"
  "libmaterialsystem.dylib:materialsystem/libmaterialsystem.dylib"
  "libshaderapidx9.dylib:materialsystem/shaderapidx9/libshaderapidx9.dylib"
  "libstdshader_dx9.dylib:materialsystem/stdshaders/libstdshader_dx9.dylib"
  "libstudiorender.dylib:studiorender/libstudiorender.dylib"
  "libdatacache.dylib:datacache/libdatacache.dylib"
  "libscenefilecache.dylib:scenefilecache/libscenefilecache.dylib"
  "libsoundemittersystem.dylib:soundemittersystem/libsoundemittersystem.dylib"
  "libvphysics.dylib:vphysics/libvphysics.dylib"
  "libinputsystem.dylib:inputsystem/libinputsystem.dylib"
  "libvgui2.dylib:vgui2/src/libvgui2.dylib"
  "libvguimatsurface.dylib:vguimatsurface/libvguimatsurface.dylib"
  "libGameUI.dylib:gameui/libGameUI.dylib"
  "libServerBrowser.dylib:serverbrowser/libServerBrowser.dylib"
  "libvideo_services.dylib:video/libvideo_services.dylib"
  "libvtex_dll.dylib:utils/vtex/libvtex_dll.dylib"
  "libclient.dylib:game/client/libclient.dylib"
  "libserver.dylib:game/server/libserver.dylib"
  "libsteam_api.dylib:stub_steam/libsteam_api.dylib"
  "libvaudio_minimp3.dylib:engine/voice_codecs/minimp3/libvaudio_minimp3.dylib"
)

# Third-party Homebrew libs needed at runtime (copied alongside so install names
# can be made relative)
THIRDPARTY=(
  "libSDL2-2.0.0.dylib:$HOMEBREW/sdl2-compat/lib/libSDL2-2.0.0.dylib"
  "libfreetype.6.dylib:$HOMEBREW/freetype/lib/libfreetype.6.dylib"
  "libfontconfig.1.dylib:$HOMEBREW/fontconfig/lib/libfontconfig.1.dylib"
  "libjpeg.10.dylib:$HOMEBREW/jpeg/lib/libjpeg.10.dylib"
  "libpng16.16.dylib:$HOMEBREW/libpng/lib/libpng16.16.dylib"
  "libintl.8.dylib:$HOMEBREW/gettext/lib/libintl.8.dylib"
)

echo "==> Verifying build outputs exist"
for pair in "${MODULES[@]}"; do
  name="${pair%%:*}"; rel="${pair#*:}"
  [ -f "$BUILD_DIR/$rel" ] || { echo "MISSING: $BUILD_DIR/$rel"; exit 1; }
done
for pair in "${THIRDPARTY[@]}"; do
  name="${pair%%:*}"; src="${pair#*:}"
  [ -f "$src" ] || { echo "MISSING thirdparty: $src"; exit 1; }
done

echo "==> Staging all dylibs into $BIN_DIR"
mkdir -p "$BIN_DIR"
for pair in "${MODULES[@]}"; do
  name="${pair%%:*}"; rel="${pair#*:}"
  cp -f "$BUILD_DIR/$rel" "$BIN_DIR/$name"
done
for pair in "${THIRDPARTY[@]}"; do
  name="${pair%%:*}"; src="${pair#*:}"
  cp -f "$src" "$BIN_DIR/$name"
done

# Fix install names: rewrite absolute paths pointing into the build dir and into
# the Homebrew dir to @loader_path/<name> so that all dylibs resolve relative to
# their own location (bin/).
echo "==> Rewriting install names to @loader_path"
fix_install_name() {
  local target="$1"
  local old="$2"
  local new="$3"
  local id
  id="$(otool -D "$target" | tail -1)"
  if [[ "$id" == "$old" ]]; then
    install_name_tool -id "$new" "$target"
  fi
  if otool -L "$target" | grep -q "$old"; then
    install_name_tool -change "$old" "$new" "$target"
  fi
}

# Build parallel arrays: absolute source path <-> staged basename for rewriting.
# (bash 3.2 on macOS has no associative arrays, so use parallel indexed arrays.)
declare -a ABS_PATHS
declare -a STAGED_NAMES
for pair in "${MODULES[@]}"; do
  name="${pair%%:*}"; rel="${pair#*:}"
  ABS_PATHS[${#ABS_PATHS[@]}]="$BUILD_DIR/$rel"
  STAGED_NAMES[${#STAGED_NAMES[@]}]="$name"
done
for pair in "${THIRDPARTY[@]}"; do
  name="${pair%%:*}"; src="${pair#*:}"
  ABS_PATHS[${#ABS_PATHS[@]}]="$src"
  STAGED_NAMES[${#STAGED_NAMES[@]}]="$name"
done

# For every dylib now in bin/, rewrite its own install name (id) and any internal
# build/homebrew references to @loader_path equivalents.
for f in "$BIN_DIR"/*.dylib; do
  base="$(basename "$f")"
  echo "  fixing $base"

  # Own id
  id="$(otool -D "$f" | tail -1)"
  case "$id" in
    /Users/jesus/Desktop/Source-Engine-macos-port/*|/opt/homebrew/opt/*)
      install_name_tool -id "@loader_path/$base" "$f"
      ;;
  esac

  # Rewrite any reference that points to a staged library
  for ((i=0; i<${#ABS_PATHS[@]}; i++)); do
    abs="${ABS_PATHS[$i]}"
    if otool -L "$f" | grep -q "$abs"; then
      install_name_tool -change "$abs" "@loader_path/${STAGED_NAMES[$i]}" "$f"
    fi
  done
done

echo "==> Re-signing all modified dylibs (ad-hoc, required after install_name_tool)"
for f in "$BIN_DIR"/*.dylib; do
  codesign -f -s - "$f" 2>/dev/null || { echo "WARN: codesign failed for $f"; }
done

echo "==> Installing launcher as hl2_osx"
cp -f "$BUILD_DIR/launcher_main/hl2_launcher" "$HL2_DIR/hl2_osx"
chmod +x "$HL2_DIR/hl2_osx"

echo "==> Deploying SDL3 runtime (required by sdl2-compat shim)"
SDL3_SRC="$HOMEBREW/sdl3/lib/libSDL3.0.dylib"
if [ -f "$SDL3_SRC" ]; then
  cp -f "$SDL3_SRC" "$BIN_DIR/libSDL3.0.dylib"
  install_name_tool -id "@loader_path/libSDL3.0.dylib" "$BIN_DIR/libSDL3.0.dylib"
  codesign -f -s - "$BIN_DIR/libSDL3.0.dylib" 2>/dev/null
  ln -sf libSDL3.0.dylib "$BIN_DIR/libSDL3.dylib"
else
  echo "WARN: SDL3 not found at $SDL3_SRC"
fi

echo "==> Replacing hl2/bin client/server with arm64 versions"
mkdir -p "$HL2_DIR/backup_bin_i386/hl2bin"
for mod in client server; do
  if [ -f "$HL2_DIR/hl2/bin/$mod.dylib" ]; then
    cp -f "$HL2_DIR/hl2/bin/$mod.dylib" "$HL2_DIR/backup_bin_i386/hl2bin/$mod.dylib"
  fi
  cp -f "$BIN_DIR/lib$mod.dylib" "$HL2_DIR/hl2/bin/$mod.dylib"
  codesign -f -s - "$HL2_DIR/hl2/bin/$mod.dylib" 2>/dev/null
done

echo "==> Removing stale i386-only modules that have no arm64 equivalent"
for stale in GameUI.dylib ServerBrowser.dylib datacache.dylib engine.dylib filesystem_stdio.dylib inputsystem.dylib launcher.dylib materialsystem.dylib scenefilecache.dylib shaderapidx9.dylib soundemittersystem.dylib stdshader_dx9.dylib studiorender.dylib vgui2.dylib vguimatsurface.dylib video_services.dylib vphysics.dylib vtex_dll.dylib bsppack.dylib shaderapiempty.dylib vaudio_miles.dylib vaudio_speex.dylib video_quicktime.dylib replay.dylib chromehtml.dylib bugreporter_filequeue.dylib bugreporter_public.dylib sourcevr.dylib; do
  rm -f "$BIN_DIR/$stale"
done
rm -rf "$BIN_DIR/osx32"

echo "==> Done. Deploy summary:"
echo "  launcher: $HL2_DIR/hl2_osx ($(file -b "$HL2_DIR/hl2_osx" | cut -d, -f1-2))"
echo "  dylibs in bin/: $(ls "$BIN_DIR"/*.dylib | wc -l | tr -d ' ') files"
