#version 330 core

in vec3 worldPos;   // 当前片元的世界坐标
in vec2 texcoord;   // 纹理坐标
in vec3 normal;     // 法向量

uniform sampler2D texture;

void main()
{
    gl_FragData[0] = texture2D(texture, texcoord);  // 写入 gcolor
    gl_FragData[1] = vec4(normalize(normal), 0.0);  // 写入 gnormal
    gl_FragData[2] = vec4(worldPos, 1.0);           // 写入 gworldpos
}
