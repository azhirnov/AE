
#include "common.glsl"

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z = 1) in;
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// @set 0 "Compute"
layout(set=0, binding=0, rgba8) writeonly uniform image2D  un_OutImage;

void main ()
{
	float4 fragColor = float4(float(gl_LocalInvocationID.x) / float(gl_WorkGroupSize.x),
							  float(gl_LocalInvocationID.y) / float(gl_WorkGroupSize.y),
							  1.0, 0.0);

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), fragColor );
}
