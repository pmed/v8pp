# important

- js call cpp use stl as param have better performance.
  use

```
static void performance_param_arr_str_cpp(const std::vector<std::string>& as, const std::vector<std::string>& cras)
```

or

```
static void performance_param_arr_str_cpp(const std::list<std::string>& as, const std::list<std::string>& cras)
```

instead of

```
static void performance_param_arr_str_v8(v8::Isolate* isolate, const v8::Local<v8::Array>& as, const v8::Local<v8::Array>& cras)
```
