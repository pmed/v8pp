#!/usr/bin/env python

import os
import subprocess
import shutil

V8_URL = 'https://chromium.googlesource.com/v8/v8.git'

def fetch(url, target):
	parts = url.split('.git@')
	if len(parts) > 1:
		url = parts[0] + '.git'
		ref = parts[1]
	else:
		ref = 'HEAD'
	print 'Fetch %s@%s to %s' % (url, ref, target)

	if not os.path.isdir(os.path.join(target, '.git')):
		subprocess.call(['git', 'init', target])
	subprocess.call(['git', 'fetch', '--depth=1', url, ref], cwd=target)
	subprocess.call(['git', 'checkout', '-B', 'Branch_' + ref, 'FETCH_HEAD'], cwd=target)
	

fetch(V8_URL, 'v8')

deps = open('v8/DEPS').read()

Var = lambda name: vars[name]
exec deps

for dep in ['v8/base/trace_event/common', 'v8/build', 'v8/testing/gtest', 'v8/tools/gyp', 'v8/tools/clang', 'v8/third_party/icu']:
	fetch(deps[dep], dep)