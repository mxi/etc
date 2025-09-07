#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface AppDelegate : NSObject<NSApplicationDelegate>
@end

@implementation AppDelegate
{
    NSWindow *_window;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    NSRect frame = NSMakeRect(0, 0, 960, 640);

    _window = [[NSWindow alloc] initWithContentRect:frame 
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];

    [_window setTitle:@"MacOS Application Basic"];
    [_window center];
    [_window orderFrontRegardless];
}

@end

int main(void)
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setDelegate:[AppDelegate alloc]];
        [NSApp run];
    }
    return 0;
}