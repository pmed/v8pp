#ifndef V8PP_PERSISTENT_PTR_HPP_INCLUDED
#define V8PP_PERSISTENT_PTR_HPP_INCLUDED

#include <cassert>

#include "from_v8.hpp"
#include "to_v8.hpp"

namespace v8pp {

/// Pointer to C++ object wrapped in V8 with Persistent handle
template<typename T>
class persistent_ptr
{
public:
	/// Create an empty persistent pointer
	persistent_ptr()
		: value_()
		, v8_value_()
	{
	}

	/// Create a persistent pointer from a  pointer to a wrapped object, store persistent handle to it
	explicit persistent_ptr(T* value)
		: value_()
	{
		reset(value);
	}

	/// Create a persistent pointer from V8 Value, store persistent handle to it
	explicit persistent_ptr(v8::Handle<v8::Value> v8_value)
		: value_()
	{
		reset(from_v8<T*>(v8_value));
	}

	/// On destroy dispose persistent handle only
	~persistent_ptr() { reset(); }

	/// Create another persistent handle on copy
	persistent_ptr(persistent_ptr const& src)
		: value_(src.value_)
	{
		if (value_)
		{
			v8_value_ = v8::Persistent<v8::Value>::New(src.v8_value_);
		}
	}

	/// Replace persistent handle on assign
	persistent_ptr& operator=(persistent_ptr src)
	{
		swap(src);
		return *this;
	}

	/// Reset with a new pointer to wrapped C++ object, replace persistent handle for it
	void reset(T* value = nullptr)
	{
		if (value != value_)
		{
			assert((value_ == nullptr) == v8_value_.IsEmpty());
			v8_value_.Dispose(); v8_value_.Clear();
			value_ = value;
			if (value_)
			{
				v8_value_ = v8::Persistent<v8::Value>::New(v8pp::to_v8(value_));
				assert(!v8_value_.IsEmpty());
			}
		}
	}

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
		std::swap(v8_value_, rhs.v8_value_);
	}

	friend void swap(persistent_ptr& lhs, persistent_ptr& rhs)
	{
		lhs.swap(rhs);
	}

private:
	T* value_;
	v8::Persistent<v8::Value> v8_value_;
};

} // namespace v8pp

#endif // V8PP_PERSISTENT_PTR_HPP_INCLUDED
