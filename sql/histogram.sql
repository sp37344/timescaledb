CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_sfunc (state INTERNAL, val REAL, MIN REAL, MAX REAL, nbuckets INTEGER) 
RETURNS INTERNAL 
AS '$libdir/timescaledb', 'hist_sfunc'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_sfunc_discrete (state INTERNAL, val REAL, threshold REAL[]) 
RETURNS INTERNAL 
AS '$libdir/timescaledb', 'hist_sfunc_discrete'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_combinefunc(state1 INTERNAL, state2 internal)
RETURNS INTERNAL
AS '$libdir/timescaledb', 'hist_combinefunc'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_serializefunc(INTERNAL)
RETURNS bytea
AS '$libdir/timescaledb', 'hist_serializefunc'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_deserializefunc(bytea, INTERNAL)
RETURNS internal
AS '$libdir/timescaledb', 'hist_deserializefunc'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_finalfunc(state INTERNAL, val REAL, MIN REAL, MAX REAL, nbuckets INTEGER) 
RETURNS INTEGER[]
AS '$libdir/timescaledb', 'hist_finalfunc'
LANGUAGE C IMMUTABLE;

-- Tell Postgres how to use the new function
DROP AGGREGATE IF EXISTS histogram (REAL, REAL, REAL, INTEGER);
CREATE AGGREGATE histogram (REAL, REAL, REAL, INTEGER) (
    SFUNC = _timescaledb_internal.hist_sfunc,
    STYPE = INTERNAL,
    COMBINEFUNC = _timescaledb_internal.hist_combinefunc,
    SERIALFUNC = _timescaledb_internal.hist_serializefunc,
    DESERIALFUNC = _timescaledb_internal.hist_deserializefunc,
    PARALLEL = SAFE,
    FINALFUNC = _timescaledb_internal.hist_finalfunc,
    FINALFUNC_EXTRA
);

-- Tell Postgres how to use the new function
DROP AGGREGATE IF EXISTS histogram (REAL, REAL[]);
CREATE AGGREGATE histogram (REAL, REAL[]) (
    SFUNC = _timescaledb_internal.hist_sfunc_discrete,
    STYPE = INTERNAL,
    COMBINEFUNC = _timescaledb_internal.hist_combinefunc,
    SERIALFUNC = _timescaledb_internal.hist_serializefunc,
    DESERIALFUNC = _timescaledb_internal.hist_deserializefunc,
    PARALLEL = SAFE,
    FINALFUNC = _timescaledb_internal.hist_finalfunc,
    FINALFUNC_EXTRA
);
