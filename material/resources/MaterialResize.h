/*
 * MaterialResize.h
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_RESOURCES_MATERIALRESIZE_H_
#define LIBS_MATERIAL_RESOURCES_MATERIALRESIZE_H_

#include "Material.h"

#include "2d/CCActionInterval.h"

NS_MD_BEGIN

class ResizeTo : public cocos2d::ActionInterval {
public:
    virtual bool init(float duration, const cocos2d::Size &size);

    virtual void startWithTarget(cocos2d::Node *t) override;
    virtual void update(float time) override;

protected:
    cocos2d::Size _sourceSize;
    cocos2d::Size _targetSize;
};

class ResizeBy : public cocos2d::ActionInterval {
public:
    static ResizeBy* create(float duration, const cocos2d::Size &size);

    virtual bool init(float duration, const cocos2d::Size &size);

    virtual void startWithTarget(cocos2d::Node *t) override;
    virtual void update(float time) override;

protected:
    cocos2d::Size _sourceSize;
    cocos2d::Size _additionalSize;
};

NS_MD_END

#endif /* LIBS_MATERIAL_RESOURCES_MATERIALRESIZE_H_ */
