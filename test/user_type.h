#pragma once

#include "v8pp/context.hpp"

#include <string>
using std::string;

extern int g_UserTypeIS_alloc_count;
struct UserTypeIS
{
	UserTypeIS(int i, string s);
	~UserTypeIS();
	int nval;
	string sval;
	int get_nvar() const { return nval; }
	void set_nvar(int x) { nval = x; }
	string get_svar() const { return sval; }
	void set_svar(string s) { sval = s; }
	static int s_allocIdx;
	int allocIdx;
};

extern std::ostream& operator<<(std::ostream& os, const UserTypeIS& ut);

void reg_usertype(v8pp::context& context);
