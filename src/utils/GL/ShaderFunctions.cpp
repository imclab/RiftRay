// ShaderFunctions.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h> // malloc, free
#endif

#include <stdio.h>
#include <string.h>

#include <string>
#include <iostream>
#include <fstream>

#include <GL/glew.h>

#if 1
#include "g_shaders.h"
#else
#include <map>
std::map<std::string, std::string> g_shaderMap;
void initShaderList() {}
#endif

// Convenience wrapper for setting uniform variables
GLint getUniLoc(const GLuint program, const GLchar *name)
{
    GLint loc;
    loc = glGetUniformLocation(program, name);
    //if (loc == -1)
    //    printf ("No such uniform named \"%s\"\n", name);
    return loc;
}

// Got this from http://www.lighthouse3d.com/opengl/glsl/index.php?oglinfo
// it prints out shader info (debugging!)
void printShaderInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
    if (infologLength > 1)
    {
        infoLog = new char[infologLength];
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n",infoLog);
        delete [] infoLog;
    }
    else printf("Shader Info Log: OK\n");
}

// Got this from http://www.lighthouse3d.com/opengl/glsl/index.php?oglinfo
// it prints out shader info (debugging!)
void printProgramInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
    if (infologLength > 1)
    {
        infoLog = new char[infologLength];
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n",infoLog);
        delete [] infoLog;
    }
    else printf("Program Info Log: OK\n");
}

/// Load a string of shader source from the given filename in the data/ directory
/// and return a copy of it.
/// @note Memory is allocated using new, delete [] it when done.
/// Thank you http://demochronicles.mccolm.org/view.php?mode=challengeArchive
const GLchar* GetShaderSourceFromFile(const char* filename)
{
    std::ifstream file;
    file.open((new std::string("../shaders/"))->append(filename).c_str(), std::ios_base::binary | std::ios::in);
    if (!file.is_open())
    {
        std::cerr << "loadShaderFile: Could not open file " << filename << std::endl;
        return NULL;
    }

    file.seekg(0, std::ios::end);
    const GLint length = (GLint)file.tellg();
    file.seekg(std::ios::beg);
    if (length == 0)
    {
        file.close();
        return NULL;
    }

    GLchar* shaderText = new char[length+1];

    for (int i=0; i <= length; i++)
    {
        if (file.good())
        {
            shaderText[i] = file.get();
        }
        else
        {
            shaderText[i] = 0;
            break;
        }
    }
    shaderText[length] = 0;
    file.close();

    //printf("-----SRC-----:\n%s<<<<<<<<\n\n", shaderText);

    const GLchar* shaderSource = shaderText;
    return shaderSource;
}

/// Retrieve shader source from a std::map of hard-coded shaders generated by
/// shader_hardcoder.py to the file shaderlist.h.
/// @note Memory is allocated using new, delete [] it when done.
const GLchar* GetShaderSourceFromTable(const char* filename)
{
    if (g_shaderMap.empty())
    {
        initShaderList();
    }

    if (g_shaderMap.count(filename) > 0)
    {
        const std::string& src = g_shaderMap[filename];
        GLchar* GLsrc = new char[src.length()+1]; // include null terminator
        memcpy(GLsrc, src.c_str(), src.length()+1);
        return GLsrc;
    }
    return NULL;
}

/// Return shader source from filename, if it can be retrieved.
/// If not, fall back to the hard-coded array in our global std::map.
const GLchar* GetShaderSource(const char* filename)
{
#ifdef _LINUX
    /// Linux exhibits odd behavior when loading shader from file - trailing
    /// garbage characters in the shader source string.
    ///@todo Why does loading shaders in Linux yield extra garbage characters?
    const GLchar* fileSrc = NULL;
#else
    const GLchar* fileSrc = GetShaderSourceFromFile(filename);
#endif
    if (fileSrc != NULL)
        return fileSrc;
    return GetShaderSourceFromTable(filename);
}

/// Once source is obtained from either file or hard-coded map, compile the
/// shader, release the string memory and return the ID.
GLuint loadShaderFile(const char* filename, const unsigned long Type)
{
    const GLchar* shaderSource = GetShaderSource(filename);

    if (shaderSource == NULL)
        return 0;
    GLint length = strlen(shaderSource);

    std::cout
        << "   "
        << filename
        << ": "
        << length
        << " bytes."
        << std::endl;

    GLuint shaderId = glCreateShader(Type);
    glShaderSource(shaderId, 1, &shaderSource, &length);
    glCompileShader(shaderId);

    delete [] shaderSource;

    return shaderId;
}

/// Append any applicable suffixes to the name given and attempt to find
/// vertex, fragment and (optionally) geometry shader source.
GLuint makeShaderByName(const char* name)
{
    if (!name)
        return 0;

    std::string vs(name);
    std::string fs(name);
    std::string gs(name);
    vs += ".vert";
    fs += ".frag";
    gs += ".geom";

    std::cout
        << std::endl
        << "makeShaderByName("
        << vs
        << ", "
        << fs
        << ", "
        << gs
        << "):__"
        << std::endl;

    GLuint vertSrc = loadShaderFile(vs.c_str(), GL_VERTEX_SHADER);
    GLuint fragSrc = loadShaderFile(fs.c_str(), GL_FRAGMENT_SHADER);
    GLuint geomSrc = loadShaderFile(gs.c_str(), GL_GEOMETRY_SHADER_EXT);

    std::cout << "   ";
    printShaderInfoLog(vertSrc);
    std::cout << "   ";
    printShaderInfoLog(fragSrc);

    GLuint program = glCreateProgram();

    glCompileShader(vertSrc);
    glCompileShader(fragSrc);

    glAttachShader(program, vertSrc);
    glAttachShader(program, fragSrc);

    // Will be deleted when program is.
    glDeleteShader(vertSrc);
    glDeleteShader(fragSrc);

    // Initialize Geometry shader state after creation, before linking.
    if (geomSrc)
    {
        printShaderInfoLog(geomSrc);
        glCompileShader(geomSrc);
        glAttachShader (program, geomSrc);
    }

    glLinkProgram(program);
    std::cout << "   ";
    printProgramInfoLog(program);

    glUseProgram(0);
    return program;
}
