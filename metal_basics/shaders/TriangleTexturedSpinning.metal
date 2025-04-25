#include <metal_stdlib>

using namespace metal;

typedef struct 
{
    float2 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float2 uv [[attribute(2)]];
} VertexInput;

typedef struct
{
    float time;
} Uniform;

typedef struct 
{
    float4 position [[position]];
    float4 color;
    float2 uv;
} VertexOutput, FragmentInput;

vertex VertexOutput vertexShader(VertexInput in [[stage_in]],
                                 Uniform constant *uniform [[buffer(1)]]) 
{
    float time = uniform->time;
    float2x2 rotation(
        cos(time),  sin(time),
        sin(time), -cos(time)
    );
    return (VertexOutput) 
    {
        .position = float4(rotation * in.position, 0.0, 1.0),
        .color = in.color,
        .uv = in.uv,
    };
}

fragment float4 fragmentShader(FragmentInput in [[stage_in]],
                               texture2d<float> texture [[texture(0)]],
                               sampler sampler [[sampler(0)]])
{
    return texture.sample(sampler, in.uv);
}
