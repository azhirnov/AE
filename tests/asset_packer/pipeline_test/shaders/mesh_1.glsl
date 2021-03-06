
#include "common.glsl"

#extension GL_NV_mesh_shader : require

layout(local_size_x=3) in;
layout(triangles) out;
layout(max_vertices=3, max_primitives=1) out;

out gl_MeshPerVertexNV {
	vec4	gl_Position;
} gl_MeshVerticesNV[]; // [max_vertices]

layout(location = 0) out MeshOutput {
	vec4	color;
} Output[]; // [max_vertices]

const vec2	g_Positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

const vec3	g_Colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main ()
{
	const uint I = gl_LocalInvocationID.x;

	gl_MeshVerticesNV[I].gl_Position	= vec4( g_Positions[I], 0.0, 1.0 );
	Output[I].color						= vec4( g_Colors[I], 1.0 );
	gl_PrimitiveIndicesNV[I]			= I;

	if ( I == 0 )
		gl_PrimitiveCountNV = 1;
}
