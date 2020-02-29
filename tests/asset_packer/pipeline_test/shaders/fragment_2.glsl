
#include "common.glsl"

layout(location=0) in  float2  in_Texcoord;

layout(location=0) out float4  out_Color;

layout(set=DS_MATERIAL, binding=0) uniform sampler2D  un_ColorTexture;

void main ()
{
	out_Color = texture( un_ColorTexture, in_Texcoord );
}
