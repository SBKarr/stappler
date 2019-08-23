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

#include "SPMemUuid.h"
#include "SPString.h"
#include "SPTime.h"

#if SPAPR
#include "apr_uuid.h"
#endif

NS_SP_EXT_BEGIN(memory)

#if SPAPR

bool uuid::parse(uuid_t &buf, const char *str) {
	return apr_uuid_parse((apr_uuid_t *)buf.data(), str) == APR_SUCCESS;
}

void uuid::format(char *buf, const uuid_t &tmp) {
	apr_uuid_format(buf, (const apr_uuid_t *)tmp.data());
}

uuid uuid::generate() {
	uuid_t tmp;
	apr_uuid_get((apr_uuid_t*)tmp.data());
	return uuid(tmp);
}

#else

struct UuidState {
	UuidState() {
	    struct {
		/* Add thread id here, if applicable, when we get to pthread or apr */
	        pid_t pid;
	        size_t threadId;
	        struct timeval t;
	        char hostname[257];
	    } r;

	    r.pid = getpid();
	    gettimeofday(&r.t, (struct timezone *)0);

	    std::hash<std::thread::id> hasher;
	    r.threadId = hasher(std::this_thread::get_id());

	    gethostname(r.hostname, 256);
	    node = stappler::string::Sha256().update(CoderSource((const uint8_t *)&r, sizeof(r))).final();
	}

	int seqnum = 0;
	stappler::string::Sha256::Buf node;
};

static thread_local UuidState tl_uuidState;

static uint64_t getCurrentTime() {
	// time magic to convert from epoch to UUID UTC
	uint64_t time_now = (Time::now().toMicros() * 10) + 0x01B21DD213814000ULL;

	thread_local uint64_t time_last = 0;
	thread_local uint64_t fudge = 0;

	if (time_last != time_now) {
		if (time_last + fudge > time_now) {
			fudge = time_last + fudge - time_now + 1;
		} else {
			fudge = 0;
		}
		time_last = time_now;
	} else {
		++fudge;
	}

	return time_now + fudge;
}

uuid uuid::generate() {
	uuid_t d;
	uint64_t timestamp = getCurrentTime();

	/* time_low, uint32 */
	d[3] = (unsigned char)timestamp;
	d[2] = (unsigned char)(timestamp >> 8);
	d[1] = (unsigned char)(timestamp >> 16);
	d[0] = (unsigned char)(timestamp >> 24);
	/* time_mid, uint16 */
	d[5] = (unsigned char)(timestamp >> 32);
	d[4] = (unsigned char)(timestamp >> 40);
	/* time_hi_and_version, uint16 */
	d[7] = (unsigned char)(timestamp >> 48);
	d[6] = (unsigned char)(((timestamp >> 56) & 0x0F) | 0x50);
	/* clock_seq_hi_and_reserved, uint8 */
	d[8] = (unsigned char)(((tl_uuidState.seqnum >> 8) & 0x3F) | 0x80);
	/* clock_seq_low, uint8 */
	d[9] = (unsigned char)tl_uuidState.seqnum;
	/* node, byte[6] */
	memcpy(&d[10], tl_uuidState.node.data(), 6);

	return uuid(d);
}

void uuid::format(char *buf, const uuid_t &d) {
	sprintf(buf, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
}

static uint8_t parse_hexpair(const char *s) {
	int result;
	int temp;

	result = s[0] - '0';
	if (result > 48) {
		result = (result - 39) << 4;
	} else if (result > 16) {
		result = (result - 7) << 4;
	} else {
		result = result << 4;
	}

	temp = s[1] - '0';
	if (temp > 48) {
		result |= temp - 39;
	} else if (temp > 16) {
		result |= temp - 7;
	} else {
		result |= temp;
	}

	return (uint8_t)result;
}

bool uuid::parse(uuid_t &d, const char *uuid_str) {
	int i;

	for (i = 0; i < 36; ++i) {
		char c = uuid_str[i];
		if (!isxdigit(c) && !(c == '-' && (i == 8 || i == 13 || i == 18 || i == 23))) {
			return false;
		}
	}

	if (uuid_str[36] != '\0') {
		return false;
	}

	d[0] = base16::hexToChar(uuid_str[0], uuid_str[1]);
	d[1] = base16::hexToChar(uuid_str[2], uuid_str[3]);
	d[2] = base16::hexToChar(uuid_str[4], uuid_str[5]);
	d[3] = base16::hexToChar(uuid_str[6], uuid_str[7]);

	d[4] = base16::hexToChar(uuid_str[9], uuid_str[10]);
	d[5] = base16::hexToChar(uuid_str[11], uuid_str[12]);

	d[6] = base16::hexToChar(uuid_str[14], uuid_str[15]);
	d[7] = base16::hexToChar(uuid_str[16], uuid_str[17]);

	d[8] = base16::hexToChar(uuid_str[19], uuid_str[20]);
	d[9] = base16::hexToChar(uuid_str[21], uuid_str[22]);

	for (i = 6; i--;) {
		d[10 + i] = parse_hexpair(&uuid_str[i*2+24]);
	}

	return true;
}

#endif

NS_SP_EXT_END(memory)
