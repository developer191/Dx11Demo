//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer cbNeverChanges : register( b0 )
{
    matrix View;
};

cbuffer cbChangeOnResize : register( b1 )
{
    matrix Projection;
};

cbuffer cbChangesEveryFrame : register( b2 )
{
    matrix World;
    float4 vMeshColor;
    float2 fBrightnessParam;
};

float4 CorrectColor(float4 val)
{
    static float3x3 outMatrix = {
        0.539008f, 0.380667f, 0.0194538f,
        -0.167682f, 1.16332f, -0.0151995f,
        -0.0524694f, 0.158219f, 0.911538f
    };
    float3 correctRGB = mul(outMatrix, val.xyz) + fBrightnessParam.x;
    correctRGB = clamp(correctRGB, fBrightnessParam.x, 0.99f);
    correctRGB = pow(abs(correctRGB), 1 / fBrightnessParam.y);
    return float4(correctRGB, 1.0f);
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

float4 PS( PS_INPUT input) : SV_Target
{
    return CorrectColor(txDiffuse.Sample(samLinear, input.Tex) * vMeshColor);
    //return txDiffuse.Sample(samLinear, input.Tex) * vMeshColor;
}
