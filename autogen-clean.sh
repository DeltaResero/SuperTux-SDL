#!/bin/sh
if [ -f Makefile ]; then
	echo "Making make clean..."
	make clean
fi

if [ -f Jamfile ]; then
	echo "Making make clean..."
	jam clean
fi

echo "Removing autogen generated files..."
rm -f aclocal.m4 config.guess config.sub config.status configure depcomp Makefile Jamconfig Makefile.in install-sh missing mkinstalldirs ltmain.sh stamp-h.in */Makefile.in */Makefile ltconfig stamp-h supertux config.h.in
rm -f -r autom4te.cache
echo "Done."
