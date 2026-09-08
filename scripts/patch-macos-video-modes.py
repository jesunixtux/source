#!/usr/bin/env python3
from pathlib import Path
import re
import sys

SOURCE = Path(__file__).resolve().parents[1] / "gameui" / "OptionsSubVideo.cpp"

FILTER_MARKER = "Only windowed modes need to fit inside the logical desktop bounds."
FALLBACK_MARKER = "A fullscreen render target may legitimately be larger than SDL's logical"

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

FILTER_RE = re.compile(
    r'#if !defined\( USE_SDL \)\s*'
    r'// don\'t show modes bigger than the desktop for windowed mode\s*'
    r'if \( bWindowed \)\s*'
    r'#endif\s*'
    r'\{\s*'
    r'if \( plist->width > desktopWidth \|\| plist->height > desktopHeight \)\s*'
    r'\{\s*'
    r'// Filter out sizes larger than our desktop\.\s*'
    r'continue;\s*'
    r'\}\s*'
    r'\}\s*',
    re.MULTILINE,
)

OLD_FALLBACK_CONDITION = (
    "if ( bNewFullscreenDisplay || ( Width > desktopWidth ) || ( Height > desktopHeight ) )"
)
NEW_FALLBACK_CONDITION = (
    "if ( bNewFullscreenDisplay || ( bWindowed && ( ( Width > desktopWidth ) || ( Height > desktopHeight ) ) ) )"
)


def main() -> int:
    text = SOURCE.read_text(encoding="utf-8")
    changed = False

    if FILTER_MARKER in text:
        print("[video-fix] fullscreen Retina resolution filter: already applied")
    else:
        text, count = FILTER_RE.subn(NEW_FILTER, text, count=1)
        if count != 1:
            raise RuntimeError("cannot find expected fullscreen resolution filter block")
        changed = True

    if FALLBACK_MARKER in text:
        print("[video-fix] SDL resolution fallback: already applied")
    elif NEW_FALLBACK_CONDITION in text:
        print("[video-fix] SDL resolution fallback: condition already fixed")
    elif OLD_FALLBACK_CONDITION in text:
        text = text.replace(
            OLD_FALLBACK_CONDITION,
            NEW_FALLBACK_CONDITION,
            1,
        )
        text = text.replace(
            "// If we are switching to a new display, or the size is greater than the desktop, then\n"
            "\t\t//\tdisplay the desktop width and height.",
            "// A fullscreen render target may legitimately be larger than SDL's logical\n"
            "\t\t// Retina desktop size. Clamp only windowed modes; fullscreen-desktop will\n"
            "\t\t// scale the chosen render target during presentation.",
            1,
        )
        changed = True
    else:
        raise RuntimeError("cannot find expected SDL resolution fallback condition")

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
