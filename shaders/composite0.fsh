#version 330 core
#define bottom 13
#define top 22
#define width 45

#extension GL_NV_shadow_samplers_cube : enable
#define baseBright vec3(1.26,1.25,1.29) 
#define baseDark vec3(0.31,0.31,0.31)
#define lightBright vec3(1.29, 1.17, 1.05)
#define lightDark vec3(0.7,0.75,0.8)

in vec3 pix;
in vec2 texcoord;
out vec4 fColor;

// 纹理数据
uniform sampler2D gcolor;
uniform sampler2D gnormal;
uniform sampler2D gdepth;
uniform sampler2D gworldpos;
uniform sampler2D shadowtex;	// shadow纹理
uniform sampler2D noisetex;
uniform samplerCube skybox;

// 透视投影近截面 / 远截面
uniform float near;
uniform float far;
uniform float u_time;

uniform mat4 shadowVP;  // 转换到光源坐标的变换矩阵

uniform vec3 lightPos;  // 光源位置
uniform vec3 cameraPos; // 相机位置


uint seed = uint(
    uint((pix.x * 0.5 + 0.5) * 512)  * uint(1973) + 
    uint((pix.y * 0.5 + 0.5) * 512) * uint(9277) + 
    uint(u_time) * uint(26699)) | uint(1);

uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}
 
float rand() {
    return float(wang_hash(seed)) / 4294967296.0;
}



// 屏幕深度转线性深度
float linearizeDepth(float depth, float near, float far) {
    return (2.0 * near) / (far + near - depth * (far - near));
}

// 阴影映射
float shadowMapping2(sampler2D tex, mat4 shadowVP, vec4 worldPos) {
	// 转换到光源坐标
	vec4 lightPos = shadowVP * worldPos;
	lightPos = vec4(lightPos.xyz/lightPos.w, 1.0);
	lightPos = lightPos*0.5 + 0.5;

    // 超出阴影贴图视野 -- 返回一个特殊值
    if(lightPos.x<0 || lightPos.x>1 || lightPos.y<0 || lightPos.y>1 || lightPos.z<0 || lightPos.z>1) {
        return 2.0;
    }

	// 计算shadowmapping
	float closestDepth = texture2D(tex, lightPos.xy).r;	// shadowmap中最近点的深度
	float currentDepth = lightPos.z;	// 当前点的深度
	float isInShadow = (currentDepth>closestDepth+0.005) ? (1.0) : (0.0);

	return isInShadow;
}

float shadowMapping(sampler2D tex, mat4 shadowVP, vec4 worldPos) {
	// 转换到光源坐标
	vec4 lightPos = shadowVP * worldPos;
	lightPos = vec4(lightPos.xyz/lightPos.w, 1.0);
	lightPos = lightPos*0.5 + 0.5;

    // 超出阴影贴图视野 -- 返回一个特殊值
    if(lightPos.x<0 || lightPos.x>1 || lightPos.y<0 || lightPos.y>1 || lightPos.z<0 || lightPos.z>1) {
        return 2.0;
    }

    int radius = 1;
    float sum = pow(radius*2+1,2);
    float shadowStrength = 0.6;
    float currentDepth = lightPos.z;	// 当前点的深度
    for(int x = -radius;x<=radius;++x)
    {
        for(int y = -radius;y<=radius;++y)
        {
            vec2 offset = vec2(x,y)/4096;
            float closestDepth = texture2D(tex,lightPos.xy+offset).r;
            if(closestDepth+0.001<=currentDepth){
                sum-=1;
            }
        }
    }
    sum /= pow(radius*2+1, 2);
    return sum*shadowStrength + (1-shadowStrength); 
}


// phong 光照计算
struct PhongStruct
{
    float ambient;
    float diffuse;
    float specular;
};
PhongStruct phong(vec3 worldPos, vec3 cameraPos, vec3 lightPos, vec3 normal)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(worldPos - cameraPos);
    vec3 L = normalize(worldPos - lightPos);
    vec3 R = reflect(L, N);

    PhongStruct phong;
    phong.ambient = 0.3;
    phong.diffuse = max(dot(N, -L), 0);
    phong.specular = pow(max(dot(-R, V), 0), 50.0) * 1.1;

    return phong;
}
float getDensity(sampler2D noisetex,vec3 pos)
{
    //高度衰减
    float mid = (bottom+top)/2.0;
    float h = top-bottom;
    float weight = 1.0-2.0*abs(mid-pos.y)/ h;
    weight = pow(weight,0.5);

    vec2 coord = pos.xz*0.0025;
    float noise = texture2D(noisetex,coord+vec2(u_time*0.001)).x;
    noise += texture2D(noisetex,coord*3.5).x/3.5;
    noise += texture2D(noisetex,coord*12.5).x/ 12.5;
    noise += texture2D(noisetex,coord*42.87).x/42.87;
    noise /=1.4472;
    noise*=weight;
    if(noise<0.4) noise = 0.0;
    

    return noise;
}

vec4 getCloud(vec3 worldPos,vec3 cameraPos,vec3 lightPos)
{
    vec3 direction = normalize(worldPos - cameraPos);   // 视线射线方向
    vec3 step = direction * 0.35;   // 步长
    vec4 colorSum = vec4(0);        // 积累的颜色
    vec3 point = cameraPos;         // 从相机出发开始测试

    if(point.y <bottom)
    {
        point +=direction*(abs(bottom -cameraPos.y)/direction.y);
    }

    // ray marching
    for(int i=0; i<300; i++) {
        point += step;
        if(bottom>point.y || point.y>top || -width>point.x || point.x>width || -width>point.z || point.z>width) {
            continue;
        }
        if(length(worldPos-cameraPos)<length(point-cameraPos)) return vec4(0);
        float density = getDensity(noisetex,point);
        vec3 L = normalize(lightPos-point);
        float lightDensity = getDensity(noisetex,point+L);
        float delta = clamp(density-lightDensity,0.0,1.0);

        density*=0.5;
        vec3 base = mix(baseBright,baseDark,density)*density;
        vec3 light = mix(lightDark, lightBright,delta);
        vec4 color = vec4(base*light,density);   // 当前点的颜色
        colorSum = colorSum + color * (1.0 - colorSum.a);   // 与累积的颜色混合
    }

    return colorSum;
}


void main()
{   
    fColor.rgb = texture2D(gcolor, texcoord).rgb*1.2;
    vec3 worldPos = texture2D(gworldpos, texcoord).xyz;
    vec3 normal = texture2D(gnormal, texcoord).xyz;
    vec3 envVec = reflect(normalize(worldPos-cameraPos),normal);
    vec3 envColor;
    for(int i = 0;i<10;++i)
    {
        envColor += texture(skybox,normalize(envVec+vec3(rand(),rand(),rand()))).xyz;
    }
    envColor.x/=10.0;
    envColor.y/=10.0;
    envColor.z/=10.0;
    
    float envLight = dot(normal,envVec)*(0.2*envColor.r + 0.7*envColor.g +0.1*envColor.b)*1.5;

    float isInShadow = shadowMapping2(shadowtex, shadowVP, vec4(worldPos, 1.0));
    PhongStruct phong = phong(worldPos, cameraPos, lightPos, normal);

    
    //如果在阴影中则只有环境光
    if(isInShadow==0) {
        fColor.rgb *= phong.ambient + phong.diffuse + phong.specular+envLight;
        
    } else if(isInShadow==1.0) {
        fColor.rgb *= phong.ambient+envLight;  // only ambient
    }
    // fColor.rgb *= phong.ambient + phong.diffuse + phong.specular+envLight;
    // fColor.rgb*=shadowMapping(shadowtex,shadowVP,vec4(worldPos,1.0));
    vec4 cloud = getCloud(worldPos,cameraPos,lightPos);
    fColor.rgb = fColor.rgb*(1.0-cloud.a ) +cloud.rgb;
}
