//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
var console = require('console')

console.log('yo', 'baby', 4.2, null)

var c2 = require('console')
c2.log('conker 2')

function pando() {
    this.friend = 1
    this.masto = require('console')
    this.whack = function() {
        this.masto.log('truckman')
    }
}

var c = new pando()
c.whack()

