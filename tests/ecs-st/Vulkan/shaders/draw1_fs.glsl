
#include "ds.glsl"

layout(location=0) in  vec3  v_Color;

void main ()
{
	out_Color = vec4(v_Color, 1.0) + draw.temp;
}
