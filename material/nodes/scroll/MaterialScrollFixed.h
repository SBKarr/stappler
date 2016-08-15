/*
 * MaterialScrollFixed.h
 *
 *  Created on: 24 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLFIXED_H_
#define LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLFIXED_H_

#include "MaterialScroll.h"

NS_MD_BEGIN

class ScrollHandlerFixed : public Scroll::Handler {
public:
	virtual ~ScrollHandlerFixed() { }
	virtual bool init(Scroll *, float);
	virtual ItemMap run(Request t, DataMap &&) override;

protected:
	float _dataSize = 0.0f;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCROLL_MATERIALSCROLLFIXED_H_ */
