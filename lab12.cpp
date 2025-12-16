#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

// ==================== Глобальные переменные ====================
HWND g_hWnd;
HDC g_hDC;
HGLRC g_hRC;

int currentScene = 1;
float tetraX = 0.0f, tetraY = 0.0f, tetraZ = -3.0f;
float cubeColorInfluence = 0.5f;
float textureMixRatio = 0.5f;
float circleScaleX = 1.0f, circleScaleY = 1.0f, circleScaleZ = 1.0f;

GLuint texture1, texture2;
GLuint programTet, programCubeTex, programCubeTwoTex, programCircle;
GLuint tetraVAO, tetraVBO, tetraEBO;
GLuint cubeVAO, cubeVBO, cubeEBO;  // Добавил cubeEBO здесь
GLuint circleVAO, circleVBO, circleEBO;

int windowWidth = 800;
int windowHeight = 600;

// ==================== Шейдеры ====================
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"out vec3 ourColor;\n"
"out vec2 TexCoord;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    ourColor = aColor;\n"
"    TexCoord = aTexCoord;\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"in vec3 ourColor;\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D texture1;\n"
"uniform float colorInfluence;\n"
"void main() {\n"
"    vec4 texColor = texture(texture1, TexCoord);\n"
"    FragColor = mix(texColor, vec4(ourColor, 1.0), colorInfluence);\n"
"}\n";

const char* fragmentTwoTextures =
"#version 330 core\n"
"in vec3 ourColor;\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D texture1;\n"
"uniform sampler2D texture2;\n"
"uniform float mixRatio;\n"
"void main() {\n"
"    vec4 tex1 = texture(texture1, TexCoord);\n"
"    vec4 tex2 = texture(texture2, TexCoord);\n"
"    FragColor = mix(tex1, tex2, mixRatio);\n"
"}\n";

const char* vertexShaderSimple =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    ourColor = aColor;\n"
"}\n";

const char* fragmentShaderSimple =
"#version 330 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"    FragColor = vec4(ourColor, 1.0);\n"
"}\n";

// ==================== Вспомогательные функции ====================
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        cout << "Shader compilation error:\n" << infoLog << endl;
    }
    return shader;
}

GLuint createProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        cout << "Program linking error:\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

GLuint loadTexture() {
    const int width = 64, height = 64;
    unsigned char* image = new unsigned char[width * height * 3];

    // Создаем шахматную текстуру
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
            int index = (i * width + j) * 3;
            image[index] = c;
            image[index + 1] = c;
            image[index + 2] = c;
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    delete[] image;
    return texture;
}

GLuint loadColorfulTexture() {
    const int width = 64, height = 64;
    unsigned char* image = new unsigned char[width * height * 3];

    // Создаем цветную текстуру (градиент)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int index = (i * width + j) * 3;
            image[index] = (unsigned char)((float)j / width * 255);     // R
            image[index + 1] = (unsigned char)((float)i / height * 255); // G
            image[index + 2] = (unsigned char)((float)(i + j) / (width + height) * 255); // B
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    delete[] image;
    return texture;
}

// ==================== Инициализация объектов ====================
void initTetrahedron() {
    // Вершины тетраэдра с цветами
    float vertices[] = {
        // Верхняя вершина
        0.0f,  0.5f,  0.0f,   1.0f, 0.0f, 0.0f,  // Красный
        // Основание - треугольник
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  // Зеленый
        0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,  // Синий
        0.0f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f   // Желтый
    };

    unsigned int indices[] = {
        0, 1, 2,  // Передняя грань
        0, 2, 3,  // Правая грань
        0, 3, 1,  // Левая грань
        1, 3, 2   // Основание
    };

    glGenVertexArrays(1, &tetraVAO);
    glGenBuffers(1, &tetraVBO);
    glGenBuffers(1, &tetraEBO);

    glBindVertexArray(tetraVAO);

    glBindBuffer(GL_ARRAY_BUFFER, tetraVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tetraEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Позиции вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Цвета вершин
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void initTexturedCube() {
    float vertices[] = {
        // Передняя грань
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  // Нижний левый
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  // Нижний правый
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  // Верхний правый
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  // Верхний левый

        // Задняя грань
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f,

        // Верхняя грань
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,

        // Нижняя грань
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,

        // Левая грань
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,

        // Правая грань
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };

    unsigned int indices[] = {
        // Передняя грань
        0, 1, 2,
        2, 3, 0,

        // Задняя грань
        4, 5, 6,
        6, 7, 4,

        // Верхняя грань
        8, 9, 10,
        10, 11, 8,

        // Нижняя грань
        12, 13, 14,
        14, 15, 12,

        // Левая грань
        16, 17, 18,
        18, 19, 16,

        // Правая грань
        20, 21, 22,
        22, 23, 20
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Позиции вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Цвета вершин
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Текстурные координаты
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void initCircle() {
    const int segments = 64;
    vector<float> vertices;
    vector<unsigned int> indices;

    // Центр круга - белый
    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    vertices.push_back(1.0f); vertices.push_back(1.0f); vertices.push_back(1.0f);

    // Вершины окружности с градиентом Hue
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = cos(angle);
        float y = sin(angle);

        // Преобразование угла в цвет HSV -> RGB
        float hue = angle / (2.0f * 3.14159f);  // 0 to 1
        float h = hue * 6.0f;
        int sector = static_cast<int>(h);
        float fraction = h - sector;
        float r = 0, g = 0, b = 0;

        switch (sector % 6) {
        case 0: r = 1; g = fraction; b = 0; break;
        case 1: r = 1 - fraction; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = fraction; break;
        case 3: r = 0; g = 1 - fraction; b = 1; break;
        case 4: r = fraction; g = 0; b = 1; break;
        case 5: r = 1; g = 0; b = 1 - fraction; break;
        }

        vertices.push_back(x); vertices.push_back(y); vertices.push_back(0.0f);
        vertices.push_back(r); vertices.push_back(g); vertices.push_back(b);
    }

    // Индексы для треугольников
    for (int i = 1; i <= segments; i++) {
        indices.push_back(0);          // Центр
        indices.push_back(i);          // Текущая вершина
        indices.push_back(i + 1);      // Следующая вершина
    }
    indices[indices.size() - 1] = 1;  // Замыкаем круг

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glGenBuffers(1, &circleEBO);

    glBindVertexArray(circleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Позиции вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Цвета вершин
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ==================== Инициализация OpenGL ====================
void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Создание шейдерных программ
    programTet = createProgram(vertexShaderSimple, fragmentShaderSimple);
    programCubeTex = createProgram(vertexShaderSource, fragmentShaderSource);
    programCubeTwoTex = createProgram(vertexShaderSource, fragmentTwoTextures);
    programCircle = createProgram(vertexShaderSimple, fragmentShaderSimple);

    // Загрузка текстур
    texture1 = loadTexture();           // Шахматная текстура
    texture2 = loadColorfulTexture();   // Цветная текстура

    // Инициализация геометрии
    initTetrahedron();
    initTexturedCube();
    initCircle();

    cout << "OpenGL initialized successfully!" << endl;
}

// ==================== Отрисовка ====================
void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Устанавливаем viewport
    glViewport(0, 0, windowWidth, windowHeight);

    // Создаем матрицы проекции и вида
    float aspect = (float)windowWidth / (float)windowHeight;
    float projection[16] = { 0 };
    projection[0] = 1.0f / aspect;
    projection[5] = 1.0f;
    projection[10] = -1.0f / (10.0f - 0.1f);
    projection[11] = -1.0f;
    projection[14] = -(10.0f * 0.1f) / (10.0f - 0.1f);
    projection[15] = 0.0f;

    float view[16] = { 0 };
    view[0] = 1.0f; view[5] = 1.0f; view[10] = 1.0f; view[15] = 1.0f;
    view[14] = -3.0f;  // Отодвигаем камеру назад

    if (currentScene == 1) {
        // Градиентный тетраэдр
        glUseProgram(programTet);

        // Матрица модели тетраэдра
        float model[16] = { 0 };
        model[0] = 1.0f; model[5] = 1.0f; model[10] = 1.0f; model[15] = 1.0f;

        // Позиция
        model[12] = tetraX;
        model[13] = tetraY;
        model[14] = tetraZ;

        // Поворот тетраэдра для лучшего обзора
        static float rotation = 0.0f;
        rotation += 0.5f;
        float angle = rotation * 3.14159f / 180.0f;
        float cosA = cos(angle);
        float sinA = sin(angle);

        // Поворот вокруг оси Y и небольшой наклон
        model[0] = cosA;
        model[2] = sinA;
        model[8] = -sinA * 0.5f;
        model[10] = cosA * 0.5f;

        // Передаем матрицы в шейдер
        GLint modelLoc = glGetUniformLocation(programTet, "model");
        GLint viewLoc = glGetUniformLocation(programTet, "view");
        GLint projLoc = glGetUniformLocation(programTet, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

        // Отрисовываем тетраэдр
        glBindVertexArray(tetraVAO);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    else if (currentScene == 2) {
        // Кубик с текстурой и цветом
        glUseProgram(programCubeTex);

        float model[16] = { 0 };
        model[0] = 1.0f; model[5] = 1.0f; model[10] = 1.0f; model[15] = 1.0f;

        // Небольшой поворот кубика для демонстрации
        static float rotation = 0.0f;
        rotation += 1.0f;
        float angle = rotation * 3.14159f / 180.0f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        model[0] = cosA;
        model[2] = sinA;
        model[8] = -sinA;
        model[10] = cosA;

        GLint modelLoc = glGetUniformLocation(programCubeTex, "model");
        GLint viewLoc = glGetUniformLocation(programCubeTex, "view");
        GLint projLoc = glGetUniformLocation(programCubeTex, "projection");
        GLint colorInfLoc = glGetUniformLocation(programCubeTex, "colorInfluence");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
        glUniform1f(colorInfLoc, cubeColorInfluence);

        // Активируем текстуру
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        GLint texLoc = glGetUniformLocation(programCubeTex, "texture1");
        glUniform1i(texLoc, 0);

        // Отрисовываем кубик
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    else if (currentScene == 3) {
        // Кубик с двумя смешанными текстурами
        glUseProgram(programCubeTwoTex);

        float model[16] = { 0 };
        model[0] = 1.0f; model[5] = 1.0f; model[10] = 1.0f; model[15] = 1.0f;

        // Вращение кубика
        static float rotation = 0.0f;
        rotation += 1.0f;
        float angle = rotation * 3.14159f / 180.0f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        model[0] = cosA;
        model[2] = sinA;
        model[8] = -sinA;
        model[10] = cosA;

        GLint modelLoc = glGetUniformLocation(programCubeTwoTex, "model");
        GLint viewLoc = glGetUniformLocation(programCubeTwoTex, "view");
        GLint projLoc = glGetUniformLocation(programCubeTwoTex, "projection");
        GLint mixRatioLoc = glGetUniformLocation(programCubeTwoTex, "mixRatio");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
        glUniform1f(mixRatioLoc, textureMixRatio);

        // Активируем первую текстуру
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        GLint tex1Loc = glGetUniformLocation(programCubeTwoTex, "texture1");
        glUniform1i(tex1Loc, 0);

        // Активируем вторую текстуру
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
        GLint tex2Loc = glGetUniformLocation(programCubeTwoTex, "texture2");
        glUniform1i(tex2Loc, 1);

        // Отрисовываем кубик
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    else if (currentScene == 4) {
        // Градиентный круг
        glUseProgram(programCircle);

        float model[16] = { 0 };
        model[0] = circleScaleX;
        model[5] = circleScaleY;
        model[10] = circleScaleZ;
        model[15] = 1.0f;

        GLint modelLoc = glGetUniformLocation(programCircle, "model");
        GLint viewLoc = glGetUniformLocation(programCircle, "view");
        GLint projLoc = glGetUniformLocation(programCircle, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

        // Отрисовываем круг
        glBindVertexArray(circleVAO);
        glDrawElements(GL_TRIANGLES, 64 * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    SwapBuffers(g_hDC);
}

// ==================== Обработка сообщений Windows ====================
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SIZE:
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        glViewport(0, 0, windowWidth, windowHeight);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {
            // Переключение сцен
        case '1': currentScene = 1; break;
        case '2': currentScene = 2; break;
        case '3': currentScene = 3; break;
        case '4': currentScene = 4; break;

            // Движение тетраэдра (сцена 1)
        case 'W': tetraY += 0.1f; break;
        case 'S': tetraY -= 0.1f; break;
        case 'A': tetraX -= 0.1f; break;
        case 'D': tetraX += 0.1f; break;
        case 'Q': tetraZ += 0.1f; break;
        case 'E': tetraZ -= 0.1f; break;

            // Влияние цвета на кубик (сцена 2)
        case VK_OEM_PLUS:
        case VK_ADD:
            cubeColorInfluence = min(cubeColorInfluence + 0.1f, 1.0f);
            cout << "Color influence: " << cubeColorInfluence << endl;
            break;
        case VK_OEM_MINUS:
        case VK_SUBTRACT:
            cubeColorInfluence = max(cubeColorInfluence - 0.1f, 0.0f);
            cout << "Color influence: " << cubeColorInfluence << endl;
            break;

            // Смешивание текстур (сцена 3)
        case 'M':
            textureMixRatio = min(textureMixRatio + 0.1f, 1.0f);
            cout << "Texture mix ratio: " << textureMixRatio << endl;
            break;
        case 'N':
            textureMixRatio = max(textureMixRatio - 0.1f, 0.0f);
            cout << "Texture mix ratio: " << textureMixRatio << endl;
            break;

            // Масштабирование круга (сцена 4)
        case 'X': circleScaleX += 0.1f; break;
        case 'C': circleScaleX = max(circleScaleX - 0.1f, 0.1f); break;
        case 'Y': circleScaleY += 0.1f; break;
        case 'U': circleScaleY = max(circleScaleY - 0.1f, 0.1f); break;
        case 'Z': circleScaleZ += 0.1f; break;
        case 'H': circleScaleZ = max(circleScaleZ - 0.1f, 0.1f); break;

        case VK_ESCAPE:
            PostQuitMessage(0);
            break;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_PAINT:
        render();
        ValidateRect(hWnd, NULL);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ==================== Создание окна ====================
bool createWindow(HINSTANCE hInstance, int nCmdShow) {
    // Регистрация класса окна
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"OpenGLWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wc)) {
        cout << "Failed to register window class" << endl;
        return false;
    }

    // Создание окна
    g_hWnd = CreateWindowEx(
        0,
        L"OpenGLWindow",
        L"OpenGL VBO Demo - 4 Scenes (Press 1-4 to switch, ESC to exit)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL,
        hInstance,
        NULL
    );

    if (!g_hWnd) {
        cout << "Failed to create window" << endl;
        return false;
    }

    // Получение контекста устройства
    g_hDC = GetDC(g_hWnd);

    // Настройка формата пикселей
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        24, // Глубина буфера
        8,  // Буфер трафарета
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(g_hDC, &pfd);
    if (!pixelFormat) {
        cout << "Failed to choose pixel format" << endl;
        return false;
    }

    if (!SetPixelFormat(g_hDC, pixelFormat, &pfd)) {
        cout << "Failed to set pixel format" << endl;
        return false;
    }

    // Создание контекста OpenGL
    g_hRC = wglCreateContext(g_hDC);
    if (!g_hRC) {
        cout << "Failed to create OpenGL context" << endl;
        return false;
    }

    if (!wglMakeCurrent(g_hDC, g_hRC)) {
        cout << "Failed to make OpenGL context current" << endl;
        return false;
    }

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cout << "GLEW init error: " << glewGetErrorString(err) << endl;
        return false;
    }

    // Проверяем поддержку OpenGL 3.3
    if (!GLEW_VERSION_3_3) {
        cout << "OpenGL 3.3 not fully supported!" << endl;
    }

    // Показать окно
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return true;
}

// ==================== Главная функция ====================
int main() {
    // Создаем консоль для отладки
    AllocConsole();
    FILE* conout;
    freopen_s(&conout, "CONOUT$", "w", stdout);
    freopen_s(&conout, "CONOUT$", "w", stderr);

    cout << "=== OpenGL VBO Demo ===" << endl;
    cout << "Scene 1: Colored tetrahedron (WASD/QE to move)" << endl;
    cout << "Scene 2: Textured cube (+/- to adjust color influence)" << endl;
    cout << "Scene 3: Dual-textured cube (M/N to adjust mix ratio)" << endl;
    cout << "Scene 4: Gradient circle (X/C,Y/U,Z/H to scale)" << endl;
    cout << "Press 1-4 to switch scenes, ESC to exit" << endl;

    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;

    // Создание окна
    if (!createWindow(hInstance, nCmdShow)) {
        cout << "Failed to create window" << endl;
        system("pause");
        return -1;
    }

    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    // Инициализация OpenGL
    initOpenGL();

    // Принудительная первая отрисовка
    render();

    // Главный цикл
    MSG msg = {};
    bool running = true;

    while (running) {
        // Обработка сообщений
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Отрисовка
        render();

        // Небольшая задержка для снижения нагрузки на CPU
        Sleep(16);
    }

    // Очистка
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hRC);
    ReleaseDC(g_hWnd, g_hDC);

    FreeConsole();
    return 0;
}