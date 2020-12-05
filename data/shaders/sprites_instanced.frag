#version 450 core
// sprites_instanced.frag 
// author: lshoek

uniform	sampler2D sprite;

uniform SpriteSheetUBO {
	vec2 sheetDims;
} ssUbo;

const float cutOff = 0.5;

in vec4 passNDC;
in vec4 passColor;
in float passRot;
flat in int passInstanceIndex;

out vec4 out_FragColor;

vec2 rotate(vec2 uv, vec2 offset, float theta) 
{
	vec2 pointCentered = uv - offset;
	mat2 rot = mat2(
		cos(theta), -sin(theta),
		sin(theta), cos(theta)
	);
	return (rot * pointCentered) + offset;
}

void main()
{
	float spriteIndex = mod(passInstanceIndex, ssUbo.sheetDims.x * ssUbo.sheetDims.y);
	vec2 spriteSize = 1.0/ssUbo.sheetDims;

	float idx = floor(mod(spriteIndex, ssUbo.sheetDims.x));
	float idy = floor(spriteIndex/ssUbo.sheetDims.x);
	vec2 spriteOffset = vec2(idx, idy)/ssUbo.sheetDims;

	// Sample full sprite
	vec2 uv = rotate(gl_PointCoord, vec2(0.5), passRot);
	uv = uv * spriteSize + spriteOffset;
	uv = clamp(uv, spriteOffset, spriteOffset + spriteSize);

	// Render point coords as color
	//vec4 col = vec4(gl_PointCoord.x, gl_PointCoord.y, 1.0, 1.0);

	// Tint by location inside volume
	//vec4 col = mix(texture(sprite, uv), passColor, passColor);

	vec4 col = texture(sprite, uv);

	if (col.a < cutOff) discard;
	out_FragColor = col;
}
