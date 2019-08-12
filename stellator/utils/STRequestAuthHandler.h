/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef STELLATOR_UTILS_STREQUESTAUTHHANDLER_H_
#define STELLATOR_UTILS_STREQUESTAUTHHANDLER_H_

#include "RequestHandler.h"
#include "SPDataWrapper.h"

NS_SA_BEGIN

/**
 * GET /do_login?name=<name>&passwd=<password> - login with name or email
 * result:
 * 	{
 * 		"user": <user_id>,
 * 		"activated": <is_user_activated>,
 * 		"session": <is_session_created>,
 * 	}
 *
 * POST /do_register - default registration process (overloadable)
 *  {
 * 		"email": <user_email>,
 * 		"password": <new_user_password>,
 * 		"givenName": <name>,
 * 		"familyName": <name>,
 * 		"middleName": <name>,
 * 		"captcha": <captcha_if_enabled>,
 * 	}
 * result:
 * 	{
 * 		"user": <user_data_if_success>,
 * 		"userExist": <bool_error_indicator>,
 * 		"emptyEmail": <bool_error_indicator>,
 * 		"emptyPassword": <bool_error_indicator>,
 * 		"emptyGivenName": <bool_error_indicator>,
 * 		"emptyFamilyName": <bool_error_indicator>,
 * 		"captcha": <bool_error_indicator>,
 * 	}
 *
 * GET /do_activate?code=<code> - activate user by activation code
 *
 */

class RequestAuthHandler : public DataHandler {
public:
	virtual bool isRequestPermitted(Request & rctx) override;
	virtual bool processDataHandler(Request &, data::Value &result, data::Value &input) override;
	virtual int onTranslateName(Request &rctx) override;

protected:
	// overload point to extract user role from user object
	virtual storage::AccessRoleId getUserRole(const data::Value &) const;

	// overload point to check if user is activated
	virtual bool isActivated(const data::Value &) const;

	// overload point to validate activation code with user object
	virtual bool validateActivationCode(const data::Value &, const BytesView &code);

	// overload point to extract users scheme
	virtual const storage::Scheme *acquireScheme(Request & rctx) const;

	// overload point to create custom Auth object
	virtual db::Auth acquireAuth(Request & rctx) const;

	// overload for custom captcha validation (if captcha is enabled and default registration process used)
	virtual bool validateCaptcha(const StringView &) const;

	// overload for custom user authorization
	virtual bool pushAuthUser(Request & rctx, int64_t userId) const;

	// overload to notify about user creation (to send activation email, etc)
	virtual void notifyCreatedUser(Request & rctx, const data::Value &) const;

	// overload to notify about user self-activation
	virtual void notifyActivatedUser(Request & rctx, const data::Value &) const;

	virtual User * tryUserLogin(Request &rctx, const StringView &name, const StringView &passwd);

	virtual bool processLogin(Request &rctx, data::Value &result);
	virtual bool processRegister(Request &rctx, data::Value &result);
	virtual bool processActivate(Request &rctx, data::Value &result);
	virtual bool processUpdate(Request &rctx, data::Value &input);

	const storage::Scheme *_userScheme = nullptr;
	storage::Transaction _storage = nullptr;
	bool _useCaptcha = false;
	bool _allowActivation = true;
};

NS_SA_END

#endif /* STELLATOR_UTILS_STREQUESTAUTHHANDLER_H_ */
