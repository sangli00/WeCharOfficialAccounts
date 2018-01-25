#nanosmg to pipelineDB
MODULE_big =my_bgw

MYBGW_VERSION=1.0

OBJS = my_bgw.o

EXTENSION = my_bgw
DATA = my_bgw--1.0.sql

SHLIB_LINK += $(filter -lm, $(LIBS)) 
USE_PGXS=1
ifdef USE_PGXS
PG_CONFIG = /usr/local/pg_10_1/bin/pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/my_bgw
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

distrib:
	rm -f *.o
	rm -rf results/ regression.diffs regression.out tmp_check/ log/
	cd .. ; tar --exclude=.svn -chvzf my_bgw-$(MYBGW_VERSION).tar.gz my_bgw

