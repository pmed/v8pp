// import { msg, PI, addNumbers } from '../../js/import_test';
// import { msg, PI, addNumbers } from '../../js/import_test.js';
// import { msg, PI, addNumbers } from './import_test';
// let import_test = require('./import_test');
// let import_test = require('./import_test.js');
// let import_test = require('../../js/import_test');
// let import_test = require('../../js/import_test.js');
include('../../js/import_test.js');
include('../../js/test_functions.js');

function cpp_call_js_func() {

    // dlog(msg); // Prints: Hello World!
    // dlog(PI); // Prints: 3.14
    dlog(addNumbers(5, 16)); // Prints: 21
};
