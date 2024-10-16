RWStructuredBuffer<float3> position_buffer : register(u0);
RWStructuredBuffer<float3> previous_position : register(u1);

cbuffer DeltaTime : register(b0)
{
    float dt;
    float idt;
    float t;
    float padding_dt;
};

cbuffer Gravity : register(b1)
{
    float3 fg;
    float padding_g;
};

cbuffer Mass : register(b2)
{
    float mass;
    float imass;
    float2 padding_m;
};

cbuffer Wind : register(b3)
{
    float w_strength_multiplier;
    float3 w_direction;
}

float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719))
{
    return frac(sin(value * 143758.5453));
}

[numthreads(1024, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{   
    float3 velocity = (position_buffer[id.x] - previous_position[id.x]) * idt * 0.9995;
    previous_position[id.x] = position_buffer[id.x];
    
    float wind_s = rand3dTo1d(position_buffer[id.x] + t);
    wind_s *= wind_s * wind_s;
   
    velocity += dt * dt * imass * (fg + w_direction * w_strength_multiplier * wind_s);
    position_buffer[id.x] += dt * velocity;
}
