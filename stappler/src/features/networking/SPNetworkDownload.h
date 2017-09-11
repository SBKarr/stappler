/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDOWNLOADTASK_H_
#define STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDOWNLOADTASK_H_

#include "SPNetworkTask.h"

NS_SP_BEGIN

class NetworkDownload : public NetworkTask {
public:
	using StartedCallback = Function<void(const NetworkDownload *download)>;
	using ProgressCallback = Function<void(const NetworkDownload *download, float progress)>;
	using CompletedCallback = Function<void(const NetworkDownload *download, bool success)>;

    NetworkDownload();
    
    virtual bool init(const String &url, const String &fileName);
    
    virtual bool execute() override;
    virtual bool performQuery() override;
    virtual void onComplete() override;

    virtual void notifyOnStarted(bool bind = false);
    virtual void notifyOnProgress(float progress, bool bind = false);
    virtual void notifyOnComplete(bool success, bool bind = false);

	virtual void setStartedCallback(StartedCallback func);
	virtual void setProgressCallback(ProgressCallback func);
	virtual void setCompletedCallback(CompletedCallback func);

protected:
	StartedCallback _onStarted;
	ProgressCallback _onProgress;
	CompletedCallback _onCompleted;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDOWNLOADTASK_H_ */
