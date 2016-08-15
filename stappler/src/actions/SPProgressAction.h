/*
 * MaterialProgress.h
 *
 *  Created on: 18 дек. 2014 г.
 *      Author: sbkarr
 */

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

    void setSourceProgress(float progress);
    float getSourceProgress();

    void setTargetProgress(float progress);
    float getTargetProgress();

    void setStartCallback(const onStartCallback &cb);
    void setUpdateCallback(const onUpdateCallback &cb);
    void setStopCallback(const onStopCallback &cb);
protected:
    float _sourceProgress = 0.0f;
    float _targetProgress = 0.0f;

    onStartCallback _onStart = nullptr;
    onUpdateCallback _onUpdate = nullptr;
    onStopCallback _onStop = nullptr;
};

NS_SP_END

#endif /* CLASSES_MATERIAL_RESOURCES_MATERIALPROGRESSACTION_H_ */
