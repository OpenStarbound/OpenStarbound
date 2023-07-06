#version 110

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;

varying vec2 fragmentTextureCoordinate;
varying float fragmentTextureIndex;
varying vec4 fragmentColor;

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

  gl_FragColor = texColor * fragmentColor;
}