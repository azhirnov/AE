
#define DS_RENDERPASS	0
#define DS_MATERIAL		1
#define DS_DRAWCALL		2
//-----------------------------------------------------------------------------

// @set 0 "PerPass"
// @set 1 "Material"
// @set 2 "DrawCall"

layout(set=DS_RENDERPASS, binding=0, std140) uniform un_GlobalConst
{
	ivec4	temp;
} globalConst;
//-----------------------------------------------------------------------------


layout(set=DS_MATERIAL, binding=0, std140) uniform un_Material
{
	vec4	temp;
} material;
//-----------------------------------------------------------------------------


layout(set=DS_DRAWCALL, binding=0, std140) uniform un_DrawCall
{
	vec4	temp;
} draw;
//-----------------------------------------------------------------------------

