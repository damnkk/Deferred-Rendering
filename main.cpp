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
    // OpenGL ����
    GLuint vao, vbo, ebo;
    GLuint diffuseTexture;  // ����������

    // ��������
    std::vector<glm::vec3> vertexPosition;
    std::vector<glm::vec2> vertexTexcoord;
    std::vector<glm::vec3> vertexNormal;

    // glDrawElements �����Ļ�������
    std::vector<int> index;

    Mesh() {}
    void bindData()
    {
        // ���������������
        glGenVertexArrays(1, &vao); // ����1�������������
        glBindVertexArray(vao);  	// �󶨶����������

        // ��������ʼ�����㻺����� ������NULL �Ȳ�������
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
            vertexPosition.size() * sizeof(glm::vec3) +
            vertexTexcoord.size() * sizeof(glm::vec2) +
            vertexNormal.size() * sizeof(glm::vec3),
            NULL, GL_STATIC_DRAW);

        // ��λ��
        GLuint offset_position = 0;
        GLuint size_position = vertexPosition.size() * sizeof(glm::vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset_position, size_position, vertexPosition.data());
        glEnableVertexAttribArray(0);   // ��ɫ���� (layout = 0) ��ʾ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_position));
        // ����������
        GLuint offset_texcoord = size_position;
        GLuint size_texcoord = vertexTexcoord.size() * sizeof(glm::vec2);
        glBufferSubData(GL_ARRAY_BUFFER, offset_texcoord, size_texcoord, vertexTexcoord.data());
        glEnableVertexAttribArray(1);   // ��ɫ���� (layout = 1) ��ʾ��������
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_texcoord));
        // ������
        GLuint offset_normal = size_position + size_texcoord;
        GLuint size_normal = vertexNormal.size() * sizeof(glm::vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset_normal, size_normal, vertexNormal.data());
        glEnableVertexAttribArray(2);   // ��ɫ���� (layout = 2) ��ʾ����
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_normal));

        // �������� ebo
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(GLuint), index.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
    void draw(GLuint program)
    {
        glBindVertexArray(vao);

        // ������
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        glUniform1i(glGetUniformLocation(program, "texture"), 0);

        // ����
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
        // �쳣����
        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "��ȡģ�ͳ��ִ���: " << import.GetErrorString() << std::endl;
            exit(-1);
        }
        // ģ���ļ����·��
        std::string rootPath = filepath.substr(0, filepath.find_last_of('/'));

        // ѭ������ mesh
        for (int i = 0; i < scene->mNumMeshes; i++)
        {
            // ���õ�ǰmesh
            meshes.push_back(Mesh());
            Mesh& mesh = meshes.back();

            // ��ȡ assimp �Ķ�ȡ���� aimesh ����
            aiMesh* aimesh = scene->mMeshes[i];

            // ���ǽ����ݴ��ݸ������Զ����mesh
            for (int j = 0; j < aimesh->mNumVertices; j++)
            {
                // ����
                glm::vec3 vvv;
                vvv.x = aimesh->mVertices[j].x;
                vvv.y = aimesh->mVertices[j].y;
                vvv.z = aimesh->mVertices[j].z;
                mesh.vertexPosition.push_back(vvv);

                // ����
                vvv.x = aimesh->mNormals[j].x;
                vvv.y = aimesh->mNormals[j].y;
                vvv.z = aimesh->mNormals[j].z;
                mesh.vertexNormal.push_back(vvv);

                // ��������: �����������롣assimp Ĭ�Ͽ����ж���������� ����ȡ��һ����0������
                glm::vec2 vv(0, 0);
                if (aimesh->mTextureCoords[0])
                {
                    vv.x = aimesh->mTextureCoords[0][j].x;
                    vv.y = aimesh->mTextureCoords[0][j].y;
                }
                mesh.vertexTexcoord.push_back(vv);
            }

            // ����в��ʣ���ô���ݲ���
            if (aimesh->mMaterialIndex >= 0)
            {
                // ��ȡ��ǰ aimesh �Ĳ��ʶ���
                aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];

                // ��ȡ diffuse ��ͼ�ļ�·������ ����ֻȡ1����ͼ ���� 0 ����
                aiString aistr;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &aistr);
                std::string texpath = aistr.C_Str();
                texpath = rootPath + '/' + texpath;   // ȡ���·��

                // ���û���ɹ�������ô������
                if (textureMap.find(texpath) == textureMap.end())
                {
                    // ��������
                    GLuint tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                    int textureWidth, textureHeight;
                    unsigned char* image = SOIL_load_image(texpath.c_str(), &textureWidth, &textureHeight, 0, SOIL_LOAD_RGB);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);   // ��������
                    delete[] image;

                    textureMap[texpath] = tex;
                }

                // ��������
                mesh.diffuseTexture = textureMap[texpath];
            }

            // ������Ƭ����
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
        // ��ģ�;���
        glm::mat4 unit(    // ��λ����
            glm::vec4(1, 0, 0, 0),
            glm::vec4(0, 1, 0, 0),
            glm::vec4(0, 0, 1, 0),
            glm::vec4(0, 0, 0, 1)
        );
        glm::mat4 scale = glm::scale(unit, this->scale);
        glm::mat4 translate = glm::translate(unit, this->translate);

        glm::mat4 rotate = unit;    // ��ת
        rotate = glm::rotate(rotate, glm::radians(this->rotate.x), glm::vec3(1, 0, 0));
        rotate = glm::rotate(rotate, glm::radians(this->rotate.y), glm::vec3(0, 1, 0));
        rotate = glm::rotate(rotate, glm::radians(this->rotate.z), glm::vec3(0, 0, 1));

        // ģ�ͱ任����
        glm::mat4 model = translate * rotate * scale;
        GLuint mlocation = glGetUniformLocation(program, "model");    // ��Ϊmodel��uniform������λ������
        glUniformMatrix4fv(mlocation, 1, GL_FALSE, glm::value_ptr(model));   // �����Ⱦ���

        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i].draw(program);
        }
    }
};

class Camera
{
public:
    // �������
    glm::vec3 position = glm::vec3(0, 0, 0);    // λ��
    glm::vec3 direction = glm::vec3(0, 0, -1);  // ���߷���
    glm::vec3 up = glm::vec3(0, 1, 0);          // ���������̶�(0,1,0)����
    float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;    // ŷ����
    float fovy = 70.0f, aspect = 1.0, zNear = 0.01, zFar = 100; // ͸��ͶӰ����
    float left = -1.0, right = 1.0, top = 1.0, bottom = -1.0; // ����ͶӰ����
    Camera() {}
    // ��ͼ�任����
    glm::mat4 getViewMatrix(bool useEulerAngle = true)
    {
        if (useEulerAngle)  // ʹ��ŷ���Ǹ����������
        {
            direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
            direction.y = sin(glm::radians(pitch));
            direction.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw)); // �������z�Ḻ����
        }
        return glm::lookAt(position, position + direction, up);
    }
    // ͶӰ����
    glm::mat4 getProjectionMatrix(bool usePerspective = true)
    {
        if (usePerspective) // ͸��ͶӰ
        {
            return glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
        }
        return glm::ortho(left, right, bottom, top, zNear, zFar);
    }
};

// ---------------------------- end of class definition ---------------------------- //

// ģ��
std::vector<Model> models;  // ����
Model screen;   // ��Ⱦһ���ķ�������Ļ
Model skybox;   // ��Ⱦһ��������������������ͼ������պ�

// ��ɫ���������
GLuint program;
GLuint debugProgram;    // ������
GLuint shadowProgram;   // ������Ӱ����ɫ���������
GLuint skyboxProgram;   // ��պл���

// ����
GLuint skyboxTexture;   // ��պ�
GLuint shadowTexture;   // ��Ӱ����

// ���
Camera camera;          // ������Ⱦ
Camera shadowCamera;    // �ӹ�Դ������Ⱦ

// ��Դ����Ӱ����
int shadowMapResolution = 1024;             // ��Ӱ��ͼ�ֱ���
GLuint shadowMapFBO;                        // �ӹ�Դ���������Ⱦ��֡����

// glut�뽻����ر���
int windowWidth = 512;  // ���ڿ�
int windowHeight = 512; // ���ڸ�
bool keyboardState[1024];   // ����״̬���� keyboardState[x]==true ��ʾ����x��

// �ӳ���Ⱦ�׶�
GLuint gbufferProgram;
GLuint gbufferFBO;  // gbuffer �׶�֡����
GLuint gcolor;      // ������ɫ����
GLuint gdepth;      // �������
GLuint gworldpos;   // ������������
GLuint gnormal;     // ��������

// ����׶�
GLuint composite0;

// --------------- end of global variable definition --------------- //

// ��ȡ�ļ����ҷ���һ�����ַ�����ʾ�ļ�����
std::string readShaderFile(std::string filepath)
{
    std::string res, line;
    std::ifstream fin(filepath);
    if (!fin.is_open())
    {
        std::cout << "�ļ� " << filepath << " ��ʧ��" << std::endl;
        exit(-1);
    }
    while (std::getline(fin, line))
    {
        res += line + '\n';
    }
    fin.close();
    return res;
}

// ��ȡ��ɫ������
GLuint getShaderProgram(std::string fshader, std::string vshader)
{
    // ��ȡshaderԴ�ļ�
    std::string vSource = readShaderFile(vshader);
    std::string fSource = readShaderFile(fshader);
    const char* vpointer = vSource.c_str();
    const char* fpointer = fSource.c_str();

    // �ݴ�
    GLint success;
    GLchar infoLog[512];

    // ���������붥����ɫ��
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)(&vpointer), NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);   // ������
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "������ɫ�� " + vshader + " �������\n" << infoLog << std::endl;
        exit(-1);
    }

    // �������ұ���Ƭ����ɫ��
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar**)(&fpointer), NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);   // ������
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Ƭ����ɫ�� " + fshader + " �������\n" << infoLog << std::endl;
        exit(-1);
    }

    // ����������ɫ����program����
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // ɾ����ɫ������
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// �����ֺ���
void mouseWheel(int wheel, int direction, int x, int y)
{
    // zFar += 1 * direction * 0.1;
    glutPostRedisplay();    // �ػ�
}

// ����˶�����
void mouse(int x, int y)
{
    // ������ת
    camera.yaw += 35 * (x - float(windowWidth) / 2.0) / windowWidth;
    camera.yaw = glm::mod(camera.yaw + 180.0f, 360.0f) - 180.0f;    // ȡģ��Χ -180 ~ 180

    camera.pitch += -35 * (y - float(windowHeight) / 2.0) / windowHeight;
    camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

    glutWarpPointer(windowWidth / 2.0, windowHeight / 2.0);
    glutPostRedisplay();    // �ػ�
}

// ���̻ص�����
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
// ���ݼ���״̬�ж��ƶ�
void move()
{
    float cameraSpeed = 0.0035f;
    // �������
    if (keyboardState['w']) camera.position += cameraSpeed * camera.direction;
    if (keyboardState['s']) camera.position -= cameraSpeed * camera.direction;
    if (keyboardState['a']) camera.position -= cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
    if (keyboardState['d']) camera.position += cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
    if (keyboardState[GLUT_KEY_CTRL_L]) camera.position.y -= cameraSpeed;
    if (keyboardState[' ']) camera.position.y += cameraSpeed;
    // ��Դλ�ÿ���
    if (keyboardState[GLUT_KEY_RIGHT]) shadowCamera.position.x += cameraSpeed;
    if (keyboardState[GLUT_KEY_LEFT]) shadowCamera.position.x -= cameraSpeed;
    if (keyboardState[GLUT_KEY_UP]) shadowCamera.position.y += cameraSpeed;
    if (keyboardState[GLUT_KEY_DOWN]) shadowCamera.position.y -= cameraSpeed;
    glutPostRedisplay();    // �ػ�
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

// ��ʼ��
void init()
{
    // ������ɫ���������
    gbufferProgram = getShaderProgram("shaders/gbuffer.fsh", "shaders/gbuffer.vsh");
    shadowProgram = getShaderProgram("shaders/shadow.fsh", "shaders/shadow.vsh");
    //debugProgram = getShaderProgram("shaders/debug.fsh", "shaders/debug.vsh");
    skyboxProgram = getShaderProgram("shaders/skybox.fsh", "shaders/skybox.vsh");
    composite0 = getShaderProgram("shaders/composite0.fsh", "shaders/composite0.vsh");

    // ------------------------------------------------------------------------ // 

    // ��ȡ obj ģ��
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

    // ��Դλ�ñ�־��
    Model vlight = Model();
    vlight.translate = glm::vec3(1, 0, -1);
    vlight.rotate = glm::vec3(-90, 0, 0);
    vlight.scale = glm::vec3(0.03, 0.03, 0.03);
    vlight.load("models/duck/12248_Bird_v1_L2.obj");
    models.push_back(vlight);

    // ------------------------------------------------------------------------ // 

    // ����һ���ķ�����ӫĻ -- ������ʾ�����е�����
    Mesh msquare;
    msquare.vertexPosition = { glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0) };
    msquare.vertexTexcoord = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1) };
    msquare.index = { 0,1,2,2,1,3 };
    msquare.bindData();
    screen.meshes.push_back(msquare);

    // ------------------------------------------------------------------------ //

    // ����һ������������պе� ��������
    Mesh cube;
    cube.vertexPosition = { // ������� 8 ������
        glm::vec3(-1, -1, -1),glm::vec3(1, -1, -1),glm::vec3(-1, 1, -1),glm::vec3(1, 1, -1),
        glm::vec3(-1, -1, 1),glm::vec3(1, -1, 1),glm::vec3(-1, 1, 1),glm::vec3(1, 1, 1)
    };
    cube.index = { 0,3,1,0,2,3,1,5,4,1,4,0,4,2,0,4,6,2,5,6,4,5,7,6,2,6,7,2,7,3,1,7,5,1,3,7 };
    cube.bindData();
    skybox.meshes.push_back(cube);

    // ������������ͼ
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

    // ����ͶӰ�������� -- �ӽ��巶Χ -- ����������һ���С����
    shadowCamera.left = -30;
    shadowCamera.right = 30;
    shadowCamera.bottom = -30;
    shadowCamera.top = 30;
    shadowCamera.position = glm::vec3(0, 4, 15);

    // ����shadow֡����
    glGenFramebuffers(1, &shadowMapFBO);
    // ������Ӱ����
    glGenTextures(1, &shadowTexture);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // ����Ӱ����󶨵� shadowMapFBO ֡����
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ------------------------------------------------------------------------ // 

    // ���� gubffer ֡����
    glGenFramebuffers(1, &gbufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);

    // ������ɫ����
    glGenTextures(1, &gcolor);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // ����ɫ����󶨵� 0 ����ɫ����
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gcolor, 0);

    // ������������
    glGenTextures(1, &gnormal);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // ����������󶨵� 1 ����ɫ����
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gnormal, 0);

    // ����������������
    glGenTextures(1, &gworldpos);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // ��������������󶨵� 2 ����ɫ����
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gworldpos, 0);

    // �����������
    glGenTextures(1, &gdepth);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // ���������󶨵���ȸ���
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gdepth, 0);

    // ָ����������
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);	// ���

    // ------------------------------------------------------------------------ // 

    glEnable(GL_DEPTH_TEST);  // ������Ȳ���
    glClearColor(1.0, 1.0, 1.0, 1.0);   // ������ɫ
}

// ��ʾ�ص�����
void display()
{
    move(); // �ƶ����� -- �������λ��

    // ���һ��������Ϊ��Դλ�õı�־��
    models.back().translate = shadowCamera.position + glm::vec3(0, 0, 2);

    // ------------------------------------------------------------------------ // 

    // �ӹ�Դ���������Ⱦ
    glUseProgram(shadowProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, shadowMapResolution, shadowMapResolution);

    // ��Դ������������ԭ��
    shadowCamera.direction = glm::normalize(glm::vec3(0, 0, 0) - shadowCamera.position);
    // ����ͼ����
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "view"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getViewMatrix(false)));
    // ��ͶӰ����
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "projection"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getProjectionMatrix(false)));

    // �ӹ�Դ������л���
    for (auto m : models)
    {
        m.draw(shadowProgram);
    }

    // ------------------------------------------------------------------------ // 

    // ������պ� -- ����� gbuffer �׶ε� 3 ��������
    glUseProgram(skyboxProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, windowWidth, windowHeight);

    // ����ͼ,ͶӰ����
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

    // ��cubemap����
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 1);

    // ���� zfar �� znear ��������պе�������������Ӿ�
    glUniform1f(glGetUniformLocation(skyboxProgram, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(skyboxProgram, "far"), camera.zFar);

    // ��������Զ�������
    skybox.translate = camera.position;

    glDepthMask(GL_FALSE);
    skybox.draw(skyboxProgram);
    glDepthMask(GL_TRUE);

    // ------------------------------------------------------------------------ // 

    // �������� -- ����� gbuffer �׶ε� 3 ��������
    glUseProgram(gbufferProgram);
 

    // ����ͼ����
    glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
    // ��ͶӰ����
    glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

    // ��������
    for (auto m : models)
    {
        m.draw(gbufferProgram);
    }

    // ------------------------------------------------------------------------ // 

    // ����׶Σ� composite0 ��ɫ��������Ⱦ
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(composite0);
    glViewport(0, 0, windowWidth, windowHeight);

    // ���� zfar �� znear ����ת�������
    glUniform1f(glGetUniformLocation(composite0, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(composite0, "far"), camera.zFar);

    // �� gcolor ����
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glUniform1i(glGetUniformLocation(composite0, "gcolor"), 1);
    // �� gnormal ����
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glUniform1i(glGetUniformLocation(composite0, "gnormal"), 2);
    // �� gworldpos ����
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glUniform1i(glGetUniformLocation(composite0, "gworldpos"), 3);
    // �� gdepth ����
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glUniform1i(glGetUniformLocation(composite0, "gdepth"), 4);
    // ����Ӱ�������
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glUniform1i(glGetUniformLocation(composite0, "shadowtex"), 5);
    

    // ���ݾ���: ת������Դ����ı任����
    glm::mat4 shadowVP = shadowCamera.getProjectionMatrix(false) * shadowCamera.getViewMatrix(false);
    glUniformMatrix4fv(glGetUniformLocation(composite0, "shadowVP"), 1, GL_FALSE, glm::value_ptr(shadowVP));

    // ���ݹ�Դλ��
    glUniform3fv(glGetUniformLocation(composite0, "lightPos"), 1, glm::value_ptr(shadowCamera.position));
    // �������λ��
    glUniform3fv(glGetUniformLocation(composite0, "cameraPos"), 1, glm::value_ptr(camera.position));

    // ����
    screen.draw(composite0);
    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------------ // 

    /*
    // debug��ɫ�����һ���ķ�������ʾ�����е�����
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);   // ��Ҫȡ����Ȳ����Ա�֤�串����ԭ������
    glUseProgram(debugProgram);
    glViewport(0, 0, windowWidth, windowHeight);

    // ���� zfar �� znear ����ת�������
    glUniform1f(glGetUniformLocation(debugProgram, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(debugProgram, "far"), camera.zFar);

    // �� gcolor ����
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glUniform1i(glGetUniformLocation(debugProgram, "gcolor"), 1);
    // �� gnormal ����
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glUniform1i(glGetUniformLocation(debugProgram, "gnormal"), 2);
    // �� gworldpos ����
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glUniform1i(glGetUniformLocation(debugProgram, "gworldpos"), 3);
    // �� gdepth ����
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glUniform1i(glGetUniformLocation(debugProgram, "gdepth"), 4);
    // ����Ӱ�������
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glUniform1i(glGetUniformLocation(debugProgram, "shadowtex"), 5);

    // ����
    screen.draw(debugProgram);
    glEnable(GL_DEPTH_TEST);
    */

    // ------------------------------------------------------------------------ // 

    glutSwapBuffers();                  // ����������
}

// -------------------------------- main -------------------------------- //

int main(int argc, char** argv)
{
    glutInit(&argc, argv);              // glut��ʼ��
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);// ���ڴ�С
    glutCreateWindow("10 - deferred render"); // ����OpenGL������

#ifdef __APPLE__
#else
    glewInit();
#endif

    init();

    // ������ƶ����� -- 
    //glutMotionFunc(mouse);  // ������²����ƶ�
    glutPassiveMotionFunc(mouse);   // ���ֱ���ƶ�
    //glutMouseWheelFunc(mouseWheel); // ��������

    // �󶨼��̺���
    glutKeyboardFunc(keyboardDown);
    glutSpecialFunc(keyboardDownSpecial);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialUpFunc(keyboardUpSpecial);

    glutDisplayFunc(display);           // ������ʾ�ص����� -- ÿִ֡��
    glutMainLoop();                     // ������ѭ��

    return 0;
}
