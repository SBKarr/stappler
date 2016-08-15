/*
 * User.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_REQUEST_USER_H_
#define SERENITY_SRC_REQUEST_USER_H_

#include "StorageObject.h"

NS_SA_BEGIN

class User : public storage::Object {
public:
	static User *create(storage::Adapter *, const String &name, const String &password);
	static User *setup(storage::Adapter *, const String &name, const String &password);
	static User *create(storage::Adapter *, data::Value &&);

	static User *get(storage::Adapter *, const String &name, const String &password);
	static User *get(storage::Adapter *, uint64_t oid);
	static bool remove(storage::Adapter *, const String &name, const String &password);

	User(data::Value &&, storage::Scheme *);

	bool validatePassword(const String &passwd) const;
	void setPassword(const String &passwd);

	bool isAdmin() const;

	const String & getName() const;
};

NS_SA_END

#endif /* SERENITY_SRC_REQUEST_USER_H_ */
