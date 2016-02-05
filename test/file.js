//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
var file    = require('file'),
    console = require('console')

var write1 = new file.writer()
var write2 = new file.writer("bunko")

console.log("write1 is writer", file.writer, write1, (write1 instanceof file.writer))

if (! write1.open("punko") || ! write2.is_open()) {
    console.log("could not load file for write")
}
else {
    write1.println("hello friend")
    write2.println("hollo frond")
    write1.println("fond of the night")
    write2.println("frond frond of the night")
    write1.close()
    write2.close()
}

if (! file.rename("punko", "newpunko"))
    console.log("could not rename file")

var read1 = new file.reader("newpunko")
if (! read1.is_open()) {
    console.log("could not load punko for read")
}
else for (var line = read1.getln(); line; line = read1.getln()) {
    console.log("tata",line)
}

var read2 = new file.reader("bunko")
if (! read2.is_open()) {
    console.log("could not load bunko for read")
}
else for (var line = read2.getln(); line; line = read2.getln()) {
    console.log("papa",line)
}

console.log("exit")
