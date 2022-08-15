#version 330 core

in vec2 texcoord;
out vec4 fColor;

uniform sampler2D gcolor;
uniform sampler2D gnormal;
uniform sampler2D gdepth;
uniform sampler2D gworldpos;
uniform sampler2D shadowtex;
uniform samplerCube skybox;

uniform float near;
uniform float far;

uniform mat4 shadowVP;
uniform vec3 lightPos;
uniform vec3 cameraPos;

float linearizeDepth(float depth,float near,float far)
{
    return (2.0 * near) / (far + near - depth * (far - near));
}

float shadowMapping(sampler2D tex,mat4 shadowVP,vec4 worldPos)
{
    vec4 lightPos = shadowVP*worldPos;
    lightPos = vec4(lightPos.xyz/lightPos.w,1.0);
    lightPos = lightPos*0.5+0.5;//这里将深度映射到了深度纹理的[0,1]区间

    if(lightPos.x<0||lightPos.x>1||lightPos.y<0||lightPos.y>1||lightPos.z<0||lightPos.z>1)
    {
        return 2.0;
    }
    float closestDepth = texture2D(tex,lightPos.xy).r;
    float currentDepth = lightPos.z;
    float isInShadow = (currentDepth>closestDepth+0.005) ? (1.0) : (0.0);

	return isInShadow;
}

struct PhongStruce
{
    float ambient;
    float diffuse;
    float specular;
};

PhongStruce phong(vec3 worldPos,vec3 cameraPos,vec3 lightPos,vec3 normal)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(worldPos-cameraPos);
    vec3 L = normalize(worldPos-lightPos);
    vec3 R = normalize(-V+(-L));

    PhongStruce phong;
    phong.ambient = 0.3;
    phong.diffuse = max(dot(N,-L),0);
    phong.specular = pow(max(dot(N,R),0),64)*1.1;

    return phong;
}

void main()
{

    
    fColor.rgb = texture2D(gcolor,texcoord).xyz;
    //fColor.rgb = texture(skybox,vec3(texcoord,1.0)).xyz;
    vec3 worldPos = texture2D(gworldpos,texcoord).xyz;
    vec3 normal = texture2D(gnormal,texcoord).xyz;

    float isInShadow = shadowMapping(shadowtex,shadowVP,vec4(worldPos,1.0));
    PhongStruce phong = phong(worldPos,cameraPos,lightPos,normal);
    
    // vec3 worldPos = texture2D(gworldpos,texcoord).xyz;
    // vec3 normal = texture2D(gnormal,texcoord).xyz;

    // vec3 V = normalize(worldPos-cameraPos);
    // vec3 R = reflect(V,normalize(normal));
    // fColor.rgb = texture(skybox,R).xyz;
    // float isInShadow = shadowMapping(shadowtex,shadowVP,vec4(worldPos,1.0));
    // PhongStruce phong = phong(worldPos,cameraPos,lightPos,normal);

    if(isInShadow==0)
    {
        fColor.rgb *= phong.ambient+phong.diffuse+phong.specular;
    }
    else if(isInShadow == 1)
    {
        fColor.rgb *= phong.ambient;
    }
}