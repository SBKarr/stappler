/*
 * StorageFile.h
 *
 *  Created on: 17 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_STORAGE_STORAGEFILE_H_
#define SERENITY_SRC_STORAGE_STORAGEFILE_H_

#include "StorageObject.h"

NS_SA_EXT_BEGIN(storage)

class File : public Object {
public:
	static String getFilesystemPath(uint64_t oid);
	static bool validateFileField(const Field &, const InputFile &);

	static data::Value createFile(Adapter *adapter, const Field &, InputFile &);
	static data::Value createFile(Adapter *adapter, const String &type, const String &path);
	static data::Value createImage(Adapter *adapter, const Field &, InputFile &);

	static data::Value getData(Adapter *adapter, uint64_t id);
	static void setData(Adapter *adapter, uint64_t id, const data::Value &);

	// remove file from filesystem
	static bool removeFile(Adapter *adapter, const Field &, const data::Value &);

	// remove file from storage and filesystem
	static bool purgeFile(Adapter *adapter, const Field &, const data::Value &);

	static Scheme *getScheme();
protected:
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEFILE_H_ */
