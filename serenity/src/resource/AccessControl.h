/*
 * AccessControl.h
 *
 *  Created on: 23 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_RESOURCE_ACCESSCONTROL_H_
#define SERENITY_SRC_RESOURCE_ACCESSCONTROL_H_

#include "InputFile.h"

NS_SA_BEGIN

/* Serenity resource access control concept:
 *
 * Every action with object is classified asЖ
 *  - Create - object creation (method POST)
 *  - Append - appending subobject into array or reference set (method POST)
 *  - Read - write object's content in reqponse of request (applicable for
 *      any operation, that returns object's content)
 *  - Update - change values of object's fields
 *  - Remove - delete object completely from storage
 *
 * Access control implemented with 3 levels:
 * 1) Access rules list - you can define access permissions for scheme with
 *    list of action/permission pair
 *
 *    - Full: action is allowed, no other checks performed
 *    - Partial - action is allowed, but can be denied by lower level check
 *    - Restrict - action is denied, no other checks performed
 *
 * 2) Scheme-level callback - you can provide callback, that checks permission
 *    for scheme and user
 *
 * 3) Object-level callback - you can provide callback, that checks permission
 *    and modify object for returning or patch for update, this callback
 *    returns boolean, instead of Permission enum.
 *    - false: action is denied
 *    - true: action is allowed
 */

class AccessControl {
public:
	using Scheme = storage::Scheme;
	using Field = storage::Field;

	enum Action : uint8_t {
		Create,
		Append,
		Read,
		Update,
		Remove,
	};

	enum Permission : uint8_t {
		Restrict, // action is not allowed
		Partial, // action can be allowed by lower-level (same as Full for lowest level)
		Full, // action is allowed without lower-level check
	};

	struct List {
		static List permissive();
		static List common(Permission def = Permission::Partial);
		static List restrictive();

		Permission create = Partial;
		Permission read = Partial;
		Permission append = Partial;
		Permission update = Partial;
		Permission remove = Partial;
	};

	using SchemeFn = Function<Permission (User *, Scheme *, Action)>;
	using ObjectFn = Function<bool (User *, Scheme *, Action, data::Value &, data::Value &, apr::array<InputFile> &)>;

	AccessControl(const SchemeFn & = nullptr, const ObjectFn & = nullptr);

	bool isAdminPrivileges() const;
	void setAdminPrivileges(bool);

	void setDefaultList(const List & l);
	void setList(Scheme *, const List & l);

	void setDefaultSchemeCallback(const SchemeFn &);
	void setSchemeCallback(Scheme *, const SchemeFn &);

	void setDefaultObjectCallback(const ObjectFn &);
	void setObjectCallback(Scheme *, const ObjectFn &);

	Permission onScheme(User *, Scheme *, Action) const;
	bool onObject(User *, Scheme *, Action, data::Value &, data::Value &, apr::array<InputFile> &) const;

protected:
	bool useAdminPrivileges(User *) const;

	bool _adminPrivileges = true;
	List _default;
	Map<Scheme *, List> _lists;

	SchemeFn _scheme = nullptr;
	Map<Scheme *, SchemeFn> _schemes;

	ObjectFn _object = nullptr;
	Map<Scheme *, ObjectFn> _objects;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_ACCESSCONTROL_H_ */
