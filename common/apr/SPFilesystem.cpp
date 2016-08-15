/*
 * SPFilesystem.cpp
 *
 *  Created on: 16 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPCommonPlatform.h"

#if (SPAPR && LINUX)

#include "SPAprAllocator.h"
#include "SPFilesystem.h"

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	const char *getDocumentRoot() {
		auto server = apr::AllocStack::get().server();

		core_server_config *sconf = (core_server_config *)ap_get_core_module_config(server->module_config);
	    return sconf->ap_document_root;
	}

	String _getPlatformPath(const String &path) {
		if (filepath::isBundled(path)) {
			return filepath::merge(String::make_weak(getDocumentRoot()), path.substr("%PLATFORM%:"_len));
		}
		return filepath::merge(String::make_weak(getDocumentRoot()), path);
	}

	String _getWritablePath() {
		auto path = getDocumentRoot();
		stappler::filesystem::mkdir(path);
		return String::make_weak(path);
	}
	String _getDocumentsPath() {
		auto path = getDocumentRoot();
		stappler::filesystem::mkdir(path);
		return path;
	}
	String _getCachesPath() {
		auto path = filepath::merge( getDocumentRoot(), "/uploads" );
		stappler::filesystem::mkdir(path);
		return path;
	}

	bool _exists(const String &path) {
		if (path.empty() || path.front() == '/' || path.compare(0, 2, "..") == 0 || path.find("/..") != String::npos) {
			return false;
		}

		return access(_getPlatformPath(path).c_str(), F_OK) != -1;
	}

	size_t _size(const String &ipath) {
		auto path = _getPlatformPath(ipath);
		return stappler::filesystem::size(path);
	}

	stappler::filesystem::ifile _openForReading(const String &path) {
		return stappler::filesystem::openForReading(_getPlatformPath(path));
	}

	size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
	size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
	size_t _tell(void *) { return 0; }
	bool _eof(void *) { return true; }
	void _close(void *) { }
}

NS_SP_PLATFORM_END
#endif
