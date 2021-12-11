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

#ifndef STELLATOR_CORE_STCONFIG_H_
#define STELLATOR_CORE_STCONFIG_H_

#include "SPCommon.h"
#include "SPTime.h"

namespace stellator::config {

// Common config rules:
// Default limits used to cover only Proof-of-concept projects with fast prototyping
// on production, all limits and keys should be defined manually for better performance
// and security
//
// Absolute limits is a balance between covering most use cases and security weakness
// You should think carefully, as it's hard to redefine them in launched project

// Initial maximum size of variable field in application/x-www-form-urlencoded
// Overridden by requests's InputConfig
constexpr size_t getMaxVarSize() { return 255; }

// Initial maximum size of request body (0 - no requests with payload allowed by default)
// Overridden by requests's InputConfig
constexpr size_t getMaxRequestSize() { return 0; }

// Initial maximum size of uploaded file (0 - no file uploads allowed by default)
// Overridden by requests's InputConfig
constexpr size_t getMaxFileSize() { return 0; }

// Fixed temporal frequency to call onFilterUpdate when uploads is processed
// Overridden by requests's InputConfig
constexpr auto getInputUpdateTime() { return stappler::TimeInterval::seconds(1); }

// Relative to upload size frequency to call onFilterUpdate when uploads is processed (.0f = every 10%)
// Overridden by requests's InputConfig
constexpr auto getInputUpdateFrequency() { return 0.1f; }

// Absolute maximum time until user is deauthenticated with default auth mechanism
// User should call `update` within this time, or requests will be blocked
// If you want larger idle time - use ExternalSession
// 720 sec (12 min) historically optimal
constexpr auto getMaxAuthTime() { return stappler::TimeInterval::seconds(720); }

// Absolute maximum of failed login attempt until user will be blocked for `getMaxAuthTime`
// If user's login is blocked, only way to bypass - remove records from __login storage scheme
constexpr auto getMaxLoginFailure() { return 4; }

// Absolute maximum of request payload size for methods, that have payload (POST historically)
// Can not be overridden
constexpr size_t getMaxInputPostSize() { return 2_GiB; }

// Absolute maximum of uploaded file size
// Can not be overridden
constexpr size_t getMaxInputFileSize() { return 2_GiB; }

// Absolute maximum of application/x-www-form-urlencoded variable size
// Can not be overridden, use applicaton/json, applicaton/cbor for larger variables
constexpr size_t getMaxInputVarSize() { return 8_KiB; }

// Default maximum for Extra/Data scheme field types
// Used when scheme-specific limits is calculated
constexpr size_t getMaxExtraFieldSize() { return 8_KiB; }

// Default minimum for Text scheme field (in bytes)
// this values should require from application to define min/max values manually, as it more secure and optimal
constexpr auto getDefaultTextMin() { return 3; }

// Default maximum for Text scheme field (in bytes)
// this values should require from application to define min/max values manually, as it more secure and optimal
constexpr auto getDefaultTextMax() { return 256; }

// Default salt for Password scheme field
// Do not use default salt in systems, that require secure login
inline auto getDefaultPasswordSalt() { return "SAUserPasswordKey"_weak; }

// Default storage time for internal key/value storage, 1 year default
// No eternal values allowed
// Be careful, default-generated server keys guaranteed to be stored only for this period since generated
inline stappler::TimeInterval getKeyValueStorageTime() { return stappler::TimeInterval::seconds(60 * 60 * 24 * 365); }

// Default storage time for internal values, stored in db, like error messages and login history
// File-based crush reports and db update reports are eternal
inline stappler::TimeInterval getInternalsStorageTime() { return stappler::TimeInterval::seconds(60 * 60 * 24 * 30); }

// Internal key for in-memory storage to store current active db adapter
// No reason to be customized
constexpr auto getStorageInterfaceKey() { return "ST.StorageInterface"; }

// Internal key prefix for in-memory storage to store additional adapter data, if required
// No reason to be customized
constexpr auto getTransactionPrefixKey() { return "ST.Tr."; } // limit for 6 chars to use with SOO opts (6 + 16 < 23)

// Internal key prefix for in-memory storage to store additional adapter data, if required
// No reason to be customized
constexpr auto getTransactionStackKey() { return "ST.Transaction.Stack"; }

// Prefix for temporary file, used to store upload data
constexpr auto getUploadTmpFilePrefix() { return "sa.upload"; }

// Prefix for temporary file, used to store data for uploaded image rescaling
constexpr auto getUploadTmpImagePrefix() { return "sa.image"; }

// Absolute maximum for storage subobject resolution system
// No resolution will be performed if this depth is reach
// Resolver requires a lot of memory, so, larger values weaken security for OOM attach
constexpr uint16_t getResourceResolverMaxDepth() { return 4; }

// Default session name for cookies
constexpr auto getDefaultSessionName() { return "SID"; }

// Default security salt for session token
constexpr auto getDefaultSessionKey() { return "SerenitySession"; }

// Internal broadcast channel name, if DB supports external broadcasts (like PostgreSQL LISTEN)
constexpr auto getStorageBroadcastChannelName() { return "serenity_broadcast"; }

// Templates update interval, if inotify is not supported or limit is reached
#if DEBUG
constexpr auto getDefaultPugTemplateUpdateInterval() { return stappler::TimeInterval::seconds(3); }
constexpr auto getDefaultDatabaseCleanupInterval() { return stappler::TimeInterval::seconds(60); }
#else
constexpr auto getDefaultPugTemplateUpdateInterval() { return stappler::TimeInterval::seconds(30); }
constexpr auto getDefaultDatabaseCleanupInterval() { return stappler::TimeInterval::seconds(180); }
#endif

// Absolute maximum of opened db connections
// Actual requires maximum is a sum of request processing threads, websocket processing threads, background threads,
// and custom threads, so, it's significally lower then this value in most scenarios
// In Serenity, first three categories are different, but in Stellator it's single thread pool
constexpr int getMaxDatabaseConnections() { return 1024; }

// Internal private key, used to generate real server's private key, if it was not defined
// Generation process requires custom server secret, defined with SerenityServerKey in Serenity
// usually, we generate it with `base64 < /dev/urandom | tr -d 'O0Il1+/' | head -c 44; printf '\n'`
//
// It's useful for development, but production requires manually defined private and public key
// since generated key pair is temporary
static constexpr auto INTERNAL_PRIVATE_KEY =
R"PubKey(-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAwA+bb6EnVO9LKDg2X2Casr7FcnCaE6O98CV2FSn0+AezS8k3
sRzbOdLiHaA5zkvAVZFF1EQrjU2rgqqCm+uZAXvSBETxHUUSDN3Ivj9kS8SzYQuk
A0eidYmoTmIokqS86LF55kXjtJLPKH+gdMOgZfJpvIeF/2fHMJw660jikyGJ5ig4
8KNYkue3pToa2gykZv1BC6q8XTK+zMbg1ueW9WfJ+SIk84AoxHYlpMnim+ngF2E6
vTxScxOGIdeDQ9RSJ0GtsN8Nrt2eYhG2RkI4349n1H1ZQ+DmOPlxVliKijLvKnJo
PDMVDFenv7eDNgPVFSX/E+OkCAvFYtK7ebzdtQIDAQABAoIBAQCoNYMW83d1JdcX
NJQ6WGtknTxkjEYFaWVSzBxtUL/h8yyO9X43OmSucgnjlI7MJQAgcQlNbR8PtLS/
zgJx+Jea+wzm/FKIJhQ2/I9yQCbhTTcliYJt9PFOK/AiJkoOlQV2bumqSg+x+NpC
R/UKDsOORg9hNPigxg1of4wCwWTfIf3VjA23ZeVokbY6cG3SlK1mu428454wP2o6
5fwYEMrspa65RaSDU3lyC/NTgWmFmGHGx/3Ok0dw52yN3NjePlgS5/CyY4x1iwrs
Le2QeSl8xIEkiagLNM0CmpjID+7gbbHcFAQ5sNKDxo0UFMbYKL3i+W3JmxNDiSHa
EDdTTQgBAoGBAOeISj4JKS66M3qEwkga89wAlMT6OIWOi7egpNMYCIKgm5G/e0ox
YTEFwjH/LkpcuSUhAYJ5DaFs8VX8F9AGr3OtdcBC89MCL3UjEvnjjjqbxot2389i
RH/wXwYVC4Lt7FqUoz5sj4xIHUUqtclluI0i1KXirYMw4by8YU4vx/8BAoGBANRb
fGWOZKdAzCZzeesVyitfOSAXHQNCFxTBpJ9rsh4J9Gz3Rm/2MCUCDYx6h2vvlAhQ
HKMxweRgZt3sq9vCISOVbBvW7RAe1X87J9UOlSgHgtQ1P3/jCM4wPCvSICg2SfEo
5P1YaDkdmnpw0Xj1t03aXHo1yJyIpab6VkSI55K1AoGATOnml+MdLium7D1r+N4T
QnNi+GiTHDL1UQPpnUJvmU1XQLyWbVgoDEv4bflyXDufOalUQg6Kq6RwK3s6Qd3m
rQvjgQH804z3TPdg12hzmB6lfzD3OoJPdRzZxEB7eXwmNxUHgbY4nYZbSt3cU9IZ
07DNaWn68AjuBG+j94BB9QECgYAe7nx+JnGO0ydpamSV04SxMJuXiwZU1SmbgmFC
P2OgcH7D6HjAEjINEfF7RtW26Ry84T5qnvLJGymgpbqatfoxvrASlgnN0U/zymAQ
7qDXRdDOrDrlm+JKdkgtcdvxP4chs303UctMln1L3GcGoXPjySyzOGZSNq06Vzh9
nxtsCQKBgQDcPy1R2BFvT4GE3AcSTYfeGkSrPY+09KCeFAIsjrpTZRdhbyD2ZyYa
iqR1BRzqFGXzVlTVm/B/yTekDjkjn6JoocUoWed6xourrpDJt/UicL6cKx7c1j4/
M1AppdVrnIu/Wfx8CKDqzXgl1u1PvHDnVMv1+nW0MlHPAi48mArmlQ==
-----END RSA PRIVATE KEY-----
)PubKey";

}

#endif /* STELLATOR_CORE_STCONFIG_H_ */
