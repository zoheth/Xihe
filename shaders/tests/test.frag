#version 450

//layout (set=0, binding=0) uniform sampler2D test_texture;

//layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 o_color;

//layout(set = 0 ,input_attachment_index = 0, binding = 0) uniform subpassInput i_light;

void main () 
{
	//vec4 light = subpassLoad(i_light);
	o_color = vec4(1.0, 1.0,1.0,0.5);
	// o_color = texture(test_texture, in_uv);
}
