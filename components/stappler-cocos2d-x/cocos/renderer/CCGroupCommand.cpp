/****************************************************************************
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


#include "renderer/CCGroupCommand.h"
#include "renderer/CCRenderer.h"
#include "base/CCDirector.h"

NS_CC_BEGIN

GroupCommandManager::GroupCommandManager()
{
	_renderGroups.reserve(10);
	_renderGroups.emplace_back();
}

GroupCommandManager::~GroupCommandManager()
{
	_renderGroups.clear();
}

bool GroupCommandManager::init()
{
    //0 is the default render group
    _groupMapping[0] = true;
    return true;
}

int GroupCommandManager::getGroupID()
{
    //Reuse old id
    if (!_unusedIDs.empty())
    {
        int groupID = *_unusedIDs.rbegin();
        _unusedIDs.pop_back();
        _groupMapping[groupID] = true;
        return groupID;
    }

    //Create new ID
//    int newID = _groupMapping.size();
    int newID = createRenderQueue();
    _groupMapping[newID] = true;

    return newID;
}

void GroupCommandManager::releaseGroupID(int groupID)
{
    _groupMapping[groupID] = false;
    _unusedIDs.push_back(groupID);
}

int GroupCommandManager::createRenderQueue()
{
    _renderGroups.emplace_back();
    return (int)_renderGroups.size() - 1;
}

GroupCommand::GroupCommand()
{
    _type = RenderCommand::Type::GROUP_COMMAND;
    _renderQueueID = Director::getInstance()->getRenderer()->getGroupCommandManager()->getGroupID();
}

void GroupCommand::init(float globalOrder, const std::vector<int> &zPath)
{
    _globalOrder = globalOrder;
    if (_globalOrder == 0.0f) {
    	_zPath = zPath;
    	while (!_zPath.empty() && _zPath.back() == 0) {
    		_zPath.pop_back();
    	}
    }
    auto manager = Director::getInstance()->getRenderer()->getGroupCommandManager();
    manager->releaseGroupID(_renderQueueID);
    _renderQueueID = manager->getGroupID();
}

GroupCommand::~GroupCommand()
{
    Director::getInstance()->getRenderer()->getGroupCommandManager()->releaseGroupID(_renderQueueID);
}

NS_CC_END