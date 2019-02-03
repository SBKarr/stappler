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

#include "Forward.h"

NS_SA_EXT_BEGIN(config)

constexpr auto getHeartbeatPause() { return 1_sec; }
constexpr auto getHeartbeatTime() { return 2_sec; }

constexpr auto getMaxAuthTime() { return 720_sec; }
constexpr auto getMaxLoginFailure() { return 4; }

constexpr size_t getMaxVarSize() { return 255; }
constexpr size_t getMaxRequestSize() { return 0; }
constexpr size_t getMaxFileSize() { return 0; }

constexpr size_t getMaxInputPostSize() { return 250_MiB; }
constexpr size_t getMaxInputFileSize() { return 250_MiB; }
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

inline auto getDefaultPasswordSalt() { return "SAUserPasswordKey"_weak; }
inline auto getInternalPasswordKey() { return "Serenity Password Salt"_weak; }

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

// MTU configured to use local loopback interface
// for network, you should use smaller value, like 1472 (1464 for PPPoE)
// constexpr auto getBroadcastProtocolMtu() { return 8_KiB; }
// constexpr auto getBroadcastSocketAddr() { return "127.0.21.255"; }
// constexpr uint16_t getBroadcastSocketPort() { return 21050; }

constexpr auto getServerToolsPrefix() { return "/__server"; }
constexpr auto getServerToolsShell() { return "/shell"; }
constexpr auto getServerToolsErrors() { return "/errors"; }
constexpr auto getServerToolsAuth() { return "/auth/"; }
constexpr auto getServerVirtualFilesystem() { return "/virtual/"; }

constexpr auto getSerenityErrorNotificatorName() { return "Serenity.ErrorNotificator"; }
constexpr auto getSerenityDBDHandleName() { return "Serenity.DBD.Handle"; }
constexpr auto getSerenityWebsocketDatabaseName() { return "Serenity.Websocket.Database"; }
constexpr auto getSerenityWebsocketHandleName() { return "Serenity.Websocket.Handle"; }

constexpr uint16_t getResourceResolverMaxDepth() { return 4; }

inline TimeInterval getKeyValueStorageTime() { return TimeInterval::seconds(60 * 60 * 24 * 365); } // one year

NS_SA_EXT_END(config)

#endif /* SERENITY_SRC_CORE_CONFIG_H_ */
