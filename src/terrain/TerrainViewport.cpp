#include "terrain/TerrainViewport.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_SWIZZLE_RGBA
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif
#ifndef GL_R16
#define GL_R16 0x822A
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif

namespace
{
constexpr float LandVertexSpacing = 128.0f;
constexpr float LandCellSize = 4096.0f;
// Skyrim terrain height range used by the native heightmap representation.
// VHGT heights are quantised in 8-GU terrain-height units, while this 16-bit
// display maps the supported absolute elevation range onto the full image.
constexpr float SkyrimHeightMin = -8192.0f;
constexpr float SkyrimHeightMax = 122878.0f;
constexpr float HeightMap16Scale = 65535.0f / (SkyrimHeightMax - SkyrimHeightMin);

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 0x8074
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif

using PFNGLGENVERTEXARRAYSPROC = void (APIENTRY*)(GLsizei, GLuint*);
using PFNGLDELETEVERTEXARRAYSPROC = void (APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBINDVERTEXARRAYPROC = void (APIENTRY*)(GLuint);
using PFNGLGENBUFFERSPROC = void (APIENTRY*)(GLsizei, GLuint*);
using PFNGLDELETEBUFFERSPROC = void (APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBINDBUFFERPROC = void (APIENTRY*)(GLenum, GLuint);
using GLsizeiptr_compat = std::ptrdiff_t;
using GLchar_compat = char;
using PFNGLBUFFERDATAPROC = void (APIENTRY*)(GLenum, GLsizeiptr_compat, const void*, GLenum);
using PFNGLCREATESHADERPROC = GLuint (APIENTRY*)(GLenum);
using PFNGLSHADERSOURCEPROC = void (APIENTRY*)(GLuint, GLsizei, const GLchar_compat* const*, const GLint*);
using PFNGLCOMPILESHADERPROC = void (APIENTRY*)(GLuint);
using PFNGLGETSHADERIVPROC = void (APIENTRY*)(GLuint, GLenum, GLint*);
using PFNGLGETSHADERINFOLOGPROC = void (APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar_compat*);
using PFNGLDELETESHADERPROC = void (APIENTRY*)(GLuint);
using PFNGLCREATEPROGRAMPROC = GLuint (APIENTRY*)();
using PFNGLATTACHSHADERPROC = void (APIENTRY*)(GLuint, GLuint);
using PFNGLLINKPROGRAMPROC = void (APIENTRY*)(GLuint);
using PFNGLGETPROGRAMIVPROC = void (APIENTRY*)(GLuint, GLenum, GLint*);
using PFNGLGETPROGRAMINFOLOGPROC = void (APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar_compat*);
using PFNGLDELETEPROGRAMPROC = void (APIENTRY*)(GLuint);
using PFNGLUSEPROGRAMPROC = void (APIENTRY*)(GLuint);
using PFNGLGETUNIFORMLOCATIONPROC = GLint (APIENTRY*)(GLuint, const char*);
using PFNGLUNIFORMMATRIX4FVPROC = void (APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat*);
using PFNGLUNIFORM3FPROC = void (APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void (APIENTRY*)(GLuint);
using PFNGLVERTEXATTRIBPOINTERPROC = void (APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using PFNGLDISABLEVERTEXATTRIBARRAYPROC = void (APIENTRY*)(GLuint);
using PFNGLGENFRAMEBUFFERSPROC = void (APIENTRY*)(GLsizei, GLuint*);
using PFNGLDELETEFRAMEBUFFERSPROC = void (APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBINDFRAMEBUFFERPROC = void (APIENTRY*)(GLenum, GLuint);
using PFNGLFRAMEBUFFERTEXTURE2DPROC = void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using PFNGLCHECKFRAMEBUFFERSTATUSPROC = GLenum (APIENTRY*)(GLenum);
using PFNGLGENRENDERBUFFERSPROC = void (APIENTRY*)(GLsizei, GLuint*);
using PFNGLDELETERENDERBUFFERSPROC = void (APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBINDRENDERBUFFERPROC = void (APIENTRY*)(GLenum, GLuint);
using PFNGLRENDERBUFFERSTORAGEPROC = void (APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei);
using PFNGLFRAMEBUFFERRENDERBUFFERPROC = void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint);
using PFNGLDRAWBUFFERSPROC = void (APIENTRY*)(GLsizei, const GLenum*);
using PFNGLPOLYGONMODEPROC = void (APIENTRY*)(GLenum, GLenum);
using PFNGLACTIVETEXTUREPROC = void (APIENTRY*)(GLenum);

struct GLApi
{
    PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
    PFNGLGENBUFFERSPROC GenBuffers = nullptr;
    PFNGLDELETEBUFFERSPROC DeleteBuffers = nullptr;
    PFNGLBINDBUFFERPROC BindBuffer = nullptr;
    PFNGLBUFFERDATAPROC BufferData = nullptr;
    PFNGLCREATESHADERPROC CreateShader = nullptr;
    PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC CompileShader = nullptr;
    PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
    PFNGLDELETESHADERPROC DeleteShader = nullptr;
    PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
    PFNGLATTACHSHADERPROC AttachShader = nullptr;
    PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
    PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
    PFNGLUSEPROGRAMPROC UseProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv = nullptr;
    PFNGLUNIFORM3FPROC Uniform3f = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer = nullptr;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray = nullptr;
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = nullptr;
    PFNGLGENRENDERBUFFERSPROC GenRenderbuffers = nullptr;
    PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers = nullptr;
    PFNGLBINDRENDERBUFFERPROC BindRenderbuffer = nullptr;
    PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage = nullptr;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer = nullptr;
    PFNGLPOLYGONMODEPROC PolygonMode = nullptr;
    PFNGLACTIVETEXTUREPROC ActiveTexture = nullptr;
};

GLApi gl;

template <typename T>
bool loadGL(T& fn, const char* name)
{
    fn = reinterpret_cast<T>(glfwGetProcAddress(name));
    return fn != nullptr;
}

bool loadGLApi()
{
    bool ok = true;
#define LOAD_GL(name) ok = loadGL(gl.name, "gl" #name) && ok
    LOAD_GL(GenVertexArrays);
    LOAD_GL(DeleteVertexArrays);
    LOAD_GL(BindVertexArray);
    LOAD_GL(GenBuffers);
    LOAD_GL(DeleteBuffers);
    LOAD_GL(BindBuffer);
    LOAD_GL(BufferData);
    LOAD_GL(CreateShader);
    LOAD_GL(ShaderSource);
    LOAD_GL(CompileShader);
    LOAD_GL(GetShaderiv);
    LOAD_GL(GetShaderInfoLog);
    LOAD_GL(DeleteShader);
    LOAD_GL(CreateProgram);
    LOAD_GL(AttachShader);
    LOAD_GL(LinkProgram);
    LOAD_GL(GetProgramiv);
    LOAD_GL(GetProgramInfoLog);
    LOAD_GL(DeleteProgram);
    LOAD_GL(UseProgram);
    LOAD_GL(GetUniformLocation);
    LOAD_GL(UniformMatrix4fv);
    LOAD_GL(Uniform3f);
    LOAD_GL(EnableVertexAttribArray);
    LOAD_GL(VertexAttribPointer);
    LOAD_GL(DisableVertexAttribArray);
    LOAD_GL(GenFramebuffers);
    LOAD_GL(DeleteFramebuffers);
    LOAD_GL(BindFramebuffer);
    LOAD_GL(FramebufferTexture2D);
    LOAD_GL(CheckFramebufferStatus);
    LOAD_GL(GenRenderbuffers);
    LOAD_GL(DeleteRenderbuffers);
    LOAD_GL(BindRenderbuffer);
    LOAD_GL(RenderbufferStorage);
    LOAD_GL(FramebufferRenderbuffer);
    LOAD_GL(PolygonMode);
    LOAD_GL(ActiveTexture);
#undef LOAD_GL
    return ok;
}

struct Vec3 { float x, y, z; };

Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y*b.z-a.z*b.y,
        a.z*b.x-a.x*b.z,
        a.x*b.y-a.y*b.x
    };
}

float length(const Vec3& v)
{
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

Vec3 normalise(const Vec3& v)
{
    const float l = length(v);
    if (l <= 0.000001f) return {0,1,0};
    return {v.x/l, v.y/l, v.z/l};
}

Vec3 rotate(const Vec3& p, float yaw, float pitch)
{
    const float cy=std::cos(yaw), sy=std::sin(yaw);
    const float cp=std::cos(pitch), sp=std::sin(pitch);
    const float x=p.x*cy-p.z*sy;
    const float z=p.x*sy+p.z*cy;
    return {x, p.y*cp-z*sp, p.y*sp+z*cp};
}

std::uint16_t encodeAbsoluteHeight16(float height)
{
    const float value = std::round((height - SkyrimHeightMin) * HeightMap16Scale);
    if (value <= 0) return 0;
    if (value >= 65535) return 65535;
    return static_cast<std::uint16_t>(value);
}

void handleViewportInput(float& yaw, float& pitch, float& distance, float& panX, float& panY)
{
    ImGuiIO& io=ImGui::GetIO();
    if (!ImGui::IsItemHovered()) return;
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        yaw += io.MouseDelta.x*0.012f;
        pitch += io.MouseDelta.y*0.012f;
        pitch=std::clamp(pitch,-1.570795f,1.570795f);
    }
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        panX += io.MouseDelta.x;
        panY += io.MouseDelta.y;
    }
    if (std::abs(io.MouseWheel)>0)
    {
        distance*=std::pow(0.85f,io.MouseWheel);
        distance = std::max(distance, 0.01f);
    }
}

std::uint32_t compileShader(GLenum type, const char* source)
{
    const GLuint shader=gl.CreateShader(type);
    gl.ShaderSource(shader,1,&source,nullptr);
    gl.CompileShader(shader);
    GLint success=0;
    gl.GetShaderiv(shader,GL_COMPILE_STATUS,&success);
    if (!success)
    {
        gl.DeleteShader(shader);
        return 0;
    }
    return shader;
}

std::uint32_t createTerrainProgram()
{
    const char* vs=R"(#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
uniform mat4 uMvp;
out vec3 vNormal;
void main()
{
    vNormal=aNormal;
    gl_Position=uMvp*vec4(aPosition,1.0);
})";

    const char* fs=R"(#version 330 core
in vec3 vNormal;
uniform vec3 uLight;
out vec4 FragColor;
void main()
{
    float d=max(dot(normalize(vNormal),normalize(uLight)),0.0);
    float v=0.18+d*0.70;
    FragColor=vec4(v,v,v,1.0);
})";

    const GLuint v=compileShader(GL_VERTEX_SHADER,vs);
    const GLuint f=compileShader(GL_FRAGMENT_SHADER,fs);
    if (!v || !f)
    {
        if (v) gl.DeleteShader(v);
        if (f) gl.DeleteShader(f);
        return 0;
    }

    const GLuint p=gl.CreateProgram();
    gl.AttachShader(p,v);
    gl.AttachShader(p,f);
    gl.LinkProgram(p);

    GLint success=0;
    gl.GetProgramiv(p,GL_LINK_STATUS,&success);
    gl.DeleteShader(v);
    gl.DeleteShader(f);
    if (!success)
    {
        gl.DeleteProgram(p);
        return 0;
    }
    return p;
}

void perspective(float fovy,float aspect,float znear,float zfar,float* m)
{
    std::fill(m,m+16,0.0f);
    const float f=1.0f/std::tan(fovy*0.5f);
    m[0]=f/aspect;
    m[5]=f;
    m[10]=(zfar+znear)/(znear-zfar);
    m[11]=-1.0f;
    m[14]=(2.0f*zfar*znear)/(znear-zfar);
}

void multiply4(const float* a,const float* b,float* out)
{
    float r[16]{};
    for(int c=0;c<4;++c)
        for(int row=0;row<4;++row)
            for(int k=0;k<4;++k)
                r[c*4+row]+=a[k*4+row]*b[c*4+k];
    std::copy(r,r+16,out);
}

void viewMatrix(float yaw,float pitch,float distance,float panX,float panY,float* m)
{
    const float cy=std::cos(yaw), sy=std::sin(yaw);
    const float cp=std::cos(pitch), sp=std::sin(pitch);
    const float tx=-panX*0.02f;
    const float ty=panY*0.02f;

    // Inverse of the terrain orbit rotation, followed by camera translation.
    const float rx[16] = {
        1,0,0,0, 0,cp,sp,0, 0,-sp,cp,0, 0,0,0,1
    };
    const float ry[16] = {
        cy,0,-sy,0, 0,1,0,0, sy,0,cy,0, 0,0,0,1
    };
    const float t[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, tx,ty,-distance,1
    };
    float r[16];
    multiply4(rx,ry,r);
    multiply4(t,r,m);
}
}

void TerrainViewport::ensureTerrainRenderer()
{
    if (terrainProgram_ != 0) return;
    if (!loadGLApi()) return;

    terrainProgram_=createTerrainProgram();
    if (!terrainProgram_) return;

    gl.GenVertexArrays(1,&terrainVao_);
    gl.GenBuffers(1,&terrainVbo_);
    gl.GenBuffers(1,&terrainEbo_);
    gl.GenFramebuffers(1,&terrainFramebuffer_);
    gl.GenRenderbuffers(1,&terrainDepthBuffer_);
}

void TerrainViewport::ensureTerrainFramebuffer(int width,int height)
{
    if (width<1 || height<1) return;
    if (terrainFramebufferWidth_==width && terrainFramebufferHeight_==height &&
        terrainColorTexture_!=0) return;

    if (terrainColorTexture_!=0) glDeleteTextures(1,&terrainColorTexture_);

    glGenTextures(1,&terrainColorTexture_);
    glBindTexture(GL_TEXTURE_2D,terrainColorTexture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);

    gl.BindFramebuffer(GL_FRAMEBUFFER,terrainFramebuffer_);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,terrainColorTexture_,0);

    gl.BindRenderbuffer(GL_RENDERBUFFER,terrainDepthBuffer_);
    gl.RenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,terrainDepthBuffer_);

    terrainFramebufferWidth_=width;
    terrainFramebufferHeight_=height;
    gl.BindFramebuffer(GL_FRAMEBUFFER,0);
}

struct GpuVertex { float x,y,z,nx,ny,nz; };

void TerrainViewport::rebuildTerrainMesh(const std::vector<TerrainWorldCell>& cells)
{
    if (terrainProgram_==0) return;

    std::vector<std::uint64_t> signature;
    signature.reserve(cells.size()*3);
    for(const auto& c:cells)
    {
        signature.push_back(
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.gridX))<<32) |
            static_cast<std::uint32_t>(c.gridY));
        signature.push_back(c.formId);
        signature.push_back(static_cast<std::uint64_t>(
            c.heightField ? c.heightField->values().size() : 0));
    }
    if(signature==terrainMeshSignature_ && terrainIndexCount_!=0) return;

    terrainMeshSignature_=signature;
    terrainIndexCount_=0;

    std::vector<GpuVertex> vertices;
    std::vector<std::uint32_t> indices;

    for(const auto& cell:cells)
    {
        if(!cell.heightField || cell.heightField->width()<2 || cell.heightField->height()<2)
            continue;
        const auto& hf=*cell.heightField;
        const float xc=(static_cast<float>(hf.width())-1.0f)*0.5f;
        const float zc=(static_cast<float>(hf.height())-1.0f)*0.5f;
        const float ox=static_cast<float>(cell.gridX)*LandCellSize;
        const float oz=static_cast<float>(cell.gridY)*LandCellSize;
        const std::uint32_t base=static_cast<std::uint32_t>(vertices.size());

        vertices.reserve(vertices.size()+hf.width()*hf.height());
        for(std::size_t y=0;y<hf.height();++y)
            for(std::size_t x=0;x<hf.width();++x)
            {
                const auto sample=[&](std::size_t sx,std::size_t sy)
                {
                    return hf.at(
                        std::min(sx,hf.width()-1),
                        std::min(sy,hf.height()-1));
                };
                const float left=sample(x?x-1:x,y);
                const float right=sample(std::min(x+1,hf.width()-1),y);
                const float back=sample(x,y?y-1:y);
                const float front=sample(x,std::min(y+1,hf.height()-1));
                Vec3 n=normalise({
                    (left-right)/(2.0f*LandVertexSpacing),
                    1.0f,
                    (back-front)/(2.0f*LandVertexSpacing)
                });
                vertices.push_back({
                    ox+(static_cast<float>(x)-xc)*LandVertexSpacing,
                    hf.at(x,y),
                    oz+(static_cast<float>(y)-zc)*LandVertexSpacing,
                    n.x,n.y,n.z
                });
            }

        const std::size_t w=hf.width();
        for(std::size_t y=0;y+1<hf.height();++y)
            for(std::size_t x=0;x+1<w;++x)
            {
                const std::uint32_t i=base+static_cast<std::uint32_t>(y*w+x);
                indices.push_back(i);
                indices.push_back(i+1);
                indices.push_back(i+static_cast<std::uint32_t>(w));
                indices.push_back(i+1);
                indices.push_back(i+static_cast<std::uint32_t>(w)+1);
                indices.push_back(i+static_cast<std::uint32_t>(w));
            }
    }

    terrainIndexCount_=indices.size();

    gl.BindVertexArray(terrainVao_);
    gl.BindBuffer(GL_ARRAY_BUFFER,terrainVbo_);
    gl.BufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr_compat>(vertices.size()*sizeof(GpuVertex)),
        vertices.data(),GL_STATIC_DRAW);
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,terrainEbo_);
    gl.BufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr_compat>(indices.size()*sizeof(std::uint32_t)),
        indices.data(),GL_STATIC_DRAW);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(GpuVertex),(void*)0);
    gl.EnableVertexAttribArray(1);
    gl.VertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(GpuVertex),
        reinterpret_cast<void*>(3*sizeof(float)));
    gl.BindVertexArray(0);
}

void TerrainViewport::renderTerrainGpu(
    const std::vector<TerrainWorldCell>& cells,
    bool wireframe,
    int width,
    int height)
{
    ensureTerrainRenderer();
    if(!terrainProgram_) return;

    ensureTerrainFramebuffer(width,height);
    rebuildTerrainMesh(cells);
    if(terrainIndexCount_==0) return;

    int minX=std::numeric_limits<int>::max(), maxX=std::numeric_limits<int>::min();
    int minY=std::numeric_limits<int>::max(), maxY=std::numeric_limits<int>::min();
    for(const auto& c:cells)
    {
        minX=std::min(minX,c.gridX); maxX=std::max(maxX,c.gridX);
        minY=std::min(minY,c.gridY); maxY=std::max(maxY,c.gridY);
    }

    const float extent=std::max(
        (static_cast<float>(maxX-minX)+1.0f)*LandCellSize,
        (static_cast<float>(maxY-minY)+1.0f)*LandCellSize);

    const float sceneScale=std::max(1.0f,extent/8192.0f);
    const float cameraDistance=distance*sceneScale;

    float p[16],v[16],mvp[16];
    perspective(0.9f,static_cast<float>(width)/static_cast<float>(height),
        32.0f,std::max(200000.0f,cameraDistance*8.0f),p);

    const float cx=(static_cast<float>(minX)+static_cast<float>(maxX))*0.5f*LandCellSize;
    const float cz=(static_cast<float>(minY)+static_cast<float>(maxY))*0.5f*LandCellSize;
    const float panWorldX=-panX*sceneScale*4.0f;
    const float panWorldY=panY*sceneScale*4.0f;

    viewMatrix(yaw,pitch,cameraDistance,
        panWorldX+cx,panWorldY+cz,v);
    multiply4(p,v,mvp);

    gl.BindFramebuffer(GL_FRAMEBUFFER,terrainFramebuffer_);
    glViewport(0,0,width,height);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    ::glClearColor(0.094f,0.102f,0.118f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    gl.UseProgram(terrainProgram_);
    const GLint mvpLoc=gl.GetUniformLocation(terrainProgram_,"uMvp");
    const GLint lightLoc=gl.GetUniformLocation(terrainProgram_,"uLight");
    gl.UniformMatrix4fv(mvpLoc,1,GL_FALSE,mvp);
    gl.Uniform3f(lightLoc,-0.45f,0.8f,0.55f);

    gl.BindVertexArray(terrainVao_);
    gl.PolygonMode(GL_FRONT_AND_BACK,wireframe?GL_LINE:GL_FILL);
    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(terrainIndexCount_),GL_UNSIGNED_INT,nullptr);
    gl.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    gl.BindVertexArray(0);
    gl.UseProgram(0);
    gl.BindFramebuffer(GL_FRAMEBUFFER,0);
}

void TerrainViewport::draw(
    const HeightField& heightField,
    bool wireframe)
{
    std::vector<TerrainWorldCell> cells{{0,0,0,&heightField}};
    drawWorldspace(cells,wireframe);
}

void TerrainViewport::drawWorldspace(
    const std::vector<TerrainWorldCell>& cells,
    bool wireframe)
{
    ImGui::TextUnformatted("Left drag: orbit    Right drag: pan    Mouse wheel: zoom");
    ImGui::SameLine();
    ImGui::Text("| OpenGL terrain | Cells: %zu | Wireframe: %s",
        cells.size(),wireframe?"on":"off");

    const ImVec2 available=ImGui::GetContentRegionAvail();
    const int width=std::max(300,static_cast<int>(available.x));
    const int height=std::max(300,static_cast<int>(std::min(500.0f,available.y)));

    const ImVec2 viewportMin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("TerrainOpenGLViewport",
        ImVec2(static_cast<float>(width),static_cast<float>(height)),
        ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);

    const bool hovered = ImGui::IsItemHovered();
    if (hovered)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            yaw += io.MouseDelta.x * 0.012f;
            pitch += io.MouseDelta.y * 0.012f;
            pitch = std::clamp(pitch, -1.570795f, 1.570795f);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            panX += io.MouseDelta.x;
            panY += io.MouseDelta.y;
        }
        if (std::abs(io.MouseWheel) > 0.0f)
        {
            distance *= std::pow(0.85f, io.MouseWheel);
            distance = std::max(distance, 0.01f);
        }
    }

    renderTerrainGpu(cells,wireframe,width,height);

    if(terrainColorTexture_!=0)
    {
        ImGui::SetCursorScreenPos(viewportMin);
        ImGui::Image(
            static_cast<ImTextureID>(
                static_cast<std::uintptr_t>(terrainColorTexture_)),
            ImVec2(static_cast<float>(width),static_cast<float>(height)),
            ImVec2(0,1),ImVec2(1,0));
    }

    ImDrawList* dl=ImGui::GetWindowDrawList();
    dl->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),
        IM_COL32(100,105,115,255));
}

void TerrainViewport::drawHeightfield16(const HeightField& heightField)
{
    if(heightField.width()==0 || heightField.height()==0)
    {
        ImGui::TextUnformatted("Heightfield is empty.");
        return;
    }

    ImGui::TextUnformatted("16-bit absolute grayscale");
    ImGui::TextUnformatted(
        "Stored pixels use the fixed Skyrim range -8192 to 122878 GU.");

    const float minHeight = heightField.minimum();
    const float maxHeight = heightField.maximum();
    const float displayRange = std::max(1.0f, maxHeight - minHeight);

    const std::size_t width=heightField.width(), height=heightField.height();
    std::vector<std::uint16_t> pixels(width*height);
    for(std::size_t y=0;y<height;++y)
        for(std::size_t x=0;x<width;++x)
            pixels[y*width+x]=encodeAbsoluteHeight16(heightField.at(x,y));

    const float mapSize=std::clamp(
        ImGui::GetContentRegionAvail().x,260.0f,520.0f);

    if(heightmapTexture_==0) glGenTextures(1,&heightmapTexture_);
    glBindTexture(GL_TEXTURE_2D,heightmapTexture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    const GLint swizzle[]={GL_RED,GL_RED,GL_RED,GL_ONE};
    glTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_RGBA,swizzle);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R16,
        static_cast<GLsizei>(width),static_cast<GLsizei>(height),
        0,GL_RED,GL_UNSIGNED_SHORT,pixels.data());
    glBindTexture(GL_TEXTURE_2D,0);

    ImGui::Text(
        "Display stretch: %.3f to %.3f GU (stored values remain absolute)",
        minHeight, maxHeight);

    std::vector<std::uint8_t> preview(width * height * 4);
    for (std::size_t y = 0; y < height; ++y)
        for (std::size_t x = 0; x < width; ++x)
        {
            const float h = heightField.at(x, y);
            const float t = std::clamp((h - minHeight) / displayRange, 0.0f, 1.0f);
            const std::uint8_t v =
                static_cast<std::uint8_t>(std::lround(t * 255.0f));
            const std::size_t i = (y * width + x) * 4;
            preview[i + 0] = v;
            preview[i + 1] = v;
            preview[i + 2] = v;
            preview[i + 3] = 255;
        }

    if (heightmapPreviewTexture_ == 0)
        glGenTextures(1, &heightmapPreviewTexture_);
    glBindTexture(GL_TEXTURE_2D, heightmapPreviewTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
        static_cast<GLsizei>(width), static_cast<GLsizei>(height),
        0, GL_RGBA, GL_UNSIGNED_BYTE, preview.data());

    ImGui::TextUnformatted("Contrast preview (relative display only):");
    ImGui::Image(static_cast<ImTextureID>(
        static_cast<std::uintptr_t>(heightmapPreviewTexture_)),
        ImVec2(mapSize, mapSize));

    const std::size_t cx=width/2, cy=height/2;
    const float centreHeight=heightField.at(cx,cy);
    const std::uint16_t centreValue=encodeAbsoluteHeight16(centreHeight);
    ImGui::Text("Centre: %.3f GU -> 0x%04X (%u)",
        centreHeight,centreValue,
        static_cast<unsigned int>(centreValue));
    ImGui::Text("Stored range: %.3f to %.3f GU",
        heightField.minimum(),heightField.maximum());
}

TerrainViewport::~TerrainViewport()
{
    if(terrainProgram_) gl.DeleteProgram(terrainProgram_);
    if(terrainVao_) gl.DeleteVertexArrays(1,&terrainVao_);
    if(terrainVbo_) gl.DeleteBuffers(1,&terrainVbo_);
    if(terrainEbo_) gl.DeleteBuffers(1,&terrainEbo_);
    if(terrainFramebuffer_) gl.DeleteFramebuffers(1,&terrainFramebuffer_);
    if(terrainDepthBuffer_) gl.DeleteRenderbuffers(1,&terrainDepthBuffer_);
    if(terrainColorTexture_) glDeleteTextures(1,&terrainColorTexture_);
    if(heightmapTexture_) glDeleteTextures(1,&heightmapTexture_);
    if(heightmapPreviewTexture_) glDeleteTextures(1,&heightmapPreviewTexture_);
}
