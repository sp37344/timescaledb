// includes copied from agg_bookend.c -- will clean up later
#include <unistd.h>

#include <postgres.h>
#include <fmgr.h>

#include <utils/datetime.h>
#include <catalog/pg_type.h>
#include <catalog/namespace.h>
#include <utils/guc.h>
#include <utils/builtins.h>

#include "utils.h"
#include "nodes/nodes.h"
#include "nodes/makefuncs.h"
#include "utils/lsyscache.h"

/* aggregate histogram:
 *	 hist(state, val, min, max, nbuckets) returns the histogram array with nbuckets
 *
 * Usage:
 *	 SELECT hist(field, min, max, nbuckets) FROM table GROUP BY parameter.
 */

PG_FUNCTION_INFO_V1(hist_sfunc);

/*
 * Generate a histogram.
 */

// hist_sfunc (state INTEGER[], val REAL, MIN REAL, MAX REAL, nbuckets INTEGER)
// example: SELECT host, COUNT(*), histogram(phase, 0.0, 360.0, 20) FROM t GROUP BY host ORDER BY host;

Datum
hist_sfunc(PG_FUNCTION_ARGS) //postgres function arguments 
{
	ArrayType 	*state = PG_ARGISNULL(0) ? NULL : PG_GETARG_ARRAYTYPE_P(0);
	Datum 		*elems; //Datum array used in constructing state array 

	float 	val = PG_GETARG_FLOAT4(1); 
	float 	min = PG_GETARG_FLOAT4(2); 
	float 	max = PG_GETARG_FLOAT4(3); 

	int 	nbuckets = PG_GETARG_INT32(4); 

	MemoryContext aggcontext; //aggcontext?

	//width_bucket uses nbuckets + 1 (!) and starts at 1
	int 	bucket = DirectFunctionCall4(width_bucket_float8, val, min, max, nbuckets - 1) - 1; 

	//Init the array with the correct number of 0's so the caller doesn't see NULLs (for loop)
	if (elems == NULL) //could also check if state is NULL 
	{
		elems = (Datum *) MemoryContextAlloc(aggcontext, sizeof(Datum) * nbuckets);
		// elems = (Datum *) palloc(sizeof(Datum) * nbuckets)

		for (int i = 0; i < nbuckets; i++) 
		{
			elems[i] = 0;
		}
	}

	//increment state
	elems[bucket] = elems[bucket] + 1;

	//create return array 
	state = construct_array(elems, nbuckets, INT4OID, sizeof(int4), false, 'i'); 

	// returns integer array 
	PG_RETURN_ARRAYTYPE_P(state); 
}




// hist_combinerfunc()