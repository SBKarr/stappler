/*
 * MaterialFormController.h
 *
 *  Created on: 22 янв. 2017 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_FORM_MATERIALFORMCONTROLLER_H_
#define MATERIAL_NODES_FORM_MATERIALFORMCONTROLLER_H_

#include "Material.h"

NS_MD_BEGIN

class FormController : public cocos2d::Component {
public:
	static EventHeader onForceCollect;
	static EventHeader onForceUpdate;

	data::Value getValue(const String &);
	void setValue(const String &, const data::Value &);

protected:
	Map<String, data::Value> _data;
};

NS_MD_END

#endif /* MATERIAL_NODES_FORM_MATERIALFORMCONTROLLER_H_ */
