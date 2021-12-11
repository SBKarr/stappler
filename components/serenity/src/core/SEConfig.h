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

#ifndef SERENITY_SRC_CORE_CONFIG_H_
#define SERENITY_SRC_CORE_CONFIG_H_

#include "SPCommon.h"
#include "SPTime.h"

#define NS_SA_EXT_BEGIN(v)		NS_SP_EXT_BEGIN(serenity) namespace v {
#define NS_SA_EXT_END(v)		NS_SP_EXT_END(serenity) }
#define USING_NS_SA_EXT(v)		using namespace stappler::serenity::v

NS_SA_EXT_BEGIN(config)

constexpr auto getHeartbeatPause() { return 1_sec; }
constexpr auto getHeartbeatTime() { return 1_sec; }

constexpr auto getMaxAuthTime() { return 720_sec; }
constexpr auto getMaxLoginFailure() { return 4; }

constexpr size_t getMaxVarSize() { return 255; }
constexpr size_t getMaxRequestSize() { return 0; }
constexpr size_t getMaxFileSize() { return 0; }

constexpr size_t getMaxInputPostSize() { return 2_GiB; }
constexpr size_t getMaxInputFileSize() { return 2_GiB; }
constexpr size_t getMaxInputVarSize() { return 8_KiB; }

constexpr size_t getMaxExtraFieldSize() { return 8_KiB; }

constexpr auto getInputUpdateTime() { return 1_sec; }
constexpr auto getInputUpdateFrequency() { return 0.1f; }

constexpr auto getUploadTmpFilePrefix() { return "sa.upload"; }
constexpr auto getUploadTmpImagePrefix() { return "sa.image"; }
constexpr auto getUploadTmpFileBuffer() { return 8_KiB; }
constexpr auto getUploadUseBuffer() { return true; }

constexpr auto getDefaultStreamBlock() { return 1_KiB; }
constexpr auto getDefaultSessionName() { return "SID"; }
constexpr auto getDefaultSessionKey() { return "SerenitySession"; }

constexpr auto getStorageBroadcastChannelName() { return "serenity_broadcast"; }

inline auto getDefaultPasswordSalt() { return "SAUserPasswordKey"_weak; }

constexpr auto getDefaultRetryTime() { return 100_msec; }

constexpr auto getDefaultTextMin() { return 3; }
constexpr auto getDefaultTextMax() { return 256; }

constexpr auto getDefaultWebsocketTtl() { return 60_sec; }
constexpr auto getDefaultWebsocketMax() { return 1_KiB; }

constexpr auto getWebsocketBufferSlots() -> size_t { return 16; }
constexpr auto getWebsocketMaxBufferSlotSize() -> size_t { return 8_KiB; }

#if DEBUG
constexpr auto getDefaultPugTemplateUpdateInterval() { return 3_sec; }
constexpr auto getDefaultDatabaseCleanupInterval() { return 60_sec; }
#else
constexpr auto getDefaultPugTemplateUpdateInterval() { return 30_sec; }
constexpr auto getDefaultDatabaseCleanupInterval() { return 180_sec; }
#endif

constexpr auto getServerToolsPrefix() { return "/__server"; }
constexpr auto getServerToolsShell() { return "/shell"; }
constexpr auto getServerToolsErrors() { return "/errors"; }
constexpr auto getServerToolsAuth() { return "/auth/"; }
constexpr auto getServerVirtualFilesystem() { return "/virtual/"; }

constexpr auto getSerenityErrorNotificatorName() { return "Serenity.ErrorNotificator"; }
constexpr auto getSerenityDBDHandleName() { return "Serenity.DBD.Handle"; }

constexpr auto getStorageInterfaceKey() { return "ST.StorageInterface"; }
constexpr auto getTransactionPrefixKey() { return "ST.Tr."; } // limit for 6 chars to use with SOO opts (6 + 16 < 23)
constexpr auto getTransactionStackKey() { return "ST.Transaction.Stack"; }

constexpr uint16_t getResourceResolverMaxDepth() { return 4; }

inline TimeInterval getKeyValueStorageTime() { return TimeInterval::seconds(60 * 60 * 24 * 365); } // one year
inline TimeInterval getInternalsStorageTime() { return TimeInterval::seconds(60 * 60 * 24 * 30); }

constexpr int getMaxDatabaseConnections() { return 1024; }

// this key used only with custom SerenityServerKey to protect database entry with real generated private key
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

NS_SA_EXT_END(config)

#endif /* SERENITY_SRC_CORE_CONFIG_H_ */
