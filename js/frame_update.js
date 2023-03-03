let performance_test = function () {
    const fruits = [];
    fruits.push("banana", "apple", "peach");
    const hello = ["hello", "world", "kitty"];
    
    const map_is1 = { 3: "eight", 4: "sixteen", 5: "thirty-two" };
    const map_is2 = { 10: "one-thousand-and-twenty-four", 11: "two-thousand-and-forty-eight", 12: "for-thousand-and-ninety-six" };

    // performance
    // const test_count = 10000;
    const test_count = 100;
    tbegin();
    for (let index = 0; index < test_count; index++) {
        performance_param_arr_str_v8(fruits, hello);
    }
    tend("performance_param_arr_str_v8");

    tbegin();
    for (let index = 0; index < test_count; index++) {
        performance_param_arr_str_cpp(fruits, hello);
    }
    tend("performance_param_arr_str_cpp");

    tbegin();
    for (let index = 0; index < test_count; index++) {
        performance_param_map_is_v8(map_is1, map_is2);
    }
    tend("performance_param_map_is_v8");

    tbegin();
    for (let index = 0; index < test_count; index++) {
        performance_param_map_is_cpp(map_is1, map_is2);
    }
    tend("performance_param_map_is_cpp");
};

let frame_update = function () {
    dlog("frame_update ", 1024, " hello world!");
    const ret_int = test_param_int(1024, 2048, 65536, 2147483649);
    dlog("ret_int:", ret_int);
    const ret_int64 = test_param_int64(4294967297, 4294967297, 4294967297, 4294967297);
    dlog("ret_int64:", ret_int64);
    const ret_string = test_param_string("hello", "world", "worldhello");
    dlog("ret_string:", ret_string);

    let ut1 = new UserTypeIS(1024, "double kill");
    let ut2 = new UserTypeIS(2048, "hellokitty");
    ++ut2.nvar;
    ut2.svar = "kitty hello";
    let ut3 = new UserTypeIS(32, "godlike");
    let ut4 = new UserTypeIS(65536, "thereisnospoon");
    let ut5 = new UserTypeIS(1048576, "whosyourdaddy");
    const ret_utype1 = test_param_utype(ut1, ut2, ut3, ut4, ut5);
    const ret_utype2 = test_param_utype(ut1, ut2, ut3, ut4, ut5);
    // dlog("ret_utype1:[", ret_utype1.nvar, ",", ret_utype1.svar, "] ret_utype2:[", ret_utype2.nvar, ",", ret_utype2.svar, "]");
    dlog("ret_utype1:[", ret_utype1, "] ret_utype2:[", ret_utype2, "]");

    const fruits = [];
    fruits.push("banana", "apple", "peach");
    const hello = ["hello", "world", "kitty"];
    const ret_arr_string = test_param_arr_string(fruits, hello);
    dlog("ret_arr_string:", ret_arr_string, " ret_arr_string.length:", ret_arr_string.length, " fruits.length:", fruits.length);

    const pow_of_2 = [2, 4, 8, 16];
    const pow_of_3 = [3, 9, 27, 81];
    test_param_arr_int(pow_of_2, pow_of_3);

    const arr_ut1 = [ut1, ut2];
    const arr_ut2 = [ut3, ut4, ut5];
    test_param_arr_utype(arr_ut1, arr_ut2);

    const set_str1 = new Set(["banana", "apple", "peach"]);
    const set_str2 = new Set(["hello", "world", "kitty"]);
    dlog("set_str1:", set_str1);
    test_param_set_string(set_str1, set_str2);

    const map_ii1 = { 3: 8, 4: 16, 5: 32 };
    const map_ii2 = { 10: 1024, 11: 2048, 12: 4096 };
    test_param_map_int_int(map_ii1, map_ii2);

    const map_is1 = { 3: "eight", 4: "sixteen", 5: "thirty-two" };
    const map_is2 = { 10: "one-thousand-and-twenty-four", 11: "two-thousand-and-forty-eight", 12: "for-thousand-and-ninety-six" };
    test_param_map_int_string(map_is1, map_is2);

    const map_si1 = { "three": 8, "four": 16, "five": 32 };
    const map_si2 = { "ten": 1024, "eleven": 2048, "twelve": 4096 };
    test_param_map_string_int(map_si1, map_si2);

    const map_ss1 = { "three": "eight", "four": "sixteen", "five": "thirty-two" };
    const map_ss2 = { "ten": "one-thousand-and-twenty-four", "eleven": "two-thousand-and-forty-eight", "twelve": "for-thousand-and-ninety-six" };
    test_param_map_string_string(map_ss1, map_ss2);

    const map_iu1 = { 3: ut1, 4: ut2, 5: ut3 };
    const map_iu2 = { 10: ut3, 11: ut4, 12: ut5 };
    test_param_map_int_utype(map_iu1, map_iu2);

    const map_su1 = { "three": ut1, "four": ut2, "five": ut3 };
    const map_su2 = { "ten": ut3, "eleven": ut4, "twelve": ut5 };
    test_param_map_string_utype(map_su1, map_su2);

    performance_test();
};

frame_update