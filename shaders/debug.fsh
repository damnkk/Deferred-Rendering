#version 330 core

in vec2 texcoord;
out vec4 fColor;

uniform sampler2D gcolor;
uniform sampler2D gnormal;
uniform sampler2D gworldpos;
uniform sampler2D gdepth;
uniform sampler2D shadowTexture;
uniform sampler2D viewnormal;
uniform sampler2D viewpos;

uniform float near;
uniform float far;

float linearizeDepth(float depth,float near,float far)
{
    return (2.0 * near) / (far + near - depth * (far - near));
}

void main()
{

    if(0<=texcoord.x && texcoord.x<=0.5 && 0<=texcoord.y && texcoord.y<=0.5)
    {
        vec2 coord = vec2(texcoord.x*2, texcoord.y*2);
        fColor = vec4(texture2D(gcolor, coord).rgb, 1); 
    }

    // 屏幕右下显示 gnormal
    if(0.5<=texcoord.x && texcoord.x<=1 && 0<=texcoord.y && texcoord.y<=0.5)
    {
        vec2 coord = vec2(texcoord.x*2-1, texcoord.y*2);
        fColor = vec4(texture2D(viewnormal, coord).rgb, 1); 
    }

    // 屏幕左上显示 gdepth
    if(0<=texcoord.x && texcoord.x<=0.5 && 0.5<=texcoord.y && texcoord.y<=1)
    {
        vec2 coord = vec2(texcoord.x*2, texcoord.y*2-1);
        float d = linearizeDepth(texture2D(gdepth, coord).r, near, far);
        //d = texture2D(shadowtex, coord).r;
        fColor = vec4(vec3(d*0.5+0.5), 1); 
        //fColor = vec4(texture2D(gcolor, coord).rgb,1.0);

        
    }

    // 屏幕右上显示 gworldpos
    if(0.5<=texcoord.x && texcoord.x<=1 && 0.5<=texcoord.y && texcoord.y<=1)
    {
        vec2 coord = vec2(texcoord.x*2-1, texcoord.y*2-1);
        fColor = vec4(texture2D(viewpos, coord).rgb, 1); 
    }
    if(0.4<=texcoord.x && texcoord.x<=0.6 && 0.4<=texcoord.y && texcoord.y<=0.6)
    {
        vec2 coord = vec2(texcoord.x*2-0.5, texcoord.y*2-0.5);
        fColor = vec4(texture2D(shadowTexture, coord).rgb, 1); 
    }

}
