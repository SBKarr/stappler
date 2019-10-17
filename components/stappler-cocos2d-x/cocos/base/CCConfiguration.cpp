/****************************************************************************
Copyright (c) 2010      Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
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

#include "SPLayout.h"
#include "base/CCConfiguration.h"
#include "platform/CCFileUtils.h"

USING_NS_SP;

NS_CC_BEGIN

static uint32_t s_supportedRenderTarget =
#ifndef GL_ES_VERSION_2_0
		toInt(Configuration::RenderTarget::RGBA8)
		| toInt(Configuration::RenderTarget::RGB8)
		| toInt(Configuration::RenderTarget::RG8)
		| toInt(Configuration::RenderTarget::R8)
		| toInt(Configuration::RenderTarget::A8);
#else
		0;
#endif

extern const char* cocos2dVersion();

Configuration* Configuration::s_sharedConfiguration = nullptr;

bool Configuration::isRenderTargetSupported(RenderTarget target) {
	return (s_supportedRenderTarget & toInt(target)) != 0;
}

Configuration::Configuration()
: _maxTextureSize(0)
, _maxModelviewStackDepth(0)
, _supportsPVRTC(false)
, _supportsETC1(false)
, _supportsS3TC(false)
, _supportsATITC(false)
, _supportsNPOT(false)
, _supportsBGRA8888(false)
, _supportsDiscardFramebuffer(false)
, _supportsShareableVAO(false)
, _maxSamplesAllowed(0)
, _maxTextureUnits(0)
, _glExtensions(nullptr)
, _maxDirLightInShader(1)
, _maxPointLightInShader(1)
, _maxSpotLightInShader(1)
{
}

bool Configuration::init()
{
	_data.setString(cocos2dVersion(), "cocos2d.x.version");


#if CC_ENABLE_PROFILERS
	_data.setBool(true, "cocos2d.x.compiled_with_profiler");
#else
	_data.setBool(false, "cocos2d.x.compiled_with_profiler");
#endif

#if CC_ENABLE_GL_STATE_CACHE == 0
	_data.setBool(false, "cocos2d.x.compiled_with_gl_state_cache");
#else
	_data.setBool(true, "cocos2d.x.compiled_with_gl_state_cache");
#endif

#if COCOS2D_DEBUG
	_data.setString("DEBUG", "cocos2d.x.build_type");
#else
	_data.setString("RELEASE", "cocos2d.x.build_type");
#endif

	return true;
}

Configuration::~Configuration()
{
}

std::string Configuration::getInfo() const
{
	// And Dump some warnings as well
#if CC_ENABLE_PROFILERS
    CCLOG("cocos2d: **** WARNING **** CC_ENABLE_PROFILERS is defined. Disable it when you finish profiling (from ccConfig.h)\n");
#endif

#if CC_ENABLE_GL_STATE_CACHE == 0
    CCLOG("cocos2d: **** WARNING **** CC_ENABLE_GL_STATE_CACHE is disabled. To improve performance, enable it (from ccConfig.h)\n");
#endif

    // Dump
    return stappler::data::toString(_data, true);
}

void Configuration::gatherGPUInfo()
{
	auto glVersion = (const char*)glGetString(GL_VERSION);

	_data.setString((const char*)glGetString(GL_VENDOR), "gl.vendor");
	_data.setString((const char*)glGetString(GL_RENDERER), "gl.renderer");
	_data.setString(glVersion, "gl.version");

	StringView vReader(glVersion);
	if (vReader.is("OpenGL ES 3.")) {
		_supportsEs30Api = true;
		s_supportedRenderTarget |= toInt(Configuration::RenderTarget::R8) | toInt(Configuration::RenderTarget::RG8)
				| toInt(Configuration::RenderTarget::RGBA8) | toInt(Configuration::RenderTarget::RGB8);
		_supportsBlendMinMax = true;
	}

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_maxTextureSize);
	_data.setInteger(_maxTextureSize, "gl.max_texture_size");

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &_maxTextureUnits);
	_data.setInteger(_maxTextureUnits, "gl.max_texture_units");

    _supportsNPOT = true;
	_data.setBool(_supportsNPOT, "gl.supports_NPOT");

    _glExtensions = (char *)glGetString(GL_EXTENSIONS);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    glGetIntegerv(GL_MAX_SAMPLES_APPLE, &_maxSamplesAllowed);
	_data.setInteger(_maxSamplesAllowed, "gl.max_samples_allowed");
#endif

#ifndef GL_ES_VERSION_2_0
	_supportsMapBuffer = true;
	_supportsBlendMinMax = true;
#endif

	StringView r(_glExtensions, strlen(_glExtensions));

    r.split<StringView::Chars<' '>>([&] (StringView &b) {
    	//stappler::log::text("GLext", b.data(), b.size());
    	if (b == "GL_OES_compressed_ETC1_RGB8_texture") {
    		_supportsETC1 = true;
    	} else if (b == "GL_EXT_texture_compression_s3tc") {
    		_supportsS3TC = true;
    	} else if (b == "GL_AMD_compressed_ATC_texture") {
    		_supportsATITC = true;
    	} else if (b == "GL_IMG_texture_compression_pvrtc") {
    		_supportsPVRTC = true;
    	} else if (b == "GL_IMG_texture_format_BGRA888") {
    		_supportsBGRA8888 = true;
    	} else if (b == "GL_EXT_discard_framebuffer") {
    		_supportsDiscardFramebuffer = true;
    	} else if (b == "GL_OES_vertex_array_object") {
    		_supportsShareableVAO = true;
    	} else if (b == "GL_EXT_blend_minmax") {
    		_supportsBlendMinMax = true;
#ifdef GL_ES_VERSION_2_0
    	} else if (b == "GL_OES_mapbuffer") {
    		_supportsMapBuffer = true;
    	} else if (b == "GL_OES_rgb8_rgba8") {
    		s_supportedRenderTarget |= (toInt(Configuration::RenderTarget::RGBA8) | toInt(Configuration::RenderTarget::RGB8));
    	} else if (b == "GL_EXT_texture_rg") {
    		s_supportedRenderTarget |= toInt(Configuration::RenderTarget::R8) | toInt(Configuration::RenderTarget::RG8);
    	} else if (b == "GL_ARM_rgba8") {
    		s_supportedRenderTarget |= toInt(Configuration::RenderTarget::RGBA8);
#endif
    	}
    });

    String extraTargets;
    if (s_supportedRenderTarget & toInt(Configuration::RenderTarget::RGBA8)) {
    	extraTargets += "RGBA8 ";
    }
    if (s_supportedRenderTarget & toInt(Configuration::RenderTarget::RGB8)) {
    	extraTargets += "RGB8 ";
    }
    if (s_supportedRenderTarget & toInt(Configuration::RenderTarget::RG8)) {
    	extraTargets += "RG8 ";
    }
    if (s_supportedRenderTarget & toInt(Configuration::RenderTarget::R8)) {
    	extraTargets += "R8 ";
    }
    if (s_supportedRenderTarget & toInt(Configuration::RenderTarget::A8)) {
    	extraTargets += "A8 ";
    }

    if (!extraTargets.empty()) {
    	extraTargets.pop_back();
    }

    _data.setString(extraTargets, "gl.extra_render_targets");
	_data.setBool(_supportsETC1, "gl.supports_ETC1");
	_data.setBool(_supportsS3TC, "gl.supports_S3TC");
	_data.setBool(_supportsATITC, "gl.supports_ATITC");
	_data.setBool(_supportsPVRTC, "gl.supports_PVRTC");
	_data.setBool(_supportsBGRA8888, "gl.supports_BGRA8888");
	_data.setBool(_supportsDiscardFramebuffer, "gl.supports_discard_framebuffer");
	_data.setBool(_supportsShareableVAO, "gl.supports_vertex_array_object");
	_data.setBool(_supportsMapBuffer, "gl.supports_map_buffer");
	_data.setBool(_supportsBlendMinMax, "gl.supports_blend_minmax");

    CHECK_GL_ERROR_DEBUG();
}

Configuration* Configuration::getInstance()
{
    if (! s_sharedConfiguration)
    {
        s_sharedConfiguration = new (std::nothrow) Configuration();
        s_sharedConfiguration->init();
    }

    return s_sharedConfiguration;
}

void Configuration::destroyInstance()
{
    CC_SAFE_RELEASE_NULL(s_sharedConfiguration);
}


bool Configuration::checkForGLExtension(const std::string &searchName) const
{
   return  (_glExtensions && strstr(_glExtensions, searchName.c_str() ) ) ? true : false;
}

//
// getters for specific variables.
// Mantained for backward compatiblity reasons only.
//
int Configuration::getMaxTextureSize() const
{
	return _maxTextureSize;
}

int Configuration::getMaxModelviewStackDepth() const
{
	return _maxModelviewStackDepth;
}

int Configuration::getMaxTextureUnits() const
{
	return _maxTextureUnits;
}

bool Configuration::supportsNPOT() const
{
	return _supportsNPOT;
}

bool Configuration::supportsPVRTC() const
{
	return _supportsPVRTC;
}

bool Configuration::supportsETC() const
{
    //GL_ETC1_RGB8_OES is not defined in old opengl version
#ifdef GL_ETC1_RGB8_OES
    return _supportsETC1;
#else
    return false;
#endif
}

bool Configuration::supportsS3TC() const
{
    return _supportsS3TC;
}

bool Configuration::supportsATITC() const
{
    return _supportsATITC;
}

bool Configuration::supportsBGRA8888() const
{
	return _supportsBGRA8888;
}

bool Configuration::supportsDiscardFramebuffer() const
{
	return _supportsDiscardFramebuffer;
}

bool Configuration::supportsShareableVAO() const
{
#if CC_TEXTURE_ATLAS_USE_VAO
    return _supportsShareableVAO;
#else
    return false;
#endif
}

bool Configuration::supportsMapBuffer() const {
	return _supportsMapBuffer;
}
bool Configuration::supportsBlendMinMax() const {
	return _supportsBlendMinMax;
}
bool Configuration::supportsEs30Api() const {
	return _supportsEs30Api;
}

int Configuration::getMaxSupportDirLightInShader() const
{
    return _maxDirLightInShader;
}

int Configuration::getMaxSupportPointLightInShader() const
{
    return _maxPointLightInShader;
}

int Configuration::getMaxSupportSpotLightInShader() const
{
    return _maxSpotLightInShader;
}

//
// generic getters for properties
//
const Value& Configuration::getValue(const std::string& key, const Value& defaultValue) const
{
	auto &ret = _data.getValue(key);
	if (ret) {
		return ret;
	} else {
		return defaultValue;
	}
}

void Configuration::setValue(const std::string& key, const Value& value)
{
	_data.setValue(value, key);
}

NS_CC_END
