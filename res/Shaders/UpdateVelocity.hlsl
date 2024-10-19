RWStructuredBuffer<float3> position : register(u0);
RWStructuredBuffer<float3> previous_position : register(u1);
RWStructuredBuffer<float3> velocity : register(u2);
RWStructuredBuffer<float3> jacobi : register(u3);

cbuffer DeltaTime : register(b0)
{
    float dt;
    float idt;
    float2 padding_dt;
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

[numthreads(32, 32, 1)]
void CSMain(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    uint sx = gid.x * 32;
    uint sy = gid.y * 32;
    uint gx = sx + tid.x;
    uint gy = sy + tid.y;
    int i = gx + gy * 64;
    
    position[i] += 0.25 * jacobi[i];
    velocity[i] = (position[i] - previous_position[i]) * idt * 0.99975;
}
