/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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
 **/

#include "XLDefine.h"
#include "XLDirector.h"
#include "XLTestAppDelegate.h"
#include "XLPlatform.h"

#include "XLTestAppShaders.cc"

namespace stappler::xenolith::app {

static AppDelegate s_delegate;

AppDelegate::AppDelegate() { }

AppDelegate::~AppDelegate() { }

bool AppDelegate::onFinishLaunching() {
	if (!Application::onFinishLaunching()) {
		return false;
	}

	return true;
}

void AppDelegate::onDeviceInit(vk::PresentationDevice *dev, const stappler::Callback<void(draw::LoaderStage &&, bool deferred)> &cb) {
	draw::LoaderStage init;
	init.name = StringView("init");
	init.programs.emplace_back(draw::ProgramParams({
		vk::ProgramSource::Glsl,
		vk::ProgramStage::Vertex,
		FilePath(StringView()),
		BytesView((const uint8_t *)vk::VertexListVert, strlen(vk::VertexListVert)),
		StringView("App::VertexListVert")
	}));
	init.programs.emplace_back(draw::ProgramParams({
		vk::ProgramSource::Glsl,
		vk::ProgramStage::Fragment,
		FilePath(StringView()),
		BytesView((const uint8_t *)vk::VertexListFrag, strlen(vk::VertexListFrag)),
		StringView("App::VertexListFrag")
	}));
	init.pipelines.emplace_back(draw::PipelineParams({
		Rect(),
		URect(),
		draw::VertexFormat::Vertex_V4F_C4F_T2F,
		draw::LayoutFormat::Vertexes,
		draw::RenderPassBind::Default,
		draw::DynamicState::Default,
		{ "App::VertexListVert", "App::VertexListFrag" },
		StringView("App::VertexDrawingPipeline")
	}));

	cb(move(init), false);
}

}
