-- CREATE OR REPLACE FUNCTION _timescaledb_internal.hist_sfunc(state, anyelement, anyelement, anyelement, integer)
-- -- // hist_sfunc (state INTEGER[], val REAL, MIN REAL, MAX REAL, nbuckets INTEGER)
-- RETURNS internal
-- AS '$libdir/timescaledb', 'hist_sfunc'
-- LANGUAGE C IMMUTABLE;

-- --This aggregate returns the histogram data for a table.
-- --Ex. histogram(temp, time) returns the temp value for the row with the lowest time
-- DROP AGGREGATE IF EXISTS histogram(anyelement, "any");
-- CREATE AGGREGATE first(anyelement, "any") (
--     SFUNC = _timescaledb_internal.first_sfunc,
--     STYPE = internal,
--     COMBINEFUNC = _timescaledb_internal.first_combinefunc,
--     SERIALFUNC = _timescaledb_internal.bookend_serializefunc,
--     DESERIALFUNC = _timescaledb_internal.bookend_deserializefunc,
--     PARALLEL = SAFE,
--     FINALFUNC = _timescaledb_internal.bookend_finalfunc,
--     FINALFUNC_EXTRA
-- );


-- CREATE OR REPLACE FUNCTION hist_sfunc (state internal, val REAL, MIN REAL, MAX REAL, nbuckets INTEGER) 
-- RETURNS INTEGER[] 
-- AS '$libdir/timescaledb', 'hist_sfunc'
-- LANGUAGE C IMMUTABLE;

-- -- Tell Postgres how to use the new function
-- DROP AGGREGATE IF EXISTS histogram (REAL, REAL, REAL, INTEGER);
-- CREATE AGGREGATE histogram (REAL, REAL, REAL, INTEGER) (
-- 	SFUNC = hist_sfunc,
-- 	STYPE = INTEGER[]
-- 	COMBINEFUNC = _timescaledb_internal.last_combinefunc,
-- 	SERIALFUNC = _timescaledb_internal.bookend_serializefunc,
-- 	DESERIALFUNC = _timescaledb_internal.bookend_deserializefunc,
-- 	PARALLEL = SAFE,
-- 	FINALFUNC = _timescaledb_internal.bookend_finalfunc,
-- 	FINALFUNC_EXTRA
-- );

CREATE OR REPLACE FUNCTION hist_sfunc (state INTEGER[], val REAL, MIN REAL, MAX REAL, nbuckets INTEGER) 
RETURNS INTEGER[] 
AS '$libdir/timescaledb', 'hist_sfunc'
LANGUAGE C IMMUTABLE;

-- Tell Postgres how to use the new function
DROP AGGREGATE IF EXISTS histogram (REAL, REAL, REAL, INTEGER);
CREATE AGGREGATE histogram (REAL, REAL, REAL, INTEGER) (
       SFUNC = hist_sfunc,
       STYPE = INTEGER[]
);