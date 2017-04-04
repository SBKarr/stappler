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

#ifndef CLASSES_MATERIAL_RESOURCES_MATERIALPROGRESSACTION_H_
#define CLASSES_MATERIAL_RESOURCES_MATERIALPROGRESSACTION_H_

#include "SPDefine.h"
#include "2d/CCActionInterval.h"

NS_SP_BEGIN

class ProgressAction : public cocos2d::ActionInterval {
public:
	typedef std::function<void(ProgressAction *a)> onStartCallback;
	typedef std::function<void(ProgressAction *a, float time)> onUpdateCallback;
	typedef std::function<void(ProgressAction *a)> onStopCallback;

public:
    virtual bool init(float duration, const onUpdateCallback &update,
    		const onStartCallback &start = nullptr, const onStopCallback &stop = nullptr);

    virtual bool init(float duration, float targetProgress, const onUpdateCallback &update,
    		const onStartCallback &start = nullptr, const onStopCallback &stop = nullptr);

    virtual bool init(float duration, float sourceProgress, float targetProgress, const onUpdateCallback &update,
    		const onStartCallback &start = nullptr, const onStopCallback &stop = nullptr);

    virtual void startWithTarget(cocos2d::Node *t) override;
    virtual void update(float time) override;
    virtual void stop() override;
    virtual void onStopped() override;

    void setSourceProgress(float progress);
    float getSourceProgress() const;

    void setTargetProgress(float progress);
    float getTargetProgress() const;

    void setForceStopCallback(bool);
    bool isForceStopCallback() const;

    void setStartCallback(const onStartCallback &cb);
    void setUpdateCallback(const onUpdateCallback &cb);
    void setStopCallback(const onStopCallback &cb);

protected:
    bool _forceStopCallback = false;
    bool _stopped = true;
    float _sourceProgress = 0.0f;
    float _targetProgress = 0.0f;

    onStartCallback _onStart = nullptr;
    onUpdateCallback _onUpdate = nullptr;
    onStopCallback _onStop = nullptr;
};

NS_SP_END

#endif /* CLASSES_MATERIAL_RESOURCES_MATERIALPROGRESSACTION_H_ */
