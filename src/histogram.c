// includes copied from agg_bookend.c -- will clean up later
#include <unistd.h>

#include <postgres.h>
#include <fmgr.h>

#include <utils/datetime.h>
#include <catalog/pg_type.h>
#include <catalog/namespace.h>
#include <utils/guc.h>
#include <utils/builtins.h>
#include <utils/array.h>

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
	MemoryContext aggcontext; 

	ArrayType 	*state = PG_ARGISNULL(0) ? NULL : PG_GETARG_ARRAYTYPE_P(0);
	Datum 		*elems; //Datum array used in constructing state array 

	float 	val = PG_GETARG_FLOAT4(1); 
	float 	min = PG_GETARG_FLOAT4(2); 
	float 	max = PG_GETARG_FLOAT4(3); 

	int 	nbuckets = PG_GETARG_INT32(4); 
	int 	i = 1;

	//width_bucket uses nbuckets + 1 (!) and starts at 1
	int 	bucket = DirectFunctionCall4(width_bucket_float8, val, min, max, nbuckets - 2); //minus three? 

	int     dims[1];
 	int     lbs[1];

	lbs[0] = 1;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* cannot be called directly because of internal-type argument */
		elog(ERROR, "hist_sfunc called in non-aggregate context");
	}

	//Init the array with the correct number of 0's so the caller doesn't see NULLs (for loop)
	if (state == NULL) //could also check if state is NULL 
	{
		if (bucket == 0) {
			i = 0;
			lbs[0] = 0;
		}

		else if (bucket == nbuckets + 1) {
			nbucket++;
		}

		elems = (Datum *) MemoryContextAlloc(aggcontext, sizeof(Datum) * (nbuckets + 1 - i));
		// elems = (Datum *) palloc(sizeof(Datum) * nbuckets);

		for (i; i <= nbuckets; i++) {
			elems[i] = (Datum) 0;
		}

		dims[0] = nbuckets;
	}

	else { //ERROR: NULL VALUE?
		//deconstruct
		Oid    	i_eltype;
	    int16  	i_typlen;
	    bool   	i_typbyval;
	    char   	i_typalign;
	    int 	n;
	    //bool 	*nulls;

		/* get input array element type */
		i_eltype = ARR_ELEMTYPE(state);
		get_typlenbyvalalign(i_eltype, &i_typlen, &i_typbyval, &i_typalign);

		//check if zero or one based 
		if (DirectFunctionCall2(array_lower, PointerGetDatum(state), 1) == 0
		
		if (array_lower(state, 1) == 0) 
		{
			lb[0] = 0;

			if (bucket > array_upper(state, 1))
			{
				deconstruct_array(state, i_eltype, i_typlen, i_typbyval, i_typalign, &elems, NULL, &n); //what to do with elems 
				n++;
			}
			else 
			{
				deconstruct_array(state, i_eltype, i_typlen, i_typbyval, i_typalign, &elems, NULL, &n); //zero based -- i think 
			}
			
		}
		else {
			if (bucket > array_upper(state, 1))
			{
				deconstruct_array(state, i_eltype, i_typlen, i_typbyval, i_typalign, &elems + 1, NULL, &n); //zero based how to append?
				n++;
			}
			else if (bucket < array_lower(state, 1)) 
			{
				deconstruct_array(state, i_eltype, i_typlen, i_typbyval, i_typalign, &elems + 1, NULL, &n); 
				lb[0] = 0;
				n++;
			}
			else {
				//deconstruct array 
				deconstruct_array(state, i_eltype, i_typlen, i_typbyval, i_typalign, &elems + 1, NULL, &n); //plus one?
			}
		}

		//is this zero based? 
		//deconstruct array with elems + 1 if there is a zero 
		dims[0] = n;
	}

	//increment state
	elems[bucket] = elems[bucket] + (Datum) 1; //is this correct if you are extracting from state?

	//create return array 
	// construct_md_array(Datum *elems, bool *nulls, int ndims, int *dims, int *lbs, Oid elmtype, int elmlen, bool elmbyval, char elmalign)
	// construct_array(Datum *elems, int nelems, Oid elmtype, int elmlen, bool elmbyval, char elmalign)	

	//state = construct_array(elems, nbuckets, INT8OID, 8, true, 'd'); 
	//state = construct_array(elems, nbuckets, INT4OID, 4, true, 'i'); 
	state = construct_md_array(elems, NULL, 1, dims, lbs, INT4OID, 4, true, 'i'); 

	// returns integer array 
	PG_RETURN_ARRAYTYPE_P(state); 
}


// hist_combinerfunc()