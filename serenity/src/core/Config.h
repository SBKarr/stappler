/*
 * SAConfig.h
 *
 *  Created on: 15 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_CORE_CONFIG_H_
#define SERENITY_SRC_CORE_CONFIG_H_

#include "Forward.h"

NS_SA_EXT_BEGIN(config)

constexpr auto getHeartbeatPause() { return 1_sec; }
constexpr auto getHeartbeatTime() { return 2_sec; }

constexpr auto getMaxAuthTime() { return 720_sec; }
constexpr auto getMaxLoginFailure() { return 3; }

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

// MTU configured to use local loopback interface
// for network, you should use smaller value, like 1472 (1464 for PPPoE)
constexpr auto getBroadcastProtocolMtu() { return 8_KiB; }
constexpr auto getBroadcastSocketAddr() { return "127.0.21.255"; }
constexpr uint16_t getBroadcastSocketPort() { return 21050; }

constexpr auto getServerToolsPrefix() { return "/__server"; }
constexpr auto getServerToolsShell() { return "/shell"; }
constexpr auto getServerToolsAuth() { return "/auth/"; }
constexpr auto getServerVirtualFilesystem() { return "/virtual/"; }

constexpr auto getSerenityErrorNotificatorName() { return "Serenity.ErrorNotificator"; }
constexpr auto getSerenityDBDHandleName() { return "Serenity.DBD.Handle"; }
constexpr auto getSerenityWebsocketDatabaseName() { return "Serenity.Websocket.Database"; }
constexpr auto getSerenityWebsocketHandleName() { return "Serenity.Websocket.Handle"; }

inline TimeInterval getKeyValueStorageTime() { return TimeInterval::seconds(60 * 60 * 24 * 365); } // one year

NS_SA_EXT_END(config)

#endif /* SERENITY_SRC_CORE_CONFIG_H_ */
