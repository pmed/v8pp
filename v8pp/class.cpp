#include "v8pp/config.hpp"

#if !V8PP_HEADER_ONLY
#include "v8pp/class.ipp"

namespace v8pp { namespace detail {

template class object_registry<raw_ptr_traits>;
template class object_registry<shared_ptr_traits>;

template
object_registry<raw_ptr_traits>& classes::add<raw_ptr_traits>(v8::Isolate* isolate,
	type_info const& type, object_registry<raw_ptr_traits>::dtor_function&& dtor);

template
void classes::remove<raw_ptr_traits>(v8::Isolate* isolate, type_info const& type);

template
object_registry<raw_ptr_traits>& classes::find<raw_ptr_traits>(v8::Isolate* isolate,
	type_info const& type);

template
object_registry<shared_ptr_traits>& classes::add<shared_ptr_traits>(v8::Isolate* isolate,
	type_info const& type, object_registry<shared_ptr_traits>::dtor_function&& dtor);

template
void classes::remove<shared_ptr_traits>(v8::Isolate* isolate, type_info const& type);

template
object_registry<shared_ptr_traits>& classes::find<shared_ptr_traits>(v8::Isolate* isolate,
	type_info const& type);

}} // namespace v8pp::detail

#endif
