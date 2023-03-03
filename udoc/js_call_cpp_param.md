### js call cpp param rules

- js call cpp function, param must be T or const T&
- only const char* can be const T*, or const T
- this is because js cannot modify param as reference or pointer, so every such usage is forbidden!!!

### allowed types:

- int
- const int&
- string
- const string&
- const char\*
- UserType
- UserType\*
- UserType&
- const UserType\*
- const UseerType&
-
