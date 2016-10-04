/*
 * SPIconSprite.h
 *
 *  Created on: 08 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_BATCHED_SPICONSPRITE_H_
#define LIBS_STAPPLER_NODES_BATCHED_SPICONSPRITE_H_

#include "SPDynamicSprite.h"
#include "SPIcon.h"

NS_SP_BEGIN

class IconSprite : public DynamicSprite {
public:
	virtual ~IconSprite() { }

	virtual bool init();
	virtual bool init(const Icon &icon);

	virtual void setIcon(const Icon &icon);
	virtual const Icon &getIcon() const;

protected:
	Icon _icon;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_BATCHED_SPICONSPRITE_H_ */
