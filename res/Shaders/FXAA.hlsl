struct VertexInput
{
    float4 position : POSITION;
    float4 normal   : NORMAL;
};

struct PixelInput
{
    float4 projected_position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PixelInput VSMain(VertexInput input)
{
    PixelInput output;

    output.projected_position = input.position;
    
    float4 p = (input.position + 1.0) * 0.5;
    p.y = 1.0 - p.y;
    float4 uv = p;
  
    output.uv = uv;
    
    return output;
}

SamplerState texture_sampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

Texture2D screen_texture : register(t0);

float4 PSMain(PixelInput input) : SV_TARGET
{   
    float3 color = screen_texture.Sample(texture_sampler, input.uv).rgb;   

    return float4(color, 1.0);
}
