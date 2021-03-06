/** Copyright (C) 2013-2014 Robert B. Colton, Harijs Grinbergs
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "../General/OpenGLHeaders.h"
#include "../General/GStextures.h"
#include "GL3shader.h"
#include "GLSLshader.h"
#include <math.h>

#include <stdio.h>      /* printf, scanf, NULL */
#include <stdlib.h>     /* malloc, free, rand */

#include <iostream>
#include <fstream>
#include <sstream>
//using namespace std;

#include <vector>

#ifdef DEBUG_MODE
  #include <string>
  #include "libEGMstd.h"
  #include "Widget_Systems/widgets_mandatory.h"
  #define get_uniform(uniter,location,usize)\
    if (location < 0) { printf("Program[%i] - Uniform location < 0 given!\n", enigma::bound_shader); return; }\
    std::map<GLint,enigma::Uniform>::iterator uniter = enigma::shaderprograms[enigma::bound_shader]->uniforms.find(location);\
    if (uniter == enigma::shaderprograms[enigma::bound_shader]->uniforms.end()){\
        printf("Program[%i] - Uniform at location %i not found!\n", enigma::bound_shader, location);\
        return;\
    }else if ( uniter->second.size != usize ){\
        printf("Program[%i] - Uniform at location %i with %i arguments is accesed by a function with %i arguments!\n", enigma::bound_shader, location, uniter->second.size, usize);\
    }
#else
    #define get_uniform(uniter,location,usize)\
    if (location < 0) return; \
    std::map<GLint,enigma::Uniform>::iterator uniter = enigma::shaderprograms[enigma::bound_shader]->uniforms.find(location);\
    if (uniter == enigma::shaderprograms[enigma::bound_shader]->uniforms.end()){\
        return;\
    }
#endif

extern GLenum shadertypes[5] = {
  GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER
};

namespace enigma
{
    std::vector<enigma::Shader*> shaders(0);
    std::vector<enigma::ShaderProgram*> shaderprograms(0);

    extern unsigned default_shader;
    extern unsigned main_shader;
    extern unsigned bound_shader;
    string getVertexShaderPrefix(){
        return "#version 140\n"
                "#define MATRIX_VIEW                                    0\n"
                "#define MATRIX_PROJECTION                              1\n"
                "#define MATRIX_WORLD                                   2\n"
                "#define MATRIX_WORLD_VIEW                              3\n"
                "#define MATRIX_WORLD_VIEW_PROJECTION                   4\n"
                "#define MATRICES_MAX                                   5\n"

                "uniform mat4 transform_matrix[MATRICES_MAX];           \n"
                "#define gm_Matrices transform_matrix \n"
                "#define modelMatrix transform_matrix[MATRIX_WORLD] \n"
                "#define modelViewMatrix transform_matrix[MATRIX_WORLD_VIEW] \n"
                "#define projectionMatrix transform_matrix[MATRIX_PROJECTION] \n"
                "#define viewMatrix transform_matrix[MATRIX_VIEW] \n"
                "#define modelViewProjectionMatrix transform_matrix[MATRIX_WORLD_VIEW_PROJECTION] \n"
                "#define in_Colour in_Color \n"

                "uniform mat3 normalMatrix;     \n"
				"#line 0 \n";
    }
    string getFragmentShaderPrefix(){
        return "#version 140\n"
                "#define MATRIX_VIEW                                    0\n"
                "#define MATRIX_PROJECTION                              1\n"
                "#define MATRIX_WORLD                                   2\n"
                "#define MATRIX_WORLD_VIEW                              3\n"
                "#define MATRIX_WORLD_VIEW_PROJECTION                   4\n"
                "#define MATRICES_MAX                                   5\n"

                "uniform mat4 transform_matrix[MATRICES_MAX];           \n"
                "#define gm_Matrices transform_matrix \n"
                "#define modelMatrix transform_matrix[MATRIX_WORLD] \n"
                "#define modelViewMatrix transform_matrix[MATRIX_WORLD_VIEW] \n"
                "#define projectionMatrix transform_matrix[MATRIX_PROJECTION] \n"
                "#define viewMatrix transform_matrix[MATRIX_VIEW] \n"
                "#define modelViewProjectionMatrix transform_matrix[MATRIX_WORLD_VIEW_PROJECTION] \n"

                "uniform mat3 normalMatrix;     \n"

                "uniform sampler2D en_TexSampler;\n"
                "#define gm_BaseTexture en_TexSampler\n"
                "uniform bool en_TexturingEnabled;\n"
                "uniform bool en_ColorEnabled;\n"
                "uniform vec4 en_bound_color;\n"
				"#line 0 \n";
    }
    string getDefaultVertexShader(){
        return  "in vec3 in_Position;                 // (x,y,z)\n"
                "in vec3 in_Normal;                  // (x,y,z)\n"
                "in vec4 in_Color;                    // (r,g,b,a)\n"
                "in vec2 in_TextureCoord;             // (u,v)\n"

                "out vec2 v_TextureCoord;\n"
                "out vec4 v_Color;\n"
                "uniform int en_ActiveLights;\n"
                "uniform bool en_ColorEnabled;\n"

                "uniform bool en_LightingEnabled;\n"
                "uniform bool en_VS_FogEnabled;\n"
                "uniform float en_FogStart;\n"
                "uniform float en_RcpFogRange;\n"

                "#define MAX_LIGHTS   8\n"

                "uniform vec4   en_AmbientColor;                              // rgba=color\n"

                "struct LightInfo {\n"
                  "vec4 Position; // Light position in eye coords\n"
                  "vec4 La; // Ambient light intensity\n"
                  "vec4 Ld; // Diffuse light intensity\n"
                  "vec4 Ls; // Specular light intensity\n"
                  "float cA, lA, qA; // Attenuation for point lights\n"
                "};\n"
                "uniform LightInfo Light[MAX_LIGHTS];\n"

                "struct MaterialInfo {\n"
                    "vec4 Ka; // Ambient reflectivity\n"
                    "vec4 Kd; // Diffuse reflectivity\n"
                    "vec4 Ks; // Specular reflectivity\n"
                    "float Shininess; // Specular shininess factor\n"
                "};\n"
                "uniform MaterialInfo Material;\n"

                "void getEyeSpace( inout vec3 norm, inout vec4 position )\n"
                "{\n"
                    "norm = normalize( normalMatrix * in_Normal );\n"
                    "position = modelViewMatrix * vec4(in_Position, 1.0);\n"
                "}\n"

                "vec4 phongModel( in vec3 norm, in vec4 position )\n"
                "{\n"
                  "vec4 total_light = vec4(0.0);\n"
                  "vec3 v = normalize(-position.xyz);\n"
                  "float attenuation;\n"
                  "for (int index = 0; index < en_ActiveLights; ++index){\n"
                      "vec3 L;\n"
                      "if (Light[index].Position.w == 0.0){ //Directional light\n"
                        "L = normalize(Light[index].Position.xyz);\n"
                        "attenuation = 1.0;\n"
                      "}else{ //Point light\n"
                        "vec3 positionToLightSource = vec3(Light[index].Position.xyz - position.xyz);\n"
                        "float distance = length(positionToLightSource);\n"
                        "L = normalize(positionToLightSource);\n"
                        "attenuation = 1.0 / (Light[index].cA + Light[index].lA * distance + Light[index].qA * distance * distance);\n"
                      "}\n"
                      "vec3 r = reflect( -L, norm );\n"
                      "total_light += Light[index].La * Material.Ka;\n"
                      "float LdotN = max( dot(norm, L), 0.0 );\n"
                      "vec4 diffuse = vec4(attenuation * vec3(Light[index].Ld) * vec3(Material.Kd) * LdotN,1.0);\n"
                      "vec4 spec = vec4(0.0);\n"
                      "if( LdotN > 0.0 )\n"
                          "spec = clamp(vec4(attenuation * vec3(Light[index].Ls) * vec3(Material.Ks) * pow( max( dot(r,v), 0.0 ), Material.Shininess ),1.0),0.0,1.0);\n"
                      "total_light += diffuse + spec;\n"
                  "}\n"
                  "return total_light;\n"
                "}\n"

                "void main()\n"
                "{\n"
                    "vec4 iColor = vec4(1.0);\n"
                    "if (en_ColorEnabled == true){\n"
                        "iColor = in_Color;\n"
					"}\n"
                    "if (en_LightingEnabled == true){\n"
                        "vec3 eyeNorm;\n"
                        "vec4 eyePosition;\n"
                        "getEyeSpace(eyeNorm, eyePosition);\n"
                        "v_Color = clamp(en_AmbientColor + phongModel( eyeNorm, eyePosition ),0.0,1.0) * iColor;\n"
                    "}else{\n"
						"v_Color = iColor;\n"
                    "}\n"
                    "gl_Position = modelViewProjectionMatrix * vec4( in_Position.xyz, 1.0);\n"

                    "v_TextureCoord = in_TextureCoord;\n"
                "}\n";
    }
    string getDefaultFragmentShader(){
        return  "in vec2 v_TextureCoord;\n"
                "in vec4 v_Color;\n"
                "out vec4 out_FragColor;\n"

                "void main()\n"
                "{\n"
                    "vec4 TexColor;\n"
                    "if (en_TexturingEnabled == true){\n"
                        "TexColor = texture2D( en_TexSampler, v_TextureCoord.st ) * v_Color;\n"
                    "}else{\n"
                        "TexColor = en_bound_color * v_Color;\n"
                    "}\n"
                    "out_FragColor = TexColor;\n"
                "}\n";
    }
    void getUniforms(int prog_id){
        int uniform_count, max_length, uniform_count_arr = 0;
        glGetProgramiv(enigma::shaderprograms[prog_id]->shaderprogram, GL_ACTIVE_UNIFORMS, &uniform_count);
        glGetProgramiv(enigma::shaderprograms[prog_id]->shaderprogram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);
        for (int i = 0; i < uniform_count; ++i)
        {
            Uniform uniform;
            char uniformName[max_length];

            glGetActiveUniform(enigma::shaderprograms[prog_id]->shaderprogram, GLuint(i), max_length, NULL, &uniform.arraySize, &uniform.type, uniformName);

            uniform.name = uniformName;
            uniform.size = getGLTypeSize(uniform.type);
            uniform.data.resize(uniform.size);
            uniform.location = glGetUniformLocation(enigma::shaderprograms[prog_id]->shaderprogram, uniformName);
            enigma::shaderprograms[prog_id]->uniform_names[uniform.name] = uniform.location;
            enigma::shaderprograms[prog_id]->uniforms[uniform.location] = uniform;
            //printf("Program - %i - found uniform - %s - with size - %i\n", prog_id, uniform.name.c_str(), uniform.size);

            if (uniform.arraySize>1){ //It's an array
                string basename(uniform.name, 0, uniform.name.length()-3);
                //This should always work, because in case of an array glGetActiveUniform returns uniform_name[0] (so the first index)
                for (int c = 1; c < uniform.arraySize; ++c){
                    Uniform suniform;
                    suniform.arraySize = uniform.arraySize;
                    suniform.size = uniform.size;
                    suniform.data.resize(uniform.size);
                    suniform.type = uniform.type;
                    std::ostringstream oss;
                    oss << basename << "[" << c << "]";
                    suniform.name = oss.str();
                    suniform.location = glGetUniformLocation(enigma::shaderprograms[prog_id]->shaderprogram, suniform.name.c_str());
                    enigma::shaderprograms[prog_id]->uniform_names[suniform.name] = suniform.location;
                    enigma::shaderprograms[prog_id]->uniforms[suniform.location] = suniform;
                    //printf("Program - %i - found uniform - %s - with size - %i\n", prog_id, suniform.name.c_str(), suniform.size);
                }
                uniform_count_arr += uniform.arraySize;
            }
        }
        shaderprograms[prog_id]->uniform_count = uniform_count+uniform_count_arr;
    }
    void getAttributes(int prog_id){
        int attribute_count, max_length, attribute_count_arr = 0;
        glGetProgramiv(enigma::shaderprograms[prog_id]->shaderprogram, GL_ACTIVE_ATTRIBUTES, &attribute_count);
        glGetProgramiv(enigma::shaderprograms[prog_id]->shaderprogram, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);
        for (int i = 0; i < attribute_count; ++i)
        {
            Attribute attribute;
            char attributeName[max_length];

            glGetActiveAttrib(enigma::shaderprograms[prog_id]->shaderprogram, GLuint(i), max_length, NULL, &attribute.arraySize, &attribute.type, attributeName);

            attribute.name = attributeName;
            attribute.size = getGLTypeSize(attribute.type);
            attribute.location = glGetAttribLocation(enigma::shaderprograms[prog_id]->shaderprogram, attributeName);
            enigma::shaderprograms[prog_id]->attribute_names[attribute.name] = attribute.location;
            enigma::shaderprograms[prog_id]->attributes[attribute.location] = attribute;
            //printf("Program - %i - found attribute - %s - with size - %i\n", prog_id, attribute.name.c_str(), attribute.size);

            if (attribute.arraySize>1){ //It's an array
                string basename(attribute.name, 0, attribute.name.length()-3);
                //This should always work, because in case of an array glGetActiveAttrib returns attribute_name[0] (so the first index)
                for (int c = 1; c < attribute.arraySize; ++c){
                    Attribute sattribute;
                    sattribute.arraySize = attribute.arraySize;
                    sattribute.size = attribute.size;
                    sattribute.type = attribute.type;
                    std::ostringstream oss;
                    oss << basename << "[" << c << "]";
                    sattribute.name = oss.str();
                    sattribute.location = glGetAttribLocation(enigma::shaderprograms[prog_id]->shaderprogram, sattribute.name.c_str());
                    enigma::shaderprograms[prog_id]->attribute_names[sattribute.name] = sattribute.location;
                    enigma::shaderprograms[prog_id]->attributes[sattribute.location] = sattribute;
                    //printf("Program - %i - found attribute - %s - with size - %i\n", prog_id, sattribute.name.c_str(), sattribute.size);
                }
                attribute_count_arr += attribute.arraySize;
            }
        }
        shaderprograms[prog_id]->attribute_count = attribute_count+attribute_count_arr;
    }
    void getDefaultUniforms(int prog_id){
        shaderprograms[prog_id]->uni_viewMatrix = enigma_user::glsl_get_uniform_location(prog_id, "transform_matrix[0]");
        shaderprograms[prog_id]->uni_projectionMatrix = enigma_user::glsl_get_uniform_location(prog_id, "transform_matrix[1]");
        shaderprograms[prog_id]->uni_modelMatrix = enigma_user::glsl_get_uniform_location(prog_id, "transform_matrix[2]");
        shaderprograms[prog_id]->uni_mvMatrix = enigma_user::glsl_get_uniform_location(prog_id, "transform_matrix[3]");
        shaderprograms[prog_id]->uni_mvpMatrix = enigma_user::glsl_get_uniform_location(prog_id, "transform_matrix[4]");
        shaderprograms[prog_id]->uni_normalMatrix = enigma_user::glsl_get_uniform_location(prog_id, "normalMatrix");
        shaderprograms[prog_id]->uni_texSampler = enigma_user::glsl_get_uniform_location(prog_id, "en_TexSampler");

        shaderprograms[prog_id]->uni_textureEnable = enigma_user::glsl_get_uniform_location(prog_id, "en_TexturingEnabled");
        shaderprograms[prog_id]->uni_colorEnable = enigma_user::glsl_get_uniform_location(prog_id, "en_ColorEnabled");
        shaderprograms[prog_id]->uni_lightEnable = enigma_user::glsl_get_uniform_location(prog_id, "en_LightingEnabled");

        shaderprograms[prog_id]->uni_color = enigma_user::glsl_get_uniform_location(prog_id, "en_bound_color");
        shaderprograms[prog_id]->uni_ambient_color = enigma_user::glsl_get_uniform_location(prog_id, "en_AmbientColor");
        shaderprograms[prog_id]->uni_lights_active = enigma_user::glsl_get_uniform_location(prog_id, "en_ActiveLights");

        char tchars[64];
        for (unsigned int i=0; i<8; ++i){
            sprintf(tchars, "Light[%d].Position", i);
            shaderprograms[prog_id]->uni_light_position[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
            sprintf(tchars, "Light[%d].La", i);
            shaderprograms[prog_id]->uni_light_ambient[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
            sprintf(tchars, "Light[%d].Ld", i);
            shaderprograms[prog_id]->uni_light_diffuse[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
            sprintf(tchars, "Light[%d].Ls", i);
            shaderprograms[prog_id]->uni_light_specular[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
            sprintf(tchars, "Light[%d].cA", i);
            shaderprograms[prog_id]->uni_light_cAttenuation[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
            sprintf(tchars, "Light[%d].lA", i);
            shaderprograms[prog_id]->uni_light_lAttenuation[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
            sprintf(tchars, "Light[%d].qA", i);
            shaderprograms[prog_id]->uni_light_qAttenuation[i] = enigma_user::glsl_get_uniform_location(prog_id, tchars);
        }
        shaderprograms[prog_id]->uni_material_ambient = enigma_user::glsl_get_uniform_location(prog_id, "Material.Ka");
        shaderprograms[prog_id]->uni_material_diffuse = enigma_user::glsl_get_uniform_location(prog_id, "Material.Kd");
        shaderprograms[prog_id]->uni_material_specular = enigma_user::glsl_get_uniform_location(prog_id, "Material.Ks");
        shaderprograms[prog_id]->uni_material_shininess = enigma_user::glsl_get_uniform_location(prog_id, "Material.Shininess");
    }
    void getDefaultAttributes(int prog_id){
        shaderprograms[prog_id]->att_vertex = enigma_user::glsl_get_attribute_location(prog_id, "in_Position");
        shaderprograms[prog_id]->att_color = enigma_user::glsl_get_attribute_location(prog_id, "in_Color");
        shaderprograms[prog_id]->att_texture = enigma_user::glsl_get_attribute_location(prog_id, "in_TextureCoord");
        shaderprograms[prog_id]->att_normal = enigma_user::glsl_get_attribute_location(prog_id, "in_Normal");
    }

    int getGLTypeSize(GLuint type){
        switch (type){
            case GL_FLOAT: return 1;
            case GL_FLOAT_VEC2: return 2;
            case GL_FLOAT_VEC3: return 3;
            case GL_FLOAT_VEC4: return 4;
            case GL_INT: return 1;
            case GL_INT_VEC2: return 2;
            case GL_INT_VEC3: return 3;
            case GL_INT_VEC4: return 4;
            case GL_UNSIGNED_INT: return 1;
            case GL_UNSIGNED_INT_VEC2: return 2;
            case GL_UNSIGNED_INT_VEC3: return 3;
            case GL_UNSIGNED_INT_VEC4: return 4;
            case GL_BOOL: return 1;
            case GL_BOOL_VEC2: return 2;
            case GL_BOOL_VEC3: return 3;
            case GL_BOOL_VEC4: return 4;
            case GL_FLOAT_MAT2: return 4;
            case GL_FLOAT_MAT3: return 9;
            case GL_FLOAT_MAT4: return 16;
            case GL_FLOAT_MAT2x3: return 6;
            case GL_FLOAT_MAT2x4: return 8;
            case GL_FLOAT_MAT3x2: return 6;
            case GL_FLOAT_MAT3x4: return 12;
            case GL_FLOAT_MAT4x2: return 8;
            case GL_FLOAT_MAT4x3: return 12;

            case GL_SAMPLER_1D: return 1;
            case GL_SAMPLER_2D: return 1;
            case GL_SAMPLER_3D: return 1;

            default: { printf("getGLTypeSize Asking size for unknown type - %i!\n", type); return 1; }
        }
    }

    //This seems very stupid for me, but I don't know any more "elegant" way - Harijs G.
    bool UATypeUIComp(UAType i, unsigned int j){
        return (i.ui == j);
    }
    bool UATypeIComp(UAType i, int j){
        return (i.i == j);
    }
    bool UATypeFComp(UAType i, float j){
        return (i.f == j);
    }
}

unsigned long getFileLength(std::ifstream& file)
{
    if(!file.good()) return 0;

    //unsigned long pos=file.tellg();
    file.seekg(0,std::ios::end);
    unsigned long len = file.tellg();
    file.seekg(std::ios::beg);

    return len;
}

namespace enigma_user
{

void glsl_shader_print_infolog(int id)
{
    GLint blen = 0;
    GLsizei slen = 0;
    glGetShaderiv(enigma::shaders[id]->shader, GL_INFO_LOG_LENGTH , &blen);
    if (blen > 1)
    {
        GLchar* compiler_log = (GLchar*)malloc(blen);
        glGetShaderInfoLog(enigma::shaders[id]->shader, blen, &slen, compiler_log);
        enigma::shaders[id]->log = (string)compiler_log;
        std::cout << compiler_log << std::endl;
    } else {
        enigma::shaders[id]->log = "Shader log empty";
        std::cout << enigma::shaders[id]->log << std::endl;
    }
}

void glsl_program_print_infolog(int id)
{
    GLint blen = 0;
    GLsizei slen = 0;
    glGetProgramiv(enigma::shaderprograms[id]->shaderprogram, GL_INFO_LOG_LENGTH , &blen);
    if (blen > 1)
    {
        GLchar* compiler_log = (GLchar*)malloc(blen);
        glGetProgramInfoLog(enigma::shaderprograms[id]->shaderprogram, blen, &slen, compiler_log);
        enigma::shaderprograms[id]->log = (string)compiler_log;
        std::cout << compiler_log << std::endl;
    } else {
        enigma::shaderprograms[id]->log = "Shader program log empty";
        std::cout << enigma::shaderprograms[id]->log << std::endl;
    }
}

int glsl_shader_create(int type)
{
    unsigned int id = enigma::shaders.size();
    enigma::shaders.push_back(new enigma::Shader(type));
    return id;
}

bool glsl_shader_load(int id, string fname)
{
    std::ifstream ifs(fname.c_str());
    std::string shaderSource((std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ));
    string source;
    if (enigma::shaders[id]->type == sh_vertex) source = enigma::getVertexShaderPrefix() + shaderSource;
    if (enigma::shaders[id]->type == sh_fragment) source = enigma::getFragmentShaderPrefix() + shaderSource;

    const char *ShaderSource = source.c_str();
    //std::cout << ShaderSource << std::endl;
    glShaderSource(enigma::shaders[id]->shader, 1, &ShaderSource, NULL);
    return true; // No Error
}

bool glsl_shader_load_string(int id, string shaderSource)
{
    string source;
    if (enigma::shaders[id]->type == sh_vertex) source = enigma::getVertexShaderPrefix() + shaderSource;
    if (enigma::shaders[id]->type == sh_fragment) source = enigma::getFragmentShaderPrefix() + shaderSource;
    const char *ShaderSource = source.c_str();
    //std::cout << ShaderSource << std::endl;
    glShaderSource(enigma::shaders[id]->shader, 1, &ShaderSource, NULL);
    return true; // No Error
}

bool glsl_shader_compile(int id)
{
    glCompileShader(enigma::shaders[id]->shader);

    GLint compiled;
    glGetProgramiv(enigma::shaders[id]->shader, GL_COMPILE_STATUS, &compiled);
    if (compiled){
        return true;
    } else {
        std::cout << "Shader[" << id << "] " << (enigma::shaders[id]->type == sh_vertex?"Vertex shader":"Pixel shader") << " - Compilation failed - Info log: " << std::endl;
        glsl_shader_print_infolog(id);
        return false;
    }
}

bool glsl_shader_get_compiled(int id) {
    GLint compiled;
    glGetProgramiv(enigma::shaders[id]->shader, GL_COMPILE_STATUS, &compiled);
    if (compiled){
        return true;
    } else {
        return false;
    }
}

string glsl_shader_get_infolog(int id)
{
    return enigma::shaders[id]->log;
}

string glsl_program_get_infolog(int id)
{
    return enigma::shaderprograms[id]->log;
}

void glsl_shader_free(int id)
{
    delete enigma::shaders[id];
}

int glsl_program_create()
{
    unsigned int id = enigma::shaderprograms.size();
    enigma::shaderprograms.push_back(new enigma::ShaderProgram());
    return id;
}

bool glsl_program_link(int id)
{
    glLinkProgram(enigma::shaderprograms[id]->shaderprogram);
    GLint linked;
    glGetProgramiv(enigma::shaderprograms[id]->shaderprogram, GL_LINK_STATUS, &linked);
    if (linked){
        enigma::getUniforms(id);
        enigma::getAttributes(id);
        enigma::getDefaultUniforms(id);
        enigma::getDefaultAttributes(id);
        return true;
    } else {
        std::cout << "Shader program[" << id << "] - Linking failed - Info log: " << std::endl;
        glsl_program_print_infolog(id);
        return false;
    }
}

bool glsl_program_validate(int id)
{
    glValidateProgram(enigma::shaderprograms[id]->shaderprogram);
    GLint validated;
    glGetProgramiv(enigma::shaderprograms[id]->shaderprogram, GL_VALIDATE_STATUS, &validated);
    if (validated){
        return true;
    } else {
        std::cout << "Shader program[" << id << "] - Validation failed - Info log: " << std::endl;
        glsl_program_print_infolog(id);
        return false;
    }
}

void glsl_program_attach(int id, int sid)
{
    glAttachShader(enigma::shaderprograms[id]->shaderprogram, enigma::shaders[sid]->shader);
}

void glsl_program_detach(int id, int sid)
{
    glDetachShader(enigma::shaderprograms[id]->shaderprogram, enigma::shaders[sid]->shader);
}

void glsl_program_set(int id)
{
    if (enigma::bound_shader != id){
        texture_reset();
        enigma::bound_shader = id;
        glUseProgram(enigma::shaderprograms[id]->shaderprogram);
    }
}

void glsl_program_reset()
{
    //if (enigma::bound_shader != enigma::main_shader){ //This doesn't work because enigma::bound_shader is the same as enigma::main_shader at start
        //NOTE: Texture must be reset first so the Global VBO can draw and let people use shaders on text.
        texture_reset();
        enigma::bound_shader = enigma::main_shader;
        glUseProgram(enigma::shaderprograms[enigma::main_shader]->shaderprogram);
    //}
}

void glsl_program_default_set(int id){
    enigma::main_shader = id;
}

void glsl_program_default_reset(){
    enigma::main_shader = enigma::default_shader;
}

void glsl_program_free(int id)
{
    delete enigma::shaderprograms[id];
}

void glsl_program_set_name(int id, string name){
	enigma::shaderprograms[id]->name = name;
}

int glsl_get_uniform_location(int program, string name) {
	//int uni = glGetUniformLocation(enigma::shaderprograms[program]->shaderprogram, name.c_str());
	std::map<string,GLint>::iterator it = enigma::shaderprograms[program]->uniform_names.find(name);
    if (it == enigma::shaderprograms[program]->uniform_names.end()){
		if (enigma::shaderprograms[program]->name == ""){
			printf("Program[%i] - Uniform %s not found!\n", program, name.c_str());
		}else{
			printf("Program[%s = %i] - Uniform %s not found!\n", enigma::shaderprograms[program]->name.c_str(), program, name.c_str());
		}
        return -1;
    }else{
        return it->second;
    }
}

int glsl_get_attribute_location(int program, string name) {
	//int uni = glGetAttribLocation(enigma::shaderprograms[program]->shaderprogram, name.c_str());
    std::map<string,GLint>::iterator it = enigma::shaderprograms[program]->attribute_names.find(name);
    if (it == enigma::shaderprograms[program]->attribute_names.end()){
		if (enigma::shaderprograms[program]->name == ""){
			printf("Program[%i] - Attribute %s not found!\n", program, name.c_str());
		}else{
			printf("Program[%s = %i] - Attribute %s not found!\n", enigma::shaderprograms[program]->name.c_str(), program, name.c_str());
		}
        return -1;
    }else{
        return it->second;
    }
}

void glsl_uniformf(int location, float v0) {
    get_uniform(it,location,1);
    if (it->second.data[0].f != v0){
        glUniform1f(location, v0);
        it->second.data[0].f = v0;
    }
}

void glsl_uniformf(int location, float v0, float v1) {
    get_uniform(it,location,2);
    if (it->second.data[0].f != v0 || it->second.data[1].f != v1){
        glUniform2f(location, v0, v1);
        it->second.data[0].f = v0, it->second.data[1].f = v1;
	}
}

void glsl_uniformf(int location, float v0, float v1, float v2) {
    get_uniform(it,location,3);
    if (it->second.data[0].f != v0 || it->second.data[1].f != v1 || it->second.data[2].f != v2){
        glUniform3f(location, v0, v1, v2);
        it->second.data[0].f = v0, it->second.data[1].f = v1, it->second.data[2].f = v2;
	}
}

void glsl_uniformf(int location, float v0, float v1, float v2, float v3) {
    get_uniform(it,location,4);
    if (it->second.data[0].f != v0 || it->second.data[1].f != v1 || it->second.data[2].f != v2 || it->second.data[3].f != v3){
        glUniform4f(location, v0, v1, v2, v3);
        it->second.data[0].f = v0, it->second.data[1].f = v1, it->second.data[2].f = v2, it->second.data[3].f = v3;
	}
}

void glsl_uniformi(int location, int v0) {
    get_uniform(it,location,1);
    if (it->second.data[0].i != v0){
        glUniform1i(location, v0);
        it->second.data[0].i = v0;
    }
}

void glsl_uniformi(int location, int v0, int v1) {
    get_uniform(it,location,2);
    if (it->second.data[0].i != v0 || it->second.data[1].i != v1){
        glUniform2i(location, v0, v1);
        it->second.data[0].i = v0, it->second.data[1].i = v1;
    }
}

void glsl_uniformi(int location, int v0, int v1, int v2) {
    get_uniform(it,location,3);
    if (it->second.data[0].i != v0 || it->second.data[1].i != v1 || it->second.data[2].i != v2){
        glUniform3i(location, v0, v1, v2);
        it->second.data[0].i = v0, it->second.data[1].i = v1, it->second.data[2].i = v2;
    }
}

void glsl_uniformi(int location, int v0, int v1, int v2, int v3) {
    get_uniform(it,location,4);
    if (it->second.data[0].i != v0 || it->second.data[1].i != v1 || it->second.data[2].i != v2 || it->second.data[3].i != v3){
        glUniform4i(location, v0, v1, v2, v3);
        it->second.data[0].i = v0, it->second.data[1].i = v1, it->second.data[2].i = v2, it->second.data[3].i = v3;
    }
}

void glsl_uniformui(int location, unsigned v0) {
    get_uniform(it,location,1);
    if (it->second.data[0].ui != v0){
        glUniform1ui(location, v0);
        it->second.data[0].ui = v0;
    }
}

void glsl_uniformui(int location, unsigned v0, unsigned v1) {
    get_uniform(it,location,2);
    if (it->second.data[0].ui != v0 || it->second.data[1].ui != v1){
        glUniform2ui(location, v0, v1);
        it->second.data[0].ui = v0, it->second.data[1].ui = v1;
    }
}

void glsl_uniformui(int location, unsigned v0, unsigned v1, unsigned v2) {
    get_uniform(it,location,3);
    if (it->second.data[0].ui != v0 || it->second.data[1].ui != v1 || it->second.data[2].ui != v2){
        glUniform3ui(location, v0, v1, v2);
        it->second.data[0].ui = v0, it->second.data[1].ui = v1, it->second.data[2].ui = v2;
    }
}

void glsl_uniformui(int location, unsigned v0, unsigned v1, unsigned v2, unsigned v3) {
    get_uniform(it,location,4);
    if (it->second.data[0].ui != v0 || it->second.data[1].ui != v1 || it->second.data[2].ui != v2 || it->second.data[3].ui != v3){
        glUniform4ui(location, v0, v1, v2, v3);
        it->second.data[0].ui = v0, it->second.data[1].ui = v1, it->second.data[2].ui = v2, it->second.data[3].ui = v3;
    }
}

////////////////////////VECTOR FUNCTIONS FOR FLOAT UNIFORMS/////////////////
void glsl_uniform1fv(int location, int size, const float *value){
    get_uniform(it,location,1);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeFComp) == false){
        glUniform1fv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].f = value[i];
        }
    }
}

void glsl_uniform2fv(int location, int size, const float *value){
    get_uniform(it,location,2);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeFComp) == false){
        glUniform2fv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].f = value[i];
        }
    }
}

void glsl_uniform3fv(int location, int size, const float *value){
    get_uniform(it,location,3);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeFComp) == false){
        glUniform3fv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].f = value[i];
        }
    }
}

void glsl_uniform4fv(int location, int size, const float *value){
    get_uniform(it,location,4);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeFComp) == false){
        glUniform4fv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].f = value[i];
        }
    }
}

////////////////////////VECTOR FUNCTIONS FOR INT UNIFORMS/////////////////
void glsl_uniform1iv(int location, int size, const int *value){
    get_uniform(it,location,1);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeIComp) == false){
        glUniform1iv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].i = value[i];
        }
    }
}

void glsl_uniform2iv(int location, int size, const int *value){
    get_uniform(it,location,2);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeIComp) == false){
        glUniform2iv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].i = value[i];
        }
    }
}

void glsl_uniform3iv(int location, int size, const int *value){
    get_uniform(it,location,3);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeIComp) == false){
        glUniform3iv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].i = value[i];
        }
    }
}

void glsl_uniform4iv(int location, int size, const int *value){
    get_uniform(it,location,4);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeIComp) == false){
        glUniform4iv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].i = value[i];
        }
    }
}

////////////////////////VECTOR FUNCTIONS FOR UNSIGNED INT UNIFORMS/////////////////
void glsl_uniform1uiv(int location, int size, const unsigned int *value){
    get_uniform(it,location,1);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeUIComp) == false){
        glUniform1uiv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].ui = value[i];
        }
    }
}

void glsl_uniform2uiv(int location, int size, const unsigned int *value){
    get_uniform(it,location,2);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeUIComp) == false){
        glUniform2uiv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].ui = value[i];
        }
    }
}

void glsl_uniform3uiv(int location, int size, const unsigned int *value){
    get_uniform(it,location,3);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeUIComp) == false){
        glUniform3uiv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].ui = value[i];
        }
    }
}

void glsl_uniform4uiv(int location, int size, const unsigned int *value){
    get_uniform(it,location,4);
    if (std::equal(it->second.data.begin(), it->second.data.end(), value, enigma::UATypeUIComp) == false){
        glUniform4uiv(location, size, value);
        for (size_t i=0; i<it->second.data.size(); ++i){
            it->second.data[i].ui = value[i];
        }
    }
}

}

