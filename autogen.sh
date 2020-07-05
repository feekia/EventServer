#!/bin/sh

aclocal
autoconf
autoheader
automake --add-missing

autoreconf --force --verbose
