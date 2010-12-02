AUTOMAKE_OPTIONS = dist-bzip2 no-dist-gzip
ACLOCAL_AMFLAGS=-I m4
DISTCLEANFILES = ChangeLog libcec/libcec.pc
EXTRA_DIST = TODO PORTING
SUBDIRS = libcec

if BUILD_CECD
SUBDIRS += cecd
endif

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libcec/libcec.pc

.PHONY: ChangeLog dist-up
ChangeLog:
	git --git-dir $(top_srcdir)/.git log > ChangeLog || touch ChangeLog

dist-hook: ChangeLog

reldir = .release/$(distdir)
dist-up: dist
	rm -rf $(reldir)
	mkdir -p $(reldir)
	cp $(distdir).tar.bz2 $(reldir)
	rsync -rv $(reldir) pbatard,cecd@frs.sourceforge.net:/home/frs/project/c/ce/cecd/
	rm -rf $(reldir)
