#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#import <assert.h>
#import <stddef.h> /* offsetof */
#import <stdint.h>
#import <time.h>

typedef struct 
{
    vector_float2 position;
    vector_float4 color;
} Vertex;

@interface Renderer : NSObject<MTKViewDelegate> 
@end

@implementation Renderer
{
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLBuffer> _vertexBuffer;
    uint64_t _lastRenderTimeNS;
}

- (instancetype)initWithMetalKitView:(MTKView *)view 
{
    if (self = [super init]) 
    {
        _device = MTLCreateSystemDefaultDevice();
        view.device = _device;
        view.delegate = self;
        
        [self setupPipeline:view];
        [self setupVertexBuffer];
        [self tickNS];
        
        _commandQueue = [_device newCommandQueue];
    }
    return self;
}

- (void)setupPipeline:(MTKView *)view 
{
    NSError *error = nil;

    /* load shader sources */
    NSString *shaderSources = 
        [NSString stringWithContentsOfFile:@"../shaders/TriangleColored.metal" 
                                  encoding:NSUTF8StringEncoding 
                                     error:&error];
    if (!shaderSources)
    {
        NSLog(@"Failed to loader \"TriangleColored.metal\": %@", error);
        NSLog(@"Ensure you are running from the build directory!");
        [NSApp terminate:nil];
    }
    
    /* create library (collection of compiled bytecode shaders) */
    id<MTLLibrary> library = [_device newLibraryWithSource:shaderSources
                                                   options:nil 
                                                     error:&error];
    if (!library) 
    {
        NSLog(@"Failed to create library: %@", error);
        [NSApp terminate:nil];
    }
    
    /* handles to compiled *GPU machine code* shaders */
    id<MTLFunction> vertexFunction = 
        [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = 
        [library newFunctionWithName:@"fragmentShader"];

    /* setup vertex descriptor (how vertex data in a buffer is laid out) */
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor new];
    /* attrib(position) */
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[0].offset = offsetof(Vertex, position);
    vertexDescriptor.attributes[0].bufferIndex = 0;
    /* attrib(color) */
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset = offsetof(Vertex, color);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    /* layout */
    vertexDescriptor.layouts[0].stride = sizeof(Vertex);
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    /* setup pipeline descriptor */
    MTLRenderPipelineDescriptor *pipelineDescriptor = 
        [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    
    /* 
     * Create the pipeline.
     *
     * It can be thought of as shader configuration. Everything that needs to be
     * fixed to make an execution of a shader fast (formats, layouts, the
     * shaders themselves, etc.) must all be specified in one bundle.
     */
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor 
                                                             error:&error];
    if (!_pipelineState) 
    {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)setupVertexBuffer 
{
    static const Vertex vertices[] = {
        /* position */       /* color */
        { {  0.0,  0.5 },    { 1.0, 0.0, 0.0, 1.0 } }, /* top (red) */
        { { -0.5, -0.5 },    { 0.0, 1.0, 0.0, 1.0 } }, /* bottom left (green) */
        { {  0.5, -0.5 },    { 0.0, 0.0, 1.0, 1.0 } }  /* bottom right (blue) */
    };
    _vertexBuffer = [_device newBufferWithBytes:vertices
                                         length:sizeof(vertices)
                                        options:MTLResourceStorageModeShared];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size 
{
    /* Respond to drawable size changes if needed */
}

- (void)drawInMTKView:(MTKView *)view 
{
    NSWindow *window = [view window];
    if (window)
    {
        double frameRate = 1.0 / [self tick];
        window.title = [NSString stringWithFormat:@"FPS: %.0f", frameRate];
    }

    id<MTLCommandBuffer> buffer = [_commandQueue commandBuffer];
    
    /* 
     * Obtain the render pass descriptor. This is the configuration of the render target
     * for the screen including the pixel format, antialiasing, other color, stencil,
     * and maybe depth attatchments.
     */
    MTLRenderPassDescriptor *desc = view.currentRenderPassDescriptor;
    if (desc) 
    {
        /* converts our rendering commands to GPU's/driver's message format. */
        id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:desc];
        
        [encoder setRenderPipelineState:_pipelineState];
        [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:3];
        [encoder endEncoding];
        
        /* present the drawable (the drawable tracks when  */
        [buffer presentDrawable:view.currentDrawable];
    }
    
    /* commit buffer */
    [buffer commit];
}

- (double)tick
{
    return 1e-9 * (double) [self tickNS];
}

- (uint64_t)tickNS
{
    uint64_t timeNowNS = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    uint64_t elapsedNS = timeNowNS - _lastRenderTimeNS;
    _lastRenderTimeNS = timeNowNS;
    return elapsedNS;
}

@end /* Renderer */

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *mtkView;
@property (nonatomic, strong) Renderer *renderer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification 
{
    /* create the window */
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"Metal Triangle";
    
    /* create metal view */
    _mtkView = [[MTKView alloc] initWithFrame:frame];
    _mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _mtkView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    _window.contentView = _mtkView;
    
    /* initial renderer */
    _renderer = [[Renderer alloc] initWithMetalKitView:_mtkView];
    
    /* show window */
    [_window center];
    [_window makeKeyAndOrderFront:nil];
}

- (bool)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender 
{
    return true;
}

- (void)applicationWillTerminate:(NSNotification *)notif 
{
    NSLog(@"Terminating!");
}

@end /* AppDelegate */

int main(void)
{
    @autoreleasepool 
    {
        AppDelegate *delegate = [[AppDelegate alloc] init];
        NSApplication.sharedApplication.delegate = delegate;
        [NSApplication.sharedApplication run];
    }
    return 0;
}
