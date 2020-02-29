
#include "common.glsl"

layout(location=0) in  float2  in_Texcoord;

layout(set=DS_MATERIAL, binding=0) uniform sampler2D  un_ColorTexture;

void main ()
{
	out_Color = texture( un_ColorTexture, in_Texcoord );
}
