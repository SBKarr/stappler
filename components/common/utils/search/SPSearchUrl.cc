/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPSearchConfiguration.h"

namespace stappler::search {

using Scheme =  chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>, chars::Chars<char, '+', '-', '.'>>;
using Ipv6 =  chars::Compose<char, chars::CharGroup<char, CharGroupId::Hexadecimial>, chars::Chars<char, ':'>>;

using Unreserved = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>, chars::Chars<char, '-', '.', '_', '~', '%'>>;
using SubDelim = chars::Chars<char, '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='>;
using GenDelim = chars::Chars<char, ':', '/', '?', '#', '[', ']', '@'>;

using UnreservedUni = chars::Compose<char, Unreserved, chars::UniChar>;

static bool validateScheme(const StringView &r) {
	auto cpy = r;
	if (cpy.is<StringView::Compose<StringView::CharGroup<CharGroupId::Alphanumeric>, StringView::Chars<'.'>>>()) {
		cpy ++;
		cpy.skipChars<Scheme>();
		if (cpy.empty()) {
			return true;
		}
	}
	return false;
}

static bool validateHost(StringView &r) {
	if (r.empty()) {
		return false;
	}
	auto cpy = r;
	if (cpy.is('[')) {
		// check ipv6
		++ cpy;
		cpy.skipChars<Ipv6>();
		if (cpy.is(']')) {
			cpy ++;
			if (cpy.empty()) {
				return true;
			}
		}
	} else {
		cpy.skipChars<Unreserved, SubDelim, chars::UniChar>();
		if (cpy.empty()) {
			auto c = r.sub(r.size() - 1, 1);
			while (c.is(',') || c.is('.') || c.is(';')) {
				r = r.sub(0, r.size() - 1);
				c = r.sub(r.size() - 1, 1);
			}
			if (c.is<StringView::Compose<StringView::CharGroup<CharGroupId::Alphanumeric>, chars::UniChar>>()) {
				StringView h(r);
				h.readUntil<chars::UniChar>();
				if (!h.empty()) {
					auto h = r;
					auto domain = h.backwardReadUntil<StringView::Chars<'.'>>();
					if (h.back() == '.' && !domain.empty()) {
						auto tmp = domain;
						tmp.skipChars<StringView::CharGroup<CharGroupId::Alphanumeric>>();
						if (!tmp.empty()) {
							if (UrlView::isValidIdnTld(domain)) {
								return true;
							}
						}
					}
				} else {
					return true;
				}
			}
		}
	}
	return false;
}

static bool validateUserOrPassword(const StringView &r) {
	auto cpy = r;
	cpy.skipChars<Unreserved, SubDelim, chars::UniChar>();
	if (cpy.empty()) {
		return true;
	}
	return false;
}

bool parseUrl(StringView &s, const Callback<void(StringViewUtf8, UrlToken)> &cb) {
	UrlToken state = UrlToken::Scheme;

	StringView tmp;
	if (s.is('[')) {
		state = UrlToken::Host;
	} else if (s.is("mailto:")) {
		cb(StringView(s, 6), UrlToken::Scheme);
		s += 6;
		cb(StringView(s, 1), UrlToken::Blank);
		++ s;
		state = UrlToken::User;
	} else {
		tmp = s.readChars<UnreservedUni>();
	}

	if (state == UrlToken::Scheme) {
		if (s.is(':')) {
			// scheme or host+port
			if (tmp.empty()) {
				return false; // error
			}

			if (s.is("://")) {
				if (!validateScheme(tmp)) {
					return false;
				}

				cb(tmp, UrlToken::Scheme);
				cb(StringView(s, 3), UrlToken::Blank);
				s += 3;

				if (s.is('[')) {
					state = UrlToken::Host;
				} else {
					state = UrlToken::User;
				}
			} else {
				// if it's port, next chars will be numbers only
				auto tmpS = s;
				tmpS ++;
				auto port = tmpS.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
				if (!port.empty() && !tmpS.is<UnreservedUni>() && !tmpS.is('@')) {
					// host + port
					if (!validateHost(tmp)) {
						return true;
					}

					cb(tmp, UrlToken::Host);
					cb(StringView(port.data() - 1, 1), UrlToken::Blank);
					cb(port, UrlToken::Port);
					s = tmpS;

					if (s.is('/')) {
						state = UrlToken::Path;
					} else if (s.is<StringView::Chars<'?'>>()) {
						state = UrlToken::Query;
					} else if (s.is('#')) {
						state = UrlToken::Fragment;
					} else {
						return true;
					}
				} else {
					tmpS = s;
					++ tmpS;
					auto arg = tmpS.readChars<UnreservedUni, SubDelim>();
					if (tmpS.is('@')) {
						// username + password
						if (!validateUserOrPassword(tmp) || !validateUserOrPassword(arg)) {
							return false;
						}

						cb(tmp, UrlToken::User);
						cb(StringView(arg.data() - 1, 1), UrlToken::Blank);
						cb(arg, UrlToken::Password);
						cb(StringView(tmpS, 1), UrlToken::Blank);
						state = UrlToken::Host;
						++ tmpS;
						s = tmpS;
					} else {
						// scheme without authority segment
						if (!validateScheme(tmp)) {
							return false;
						}
						cb(tmp, UrlToken::Scheme);
						state = UrlToken::Path;
						cb(s.sub(0, 1), UrlToken::Blank);
						++ s;
					}
				}
			}
		} else if (s.is('@')) {
			if (tmp.empty() || !validateUserOrPassword(tmp)) {
				return false;
			}
			cb(tmp, UrlToken::User);
			state = UrlToken::Host;
			cb(StringView(s, 1), UrlToken::Blank);
			++ s;
		} else if (s.is('/')) {
			// host + path
			if (!tmp.empty()) {
				if (!validateHost(tmp)) {
					return false;
				}
				cb(tmp, UrlToken::Host);
			}
			state = UrlToken::Path;
		} else if (s.is('?')) {
			// host + query
			if (tmp.empty()) {
				return false;
			}
			if (!validateHost(tmp)) {
				return false;
			}
			cb(tmp, UrlToken::Host);
			state = UrlToken::Query;
		} else if (s.is('#')) {
			// host + fragment
			if (tmp.empty()) {
				return false;
			}
			if (!validateHost(tmp)) {
				return false;
			}
			cb(tmp, UrlToken::Host);
			state = UrlToken::Fragment;
		} else {
			// assume host-only
			if (!tmp.empty()) {
				if (!validateHost(tmp)) {
					return false;
				}
				cb(tmp, UrlToken::Host);
				return true;
			}
			return false;
		}
	}

	if (state == UrlToken::User) {
		auto tmp_s = s;
		tmp = tmp_s.readChars<UnreservedUni, SubDelim>();

		if (tmp_s.is('@')) {
			// user only part
			if (!validateUserOrPassword(tmp)) {
				return false;
			}
			cb(tmp, UrlToken::User);
			state = UrlToken::Host;
			cb(StringView(tmp_s, 1), UrlToken::Blank);
			++ tmp_s;
			s = tmp_s;
		} else if (tmp_s.is(':')) {
			// user + password or host + port
			tmp_s ++;
			auto tmpS = tmp_s;

			// if it's port, next chars will be numbers only
			auto port = tmpS.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
			if (!port.empty() && !tmpS.is('@')) {
				// host + port
				if (!validateHost(tmp)) {
					return true;
				}

				cb(tmp, UrlToken::Host);
				cb(StringView(port.data() - 1, 1), UrlToken::Blank);
				cb(port, UrlToken::Port);
				s = tmpS;

				if (s.is('/')) {
					state = UrlToken::Path;
				} else if (s.is('?')) {
					state = UrlToken::Query;
				} else if (s.is('#')) {
					state = UrlToken::Fragment;
				} else {
					return true;
				}
			} else {
				// user + password
				if (!validateUserOrPassword(tmp)) {
					return false;
				}
				cb(tmp, UrlToken::User);

				if (tmpS.is('@')) {
					cb(StringView(port.data() - 1, 1), UrlToken::Blank);
					cb(port, UrlToken::Password);
					++ tmpS;
					state = UrlToken::Host;
				} else {
					tmp = tmp_s.readChars<UnreservedUni, SubDelim>();
					if (!tmp_s.is('@')) {
						return false;
					}
					++ tmp_s;
					if (!validateUserOrPassword(tmp)) {
						return false;
					}
					cb(StringView(tmp.data() - 1, 1), UrlToken::Blank);
					cb(tmp, UrlToken::Password);
					s = tmp_s;
					cb(StringView(s.data() - 1, 1), UrlToken::Blank);
					state = UrlToken::Host;
				}
			}
		} else {
			// host
			if (!validateHost(tmp)) {
				return false;
			}

			cb(tmp, UrlToken::Host);
			s = tmp_s;

			if (tmp_s.is('/')) {
				state = UrlToken::Path;
			} else if (tmp_s.is('?')) {
				state = UrlToken::Query;
			} else if (tmp_s.is('#')) {
				state = UrlToken::Fragment;
			} else {
				return true;
			}
		}
	}

	if (state == UrlToken::Host) {
		bool stop = false;
		if (s.is('[')) {
			auto t = s;
			++ t;
			tmp = t.readChars<UnreservedUni, SubDelim, StringView::Chars<':'>>();
			if (t.is(']')) {
				++ t;
				tmp = StringView(s.data(), t.data() - s.data());
				s = t;
			} else {
				return false;
			}
		} else {
			tmp = s.readChars<UnreservedUni, SubDelim, StringView::Chars<'[', ']'>>();
		}
		if (!validateHost(tmp)) {
			return false;
		}
		cb(tmp, UrlToken::Host);
		if (s.is(':')) {
			auto tmp2 = s;
			++ tmp2;
			auto port = tmp2.readChars<StringView::CharGroup<CharGroupId::Numbers>>();
			if (port.empty() || s.is<UnreservedUni>()) {
				state = UrlToken::Path;
				stop = true;
			} else {
				cb(StringView(port.data() - 1, 1), UrlToken::Blank);
				cb(port, UrlToken::Port);
				s = tmp2;
			}
		}
		if (stop) {
			// do nothing
		} else if (s.is('/')) {
			state = UrlToken::Path;
		} else if (s.is('?')) {
			state = UrlToken::Query;
		} else if (s.is('#')) {
			state = UrlToken::Fragment;
		} else {
			return true;
		}
	}

	if (state == UrlToken::Path) {
		tmp = s.readChars<UnreservedUni, SubDelim, StringView::Chars<'/', ':', '@'>>();
		if (!tmp.empty()) {
			cb(tmp, UrlToken::Path);
		}

		if (s.is('?')) {
			state = UrlToken::Query;
		} else if (s.is('#')) {
			state = UrlToken::Fragment;
		} else {
			return true;
		}
	}

	if (state == UrlToken::Query) {
		tmp = s.readChars<UnreservedUni, SubDelim, StringView::Chars<'/', ':', '@', '?', '[', ']'>>();
		if (!tmp.empty()) {
			if (tmp.is('?')) {
				cb(StringView(tmp, 1), UrlToken::Blank);
				++ tmp;
			}
			if (!tmp.empty()) {
				cb(tmp, UrlToken::Query);
			}
		}

		if (s.is('#')) {
			state = UrlToken::Fragment;
		} else {
			return true;
		}
	}

	if (state == UrlToken::Fragment) {
		tmp = s.readChars<UnreservedUni, SubDelim, StringView::Chars<'/', ':', '@', '?', '#', '[', ']'>>();
		if (!tmp.empty()) {
			if (tmp.is('#')) {
				cb(StringView(tmp, 1), UrlToken::Blank);
				++ tmp;
			}
			if (!tmp.empty()) {
				cb(tmp, UrlToken::Fragment);
			}
		}
	}

	return true;
}

}
