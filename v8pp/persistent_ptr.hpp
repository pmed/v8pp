#ifndef V8PP_PERSISTENT_PTR_HPP_INCLUDED
#define V8PP_PERSISTENT_PTR_HPP_INCLUDED

#include <cassert>

#include <boost/config.hpp>
#include "v8pp/convert.hpp"

namespace v8pp {

/// Moveable unique persistent
template<typename T>
struct persistent : public v8::UniquePersistent<T>
{
	typedef v8::UniquePersistent<T> base_class;

	persistent()
		: base_class()
	{
	}

	template<typename S>
	persistent(v8::Isolate* isolate, v8::Handle<S> const& handle)
		: base_class(isolate, handle)
	{
	}

	template<typename S>
	persistent(v8::Isolate* isolate, v8::PersistentBase<S> const& handle)
		: base_class(isolate, handle)
	{
	}

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
	persistent(persistent const&) = delete;
	persistent& operator=(persistent const&) = delete;
#endif

	persistent(persistent&& src)
		: base_class(src.Pass())
	{
	}

	persistent& operator=(persistent&& src)
	{
		if (&src != this)
		{
			base_class::operator=(src.Pass());
		}
		return *this;
	}
};

/// Pointer to C++ object wrapped in V8 with Persistent handle
template<typename T>
class persistent_ptr
{
public:
	/// Create an empty persistent pointer
	persistent_ptr()
		: value_()
		, handle_()
	{
	}

	/// Create a persistent pointer from a  pointer to a wrapped object, store persistent handle to it
	explicit persistent_ptr(v8::Isolate* isolate,  T* value)
		: value_()
	{
		reset(isolate, value);
	}

	/// Create a persistent pointer from V8 Value, store persistent handle to it
	explicit persistent_ptr(v8::Isolate* isolate, v8::Handle<v8::Value> handle)
		: value_()
	{
		reset(isolate, from_v8<T*>(isolate, handle));
	}

	persistent_ptr(persistent_ptr&& src)
		: value_(src.value_)
		, handle_(std::move(src.handle_))
	{
		src.value_ = nullptr;
	}


	persistent_ptr& operator=(persistent_ptr&& src)
	{
		if (&src != this)
		{
			value_ = src.value_;
			src.value_ = nullptr;
			handle_ = std::move(src.handle_);
		}
		return *this;
	}

	/// On destroy dispose persistent handle only
	~persistent_ptr() { reset(); }

	/// Reset with a new pointer to wrapped C++ object, replace persistent handle for it
	void reset(v8::Isolate* isolate, T* value)
	{
		if (value != value_)
		{
			assert((value_ == nullptr) == handle_.IsEmpty());
			handle_.Reset();
			value_ = value;
			if (value_)
			{
				handle_.Reset(isolate, v8pp::to_v8(isolate, value_));
				assert(!handle_.IsEmpty());
			}
		}
	}

	void reset() { reset(nullptr, nullptr); }

	/// Get pointer to the wrapped C++ object
	T* get() { return value_; }
	T const* get() const { return value_; }

	typedef T* (persistent_ptr<T>::*unspecfied_bool_type);

	/// Safe bool cast
	operator unspecfied_bool_type() const
	{
		return value_? &persistent_ptr<T>::value_ : nullptr;
	}

	/// Dereference pointer, valid if get() != nullptr
	T& operator*() { assert(value_); return *value_; }
	T const& operator*() const { assert(value_); return *value_; }

	T* operator->() { assert(value_); return value_; }
	T const* operator->() const { assert(value_); return value_; }

	bool operator==(persistent_ptr const& rhs) const { return value_ == rhs.value_; }
	bool operator!=(persistent_ptr const& rhs) const { return value_ != rhs.value_; }

	void swap(persistent_ptr& rhs)
	{
		std::swap(value_, rhs.value_);
		std::swap(handle_, rhs.handle_);
	}

	friend void swap(persistent_ptr& lhs, persistent_ptr& rhs)
	{
		lhs.swap(rhs);
	}

private:
	T* value_;
	v8pp::persistent<v8::Value> handle_;
};

} // namespace v8pp

#endif // V8PP_PERSISTENT_PTR_HPP_INCLUDED
