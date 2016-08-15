/*
 * CouchbaseCommand.h
 *
 *  Created on: 15 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_COUCHBASE_COUCHBASECOMMAND_H_
#define SERENITY_SRC_COUCHBASE_COUCHBASECOMMAND_H_

#include "Define.h"

#ifndef NOCB

#include "SPDataCbor.h"
#include "SPData.h"

#define LCB_NO_DEPR_CXX_CTORS 1
#include <libcouchbase/couchbase.h>

NS_SA_EXT_BEGIN(couchbase)

enum class StoreOperation {
	Add = 0x01, /** Add the item to the cache, but fail if the object exists already */
	Replace = 0x02, /** Replace the existing object in the cache */
	Set = 0x03, /** Unconditionally set the object in the cache */
	Append = 0x04, /** Append this object to the existing object */
	Prepend = 0x05 /** Prepend this object to the existing object */
};

struct GetCmd {
	Bytes key;
	lcb_time_t exptime;
	bool lock;

	template <typename Key>
	GetCmd(Key &&k, lcb_time_t t, bool l)
	: key(std::forward<Key>(k)), exptime(t), lock(l) { }
	GetCmd(const GetCmd &cmd) = default;
	GetCmd(GetCmd &&cmd) = default;
	GetCmd & operator= (const GetCmd &cmd) = default;
	GetCmd & operator= (GetCmd &&cmd) = default;
};

struct StoreCmd {
	Bytes key;
	Bytes data;
	StoreOperation op;
	uint32_t flags;
	lcb_time_t exptime;
	lcb_cas_t cas;

	template <typename Key, typename Data>
	StoreCmd(Key &&k, Data &&d, StoreOperation op, uint32_t f, lcb_time_t t, lcb_cas_t c)
	: key(std::forward<Key>(k)), data(std::forward<Data>(d)), op(op), flags(f), exptime(t), cas(c) { }
	StoreCmd(const StoreCmd &cmd) = default;
	StoreCmd(StoreCmd &&cmd) = default;
	StoreCmd & operator= (const StoreCmd &cmd) = default;
	StoreCmd & operator= (StoreCmd &&cmd) = default;
};

struct MathCmd {
	Bytes key;
	int64_t delta;
	bool create;
	uint64_t initial;
	lcb_time_t exptime;

	template <typename Key>
	MathCmd(Key &&k, int64_t delta, bool create, uint64_t initial, lcb_time_t t)
	: key(std::forward<Key>(k), t), delta(delta), create(create), initial(initial), exptime(t)  { }
	MathCmd(const MathCmd &cmd) = default;
	MathCmd(MathCmd &&cmd) = default;
	MathCmd & operator= (const MathCmd &cmd) = default;
	MathCmd & operator= (MathCmd &&cmd) = default;
};

struct ObserveCmd {
	Bytes key;

	template <typename Key>
	ObserveCmd(Key &&k) : key(std::forward<Key>(k)) { }
	ObserveCmd(const ObserveCmd &cmd) = default;
	ObserveCmd(ObserveCmd &&cmd) = default;
	ObserveCmd & operator= (const ObserveCmd &cmd) = default;
	ObserveCmd & operator= (ObserveCmd &&cmd) = default;
};

struct TouchCmd {
	Bytes key;
	lcb_time_t exptime;

	template <typename Key>
	TouchCmd(Key &&k, lcb_time_t t) : key(std::forward<Key>(k)),  exptime(t) { }
	TouchCmd(const TouchCmd &cmd) = default;
	TouchCmd(TouchCmd &&cmd) = default;
	TouchCmd & operator= (const TouchCmd &cmd) = default;
	TouchCmd & operator= (TouchCmd &&cmd) = default;
};

struct UnlockCmd {
	Bytes key;
	lcb_cas_t cas;

	template <typename Key>
	UnlockCmd(Key &&k, lcb_cas_t c) : key(std::forward<Key>(k)), cas(c) { }
	UnlockCmd(const UnlockCmd &cmd) = default;
	UnlockCmd(UnlockCmd &&cmd) = default;
	UnlockCmd & operator= (const UnlockCmd &cmd) = default;
	UnlockCmd & operator= (UnlockCmd &&cmd) = default;
};

struct RemoveCmd {
	Bytes key;
	lcb_cas_t cas;

	template <typename Key>
	RemoveCmd(Key &&k, lcb_cas_t c) : key(std::forward<Key>(k)), cas(c) { }
	RemoveCmd(const RemoveCmd &cmd) = default;
	RemoveCmd(RemoveCmd &&cmd) = default;
	RemoveCmd & operator= (const RemoveCmd &cmd) = default;
	RemoveCmd & operator= (RemoveCmd &&cmd) = default;
};

struct VersionCmd {
	VersionCmd() { }
	VersionCmd(const VersionCmd &cmd) = default;
	VersionCmd(VersionCmd &&cmd) = default;
	VersionCmd & operator= (const VersionCmd &cmd) = default;
	VersionCmd & operator= (VersionCmd &&cmd) = default;
};

struct FlushCmd {
	FlushCmd() { }
	FlushCmd(const FlushCmd &cmd) = default;
	FlushCmd(FlushCmd &&cmd) = default;
	FlushCmd & operator= (const FlushCmd &cmd) = default;
	FlushCmd & operator= (FlushCmd &&cmd) = default;
};

using GetSig = void (GetCmd &, lcb_error_t, const Bytes &, lcb_cas_t, lcb_uint32_t);
using DataSig = void (GetCmd &, lcb_error_t, data::Value &, lcb_cas_t, lcb_uint32_t);
using VersionSig = void (VersionCmd &, lcb_error_t, const String &);
using StoreSig = void (StoreCmd &, lcb_error_t, lcb_storage_t, lcb_cas_t);
using TouchSig = void (TouchCmd &, lcb_error_t, lcb_cas_t);
using UnlockSig = void (UnlockCmd &, lcb_error_t);
using MathSig = void (MathCmd &, lcb_error_t, lcb_uint64_t, lcb_cas_t);
using ObserveSig = void (ObserveCmd &, lcb_error_t, lcb_cas_t, lcb_observe_t, bool, lcb_time_t, lcb_time_t);
using RemoveSig = void (RemoveCmd &, lcb_error_t, lcb_cas_t);
using FlushSig = void (FlushCmd &, lcb_error_t);

template <typename Cmd, typename Sig>
struct CallbackRec {
	apr::callback_ref<Sig> cb;
	Cmd *data;

	template <typename Cb>
	CallbackRec(Cb &&c, Cmd *t) : cb(std::forward<Cb>(c)), data(t) { }
};

template <typename Cb, typename Cmd, typename Sig>
struct Command {
	Cmd cmd;
	apr::callback<Cb, Sig> callback;
	CallbackRec<Cmd, Sig> rec;

	template <typename ... Args>
	Command(Cb &&cb, Args && ... args)
	: cmd(std::forward<Args>(args)...)
	, callback(std::move(cb))
	, rec(callback, &cmd) { }
};

template <typename Cb> using GetCmdStorage = Command<Cb, GetCmd, GetSig>;
template <typename Cb> using StoreCmdStorage = Command<Cb, StoreCmd, StoreSig>;
template <typename Cb> using MathCmdStorage = Command<Cb, MathCmd, MathSig>;
template <typename Cb> using ObserveCmdStorage = Command<Cb, ObserveCmd, ObserveSig>;
template <typename Cb> using TouchCmdStorage = Command<Cb, TouchCmd, TouchSig>;
template <typename Cb> using UnlockCmdStorage = Command<Cb, UnlockCmd, UnlockSig>;
template <typename Cb> using RemoveCmdStorage = Command<Cb, RemoveCmd, RemoveSig>;
template <typename Cb> using VersionCmdStorage = Command<Cb, VersionCmd, VersionSig>;
template <typename Cb> using FlushCmdStorage = Command<Cb, FlushCmd, FlushSig>;

using GetRec = CallbackRec<GetCmd, GetSig>;
using StoreRec = CallbackRec<StoreCmd, StoreSig>;
using MathRec = CallbackRec<MathCmd, MathSig>;
using ObserveRec = CallbackRec<ObserveCmd, ObserveSig>;
using TouchRec = CallbackRec<TouchCmd, TouchSig>;
using UnlockRec = CallbackRec<UnlockCmd, UnlockSig>;
using RemoveRec = CallbackRec<RemoveCmd, RemoveSig>;
using VersionRec = CallbackRec<VersionCmd, VersionSig>;
using FlushRec = CallbackRec<FlushCmd, FlushSig>;

using Time = ValueWrapper<lcb_time_t, class lcb_time_flag>;
using Cas = ValueWrapper<lcb_cas_t, class lcb_cas_flag>;
using Flag = ValueWrapper<uint32_t, class lcb_flag>;

template <typename Cb, typename Key> inline GetCmdStorage<Cb>
Get(Cb &&cb, Key &&k, Time t = Time(0), bool l = false) {
	return GetCmdStorage<Cb>(std::move(cb), std::forward<Key>(k), t.get(), l);
}

template <typename Cb, typename Key, typename Data> inline StoreCmdStorage<Cb>
Store(Cb &&cb, Key &&k, Data &&d, StoreOperation op = StoreOperation::Set, Flag f = Flag(0), Time t = Time(0), Cas c = Cas(0)) {
	return StoreCmdStorage<Cb>(std::move(cb), std::forward<Key>(k), std::forward<Data>(d), op, f.get(), t.get(), c.get());
}

template <typename Cb, typename Key>  inline MathCmdStorage<Cb>
Math(Cb &&cb, Key &&k, int64_t delta, bool create = false, uint64_t initial = 0, Time t = Time(0)) {
	return MathCmdStorage<Cb>(std::move(cb), std::forward<Key>(k), delta, create, initial, t.get());
}

template <typename Cb, typename Key>  inline ObserveCmdStorage<Cb>
Observe(Cb &&cb, Key &&k) {
	return ObserveCmdStorage<Cb>(std::move(cb), std::forward<Key>(k));
}

template <typename Cb, typename Key>  inline TouchCmdStorage<Cb>
Touch(Cb &&cb, Key &&k, Time t = Time(0)) {
	return TouchCmdStorage<Cb>(std::move(cb), std::forward<Key>(k), t.get());
}

template <typename Cb, typename Key>  inline UnlockCmdStorage<Cb>
Unlock(Cb &&cb, Key &&k, Cas c = Cas(0)) {
	return UnlockCmdStorage<Cb>(std::move(cb), std::forward<Key>(k), c.get());
}

template <typename Cb, typename Key>  inline RemoveCmdStorage<Cb>
Remove(Cb &&cb, Key &&k, Cas c = Cas(0)) {
	return RemoveCmdStorage<Cb>(std::move(cb), std::forward<Key>(k), c.get());
}

template <typename Cb>  inline VersionCmdStorage<Cb>
Version(Cb &&cb) {
	return VersionCmdStorage<Cb>(std::move(cb));
}

template <typename Cb>  inline FlushCmdStorage<Cb>
Flush(Cb &&cb) {
	return FlushCmdStorage<Cb>(std::move(cb));
}

template <typename Cb, typename Key> inline auto
GetData(Cb &&cb, Key &&k, Time t = Time(0), bool l = false) {
	return Get(
			[ncb = std::move(cb)] (GetCmd &cmd, lcb_error_t e, const Bytes &b, lcb_cas_t c, lcb_uint32_t f) {
		auto val = data::read(b);
		ncb(cmd, e, val, c, f);
	}, std::forward<Key>(k), t, l);
}

template <typename Cb, typename Key>  inline StoreCmdStorage<Cb>
StoreData(Cb &&cb, Key &&k, const data::Value &d, Flag f = Flag(0), Time t = Time(0), Cas c = Cas(0)) {
	return Store(std::move(cb), std::forward<Key>(k), data::write(d, data::EncodeFormat(data::EncodeFormat::Cbor)),
			StoreOperation::Set, f, t, c);
}

template <typename Cb, typename Key>  inline StoreCmdStorage<Cb>
StoreObject(Cb &&cb, Key &&k, const data::Value &d, Flag f = Flag(0), Time t = Time(0), Cas c = Cas(0)) {
	return Store(std::move(cb), std::forward<Key>(k), data::cbor::writeCborObject(d), f, t, c);
}

template <typename Cb, typename Key>  inline StoreCmdStorage<Cb>
StoreArray(Cb &&cb, Key &&k, const data::Value &d, Flag f = Flag(0), Time t = Time(0), Cas c = Cas(0)) {
	return Store(std::move(cb), std::forward<Key>(k), data::cbor::writeCborArray(d), f, t, c);
}

NS_SA_EXT_END(couchbase)

#endif // NOCB

#endif /* SERENITY_SRC_COUCHBASE_COUCHBASECOMMAND_H_ */
