PG_CONFIG = pg_config
MIN_SUPPORTED_VERSION = $(shell cat src/init.c | grep -m 1 'MIN_SUPPORTED_VERSION_STR' | sed "s/^\#define MIN_SUPPORTED_VERSION_STR \"\([0-9]*\.[0-9]*\)\"$\/\1/g")

# will need to change when we support more than one version
ifneq ($(MIN_SUPPORTED_VERSION), $(shell $(PG_CONFIG) --version | sed "s/^PostgreSQL \([0-9]*\.[0-9]*\)\.[0-9]*$\/\1/g"))
$(error "TimescaleDB requires PostgreSQL $(MIN_SUPPORTED_VERSION)")
endif

EXTENSION = timescaledb
EXT_VERSION = $(shell cat timescaledb.control | grep 'default' | sed "s/^default_version = '\(.*\)'/\1/g")

# Get list of SQL files that need to be loaded. 
SQL_FILES = $(shell cat sql/setup_load_order.txt sql/functions_load_order.txt sql/init_load_order.txt)

# Get versions that update scripts should be generated for. (ex. '0.1.0--0.1.1').
UPDATE_VERSIONS = $(shell ls -1 sql/updates/*.sql | xargs -0 basename | grep "$(EXT_VERSION).sql" | sed "s/\.sql//g"| sed "s/^pre-//g;s/^post-//g"| head -1)

UPDATE_VERSIONS_PRE = $(shell ls -1 sql/updates/pre-$(UPDATE_VERSIONS).sql)
UPDATE_VERSIONS_POST = $(shell ls -1 sql/updates/post-$(UPDATE_VERSIONS).sql)

# Get SQL files that are needed to build update script.
UPDATE_SQL_FILES = $(shell echo ${UPDATE_VERSIONS_PRE} && cat sql/functions_load_order.txt && echo ${UPDATE_VERSIONS_POST} )

EXT_SQL_FILE = sql/$(EXTENSION)--$(EXT_VERSION).sql
UPDATE_FILE = sql/$(EXTENSION)--$(UPDATE_VERSIONS).sql

MANUAL_UPDATE_FILES = $(shell echo sql/setup_main.sql && cat sql/setup_load_order.txt sql/init_load_order.txt)

# List of files to add to extension apart from the ones built now, typically previous update files.
EXTRA_DATA_FILES = $(shell cat extra_extension_files.txt )
EXT_GIT_COMMIT := $(shell git describe --abbrev=4 --dirty --always --tags || echo $(EXT_GIT_COMMIT))

DATA = $(EXT_SQL_FILE) $(UPDATE_FILE) $(EXTRA_DATA_FILES)
MODULE_big = $(EXTENSION)

SRCS = \
	src/init.c \
	src/extension.c \
	src/utils.c \
	src/catalog.c \
	src/metadata_queries.c \
	src/cache.c \
	src/cache_invalidate.c \
	src/chunk.c \
	src/scanner.c \
	src/histogram.c \
	src/hypertable_cache.c \
	src/hypertable.c \
	src/dimension.c \
	src/dimension_slice.c \
	src/ddl_utils.c \
	src/chunk_constraint.c \
	src/partitioning.c \
	src/planner_utils.c \
	src/planner.c \
	src/executor.c \
	src/process_utility.c \
	src/copy.c \
	src/sort_transform.c \
	src/hypertable_insert.c \
	src/chunk_dispatch.c \
	src/chunk_dispatch_info.c \
	src/chunk_dispatch_state.c \
	src/chunk_dispatch_plan.c \
	src/chunk_insert_state.c \
	src/agg_bookend.c \
	src/subspace_store.c \
	src/guc.c \
	src/compat.c \
	src/version.c

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

MKFILE_PATH := $(abspath $(MAKEFILE_LIST))
CURRENT_DIR = $(dir $(MKFILE_PATH))

TEST_PGPORT ?= 15432
TEST_PGHOST ?= localhost
TEST_PGUSER ?= postgres
TEST_DIR = test
TEST_CLUSTER ?= $(TEST_DIR)/testcluster
TEST_INSTANCE_OPTS ?= \
	--create-role=$(TEST_PGUSER) \
	--temp-instance=$(TEST_CLUSTER) \
	--temp-config=$(TEST_DIR)/postgresql.conf

EXTRA_REGRESS_OPTS ?=

export TEST_PGUSER

TESTS = $(sort $(wildcard test/sql/*.sql))
USE_MODULE_DB=true
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = \
	$(TEST_INSTANCE_OPTS) \
	--inputdir=$(TEST_DIR) \
	--outputdir=$(TEST_DIR) \
	--launcher=$(TEST_DIR)/runner.sh \
	--host=$(TEST_PGHOST) \
	--port=$(TEST_PGPORT) \
	--user=$(TEST_PGUSER) \
	--load-language=plpgsql \
	--load-extension=$(EXTENSION) \
	$(EXTRA_REGRESS_OPTS)

PGXS := $(shell $(PG_CONFIG) --pgxs)

EXTRA_CLEAN = $(EXT_SQL_FILE) $(DEPS)

include $(PGXS)
override CFLAGS += -DINCLUDE_PACKAGE_SUPPORT=0 -MMD -DEXT_GIT_COMMIT=\"$(EXT_GIT_COMMIT)\"
override pg_regress_clean_files = test/results/ test/regression.diffs test/regression.out tmp_check/ log/ $(TEST_CLUSTER)
-include $(DEPS)

all: $(EXT_SQL_FILE) $(UPDATE_FILE) $(SQL_FILES) $(UPDATE_SQL_FILES)

$(EXT_SQL_FILE): $(SQL_FILES)
	@echo generating $(EXT_SQL_FILE)
	@cat $^ > $@

$(UPDATE_FILE): $(UPDATE_SQL_FILES)
	@echo generating $(UPDATE_FILE)
	@cat $^ > $@

check-sql-files:
	@echo $(SQL_FILES)

install: $(EXT_SQL_FILES)

clean: clean-sql-files clean-extra

clean-sql-files:
	@rm -f $(EXT_SQL_FILE) $(UPDATE_FILE)

clean-extra:
	@rm -f src/*~

package: clean $(EXT_SQL_FILE)
	@mkdir -p package/lib
	@mkdir -p package/extension
	$(install_sh) -m 755 $(EXTENSION).so 'package/lib/$(EXTENSION).so'
	$(install_sh) -m 644 $(EXTENSION).control 'package/extension/'
	$(install_sh) -m 644 $(EXT_SQL_FILE) 'package/extension/'

typedef.list: clean $(OBJS)
	./scripts/generate_typedef.sh

pgindent: typedef.list
	pgindent --typedef=typedef.list

manualupdate:
	echo $(MANUAL_UPDATE_FILES)

.PHONY: check-sql-files all
