//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/property.hpp"
#include "v8pp/context.hpp"

#include "test.hpp"

int get1();
void set1(int);

int get2(v8::Isolate*);
void set2(v8::Isolate*, int);

void get3(v8::Local<v8::String> name,
	v8::PropertyCallbackInfo<v8::Value> const& info);
void set3(v8::Local<v8::String> name, v8::Local<v8::Value> value,
	v8::PropertyCallbackInfo<void> const& info);

struct X
{
	bool get1() const;
	void set1(int);

	bool get2(v8::Isolate*);
	void set2(v8::Isolate*, int);

	void get3(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info);
	void set3(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info);
};

// external accessors
int external_get1(const X&);
void external_set1(X&, int);

int external_get2(const X&, v8::Isolate*);
void external_set2(X&, v8::Isolate*, int);

void external_get3(const X&, v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info);
void external_set3(X&, v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info);

using namespace v8pp::detail;

//property metafunctions
static_assert(is_getter<decltype(&get1)>::value,
	"getter function");
static_assert(is_getter<decltype(&X::get1)>::value,
	"getter member function");
static_assert(is_getter<decltype(&external_get1), 1>::value,
	"external getter function");

static_assert(is_setter<decltype(&set1)>::value,
	"setter function");
static_assert(is_setter<decltype(&X::set1)>::value,
	"setter member function");
static_assert(is_setter<decltype(&external_set1), 1>::value,
	"external setter function");

static_assert(is_isolate_getter<decltype(&get2)>::value,
	"isolate getter function");
static_assert(is_isolate_getter<decltype(&X::get2)>::value,
	"isolate getter member function");
static_assert(is_isolate_getter<decltype(&external_get2), 1>::value,
	"external isolate getter function");

static_assert(is_isolate_setter<decltype(&set2)>::value,
	"isolate setter function");
static_assert(is_isolate_setter<decltype(&X::set2)>::value,
	"isolate setter member function");
static_assert(is_isolate_setter<decltype(&external_set2), 1>::value,
	"external isolate setter function");

static_assert(is_direct_getter<decltype(&get3)>::value,
	"direct getter function");
static_assert(is_direct_getter<decltype(&X::get3)>::value,
	"direct getter member function");
static_assert(is_direct_getter<decltype(&external_get3), 1>::value,
	"external getter function");

static_assert(is_direct_setter<decltype(&set3)>::value,
	"direct setter function");
static_assert(is_direct_setter<decltype(&X::set3)>::value,
	"direct setter member function");
static_assert(is_direct_setter<decltype(&external_set3), 1>::value,
	"external setter function");

// tag selectors
static_assert(std::is_same<select_getter_tag<decltype(&get1)>,
	getter_tag>::value, "getter function tag");
static_assert(std::is_same<select_getter_tag<decltype(&X::get1)>,
	getter_tag>::value, "getter member function tag");

static_assert(std::is_same<select_setter_tag<decltype(&set1)>,
	setter_tag>::value, "setter function tag");
static_assert(std::is_same<select_setter_tag<decltype(&X::set1)>,
	setter_tag>::value, "setter member function tag");

static_assert(std::is_same<select_getter_tag<decltype(&get2)>,
	isolate_getter_tag>::value, "isolate getter function tag");
static_assert(std::is_same<select_getter_tag<decltype(&X::get2)>,
	isolate_getter_tag>::value, "isolate getter member function tag");

static_assert(std::is_same<select_setter_tag<decltype(&set2)>,
	isolate_setter_tag>::value, "isolate setter function tag");
static_assert(std::is_same<select_setter_tag<decltype(&X::set2)>,
	isolate_setter_tag>::value, "isolate setter member function tag");

static_assert(std::is_same<select_getter_tag<decltype(&get3)>,
	direct_getter_tag>::value, "direct getter function tag");
static_assert(std::is_same<select_getter_tag<decltype(&X::get3)>,
	direct_getter_tag>::value, "direct getter member function tag");

static_assert(std::is_same<select_setter_tag<decltype(&set3)>,
	direct_setter_tag>::value, "direct setter function tag");
static_assert(std::is_same<select_setter_tag<decltype(&X::set3)>,
	direct_setter_tag>::value, "direct setter member function tag");


void test_property()
{
}
