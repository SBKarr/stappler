/*
 * SPDataUtils.h
 *
 *  Created on: 22 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_DATA_SPDATAUTILS_HPP_
#define COMMON_DATA_SPDATAUTILS_HPP_

NS_SP_EXT_BEGIN(data)

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits> & stream, EncodeFormat f) {
	stream.iword( EncodeFormat::EncodeStreamIndex ) = f.flag();
	return stream;
}

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits> & stream, EncodeFormat::Format f) {
	EncodeFormat fmt(f);
	stream << fmt;
	return stream;
}

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits> & stream, const data::Value &val) {
	EncodeFormat fmt(stream.iword( EncodeFormat::EncodeStreamIndex ));
	write(stream, val, fmt);
	return stream;
}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATAUTILS_HPP_ */
