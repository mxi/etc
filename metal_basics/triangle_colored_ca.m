/* triangle_colored_ca.m but this time using CAMetalLayer rather than MTKView */
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/Foundation.h>
#import <simd/vector_types.h>

typedef struct 
{
    vector_float2 position;
    vector_float4 color;
} Vertex;

@interface RenderView : NSView <CALayerDelegate> @end

@implementation RenderView
{
    id <MTLDevice> _device;
    id <MTLCommandQueue> _commandQueue;
    id <MTLBuffer> _vertexBuffer;
    id <MTLLibrary> _shaderLibrary;
    id <MTLFunction> _vertexShader;
    id <MTLFunction> _fragmentShader;
    id <MTLRenderPipelineState> _pipelineState;
    CAMetalLayer *_metalLayer;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self initMetal];
        [self initModel];
        [self initShaders];
        [self initPipeline];

        [self setWantsLayer:true];

        NSLog(@"%ld", (long) self.layerContentsRedrawPolicy);

        /* It turns out that, by default, on macOS 13.5, this policy defaults to "RedrawNever" for some reason */
        [self setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];
    }
    return self;
}

- (CALayer *)makeBackingLayer 
{
    _metalLayer = [CAMetalLayer layer];

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    // _metalLayer.framebufferOnly = YES;
    _metalLayer.frame = self.layer.frame;
    _metalLayer.delegate = self;

    NSLog(@"Backing layer made!");

    return _metalLayer;
}


- (void)initMetal
{
    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];
}

- (void)initModel
{
    static const Vertex vertices[] = 
    {
        /* position */       /* color */
        { {  0.0,  0.5 },    { 1.0, 0.0, 0.0, 1.0 } }, /* top (red) */
        { { -0.5, -0.5 },    { 0.0, 1.0, 0.0, 1.0 } }, /* bottom left (green) */
        { {  0.5, -0.5 },    { 0.0, 0.0, 1.0, 1.0 } }  /* bottom right (blue) */
    };

    _vertexBuffer = [_device newBufferWithBytes:vertices
                                            length:sizeof(vertices)
                                        options:MTLResourceStorageModeShared];
}

- (void)initShaders
{
    NSError *error = nil;

    NSString *shaderSource = [NSString stringWithContentsOfFile:@"../shaders/TriangleColored.metal" 
                                                       encoding:NSUTF8StringEncoding 
                                                           error:nil];
    if (!shaderSource)
    {
        NSLog(@"Failed to load shaders!");
        [NSApp terminate:nil];
    }

    _shaderLibrary = [_device newLibraryWithSource:shaderSource 
                                           options:nil
                                             error:&error];
    if (!_shaderLibrary)
    {
        NSLog(@"Failed to compile shaders: %@", error);
        [NSApp terminate:nil];
    }

    _vertexShader = [_shaderLibrary newFunctionWithName:@"vertexShader"];
    _fragmentShader = [_shaderLibrary newFunctionWithName:@"fragmentShader"];

    NSLog(@"vertexShader: %@", _vertexShader);
    NSLog(@"fragmentShader: %@", _fragmentShader);
}

- (void)initPipeline
{
    NSError *error = nil;

    // MTLVertexDescriptor *vertexDesc = [[MTLVertexDescriptor alloc] init];
    MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor new];
    
    /* How to assemble a vertex from potentially different bufferes */
    vertexDesc.attributes[0].offset = offsetof(Vertex, position);
    vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[0].bufferIndex = 0;

    vertexDesc.attributes[1].offset = offsetof(Vertex, color);
    vertexDesc.attributes[1].format = MTLVertexFormatFloat4;
    vertexDesc.attributes[1].bufferIndex = 0;

    /* 
     * These aren't really related to the attributes. This specifies how the n'th buffer (referenced by an attribute) 
     * is to be configured to fetch vertex data from.
     */
    vertexDesc.layouts[0].stride = sizeof(Vertex);
    vertexDesc.layouts[0].stepRate = 1;
    vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexBuffers[0].mutability = MTLMutabilityImmutable;
    pipelineDesc.vertexDescriptor = vertexDesc;
    pipelineDesc.vertexFunction = _vertexShader;
    pipelineDesc.fragmentFunction = _fragmentShader;
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDesc 
                                                             error:&error];

    if (!_pipelineState)
    {
        NSLog(@"Failed to construct pipeline: %@", error);
        [NSApp terminate:nil];
    }
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
    NSLog(@"displayLayer()");
    [self render];
}

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx 
{
    NSLog(@"drawLayer()");
    [self render];
}

- (void)drawRect:(NSRect)dirtyRect 
{
    NSLog(@"drawRect()");
    [self render];
}

- (void)updateLayer
{
    NSLog(@"updateLayer()");
    [self render];
}

- (void)render
{
    NSLog(@"render()");

    id <MTLCommandBuffer> buffer = [_commandQueue commandBuffer];

    id <CAMetalDrawable> drawable = [_metalLayer nextDrawable];

    if (!drawable)
    {
        /* skip this frame I guess */
        return;
    }

    MTLRenderPassDescriptor *desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.colorAttachments[0].texture = drawable.texture;
    desc.colorAttachments[0].loadAction = MTLLoadActionClear;
    desc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

    NSLog(@"RPD: %@", desc);

    id <MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:desc];

    [encoder setRenderPipelineState:_pipelineState];
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle 
                vertexStart:0 
                vertexCount:3];
    [encoder endEncoding];

    [buffer presentDrawable:drawable];

    [buffer commit];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate> @end

@implementation AppDelegate
{
    NSWindow *_window;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification 
{
    NSRect frame = NSMakeRect(0, 0, 960, 640);

    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"Metal Triangle";
    _window.contentView = [[RenderView alloc] init];
    
    [_window center];
    [_window orderFrontRegardless];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender 
{
    return YES;
}

@end /* impl AppDelegate */

int main(void)
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setDelegate:[AppDelegate alloc]];
        [NSApp run];
    }
}
