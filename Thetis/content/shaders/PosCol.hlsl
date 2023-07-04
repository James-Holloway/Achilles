struct ModelViewProjection
{
	matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
	float3 Position : POSITION;
	float3 Color : COLOR;
};

struct VS_OUT
{
	float4 Color : COLOR;
	float4 Position : SV_Position;
};

VS_OUT VS(VertexPosColor v)
{
	VS_OUT o;
	o.Position = mul(ModelViewProjectionCB.MVP, float4(v.Position, 1.0f));
	o.Color = float4(v.Color, 1.0f);
	
	return o;
}

float4 PS(VS_OUT i) : SV_Target
{
	return i.Color;
}