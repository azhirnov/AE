
#include "common.glsl"

layout(location=0) in  float3  in_Position;
layout(location=1) in  float2  in_Texcoord;

layout(location=0) out float2  out_Texcoord;

layout(set=DS_DRAWCMD, binding=0) uniform DrawCmdUBuf
{
	mat4x4		mvp;

} drawUB;


void main ()
{
	gl_Position  = drawUB.mvp * float4(in_Position, 1.0);
	out_Texcoord = in_Texcoord;
}
