// Print the on-screen game window ID for one process, not the desktop.
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
int main(int argc, char **argv) {
    @autoreleasepool {
        if (argc != 2) return 2;
        int pid = atoi(argv[1]);
        NSArray *windows = CFBridgingRelease(CGWindowListCopyWindowInfo(
            kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
            kCGNullWindowID));
        for (NSDictionary *window in windows) {
            if ([window[(id)kCGWindowOwnerPID] intValue] == pid &&
                [window[(id)kCGWindowLayer] intValue] == 0) {
                printf("%u\n", [window[(id)kCGWindowNumber] unsignedIntValue]);
                return 0;
            }
        }
    }
    return 1;
}
