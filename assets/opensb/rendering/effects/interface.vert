#version 130

uniform vec2 textureSize0;
uniform vec2 textureSize1;
uniform vec2 textureSize2;
uniform vec2 textureSize3;
uniform vec2 screenSize;
uniform mat3 vertexTransform;

in vec2 vertexPosition;
in vec4 vertexColor;
in vec2 vertexTextureCoordinate;
in int vertexData;

out vec2 fragmentTextureCoordinate;
flat out int fragmentTextureIndex;
out vec4 fragmentColor;

void main() {
  vec2 screenPosition = (vertexTransform * vec3(vertexPosition, 1.0)).xy;
  gl_Position = vec4(screenPosition / screenSize * 2.0 - 1.0, 0.0, 1.0);
  
  int vertexTextureIndex = vertexData & 0x3;
  if (vertexTextureIndex == 3)
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize3;
  else if (vertexTextureIndex == 2)
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize2;
  else if (vertexTextureIndex == 1)
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize1;
  else
    fragmentTextureCoordinate = vertexTextureCoordinate / textureSize0;

  fragmentTextureIndex = vertexTextureIndex;
  fragmentColor = vertexColor;
}