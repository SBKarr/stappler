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

#ifndef COMMON_PLATFORM_SPCOMMONPLATFORM_H_
#define COMMON_PLATFORM_SPCOMMONPLATFORM_H_

/* Do NOT include this header into public headers! */

#include "SPFilesystem.h"
#include "SPThreadTaskQueue.h"

/* Platform-depended functions interface */
namespace stappler::platform {

namespace filesystem {
	String _getWritablePath();
	String _getDocumentsPath();
	String _getCachesPath();

	bool _exists(const StringView &); // check if file exists in application bundle filesystem
	size_t _size(const StringView &);

	stappler::filesystem::ifile _openForReading(const StringView &);
	size_t _read(void *, uint8_t *buf, size_t nbytes);
	size_t _seek(void *, int64_t offset, io::Seek s);
	size_t _tell(void *);
	bool _eof(void *);
	void _close(void *);
}

namespace proc {
	bool _isArmNeonSupported();
	void _workerThread(thread::ThreadHandlerInterface *tm);
}

}

#endif /* COMMON_PLATFORM_SPCOMMONPLATFORM_H_ */
