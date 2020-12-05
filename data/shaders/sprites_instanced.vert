#version 450 core
// sprites_instanced.vert 
// author: lshoek

uniform nap
{
  mat4 projectionMatrix;
  mat4 viewMatrix;
  mat4 modelMatrix;
} mvp;

uniform UBO
{
  float t;
  float pointSize;
  float volumeScale;
  float timeScale;
  float variationScale;
  vec3 cameraPosition;
  int volumeSize;
} ubo;

in vec3 in_Position;
//in vec4 color;

out vec4 passNDC; // normalized device coordinate
out vec4 passColor;
out float passRot;
flat out int passInstanceIndex;

float mod289(float x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x)   { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x)   { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x)   { return x - floor(x * (1.0 / 289.0)) * 289.0; }

float rand(vec2 co) { return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
float rand (vec2 co, float l) {return rand(vec2(rand(co), l));}
// float rand (vec2 co, float l, float t) {return rand(vec2(rand(co, l), t));}

float permute(float x) { return mod289(((x*34.0)+1.0)*x); }
vec3 permute(vec3 x)   { return mod289(((x*34.0)+1.0)*x); }
vec4 permute(vec4 x)   { return mod289(((x*34.0)+1.0)*x); }

float taylorInvSqrt(float r) { return 1.79284291400159 - 0.85373472095314 * r; }
vec4 taylorInvSqrt(vec4 r)   { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v);

void main()
{
	// retrieve indices
	int id = gl_InstanceIndex;
	int x_index = id/(ubo.volumeSize*ubo.volumeSize);
	int y_index = (id-x_index*ubo.volumeSize*ubo.volumeSize)/ubo.volumeSize; 
	int z_index = id-x_index*ubo.volumeSize*ubo.volumeSize - y_index*ubo.volumeSize;

	// convert to locations
  vec4 idlePos = vec4(
    (float(x_index) - ubo.volumeSize*0.5) * ubo.volumeScale,
    (float(y_index) - ubo.volumeSize*0.5) * ubo.volumeScale,
    (float(z_index) - ubo.volumeSize*0.5) * ubo.volumeScale,
    1.0);

	// id
	float noise_id = snoise(idlePos.xyz);

	// generate variation
	float swayNoiseX = snoise(idlePos.xyz * ubo.t * ubo.timeScale) * ubo.variationScale;
	float swayNoiseY = snoise(idlePos.zyx * ubo.t * ubo.timeScale) * ubo.variationScale;
	float swayNoiseZ = snoise(idlePos.yxz * ubo.t * ubo.timeScale) * ubo.variationScale;

  vec4 worldPos = vec4(
    in_Position.x + idlePos.x + swayNoiseX,
    in_Position.y + idlePos.y + swayNoiseY,
    in_Position.z + idlePos.z + swayNoiseZ,
    1.0);

	float rot_sign = mix(-1.0, 1.0, step(noise_id, 0.5));
	float rot_mult = 48.0;

	passRot = ubo.t * rot_mult * ubo.timeScale * noise_id * rot_sign;

	vec4 transPos = mvp.projectionMatrix * mvp.viewMatrix * mvp.modelMatrix * worldPos;
	passNDC = transPos / transPos.w;

  passColor = vec4(
    float(x_index) / ubo.volumeSize,
    float(y_index) / ubo.volumeSize,
    float(z_index) / ubo.volumeSize,
    0.0);

  passInstanceIndex = id;

	gl_Position = transPos;

	// point texture size
	float p_size = ubo.pointSize + (ubo.pointSize*noise_id) + noise_id;
	gl_PointSize = p_size / distance(worldPos.xyz, ubo.cameraPosition);
}



//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
// 

float snoise(vec3 v)
{ 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}
