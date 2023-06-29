#version 110

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform bool lightMapEnabled;
uniform vec2 lightMapSize;
uniform vec2 tileLightMapSize;
uniform sampler2D lightMap;
uniform sampler2D tileLightMap;
uniform float lightMapMultiplier;

varying vec2 fragmentTextureCoordinate;
varying float fragmentTextureIndex;
varying vec4 fragmentColor;
varying float fragmentLightMapMultiplier;
varying vec2 fragmentLightMapCoordinate;

vec4 cubic(float v) {
  vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
  vec4 s = n * n * n;
  float x = s.x;
  float y = s.y - 4.0 * s.x;
  float z = s.z - 4.0 * s.y + 6.0 * s.x;
  float w = 6.0 - x - y - z;
  return vec4(x, y, z, w);
}

vec4 bicubicSample(sampler2D texture, vec2 texcoord, vec2 texscale) {
  texcoord = texcoord - vec2(0.5, 0.5);

  float fx = fract(texcoord.x);
  float fy = fract(texcoord.y);
  texcoord.x -= fx;
  texcoord.y -= fy;

  vec4 xcubic = cubic(fx);
  vec4 ycubic = cubic(fy);

  vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
  vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
  vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

  vec4 sample0 = texture2D(texture, vec2(offset.x, offset.z) * texscale);
  vec4 sample1 = texture2D(texture, vec2(offset.y, offset.z) * texscale);
  vec4 sample2 = texture2D(texture, vec2(offset.x, offset.w) * texscale);
  vec4 sample3 = texture2D(texture, vec2(offset.y, offset.w) * texscale);

  float sx = s.x / (s.x + s.y);
  float sy = s.z / (s.z + s.w);

  return mix(
    mix(sample3, sample2, sx),
    mix(sample1, sample0, sx), sy);
}

vec4 sampleLightMap(vec2 texcoord, vec2 texscale) {
  vec4 a = bicubicSample(lightMap, texcoord, texscale);
  vec4 b = bicubicSample(tileLightMap, texcoord, texscale);
  return mix(a, b, b.z);
}

void main() {
  vec4 texColor;
  if (fragmentTextureIndex > 2.9) {
    texColor = texture2D(texture3, fragmentTextureCoordinate);
  } else if (fragmentTextureIndex > 1.9) {
    texColor = texture2D(texture2, fragmentTextureCoordinate);
  } else if (fragmentTextureIndex > 0.9) {
    texColor = texture2D(texture1, fragmentTextureCoordinate);
  } else {
    texColor = texture2D(texture0, fragmentTextureCoordinate);
  }
  if (texColor.a <= 0.0)
    discard;

  vec4 finalColor = texColor * fragmentColor;
  float finalLightMapMultiplier = fragmentLightMapMultiplier * lightMapMultiplier;
  if (lightMapEnabled && finalLightMapMultiplier > 0.0)
    finalColor.rgb *= sampleLightMap(fragmentLightMapCoordinate, 1.0 / lightMapSize).rgb * finalLightMapMultiplier;
  gl_FragColor = finalColor;
}