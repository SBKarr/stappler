//
//  SPSignalHandler.m
//  stappler
//
//  Created by SBKarr on 10.10.16.
//  Copyright © 2016 SBKarr. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPNetworkHandle.h"
#include <libkern/OSAtomic.h>
#include <execinfo.h>
#include <cxxabi.h>

#include <sstream>

NS_SP_EXTERN_BEGIN

stappler::data::Value s_applicationData;
std::string s_cachePath;

struct sigaction OldAbrt;
struct sigaction OldIll;
struct sigaction OldSegv;
struct sigaction OldBus;

struct sigaction NewAbrt;
struct sigaction NewIll;
struct sigaction NewSegv;
struct sigaction NewBus;

void rethrowSignal(int sig, siginfo_t *info, void *ptr, struct sigaction *old) {
	if (old->sa_flags & SA_SIGINFO) {
		if (old->sa_sigaction) {
			old->sa_sigaction(sig, info, ptr);
		}
	} else if (old->sa_handler != SIG_DFL && old->sa_handler != SIG_IGN) {
		old->sa_handler(sig);
	} else if (old->sa_handler == SIG_DFL) {
		raise(sig);
	}
}

void saveBacktrace(siginfo_t *info) {
	char buf[1_KiB] = { 0 };
	strcpy(buf, s_cachePath.c_str());
	if (s_cachePath.back() == '/') {
		strcat(buf, "exception.json");
	} else {
		strcat(buf, "/exception.json");
	}

	FILE *f = fopen(buf, "w");
	fprintf(f, "{\"signo\":%d,\"errno\":%d,\"code\":%d,\"status\":%d,\"backtrace\":\"",
			info->si_signo, info->si_errno, info->si_code, info->si_status);

	void* bt[128] = { 0 };
	int bt_size = backtrace(bt, 128);
	char **bt_syms = backtrace_symbols(bt, bt_size);
	for (int i = 0; i < bt_size; i++) {
		fprintf(f, "%s\n", bt_syms[i]);
	}

	fwrite("\"}", 2, 1, f);
	fclose(f);

	free(bt_syms);
}

void SignalHandler(int sig, siginfo_t *info, void *ptr) {
	saveBacktrace(info);

	sigaction(SIGABRT, &OldAbrt, nullptr);
	sigaction(SIGILL, &OldIll, nullptr);
	sigaction(SIGSEGV, &OldSegv, nullptr);
	sigaction(SIGBUS, &OldBus, nullptr);

	switch (sig) {
		case SIGABRT: rethrowSignal(sig, info, ptr, &OldAbrt); break;
		case SIGILL: rethrowSignal(sig, info, ptr, &OldIll); break;
		case SIGSEGV: rethrowSignal(sig, info, ptr, &OldSegv); break;
		case SIGBUS: rethrowSignal(sig, info, ptr, &OldBus); break;
		default: break;
	 }

	abort();
}

void setSignalHandler() {
	NewAbrt.sa_sigaction = &SignalHandler;
	NewAbrt.sa_flags = SA_SIGINFO;

	NewIll.sa_sigaction = &SignalHandler;
	NewIll.sa_flags = SA_SIGINFO;

	NewSegv.sa_sigaction = &SignalHandler;
	NewSegv.sa_flags = SA_SIGINFO;

	NewBus.sa_sigaction = &SignalHandler;
	NewBus.sa_flags = SA_SIGINFO;

	sigaction(SIGABRT, &NewAbrt, &OldAbrt);
	sigaction(SIGILL, &NewIll, &OldIll);
	sigaction(SIGSEGV, &NewSegv, &OldSegv);
	sigaction(SIGBUS, &NewBus, &OldBus);
}

NS_SP_EXTERN_END

NS_SP_PLATFORM_BEGIN

namespace debug {
	data::Value getExceptionData(const String &path) {
		data::Value d(s_applicationData);
		d.setValue(data::readFile(path), "info");
		return d;
	}
	void sendFile(const String &addr, const String &path, data::EncodeFormat fmt) {
		NetworkHandle h;
		h.init(NetworkHandle::Method::Post, addr);
		h.setSendData(getExceptionData(path), fmt);
		h.perform();
	}

	void _init(const String &addr, data::EncodeFormat fmt) {
		s_cachePath = filesystem::_getCachesPath();
		s_applicationData.setString(device::_userAgent(), "agent");
		s_applicationData.setString([[UIDevice currentDevice] systemVersion].UTF8String, "os");
		s_applicationData.setString([[UIDevice currentDevice] model].UTF8String, "model");
		s_applicationData.setString(device::_applicationName(), "application");
		s_applicationData.setString(device::_applicationVersion(), "version");
		s_applicationData.setString(device::_bundleName(), "bundle");
		s_applicationData.setString(device::_userLanguage(), "language");
		s_applicationData.setString(device::_deviceIdentifier(), "identifier");
		s_applicationData.setString([NSDateFormatter localizedStringFromDate:[NSDate date] 
                                                      dateStyle:NSDateFormatterShortStyle 
                                                      timeStyle:NSDateFormatterFullStyle].UTF8String, "date");

		auto path = filepath::merge(stappler::filesystem::cachesPath(), "exception.json");
		if (stappler::filesystem::exists(path)) {
			if (!addr.empty()) {
				sendFile(addr, path, fmt);
			} else {
				log::text("ExceptionInfo", data::toString(getExceptionData(path), true));
			}
			stappler::filesystem::remove(path);
		}

		setSignalHandler();
	}
	void _backtrace(StringStream &stream) {
		void* bt[128] = { 0 };
		int bt_size = backtrace(bt, 128);
		char **bt_syms = backtrace_symbols(bt, bt_size);
		stream << "\n";
		for (int i = 2; i < bt_size; i++) {
			stream << bt_syms[i] << "\n";
		}
		free(bt_syms);
	}
}

NS_SP_PLATFORM_END
