#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#define ASSERT(x) if(!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

static void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << "): " << 
            function << " " << file << ":" << line << std::endl;
        return false;
    }

    return true;
}

struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

static ShaderProgramSource ParseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << "\n";
        }
    }

    return { ss[0].str(), ss[1].str() };
}

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    // (basically creating a shader object) Giving an id to the Shader we are going to create
    GLCall(unsigned int id = glCreateShader(type));
    const char* src = source.c_str();

    // Putting/Replacing the shader source in shader object
    GLCall(glShaderSource(id, 1, &src, nullptr));

    // Compiling the shader
    GLCall(glCompileShader(id));

    // Error handling while compiling Shaders
    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if (result == GL_FALSE)
    {
        int length;
        GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        char* message = (char*)alloca(length * sizeof(char));
        GLCall(glGetShaderInfoLog(id, length, &length, message));
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cout << "Message: " << message << std::endl;
        GLCall(glDeleteShader(id));
        return 0;
    }

    return id;
}

static int CreateShaders(const std::string& vertexShader, const std::string& fragmentShader)
{
    GLCall(unsigned int program = glCreateProgram());
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    
    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));
    GLCall(glLinkProgram(program));

    // Error handling while linking multiple shaders
    int result;
    GLCall(glGetProgramiv(program, GL_LINK_STATUS, &result));
    if (result == GL_FALSE)
    {
        int length;
        GLCall(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
        char* message = (char*)alloca(length * sizeof(char));
        GLCall(glGetProgramInfoLog(program, length, &length, message));
        std::cout << "Failed to link program!"<< std::endl;
        std::cout << "Message: " << message << std::endl;
        GLCall(glDeleteProgram(program));
        return 0;
    }

    GLCall(glValidateProgram(program));

    GLCall(glDeleteShader(vs));
    GLCall(glDeleteShader(fs));

    return program;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;
    
    // The Compatibility Profile creates a Vertex Array Object for us by default and is adressed with a '0'
    // where as, the Core Profile does not and we need to explicitly create a VAO by ourselves or else we
    // won't see the triangle
    //Sets OpenGL version to 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Legacy OpenGL won't work in CORE_PROFILE but will work in COMPAT_PROFILE
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // Sync the refresh rate of the monitor with the rendering loop
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
        std::cout << "Error!" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    // Vertex Data for the Triangle
    float positions[8] = {
        -0.5f, -0.5f,   // 0
         0.5f, -0.5f,   // 1
         0.5f,  0.5f,   // 2
        -0.5f,  0.5f    // 3
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Creating a Vertex Array Object(VAO), because using CORE_PROFILE now
    unsigned int vao;
    GLCall(glCreateVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    // 1. Create a buffer, 
    // 2. Give it a number, 
    // 3. Bind and tell what kind of buffer it is, 
    // 4. finally tell what kind of data is in it
    unsigned int buffer;
    GLCall(glGenBuffers(1, &buffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), positions, GL_STATIC_DRAW));

    // Enables the Vertex Array
    GLCall(glEnableVertexAttribArray(0));
    // Defines the attributes of the Array to tell OpenGL what to do with the points
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));

    unsigned int ibo;   // index buffer object
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW));

    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
    unsigned int shader = CreateShaders(source.VertexSource, source.FragmentSource);
    GLCall(glUseProgram(shader));

    GLCall(int location = glGetUniformLocation(shader, "u_Color"));
    ASSERT(location != -1);
    GLCall(glUniform4f(location, 0.2f, 0.5f, 0.8f, 1.0f));

    // Unbinding everything
    GLCall(glBindVertexArray(0));
    GLCall(glUseProgram(0));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    float r = 0.0f;
    float increment = 0.01f;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        // All the binding need to be done before the traingle is rendered //
        GLCall(glUseProgram(shader));
        GLCall(glUniform4f(location, r, 0.5f, 0.8f, 1.0f));

        // No need to bind the vertex buffer(1) or do the set VertexAttribePointer(2 & 3) because we are using VAOs
        // But we do need to bind the index buffer
        GLCall(glBindVertexArray(vao));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo)); // binding index buffer
        /*
        1. GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
        2. GLCall(glEnableVertexAttribArray(0));
        3. GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));*/
        // --------------------------------------------------------------- //

        // Using Indices to and Index Buffer Obj to draw 2 triangles which looks like a square
        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

        if (r > 1.0f)
            increment = -0.01f;
        else if (r < 0.0f)
            increment = 0.01f;

        r += increment;

        /* Swap front and back buffers (and) Poll for and process events */
        GLCall(glfwSwapBuffers(window));
        GLCall(glfwPollEvents());
    }

    GLCall(glDeleteProgram(shader));

    glfwTerminate();
    return 0;
}
