/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "ServerComponent.h"
#include "RequestHandler.h"
#include "Networking.h"
#include "Tools.h"

#include "VirtualGui.cc"

NS_SA_EXT_BEGIN(test)

class VirtualComponent : public ServerComponent {
public:
	VirtualComponent(Server &serv, const String &name, const data::Value &dict);
	virtual ~VirtualComponent() { }

	virtual void onChildInit(Server &) override;

protected:

};

VirtualComponent::VirtualComponent(Server &serv, const String &name, const data::Value &dict)
: ServerComponent(serv, name.empty()?"Virtual":name, dict) {

}

void VirtualComponent::onChildInit(Server &serv) {
	serv.addHandler("/docs/", SA_HANDLER(VirtualGui));
}

SP_EXTERN_C ServerComponent * CreateVirtualHandler(Server &serv, const String &name, const data::Value &dict) {
	return new VirtualComponent(serv, name, dict);
}

NS_SA_EXT_END(test)
