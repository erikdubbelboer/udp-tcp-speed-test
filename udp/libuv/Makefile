#
# Copyright Erik Dubbelboer. and other contributors. All rights reserved.
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

-include config.mk

BUILDTYPE ?= Release
PYTHON ?= python


all: client


client: config.gypi out/Makefile
	$(MAKE) -C out BUILDTYPE=$(BUILDTYPE) V=$(V) client
	ln -fs out/$(BUILDTYPE)/client client


out/Makefile: common.gypi deps/libuv/uv.gyp config.gypi client.gyp
	$(PYTHON) tools/gyp_client -f make

config.gypi: configure
	./configure

config.mk: configure
	./configure


clean:
	-rm -rf out/Makefile client out/$(BUILDTYPE)/client
	-find out/ -name '*.o' -o -name '*.a' | xargs rm -rf

distclean: clean
	-rm -rf out
	-rm -f config.gypi
	-rm -f config.mk


.PHONY: all client

