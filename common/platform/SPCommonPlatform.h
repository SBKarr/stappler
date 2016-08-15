/*
 * SPCommonPlatform.h
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_PLATFORM_SPCOMMONPLATFORM_H_
#define COMMON_PLATFORM_SPCOMMONPLATFORM_H_

/* Do NOT include this header into public headers! */

#include "SPFilesystem.h"

#define NS_SP_PLATFORM_BEGIN		namespace stappler { namespace platform {
#define NS_SP_PLATFORM_END		} }

/* Platform-depended functions interface */
NS_SP_PLATFORM_BEGIN

namespace filesystem {
	String _getWritablePath();
	String _getDocumentsPath();
	String _getCachesPath();

	bool _exists(const String &); // check if file exists in application bundle filesystem
	size_t _size(const String &);

	stappler::filesystem::ifile _openForReading(const String &);
	size_t _read(void *, uint8_t *buf, size_t nbytes);
	size_t _seek(void *, int64_t offset, io::Seek s);
	size_t _tell(void *);
	bool _eof(void *);
	void _close(void *);
}

NS_SP_PLATFORM_END

#endif /* COMMON_PLATFORM_SPCOMMONPLATFORM_H_ */
