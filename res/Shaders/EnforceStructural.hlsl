struct LinearConstraint
{
    int idx_a;
    int idx_b;
    float distance;
    float padding;
};

RWStructuredBuffer<float3> position_buffer : register(u0);
StructuredBuffer<LinearConstraint> constraints : register(t0);

cbuffer DeltaTime : register(b0)
{
    float dt;
    float idt;
    float2 padding_dt;
};

cbuffer Compliance : register(b1)
{
    float alpha;
    float3 padding_c;
};

cbuffer Mass : register(b2)
{
    float mass;
    float imass;
    float2 padding_m;
};

cbuffer Resolution : register(b4)
{
    uint resolution;
    uint z_multiplier;
    float2 padding_r;
}

[numthreads(32, 4, 1)]
void CSMain(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    uint sz = z_multiplier * gid.z;
    uint sx = gid.x * 32;
    uint sy = gid.y * 4;
    uint gx = sx + tid.x;
    uint gy = sy + tid.y;
    int i = sz + gx + gy * resolution;
    
    if(i >= sz + z_multiplier)
    {
        return;
    }
    
    LinearConstraint constraint = constraints[i];
    float3 a = position_buffer[constraint.idx_a];
    float3 b = position_buffer[constraint.idx_b];
    
    float3 diff = a - b;
    float dst_sqr = dot(diff, diff);
    float idst = rsqrt(dst_sqr + 0.000001);
    float dst = dst_sqr * idst;
    
    
    float C = dst - constraint.distance;
    float3 ga = diff * idst;
    
    float lambda = -C / (imass * (2.0) + alpha * idt);
    float m = lambda * imass;
    a += m * ga;
    b -= m * ga;
    
    position_buffer[constraint.idx_a] = a;
    position_buffer[constraint.idx_b] = b;
}
