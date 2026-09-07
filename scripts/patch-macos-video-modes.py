#!/usr/bin/env python3
from pathlib import Path
import sys

SOURCE = Path(__file__).resolve().parents[1] / "gameui" / "OptionsSubVideo.cpp"

OLD_FILTER = '''#if !defined( USE_SDL )
		// don't show modes bigger than the desktop for windowed mode
		if ( bWindowed )
#endif
		{
			if ( plist->width > desktopWidth || plist->height > desktopHeight )
			{
				// Filter out sizes larger than our desktop.
				continue;
			}
		}
'''

NEW_FILTER = '''		// Only windowed modes need to fit inside the logical desktop bounds.
		// On macOS/Retina SDL reports logical desktop dimensions, while GLM can
		// expose render resolutions in backing pixels. Filtering fullscreen modes
		// against the logical size hides valid 16:9 targets such as 1920x1080.
		// Fullscreen uses SDL_WINDOW_FULLSCREEN_DESKTOP in this port and the
		// renderer scales/letterboxes the selected render target at presentation.
		if ( bWindowed )
		{
			if ( plist->width > desktopWidth || plist->height > desktopHeight )
			{
				continue;
			}
		}
'''

OLD_FALLBACK = '''#if defined( USE_SDL )
		// If we are switching to a new display, or the size is greater than the desktop, then
		//\tdisplay the desktop width and height.
		if ( bNewFullscreenDisplay || ( Width > desktopWidth ) || ( Height > desktopHeight ) )
		{
			Width = desktopWidth;
			Height = desktopHeight;
		}
#endif
'''

NEW_FALLBACK = '''#if defined( USE_SDL )
		// A fullscreen render target may legitimately be larger than SDL's logical
		// Retina desktop size. Clamp only windowed modes; fullscreen-desktop will
		// scale the chosen render target during presentation.
		if ( bNewFullscreenDisplay || ( bWindowed && ( ( Width > desktopWidth ) || ( Height > desktopHeight ) ) ) )
		{
			Width = desktopWidth;
			Height = desktopHeight;
		}
#endif
'''


def replace_once(text: str, old: str, new: str, name: str) -> tuple[str, bool]:
    if new in text:
        print(f"[video-fix] {name}: already applied")
        return text, False
    if old not in text:
        raise RuntimeError(f"cannot find expected block for {name}; source may have changed")
    return text.replace(old, new, 1), True


def main() -> int:
    text = SOURCE.read_text(encoding="utf-8")
    changed = False

    text, did_change = replace_once(text, OLD_FILTER, NEW_FILTER, "fullscreen Retina resolution filter")
    changed |= did_change
    text, did_change = replace_once(text, OLD_FALLBACK, NEW_FALLBACK, "SDL resolution fallback")
    changed |= did_change

    if changed:
        SOURCE.write_text(text, encoding="utf-8")
        print(f"[video-fix] patched {SOURCE}")
    else:
        print("[video-fix] no changes needed")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"[video-fix] ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
