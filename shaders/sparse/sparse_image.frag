#version 450
#extension GL_ARB_sparse_texture2 : enable

layout(binding = 1) uniform sampler2D tex;

layout(binding = 2) uniform SettingsData {
	bool color_highlight;
	int  min_lod;
	int  max_lod;
} settings;

layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 out_color;

vec3 color_blend_table[5] = 
{
	{1.00, 1.00, 1.00},
	{0.80, 0.60, 0.40},
	{0.60, 0.80, 0.60},
	{0.40, 0.60, 0.80},
	{0.20, 0.20, 0.20},
};

void main()
{
	vec4 color = vec4(0.0);

	int lod = settings.min_lod;
	int residency_code = sparseTextureLodARB(tex, texcoord, lod, color);

	for(++lod; (lod <= settings.max_lod) && !sparseTexelsResidentARB(residency_code); ++lod)
	{
		residency_code = sparseTextureLodARB(tex, texcoord, lod, color);
	}

	if(settings.color_highlight)
	{
		color.rgb *= color_blend_table[lod - 1];
	}
	out_color = color;
}