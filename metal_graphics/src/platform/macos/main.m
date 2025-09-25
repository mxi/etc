#import <time.h>

#import <mach/mach_time.h>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#import "../glue.h"

@interface MGInstance : NSView <NSApplicationDelegate, CALayerDelegate> @end

@implementation MGInstance
{
    mach_timebase_info_data_t _timebaseInfo;
    uint64_t _timeNowNS;
    uint64_t _timeRenderTargetNS;
    uint64_t _videoTimeNowNS;
    uint64_t _videoTimeRenderTargetNS;

    uint64_t _cvCallbackTimeNS;

    uint64_t _cmdScheduleTimeNS;
    uint64_t _cmdCompletedTimeNS;
    uint64_t _cmdCommitTimeNS;

    NSWindow *_window;
    CFRunLoopRef _runLoop;
    CFRunLoopSourceRef _runLoopSource;
    CVDisplayLinkRef _displayLink;
    
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;
    CAMetalLayer *_metalLayer;
}

static uint64_t machTicksToNanoseconds(mach_timebase_info_data_t base, uint64_t ticks)
{
    return ticks * base.numer / base.denom;
}

static CVReturn dispatchRenderLoop(
    CVDisplayLinkRef displayLink,
    const CVTimeStamp* now,
    const CVTimeStamp* outputTime,
    CVOptionFlags flagsIn,
    CVOptionFlags* flagsOut,
    void* displayLinkContext) 
{
    (void) displayLink;
    (void) flagsIn;
    (void) flagsOut;

    MGInstance *inst = (__bridge MGInstance *) displayLinkContext;

    inst->_cvCallbackTimeNS = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

    inst->_timeNowNS = machTicksToNanoseconds(inst->_timebaseInfo, now->hostTime);
    inst->_timeRenderTargetNS = machTicksToNanoseconds(inst->_timebaseInfo, outputTime->hostTime);

    inst->_videoTimeNowNS = (1000 * 1000 * 1000) * now->videoTime / now->videoTimeScale;
    inst->_videoTimeRenderTargetNS = (1000 * 1000 * 1000) * outputTime->videoTime / outputTime->videoTimeScale;

    CFRunLoopSourceSignal(inst->_runLoopSource);
    CFRunLoopWakeUp(inst->_runLoop);

    return kCVReturnSuccess;
}

static void runRenderLoop(void *context)
{
    [((__bridge MGInstance *) context) render];
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self setupCVDisplayLink];

        [self setWantsLayer:true];

        /* It turns out that, by default, on macOS 13.5, this policy defaults to "RedrawNever" */
        [self setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];
    }
    return self;
}

- (void)setupCVDisplayLink
{
    CVReturn cvRet = kCVReturnSuccess;

    /* TODO: create links for each display; while you're at it, listen for display config changes.  */
    CGDirectDisplayID mainDisplayId = (CGDirectDisplayID) 
        [[[NSScreen mainScreen] deviceDescription][@"NSScreenNumber"] unsignedIntegerValue];
    
    cvRet = CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);

    if (cvRet != kCVReturnSuccess)
    {
        NSLog(@"CVDisplayLinkCreateWithActiveCGDisplays() failed.");
        exit(1);
    }

    _runLoop = CFRunLoopGetCurrent();

    CFRunLoopSourceContext ctx = (CFRunLoopSourceContext) {
        .info = (__bridge void *) self,
        .perform = runRenderLoop,
    };

    _runLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &ctx);

    if (!_runLoopSource)
    {
        NSLog(@"CFRunLoopSourceCreate() failed.");
        exit(1);
    }

    CFRunLoopAddSource(_runLoop, _runLoopSource, kCFRunLoopDefaultMode);

    CVDisplayLinkSetOutputCallback(_displayLink, dispatchRenderLoop, (__bridge void *) self);

    cvRet = CVDisplayLinkSetCurrentCGDisplay(_displayLink, mainDisplayId);

    if (cvRet != kCVReturnSuccess)
    {
        NSLog(@"CVDisplayLinkSetCurrentCGDisplay() failed.");
        exit(1);
    }
    
    CVDisplayLinkStart(_displayLink);
}

- (CALayer *)makeBackingLayer 
{
    _device = MTLCreateSystemDefaultDevice();

    _commandQueue = [_device newCommandQueue];

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly = YES;
    _metalLayer.frame = self.layer.frame;
    _metalLayer.delegate = self;

    return _metalLayer;
}

- (void)viewDidMoveToWindow 
{
    CGRect frame = self.frame;
    CGPoint o = frame.origin;
    CGSize s = frame.size;
    NSLog(@"viewDidMoveToWindow(): (%.0f, %.0f, %.0f, %.0f)", o.x, o.y, s.width, s.height);
}

- (void)displayLayer:(CALayer *)layer
{
    [self render];
}

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx 
{
    [self render];
}

- (void)drawRect:(NSRect)dirtyRect 
{
    [self render];
}

- (void)updateLayer
{
    [self render];
}

- (void)render
{
    uint64_t now = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

    uint64_t penalty = now - _cvCallbackTimeNS;
    uint64_t allowance = _timeRenderTargetNS - _timeNowNS;
    uint64_t commitToComplete = _cmdCompletedTimeNS - _cmdCommitTimeNS;

    NSLog(@"render(): penalty = %.3f(ms), allowance = %.3f(ms), commitToComplete = %.3f(ms)", penalty * 1e-6, allowance * 1e-6, commitToComplete * 1e-6);

    id <MTLCommandBuffer> buffer = [_commandQueue commandBuffer];

    id <CAMetalDrawable> drawable = [_metalLayer nextDrawable];

    if (!drawable)
    {
        return;
    }

    MTLRenderPassDescriptor *desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.colorAttachments[0].texture = drawable.texture;
    desc.colorAttachments[0].loadAction = MTLLoadActionClear;
    desc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

    // id <MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:desc];

    [buffer presentDrawable:drawable];

    [buffer addScheduledHandler:^(id<MTLCommandBuffer> buf) {
        _cmdScheduleTimeNS = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    }];

    [buffer addCompletedHandler:^(id<MTLCommandBuffer> buf) {
        _cmdCompletedTimeNS = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    }];

    [buffer commit];

    _cmdCommitTimeNS = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender 
{
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification 
{
    mg_init();

    mach_timebase_info(&_timebaseInfo);

    NSRect initFrame = NSMakeRect(0, 0, 960, 640);

    [self initWithFrame:initFrame];

    _window = [[NSWindow alloc] initWithContentRect:initFrame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];

    [_window setContentView:self];

    [_window center];
    [_window orderFrontRegardless];
}

- (void)applicationWillTerminate:(NSNotification *)notification 
{
    mg_shutdown();
}

@end /* implementation MGInstance */

int main(void)
{
    NSBundle *bundle = [NSBundle mainBundle];

    if (bundle)
    {
        NSLog(@"Bundle path: %@", [bundle bundlePath]);
    }

    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setDelegate:[MGInstance alloc]];
        [NSApp run];
    }
}