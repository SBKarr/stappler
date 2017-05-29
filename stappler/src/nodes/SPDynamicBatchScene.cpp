// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "SPDynamicBatchScene.h"
#include "SPDynamicAtlas.h"
#include "SPDynamicBatchCommand.h"
#include "SPDynamicQuadArray.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCGroupCommand.h"
#include "SPString.h"
#include "SPEventListener.h"
#include "SPDevice.h"
#include "SPStencilCache.h"

NS_SP_BEGIN

//#define SP_DYNAMIC_BATCH_LOG(...) log(__VA_ARGS__)
#define SP_DYNAMIC_BATCH_LOG(...)

class DynamicBatchSceneRenderer : public cocos2d::Renderer {
public:
	struct CommandZPathCompare {
		using CmdConstPtr = const cocos2d::RenderCommand *;

		bool operator() (const CmdConstPtr &l, const CmdConstPtr &r) {
			auto &lz = l->getZPath();
			auto &rz = r->getZPath();

			auto lzmax = lz.size();
			auto rzmax = rz.size();

			auto max = MAX(lzmax, rzmax);
			for (size_t i = 0; i < max; i++) {
				auto lzi = (i < lzmax)?lz[i]:0;
				auto rzi = (i < rzmax)?rz[i]:0;
				if (lzi < rzi) {
					return true;
				} else if (lzi > rzi) {
					return false;
				}
			}
			return false;
		}
	};

	DynamicBatchSceneRenderer(cocos2d::Renderer *r, DynamicBatchScene *scene) : cocos2d::Renderer(r->getGroupCommandManager()) {
		_scene = scene;
		_commandGroupStack = r->getCommandGroupStack();
	}

	void update(const Mat4 &transform) {
		int32_t id = 0;
		auto &rg = (*_renderGroups);
		for (auto &it : rg) {
			auto &q = it.getSubQueue(cocos2d::RenderQueue::GLOBALZ_ZERO);
			if (!q.empty()) {
				std::stable_sort(q.begin(), q.end(), CommandZPathCompare());
				update(transform, id, q);
			}
			id ++;
		}
	}

	void visitQueue(const Vector<cocos2d::RenderCommand*> &q, const Function<void(DynamicBatchCommand *)> &cb) {
		for (auto it : q) {
			if (it) {
				switch (it->getType()) {
				case cocos2d::RenderCommand::Type::GROUP_COMMAND:
			        visitQueue(
			        		(*_renderGroups)[static_cast<cocos2d::GroupCommand*>(it)->getRenderQueueID()]
											 .getSubQueue(cocos2d::RenderQueue::GLOBALZ_ZERO), cb);
					break;
				case cocos2d::RenderCommand::Type::SP_DYNAMIC_COMMAND:
					cb(static_cast<DynamicBatchCommand *>(it));
					break;
				default:
					break;
				}
			}
		}
	}

	void visit(const Function<void(DynamicBatchCommand *)> &cb) {
		auto &rg = _renderGroups->at(0);
		visitQueue(rg.getSubQueue(cocos2d::RenderQueue::GLOBALZ_ZERO), cb);
	}

protected:
	using CommandIterator = std::vector<cocos2d::RenderCommand *>::iterator;

	void update(const Mat4 &transform, int32_t id, std::vector<cocos2d::RenderCommand *>&q) {
		for (auto cmdIt = q.begin(); cmdIt != q.end(); cmdIt++) {
			if ((*cmdIt)->getType() == cocos2d::RenderCommand::Type::SP_DYNAMIC_COMMAND) {
				if (!update(transform, cmdIt, id)) {
					*cmdIt = nullptr;
				}
			}
		}
	}

	bool update(const Mat4 &transform, CommandIterator &cmdIt, int32_t id) {
		auto cmd = static_cast<DynamicBatchCommand *>(*cmdIt);
		auto mid = cmd->getMaterialId(id);
		auto mit = _materialMap.find(mid);
		if (mit != _materialMap.end()) {
			merge(mid, mit->second, cmd, transform);
			return  false;
		} else {
			_materialMap.insert(std::make_pair(mid, cmdIt));
		}
		return true;
	}

	void merge(uint32_t id, CommandIterator &cmdIt, DynamicBatchCommand *cmd, const Mat4 &transform) {
		DynamicBatchScene::AtlasCacheNode &a = _scene->getAtlasForMaterial(id, cmd);

		if ((*cmdIt) != &a.cmd) {
			auto origCmd = static_cast<DynamicBatchCommand *>(*cmdIt);
			*cmdIt = &a.cmd;

			auto &quads = origCmd->getAtlas()->getQuads();
			for (auto it : quads) {
				it->updateTransform(origCmd->getTransform());
				a.set.insert(it);
			}
		}

		auto &quads = cmd->getAtlas()->getQuads();
		for (auto it : quads) {
			it->updateTransform(cmd->getTransform());
			a.set.insert(it);
		}
	}

	std::map<uint32_t, CommandIterator> _materialMap;
	DynamicBatchScene *_scene = nullptr;
};

SP_DECLARE_EVENT_CLASS(DynamicBatchScene, onFrameBegin);
SP_DECLARE_EVENT_CLASS(DynamicBatchScene, onFrameEnd);

bool DynamicBatchScene::init() {
	if (!Scene::init()) {
		return false;
	}

	auto el = Rc<EventListener>::create();
	el->onEvent(Device::onBackground, [this] (const Event *) {
		_map.clear();
	});
	addComponent(el);

	return true;
}

void DynamicBatchScene::visit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, ZPath &zPath) {
	onFrameBegin(this);

	if (_clearDelay == 1) {
		_shouldClear = true;
		_clearDelay = 0;
	} else if (_clearDelay > 0) {
		-- _clearDelay;
	}

	if (_batchingEnabled) {
		for (auto &it : _map) {
			it.second.cmdInit = false;
		}

		DynamicBatchSceneRenderer sr(r, this);

		_beginCommand.init(_globalZOrder, ZPath{minOf<int>()});
		_beginCommand.func = CC_CALLBACK_0(DynamicBatchScene::onBeforeFrame, this);
		sr.addCommand(&_beginCommand);

		cocos2d::Scene::visit(&sr, t, f, zPath);

		_endCommand.init(_globalZOrder, ZPath{maxOf<int>()});
		_endCommand.func = CC_CALLBACK_0(DynamicBatchScene::onAfterFrame, this);
		sr.addCommand(&_endCommand);

		sr.update(_modelViewTransform);

		if (_stencilOptEnabled) {
			auto s = StencilCache::getInstance();
			s->enable();
			s->clear();

			// log::text("Cmd", "Begin");
			uint8_t stencilIdx = 0;
			sr.visit([&] (DynamicBatchCommand *cmd) {
				// StringStream out;

				if (cmd->isStencil()) {
					stencilIdx = cmd->makeStencil();
				} else {
					cmd->setStencilIndex(stencilIdx);
				}

				/* out << "stencil: " << int(cmd->getStencilIndex()) << " ";
				out << "batch: " << cmd->isBatch() << " - ";

				auto &path = cmd->getZPath();
				for (auto &it : path) {
					out << it << " ";
				}

				log::text("Cmd", out.str());*/
			});

			s->disable();
		}

		for (auto &it : _map) {
			if (it.second.cmdInit && !it.second.set.empty()) {
				it.second.atlas->updateQuadArrays(std::move(it.second.set));
				it.second.set.clear();
			}
		}

		if (_shouldClear) {
			std::vector<uint32_t> unused;
			for (auto &it : _map) {
				if (!it.second.cmdInit) {
					unused.push_back(it.first);
				}
			}
			for (auto &it : unused) {
				_map.erase(it);
			}
			_shouldClear = false;
		}
	} else {
		_beginCommand.init(_globalZOrder, ZPath{minOf<int>()});
		_beginCommand.func = CC_CALLBACK_0(DynamicBatchScene::onBeforeFrame, this);
		r->addCommand(&_beginCommand);

		cocos2d::Scene::visit(r, t, f, zPath);

		_endCommand.init(_globalZOrder, ZPath{maxOf<int>()});
		_endCommand.func = CC_CALLBACK_0(DynamicBatchScene::onAfterFrame, this);
		r->addCommand(&_endCommand);
	}

	onFrameEnd(this);
}


DynamicBatchScene::AtlasCacheNode::AtlasCacheNode(Rc<DynamicAtlas> &&atlas)
: atlas(atlas), cmd(true), cmdInit(false) { }

DynamicBatchScene::AtlasCacheNode &DynamicBatchScene::getAtlasForMaterial(uint32_t id, DynamicBatchCommand *cmd) {
	auto it = _map.find(id);
	if (it == _map.end()) {
		cocos2d::Texture2D *tex = cmd->getAtlas()->getTexture();
		it = _map.emplace(id, Rc<DynamicAtlas>::create(tex)).first;
	}

	if (!it->second.cmdInit) {
		auto zPath = cmd->getZPath();
		it->second.cmd.init(0.0f, cmd->getProgram(), cmd->getBlendFunc(), it->second.atlas, Mat4::IDENTITY, std::move(zPath), cmd->isNormalized(), cmd->isStencil());
		it->second.cmdInit = true;
	}
	return it->second;
}

void DynamicBatchScene::setBatchingEnabled(bool value) {
	_batchingEnabled = value;
}
bool DynamicBatchScene::isBatchingEnabled() const {
	return _batchingEnabled;
}

void DynamicBatchScene::clearCachedMaterials(bool force) {
	if (force) {
		_shouldClear = true;
	} else {
		_clearDelay = 10;
	}
}

void DynamicBatchScene::onBeforeFrame() {
	_frameStart = Time::now();
	if (_stencilOptEnabled) {
		StencilCache::getInstance()->enable();
	}
}

void DynamicBatchScene::onAfterFrame() {
	if (_stencilOptEnabled) {
		StencilCache::getInstance()->disable();
	}

	_frameTime += (Time::now() - _frameStart);
	++ _frames;

	//log::format("Frame", "%llu", _frameTime.toMicroseconds() / _frames);
}

NS_SP_END
