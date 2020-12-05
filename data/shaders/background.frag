#version 450 core

vec3 color_outer = vec3(16, 0, 43)/255.0;
vec3 color_inner = vec3(128, 255, 219)/255.0;

float edge = 0.5;
float diff = 1.0;

float flashScale = 0.375;
float timeScale = 3.0;

in vec3 passUVs;

uniform UBO {
	float t;
} ubo;
//uniform sampler2D	texture;

out vec4 out_Color;

void main() 
{
	//float dist = distance(passUVs.xy, vec2(0.5, 0.5)); // radial
	//float dist = distance(passUVs.x, 0.5); // horizontal
	float dist = distance(passUVs.y, 0.5); // vertical

	float pct = 1.0 - dist*2.0;
	float amp = ((sin(ubo.t*timeScale)+1.0)*0.5);
	float d = diff*0.5 + amp*flashScale;
	pct = smoothstep(edge-d, edge+d, pct)*0.5;

	vec3 col = mix(color_outer, color_inner, pct);
	//vec3 col = vec3(pct);

	//out_Color =  vec4(passUVs.x, passUVs.y, 1.0, 1.0); // debug
	out_Color = vec4(col, 1.0);
}
