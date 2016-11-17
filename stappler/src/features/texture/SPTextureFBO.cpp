/*
 * SPTextureFBO.cpp
 *
 *  Created on: 04 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPTextureFBO.h"
#include "SPDynamicAtlas.h"
#include "SPDynamicBatchCommand.h"
#include "2d/CCRenderTexture.h"
#include "2d/CCScene.h"
#include "2d/CCCamera.h"

#include "base/CCDirector.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"

NS_SP_BEGIN

TextureFBO::~TextureFBO() {
	CC_SAFE_RELEASE(_image);
}

bool TextureFBO::init(Image *img, PixelFormat format, uint16_t w, uint16_t h, size_t capacity) {
	if (format == PixelFormat::RGB888 || format == PixelFormat::RGBA8888) {
		_image = img;
		CC_SAFE_RETAIN(_image);
		_quads.setCapacity(capacity);
		_format = format;
		_width = w;
		_height = h;
		return true;
	}
	return false;
}

uint8_t TextureFBO::getBytesPerPixel() const {
	switch (_format) {
	case PixelFormat::RGB888:
	case PixelFormat::RGBA8888:
		return 8;
		break;
	default:
		break;
	}
	return 0;
}
PixelFormat TextureFBO::getFormat() const {
	return _format;
}
uint8_t TextureFBO::getComponentsCount() const {
	switch (_format) {
	case PixelFormat::RGB888:
		return 3;
		break;
	case PixelFormat::RGBA8888:
		return 4;
		break;
	default:
		break;
	}
	return 0;
}
uint8_t TextureFBO::getBitsPerComponent() const {
	switch (_format) {
	case PixelFormat::RGB888:
	case PixelFormat::RGBA8888:
		return 8;
		break;
	default:
		break;
	}
	return 0;
}

uint16_t TextureFBO::getWidth() const {
	return _width;
}
uint16_t TextureFBO::getHeight() const {
	return _height;
}

cocos2d::Texture2D *TextureFBO::generateTexture() const {
	auto director = cocos2d::Director::getInstance();
	auto renderer = director->getRenderer();
	auto scene = director->getRunningScene();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

    cocos2d::Camera* defaultCamera = nullptr;
    for (auto& camera : scene->getCameras()) {
        if (camera->getCameraFlag() == cocos2d::CameraFlag::DEFAULT) {
            defaultCamera = camera;
            continue;
        }
    }

	DynamicAtlas *a = new DynamicAtlas();
	a->init(_image->retainTexture(), false);
	a->addQuadArray(const_cast<DynamicQuadArray *>(&_quads));

    auto shader = cocos2d::GLProgramCache::getInstance()->getGLProgram(
    		cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_A8_COLOR);

	cocos2d::RenderTexture *rt = new cocos2d::RenderTexture();
	rt->initWithWidthAndHeight(_width, _height, convertPixelFormat(_format));


	DynamicBatchCommand cmd;
	cmd.init(0.0f, shader, cocos2d::BlendFunc::ALPHA_PREMULTIPLIED, a, cocos2d::Mat4::IDENTITY, std::vector<int>(), false);

    if (defaultCamera) {
    	cocos2d::Camera::setVisitingCamera(defaultCamera);
        director->pushMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
        director->loadMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION, cocos2d::Camera::getVisitingCamera()->getViewProjectionMatrix());

    	rt->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    	renderer->addCommand(&cmd);
        rt->end();

        director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
    }

    cocos2d::Camera::setVisitingCamera(nullptr);

    renderer->render();

	rt->autorelease();
	a->release();
    director->popMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

	auto tex = rt->getSprite()->getTexture();
	tex->setAliasTexParameters();
	_image->releaseTexture();
	return tex;
}

bool TextureFBO::drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const cocos2d::Color4B &color) {
	auto id = _quads.emplace();
	_quads.setTextureRect(id, cocos2d::Rect(_image->getWidth() - 1, _image->getHeight() - 1, 1, 1),
			_image->getWidth(), _image->getHeight(), false, false);
	_quads.setGeometry(id, cocos2d::Vec2(x, y), cocos2d::Size(width, height), 0.0f);
	_quads.setColor(id, color);
	return true;
}

bool TextureFBO::drawOutline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t outline, const cocos2d::Color4B &color) {
	return false;
}

NS_SP_END
