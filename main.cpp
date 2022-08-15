// std c++
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>

// glew glut
#include <GL/glew.h>
#include <GL/freeglut.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// SOIL
#include <SOIL2/SOIL2.h>

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// --------------------- end of include --------------------- //

class Mesh
{
public:
    // OpenGL 对象
    GLuint vao, vbo, ebo;
    GLuint diffuseTexture;  // 漫反射纹理

    // 顶点属性
    std::vector<glm::vec3> vertexPosition;
    std::vector<glm::vec2> vertexTexcoord;
    std::vector<glm::vec3> vertexNormal;

    // glDrawElements 函数的绘制索引
    std::vector<int> index;

    Mesh() {}
    void bindData()
    {
        // 创建顶点数组对象
        glGenVertexArrays(1, &vao); // 分配1个顶点数组对象
        glBindVertexArray(vao);  	// 绑定顶点数组对象

        // 创建并初始化顶点缓存对象 这里填NULL 先不传数据
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
            vertexPosition.size() * sizeof(glm::vec3) +
            vertexTexcoord.size() * sizeof(glm::vec2) +
            vertexNormal.size() * sizeof(glm::vec3),
            NULL, GL_STATIC_DRAW);

        // 传位置
        GLuint offset_position = 0;
        GLuint size_position = vertexPosition.size() * sizeof(glm::vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset_position, size_position, vertexPosition.data());
        glEnableVertexAttribArray(0);   // 着色器中 (layout = 0) 表示顶点位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_position));
        // 传纹理坐标
        GLuint offset_texcoord = size_position;
        GLuint size_texcoord = vertexTexcoord.size() * sizeof(glm::vec2);
        glBufferSubData(GL_ARRAY_BUFFER, offset_texcoord, size_texcoord, vertexTexcoord.data());
        glEnableVertexAttribArray(1);   // 着色器中 (layout = 1) 表示纹理坐标
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_texcoord));
        // 传法线
        GLuint offset_normal = size_position + size_texcoord;
        GLuint size_normal = vertexNormal.size() * sizeof(glm::vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset_normal, size_normal, vertexNormal.data());
        glEnableVertexAttribArray(2);   // 着色器中 (layout = 2) 表示法线
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_normal));

        // 传索引到 ebo
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(GLuint), index.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
    void draw(GLuint program)
    {
        glBindVertexArray(vao);

        // 传纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        glUniform1i(glGetUniformLocation(program, "texture"), 0);

        // 绘制
        glDrawElements(GL_TRIANGLES, this->index.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

class Model
{
public:
    std::vector<Mesh> meshes;
    std::map<std::string, GLuint> textureMap;
    glm::vec3 translate = glm::vec3(0, 0, 0), rotate = glm::vec3(0, 0, 0), scale = glm::vec3(1, 1, 1);
    Model() {}
    void load(std::string filepath)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);
        // 异常处理
        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "读取模型出现错误: " << import.GetErrorString() << std::endl;
            exit(-1);
        }
        // 模型文件相对路径
        std::string rootPath = filepath.substr(0, filepath.find_last_of('/'));

        // 循环生成 mesh
        for (int i = 0; i < scene->mNumMeshes; i++)
        {
            // 引用当前mesh
            meshes.push_back(Mesh());
            Mesh& mesh = meshes.back();

            // 获取 assimp 的读取到的 aimesh 对象
            aiMesh* aimesh = scene->mMeshes[i];

            // 我们将数据传递给我们自定义的mesh
            for (int j = 0; j < aimesh->mNumVertices; j++)
            {
                // 顶点
                glm::vec3 vvv;
                vvv.x = aimesh->mVertices[j].x;
                vvv.y = aimesh->mVertices[j].y;
                vvv.z = aimesh->mVertices[j].z;
                mesh.vertexPosition.push_back(vvv);

                // 法线
                vvv.x = aimesh->mNormals[j].x;
                vvv.y = aimesh->mNormals[j].y;
                vvv.z = aimesh->mNormals[j].z;
                mesh.vertexNormal.push_back(vvv);

                // 纹理坐标: 如果存在则加入。assimp 默认可以有多个纹理坐标 我们取第一个（0）即可
                glm::vec2 vv(0, 0);
                if (aimesh->mTextureCoords[0])
                {
                    vv.x = aimesh->mTextureCoords[0][j].x;
                    vv.y = aimesh->mTextureCoords[0][j].y;
                }
                mesh.vertexTexcoord.push_back(vv);
            }

            // 如果有材质，那么传递材质
            if (aimesh->mMaterialIndex >= 0)
            {
                // 获取当前 aimesh 的材质对象
                aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];

                // 获取 diffuse 贴图文件路径名称 我们只取1张贴图 故填 0 即可
                aiString aistr;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &aistr);
                std::string texpath = aistr.C_Str();
                texpath = rootPath + '/' + texpath;   // 取相对路径

                // 如果没生成过纹理，那么生成它
                if (textureMap.find(texpath) == textureMap.end())
                {
                    // 生成纹理
                    GLuint tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                    int textureWidth, textureHeight;
                    unsigned char* image = SOIL_load_image(texpath.c_str(), &textureWidth, &textureHeight, 0, SOIL_LOAD_RGB);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);   // 生成纹理
                    delete[] image;

                    textureMap[texpath] = tex;
                }

                // 传递纹理
                mesh.diffuseTexture = textureMap[texpath];
            }

            // 传递面片索引
            for (GLuint j = 0; j < aimesh->mNumFaces; j++)
            {
                aiFace face = aimesh->mFaces[j];
                for (GLuint k = 0; k < face.mNumIndices; k++)
                {
                    mesh.index.push_back(face.mIndices[k]);
                }
            }

            mesh.bindData();
        }
    }
    void draw(GLuint program)
    {
        // 传模型矩阵
        glm::mat4 unit(    // 单位矩阵
            glm::vec4(1, 0, 0, 0),
            glm::vec4(0, 1, 0, 0),
            glm::vec4(0, 0, 1, 0),
            glm::vec4(0, 0, 0, 1)
        );
        glm::mat4 scale = glm::scale(unit, this->scale);
        glm::mat4 translate = glm::translate(unit, this->translate);

        glm::mat4 rotate = unit;    // 旋转
        rotate = glm::rotate(rotate, glm::radians(this->rotate.x), glm::vec3(1, 0, 0));
        rotate = glm::rotate(rotate, glm::radians(this->rotate.y), glm::vec3(0, 1, 0));
        rotate = glm::rotate(rotate, glm::radians(this->rotate.z), glm::vec3(0, 0, 1));

        // 模型变换矩阵
        glm::mat4 model = translate * rotate * scale;
        GLuint mlocation = glGetUniformLocation(program, "model");    // 名为model的uniform变量的位置索引
        glUniformMatrix4fv(mlocation, 1, GL_FALSE, glm::value_ptr(model));   // 列优先矩阵

        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i].draw(program);
        }
    }
};

class Camera
{
public:
    // 相机参数
    glm::vec3 position = glm::vec3(0, 0, 0);    // 位置
    glm::vec3 direction = glm::vec3(0, 0, -1);  // 视线方向
    glm::vec3 up = glm::vec3(0, 1, 0);          // 上向量，固定(0,1,0)不变
    float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;    // 欧拉角
    float fovy = 70.0f, aspect = 1.0, zNear = 0.01, zFar = 100; // 透视投影参数
    float left = -1.0, right = 1.0, top = 1.0, bottom = -1.0; // 正交投影参数
    Camera() {}
    // 视图变换矩阵
    glm::mat4 getViewMatrix(bool useEulerAngle = true)
    {
        if (useEulerAngle)  // 使用欧拉角更新相机朝向
        {
            direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
            direction.y = sin(glm::radians(pitch));
            direction.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw)); // 相机看向z轴负方向
        }
        return glm::lookAt(position, position + direction, up);
    }
    // 投影矩阵
    glm::mat4 getProjectionMatrix(bool usePerspective = true)
    {
        if (usePerspective) // 透视投影
        {
            return glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
        }
        return glm::ortho(left, right, bottom, top, zNear, zFar);
    }
};

// ---------------------------- end of class definition ---------------------------- //

// 模型
std::vector<Model> models;  // 场景
Model screen;   // 渲染一个四方形做屏幕
Model skybox;   // 渲染一个立方体用于立方体贴图绘制天空盒

// 着色器程序对象
GLuint program;
GLuint debugProgram;    // 调试用
GLuint shadowProgram;   // 绘制阴影的着色器程序对象
GLuint skyboxProgram;   // 天空盒绘制

// 纹理
GLuint skyboxTexture;   // 天空盒
GLuint shadowTexture;   // 阴影纹理

// 相机
Camera camera;          // 正常渲染
Camera shadowCamera;    // 从光源方向渲染

// 光源与阴影参数
int shadowMapResolution = 1024;             // 阴影贴图分辨率
GLuint shadowMapFBO;                        // 从光源方向进行渲染的帧缓冲

// glut与交互相关变量
int windowWidth = 512;  // 窗口宽
int windowHeight = 512; // 窗口高
bool keyboardState[1024];   // 键盘状态数组 keyboardState[x]==true 表示按下x键

// 延迟渲染阶段
GLuint gbufferProgram;
GLuint gbufferFBO;  // gbuffer 阶段帧缓冲
GLuint gcolor;      // 基本颜色纹理
GLuint gdepth;      // 深度纹理
GLuint gworldpos;   // 世界坐标纹理
GLuint gnormal;     // 法线纹理

// 后处理阶段
GLuint composite0;

// --------------- end of global variable definition --------------- //

// 读取文件并且返回一个长字符串表示文件内容
std::string readShaderFile(std::string filepath)
{
    std::string res, line;
    std::ifstream fin(filepath);
    if (!fin.is_open())
    {
        std::cout << "文件 " << filepath << " 打开失败" << std::endl;
        exit(-1);
    }
    while (std::getline(fin, line))
    {
        res += line + '\n';
    }
    fin.close();
    return res;
}

// 获取着色器对象
GLuint getShaderProgram(std::string fshader, std::string vshader)
{
    // 读取shader源文件
    std::string vSource = readShaderFile(vshader);
    std::string fSource = readShaderFile(fshader);
    const char* vpointer = vSource.c_str();
    const char* fpointer = fSource.c_str();

    // 容错
    GLint success;
    GLchar infoLog[512];

    // 创建并编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)(&vpointer), NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);   // 错误检测
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "顶点着色器 " + vshader + " 编译错误\n" << infoLog << std::endl;
        exit(-1);
    }

    // 创建并且编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar**)(&fpointer), NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);   // 错误检测
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "片段着色器 " + fshader + " 编译错误\n" << infoLog << std::endl;
        exit(-1);
    }

    // 链接两个着色器到program对象
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 删除着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// 鼠标滚轮函数
void mouseWheel(int wheel, int direction, int x, int y)
{
    // zFar += 1 * direction * 0.1;
    glutPostRedisplay();    // 重绘
}

// 鼠标运动函数
void mouse(int x, int y)
{
    // 调整旋转
    camera.yaw += 35 * (x - float(windowWidth) / 2.0) / windowWidth;
    camera.yaw = glm::mod(camera.yaw + 180.0f, 360.0f) - 180.0f;    // 取模范围 -180 ~ 180

    camera.pitch += -35 * (y - float(windowHeight) / 2.0) / windowHeight;
    camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

    glutWarpPointer(windowWidth / 2.0, windowHeight / 2.0);
    glutPostRedisplay();    // 重绘
}

// 键盘回调函数
void keyboardDown(unsigned char key, int x, int y)
{
    keyboardState[key] = true;
}
void keyboardDownSpecial(int key, int x, int y)
{
    keyboardState[key] = true;
}
void keyboardUp(unsigned char key, int x, int y)
{
    keyboardState[key] = false;
}
void keyboardUpSpecial(int key, int x, int y)
{
    keyboardState[key] = false;
}
// 根据键盘状态判断移动
void move()
{
    float cameraSpeed = 0.0035f;
    // 相机控制
    if (keyboardState['w']) camera.position += cameraSpeed * camera.direction;
    if (keyboardState['s']) camera.position -= cameraSpeed * camera.direction;
    if (keyboardState['a']) camera.position -= cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
    if (keyboardState['d']) camera.position += cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
    if (keyboardState[GLUT_KEY_CTRL_L]) camera.position.y -= cameraSpeed;
    if (keyboardState[' ']) camera.position.y += cameraSpeed;
    // 光源位置控制
    if (keyboardState[GLUT_KEY_RIGHT]) shadowCamera.position.x += cameraSpeed;
    if (keyboardState[GLUT_KEY_LEFT]) shadowCamera.position.x -= cameraSpeed;
    if (keyboardState[GLUT_KEY_UP]) shadowCamera.position.y += cameraSpeed;
    if (keyboardState[GLUT_KEY_DOWN]) shadowCamera.position.y -= cameraSpeed;
    glutPostRedisplay();    // 重绘
}

GLuint loadCubemap(std::vector<const GLchar*> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glActiveTexture(GL_TEXTURE0);

    int width, height;
    unsigned char* image;

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for (GLuint i = 0; i < faces.size(); i++)
    {
        image = SOIL_load_image(faces[i], &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

// 初始化
void init()
{
    // 生成着色器程序对象
    gbufferProgram = getShaderProgram("shaders/gbuffer.fsh", "shaders/gbuffer.vsh");
    shadowProgram = getShaderProgram("shaders/shadow.fsh", "shaders/shadow.vsh");
    //debugProgram = getShaderProgram("shaders/debug.fsh", "shaders/debug.vsh");
    skyboxProgram = getShaderProgram("shaders/skybox.fsh", "shaders/skybox.vsh");
    composite0 = getShaderProgram("shaders/composite0.fsh", "shaders/composite0.vsh");

    // ------------------------------------------------------------------------ // 

    // 读取 obj 模型
    Model tree1 = Model();
    tree1.translate = glm::vec3(2.5, 0, 2);
    tree1.scale = glm::vec3(0.0025, 0.0025, 0.0025);
    tree1.load("models/tree/tree02.obj");
    models.push_back(tree1);

    Model tree2 = Model();
    tree2.translate = glm::vec3(10, 0, 7);
    tree2.scale = glm::vec3(0.0015, 0.0015, 0.0015);
    tree2.load("models/tree/tree02.obj");
    models.push_back(tree2);

    Model plane = Model();
    plane.translate = glm::vec3(0, -1.1, 0);
    plane.scale = glm::vec3(10, 10, 10);
    plane.rotate = glm::vec3(0, 0, 0);
    plane.load("models/plane/plane.obj");
    models.push_back(plane);

    // 光源位置标志物
    Model vlight = Model();
    vlight.translate = glm::vec3(1, 0, -1);
    vlight.rotate = glm::vec3(-90, 0, 0);
    vlight.scale = glm::vec3(0.03, 0.03, 0.03);
    vlight.load("models/duck/12248_Bird_v1_L2.obj");
    models.push_back(vlight);

    // ------------------------------------------------------------------------ // 

    // 生成一个四方形做荧幕 -- 用以显示纹理中的数据
    Mesh msquare;
    msquare.vertexPosition = { glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0) };
    msquare.vertexTexcoord = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1) };
    msquare.index = { 0,1,2,2,1,3 };
    msquare.bindData();
    screen.meshes.push_back(msquare);

    // ------------------------------------------------------------------------ //

    // 生成一个立方体做天空盒的 “画布”
    Mesh cube;
    cube.vertexPosition = { // 立方体的 8 个顶点
        glm::vec3(-1, -1, -1),glm::vec3(1, -1, -1),glm::vec3(-1, 1, -1),glm::vec3(1, 1, -1),
        glm::vec3(-1, -1, 1),glm::vec3(1, -1, 1),glm::vec3(-1, 1, 1),glm::vec3(1, 1, 1)
    };
    cube.index = { 0,3,1,0,2,3,1,5,4,1,4,0,4,2,0,4,6,2,5,6,4,5,7,6,2,6,7,2,7,3,1,7,5,1,3,7 };
    cube.bindData();
    skybox.meshes.push_back(cube);

    // 加载立方体贴图
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.jpg");
    faces.push_back("skybox/left.jpg");
    faces.push_back("skybox/top.jpg");
    faces.push_back("skybox/bottom.jpg");
    faces.push_back("skybox/front.jpg");
    faces.push_back("skybox/back.jpg");
    /*
    faces.push_back("skybox/DOOM16RT.png");
    faces.push_back("skybox/DOOM16LF.png");
    faces.push_back("skybox/DOOM16UP.png");
    faces.push_back("skybox/DOOM16DN.png");
    faces.push_back("skybox/DOOM16FT.png");
    faces.push_back("skybox/DOOM16BK.png");
    */
    skyboxTexture = loadCubemap(faces);

    // ------------------------------------------------------------------------ // 

    // 正交投影参数配置 -- 视界体范围 -- 调整到场景一般大小即可
    shadowCamera.left = -30;
    shadowCamera.right = 30;
    shadowCamera.bottom = -30;
    shadowCamera.top = 30;
    shadowCamera.position = glm::vec3(0, 4, 15);

    // 创建shadow帧缓冲
    glGenFramebuffers(1, &shadowMapFBO);
    // 创建阴影纹理
    glGenTextures(1, &shadowTexture);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 将阴影纹理绑定到 shadowMapFBO 帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ------------------------------------------------------------------------ // 

    // 创建 gubffer 帧缓冲
    glGenFramebuffers(1, &gbufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);

    // 创建颜色纹理
    glGenTextures(1, &gcolor);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 将颜色纹理绑定到 0 号颜色附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gcolor, 0);

    // 创建法线纹理
    glGenTextures(1, &gnormal);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 将法线纹理绑定到 1 号颜色附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gnormal, 0);

    // 创建世界坐标纹理
    glGenTextures(1, &gworldpos);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 将世界坐标纹理绑定到 2 号颜色附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gworldpos, 0);

    // 创建深度纹理
    glGenTextures(1, &gdepth);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 将深度纹理绑定到深度附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gdepth, 0);

    // 指定附件索引
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);	// 解绑

    // ------------------------------------------------------------------------ // 

    glEnable(GL_DEPTH_TEST);  // 开启深度测试
    glClearColor(1.0, 1.0, 1.0, 1.0);   // 背景颜色
}

// 显示回调函数
void display()
{
    move(); // 移动控制 -- 控制相机位置

    // 最后一个物体作为光源位置的标志物
    models.back().translate = shadowCamera.position + glm::vec3(0, 0, 2);

    // ------------------------------------------------------------------------ // 

    // 从光源方向进行渲染
    glUseProgram(shadowProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, shadowMapResolution, shadowMapResolution);

    // 光源看向世界坐标原点
    shadowCamera.direction = glm::normalize(glm::vec3(0, 0, 0) - shadowCamera.position);
    // 传视图矩阵
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "view"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getViewMatrix(false)));
    // 传投影矩阵
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "projection"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getProjectionMatrix(false)));

    // 从光源方向进行绘制
    for (auto m : models)
    {
        m.draw(shadowProgram);
    }

    // ------------------------------------------------------------------------ // 

    // 绘制天空盒 -- 输出到 gbuffer 阶段的 3 张纹理中
    glUseProgram(skyboxProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, windowWidth, windowHeight);

    // 传视图,投影矩阵
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

    // 传cubemap纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 1);

    // 传递 zfar 和 znear 方便让天空盒的坐标置于最大视距
    glUniform1f(glGetUniformLocation(skyboxProgram, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(skyboxProgram, "far"), camera.zFar);

    // 立方体永远跟随相机
    skybox.translate = camera.position;

    glDepthMask(GL_FALSE);
    skybox.draw(skyboxProgram);
    glDepthMask(GL_TRUE);

    // ------------------------------------------------------------------------ // 

    // 正常绘制 -- 输出到 gbuffer 阶段的 3 张纹理中
    glUseProgram(gbufferProgram);
 

    // 传视图矩阵
    glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
    // 传投影矩阵
    glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

    // 正常绘制
    for (auto m : models)
    {
        m.draw(gbufferProgram);
    }

    // ------------------------------------------------------------------------ // 

    // 后处理阶段： composite0 着色器进行渲染
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(composite0);
    glViewport(0, 0, windowWidth, windowHeight);

    // 传递 zfar 和 znear 方便转线性深度
    glUniform1f(glGetUniformLocation(composite0, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(composite0, "far"), camera.zFar);

    // 传 gcolor 纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glUniform1i(glGetUniformLocation(composite0, "gcolor"), 1);
    // 传 gnormal 纹理
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glUniform1i(glGetUniformLocation(composite0, "gnormal"), 2);
    // 传 gworldpos 纹理
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glUniform1i(glGetUniformLocation(composite0, "gworldpos"), 3);
    // 传 gdepth 纹理
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glUniform1i(glGetUniformLocation(composite0, "gdepth"), 4);
    // 传阴影深度纹理
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glUniform1i(glGetUniformLocation(composite0, "shadowtex"), 5);
    

    // 传递矩阵: 转换到光源坐标的变换矩阵
    glm::mat4 shadowVP = shadowCamera.getProjectionMatrix(false) * shadowCamera.getViewMatrix(false);
    glUniformMatrix4fv(glGetUniformLocation(composite0, "shadowVP"), 1, GL_FALSE, glm::value_ptr(shadowVP));

    // 传递光源位置
    glUniform3fv(glGetUniformLocation(composite0, "lightPos"), 1, glm::value_ptr(shadowCamera.position));
    // 传递相机位置
    glUniform3fv(glGetUniformLocation(composite0, "cameraPos"), 1, glm::value_ptr(camera.position));

    // 绘制
    screen.draw(composite0);
    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------------ // 

    /*
    // debug着色器输出一个四方形以显示纹理中的数据
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);   // 需要取消深度测试以保证其覆盖在原画面上
    glUseProgram(debugProgram);
    glViewport(0, 0, windowWidth, windowHeight);

    // 传递 zfar 和 znear 方便转线性深度
    glUniform1f(glGetUniformLocation(debugProgram, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(debugProgram, "far"), camera.zFar);

    // 传 gcolor 纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glUniform1i(glGetUniformLocation(debugProgram, "gcolor"), 1);
    // 传 gnormal 纹理
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glUniform1i(glGetUniformLocation(debugProgram, "gnormal"), 2);
    // 传 gworldpos 纹理
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glUniform1i(glGetUniformLocation(debugProgram, "gworldpos"), 3);
    // 传 gdepth 纹理
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glUniform1i(glGetUniformLocation(debugProgram, "gdepth"), 4);
    // 传阴影深度纹理
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glUniform1i(glGetUniformLocation(debugProgram, "shadowtex"), 5);

    // 绘制
    screen.draw(debugProgram);
    glEnable(GL_DEPTH_TEST);
    */

    // ------------------------------------------------------------------------ // 

    glutSwapBuffers();                  // 交换缓冲区
}

// -------------------------------- main -------------------------------- //

int main(int argc, char** argv)
{
    glutInit(&argc, argv);              // glut初始化
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);// 窗口大小
    glutCreateWindow("10 - deferred render"); // 创建OpenGL上下文

#ifdef __APPLE__
#else
    glewInit();
#endif

    init();

    // 绑定鼠标移动函数 -- 
    //glutMotionFunc(mouse);  // 左键按下并且移动
    glutPassiveMotionFunc(mouse);   // 鼠标直接移动
    //glutMouseWheelFunc(mouseWheel); // 滚轮缩放

    // 绑定键盘函数
    glutKeyboardFunc(keyboardDown);
    glutSpecialFunc(keyboardDownSpecial);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialUpFunc(keyboardUpSpecial);

    glutDisplayFunc(display);           // 设置显示回调函数 -- 每帧执行
    glutMainLoop();                     // 进入主循环

    return 0;
}
