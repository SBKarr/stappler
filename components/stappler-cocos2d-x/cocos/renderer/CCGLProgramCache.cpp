/****************************************************************************
Copyright (c) 2011      Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "renderer/CCGLProgramCache.h"

#include "renderer/CCGLProgram.h"
#include "renderer/ccShaders.h"
#include "base/ccMacros.h"
#include "base/CCConfiguration.h"

NS_CC_BEGIN

enum {
    kShaderType_PositionTextureColor,
    kShaderType_PositionTextureColor_noMVP,
    kShaderType_PositionColor,
    kShaderType_PositionColor_noMVP,
    kShaderType_PositionTexture,
    kShaderType_PositionTexture_uColor,
    kShaderType_PositionTextureA8Color,
    kShaderType_Position_uColor,
    kShaderType_MAX,
};

static GLProgramCache *_sharedGLProgramCache = 0;

GLProgramCache* GLProgramCache::getInstance()
{
    if (!_sharedGLProgramCache) {
        _sharedGLProgramCache = new (std::nothrow) GLProgramCache();
        if (!_sharedGLProgramCache->init())
        {
            CC_SAFE_DELETE(_sharedGLProgramCache);
        }
    }
    return _sharedGLProgramCache;
}

void GLProgramCache::destroyInstance()
{
    CC_SAFE_RELEASE_NULL(_sharedGLProgramCache);
}

GLProgramCache::GLProgramCache()
: _programs()
{

}

GLProgramCache::~GLProgramCache()
{
    for( auto it = _programs.begin(); it != _programs.end(); ++it ) {
        (it->second)->release();
    }

    CCLOGINFO("deallocing GLProgramCache: %p", this);
}

bool GLProgramCache::init()
{
    loadDefaultGLPrograms();
    return true;
}

void GLProgramCache::loadDefaultGLPrograms()
{
    // Position Texture Color shader
    GLProgram *p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionTextureColor);
    _programs.insert( std::make_pair( GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR, p ) );

    // Position Texture Color without MVP shader
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionTextureColor_noMVP);
    _programs.insert( std::make_pair( GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_NO_MVP, p ) );

    //
    // Position, Color shader
    //
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionColor);
    _programs.insert( std::make_pair(GLProgram::SHADER_NAME_POSITION_COLOR, p) );

    //
    // Position, Color shader no MVP
    //
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionColor_noMVP);
    _programs.insert( std::make_pair(GLProgram::SHADER_NAME_POSITION_COLOR_NO_MVP, p) );

    //
    // Position Texture shader
    //
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionTexture);
    _programs.insert( std::make_pair( GLProgram::SHADER_NAME_POSITION_TEXTURE, p) );

    //
    // Position, Texture attribs, 1 Color as uniform shader
    //
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionTexture_uColor);
    _programs.insert( std::make_pair( GLProgram::SHADER_NAME_POSITION_TEXTURE_U_COLOR, p) );

    //
    // Position Texture A8 Color shader
    //
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_PositionTextureA8Color);
    _programs.insert( std::make_pair(GLProgram::SHADER_NAME_POSITION_TEXTURE_A8_COLOR, p) );

    //
    // Position and 1 color passed as a uniform (to simulate glColor4ub )
    //
    p = new (std::nothrow) GLProgram();
    loadDefaultGLProgram(p, kShaderType_Position_uColor);
    _programs.insert( std::make_pair(GLProgram::SHADER_NAME_POSITION_U_COLOR, p) );
}

void GLProgramCache::reloadDefaultGLPrograms()
{
    // reset all programs and reload them

    // Position Texture Color shader
    GLProgram *p = getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_PositionTextureColor);

    // Position Texture Color without MVP shader
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_NO_MVP);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_PositionTextureColor_noMVP);

    //
    // Position, Color shader
    //
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_COLOR);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_PositionColor);

    //
    // Position, Color shader no MVP
    //
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_COLOR_NO_MVP);
    loadDefaultGLProgram(p, kShaderType_PositionColor_noMVP);

    //
    // Position Texture shader
    //
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_PositionTexture);

    //
    // Position, Texture attribs, 1 Color as uniform shader
    //
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE_U_COLOR);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_PositionTexture_uColor);

    //
    // Position Texture A8 Color shader
    //
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE_A8_COLOR);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_PositionTextureA8Color);

    //
    // Position and 1 color passed as a uniform (to simulate glColor4ub )
    //
    p = getGLProgram(GLProgram::SHADER_NAME_POSITION_U_COLOR);
    p->reset();
    loadDefaultGLProgram(p, kShaderType_Position_uColor);
}

void GLProgramCache::loadDefaultGLProgram(GLProgram *p, int type)
{
    switch (type) {
        case kShaderType_PositionTextureColor:
            p->initWithByteArrays(ccPositionTextureColor_vert, ccPositionTextureColor_frag);
            break;
        case kShaderType_PositionTextureColor_noMVP:
            p->initWithByteArrays(ccPositionTextureColor_noMVP_vert, ccPositionTextureColor_noMVP_frag);
            break;
        case kShaderType_PositionColor:
            p->initWithByteArrays(ccPositionColor_vert ,ccPositionColor_frag);
            break;
        case kShaderType_PositionColor_noMVP:
            p->initWithByteArrays(ccPositionTextureColor_noMVP_vert ,ccPositionColor_frag);
            break;
        case kShaderType_PositionTexture:
            p->initWithByteArrays(ccPositionTexture_vert ,ccPositionTexture_frag);
            break;
        case kShaderType_PositionTexture_uColor:
            p->initWithByteArrays(ccPositionTexture_uColor_vert, ccPositionTexture_uColor_frag);
            break;
        case kShaderType_PositionTextureA8Color:
            p->initWithByteArrays(ccPositionTextureA8Color_vert, ccPositionTextureA8Color_frag);
            break;
        case kShaderType_Position_uColor:
            p->initWithByteArrays(ccPosition_uColor_vert, ccPosition_uColor_frag);
            p->bindAttribLocation("aVertex", GLProgram::VERTEX_ATTRIB_POSITION);
            break;
        default:
            CCLOG("cocos2d: %s:%d, error shader type", __FUNCTION__, __LINE__);
            return;
    }

    p->link();
    p->updateUniforms();

    CHECK_GL_ERROR_DEBUG();
}

GLProgram* GLProgramCache::getGLProgram(const std::string &key)
{
    auto it = _programs.find(key);
    if( it != _programs.end() )
        return it->second;
    return nullptr;
}

void GLProgramCache::addGLProgram(GLProgram* program, const std::string &key)
{
    // release old one
    auto prev = getGLProgram(key);
    if( prev == program )
        return;

    _programs.erase(key);
    CC_SAFE_RELEASE_NULL(prev);

    if (program)
        program->retain();
    _programs[key] = program;
}

std::string GLProgramCache::getShaderMacrosForLight() const
{
    GLchar def[256];
    auto conf = Configuration::getInstance();

    snprintf(def, sizeof(def)-1, "\n#define MAX_DIRECTIONAL_LIGHT_NUM %d \n"
            "\n#define MAX_POINT_LIGHT_NUM %d \n"
            "\n#define MAX_SPOT_LIGHT_NUM %d \n",
             conf->getMaxSupportDirLightInShader(),
             conf->getMaxSupportPointLightInShader(),
             conf->getMaxSupportSpotLightInShader());
    return std::string(def);
}

NS_CC_END
