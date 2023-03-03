functions check:

| function                                                    | js call c++                             | c++ call js                                                      |
| ----------------------------------------------------------- | --------------------------------------- | ---------------------------------------------------------------- |
| return/param int64                                          | 1                                       | 1                                                                |
| return/param pointer of int64                               | x                                       | x                                                                |
| return/param reference of int64                             | const &                                 | both & / const & ok                                              |
| return/param uint64                                         | 1                                       | 1                                                                |
| return/param pointer of uint64                              | x                                       | x                                                                |
| return/param reference of uint64                            | const &                                 | both & / const & ok                                              |
| return/param int                                            | 1                                       | 1                                                                |
| return/param pointer of int                                 | x                                       | x                                                                |
| return/param reference of int                               | only const int&                         | both & / const & ok                                              |
| return/param string                                         | 1                                       | 1                                                                |
| return/param pointer of string                              | x                                       | x                                                                |
| return/param reference of string                            | only const string& / const char\*       | both & / const & ok                                              |
| return/param user type                                      | 1                                       | stuck... usertype cannot be used as param/return, only tuple can |
| return/param pointer of user type                           | 1                                       |                                                                  |
| return/param reference of user type                         | 1                                       |                                                                  |
| return/param constreference of user type                    | 1                                       |                                                                  |
| return/param array of int                                   | 1                                       |                                                                  |
| return/param consts reference of array of int               | 1                                       |                                                                  |
| return/param array of string                                | 1                                       |                                                                  |
| return/param consts reference of array of string            | 1                                       |                                                                  |
| return/param array of user type                             | 1                                       |                                                                  |
| return/param consts reference of array of user type         | 1                                       |                                                                  |
| return/param set of xxx                                     | new Set not allowed, use Map<xxx, bool> |                                                                  |
| return/param map of int => int                              | 1                                       |                                                                  |
| return/param consts reference of map of int => int          | 1                                       |                                                                  |
| return/param map of int => string                           | 1                                       |                                                                  |
| return/param consts reference of map of int => string       | 1                                       |                                                                  |
| return/param map of string => int                           | 1                                       |                                                                  |
| return/param consts reference of map of string => int       | 1                                       |                                                                  |
| return/param map of string => string                        | 1                                       |                                                                  |
| return/param consts reference of map of string => string    | 1                                       |                                                                  |
| return/param map of int => user type                        | 1                                       |                                                                  |
| return/param consts reference of map of int => user type    | 1                                       |                                                                  |
| return/param map of string => user type                     | 1                                       |                                                                  |
| return/param consts reference of map of string => user type | 1                                       |                                                                  |
