# Copyright (C) 2005 Open Source Telecom Corporation.
#    
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies
EXTRA_DIST = Doxyfile Doxyfile.in *.tex

if DOXY
DOXYFILES = doxy

doxy:	Doxyfile
	(cd @srcdir@ ; $(DOXYGEN) Doxyfile)
	touch doxy
else
DOXYFILES = 
endif

if MAKE_PDF
PDFFILES = install.pdf script.pdf

script.pdf: script.tex
	-$(PDFLATEX) @srcdir@/script.tex

install.pdf: install.tex
	-$(PDFLATEX) @srcdir@/install.tex

else
PDFFILES =
endif

if MAKE_HTML
HTMLFILES = install.html script.html


install.html: install.tex
	(cd @srcdir@ ; $(LATEX2HTML) -nosubdir -split 0 install.tex)
	lynx --dump @srcdir@/install.html >install.text


script.html: script.tex
	(cd @srcdir@ ; $(LATEX2HTML) -nosubdir -split 0 script.tex)
	lynx --dump @srcdir@/script.html >script.text   

else
HTMLFILES =
endif

noinst_DATA = $(DOXYFILES) $(HTMLFILES) $(PDFFILES)
