#include "user_type.h"
#include "v8pp/class.hpp"
#include "v8pp/context.hpp"

#include <iostream>
using std::cout;
using std::endl;

int g_UserTypeIS_alloc_count = 0;
int UserTypeIS::s_allocIdx = 0;

UserTypeIS::UserTypeIS(int i, string s)
	: nval(i), sval(s), allocIdx(++s_allocIdx)
{
	++g_UserTypeIS_alloc_count;
	cout << __func__ << " (" << allocIdx << "," << nval << " " << sval << " count:" << g_UserTypeIS_alloc_count << ")"
		 << " addr:" << (int64_t)this << endl;
}

UserTypeIS::~UserTypeIS()
{
	--g_UserTypeIS_alloc_count;
	cout << __func__ << " (" << allocIdx << "," << nval << " " << sval << " count:" << g_UserTypeIS_alloc_count << ")"
		 << " addr:" << (int64_t)this << endl;
}

std::ostream& operator<<(std::ostream& os, const UserTypeIS& ut)
{
	os << "[nval:" << ut.nval << " sval:" << ut.sval << "]";
	return os;
}

void reg_usertype(v8pp::context& context)
{
	// bind class
	v8::Isolate* isolate = context.isolate();

	v8pp::class_<UserTypeIS, v8pp::raw_ptr_traits> UserTypeIS_class(isolate);
	UserTypeIS_class.ctor<int, string>();
	UserTypeIS_class.var("nvar", &UserTypeIS::nval);
	UserTypeIS_class.var("svar", &UserTypeIS::sval);
	UserTypeIS_class.function("func_set_nvar", &UserTypeIS::set_nvar);
	UserTypeIS_class.property("nvar_readonly", &UserTypeIS::get_nvar);

	// set class into the module template
	// umod.class_("UserTypeIS", UserTypeIS_class);
	context.class_("UserTypeIS", UserTypeIS_class);
}
