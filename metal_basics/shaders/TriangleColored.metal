#include <metal_stdlib>

using namespace metal;

typedef struct 
{
    float2 position [[attribute(0)]];
    float4 color [[attribute(1)]];
} VertexInput;

typedef struct
{
    float2 position;
    float4 color;
} Test;

typedef struct 
{
    float4 position [[position]];
    float4 color;
} VertexOutput, FragmentInput;

vertex VertexOutput vertexShader(VertexInput in [[stage_in]]) 
{
    return (VertexOutput) {
        .position = float4(in.position, 0.0, 1.0),
        .color = in.color,
    };
}

fragment float4 fragmentShader(FragmentInput in [[stage_in]]) 
{
    return in.color;
}
