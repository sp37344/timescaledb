#include <postgres.h>
#include <catalog/pg_type.h>
#include <utils/builtins.h>
#include <utils/array.h>
#include "nodes/makefuncs.h"
#include "utils/lsyscache.h"
#include <netinet/in.h>

/* aggregate histogram:
 *	 hist(state, val, min, max, nbuckets) returns the histogram array with nbuckets
 *	 hist(state, val, thresholds[]) returns the histogram array 
 *
 * Usage:
 *	 SELECT hist(field, min, max, nbuckets) FROM table GROUP BY parameter.
 *	 SELECT hist(field, thresholds) FROM table GROUP BY parameter.
 */

PG_FUNCTION_INFO_V1(hist_sfunc);
PG_FUNCTION_INFO_V1(hist_sfunc_discrete);
PG_FUNCTION_INFO_V1(hist_combinefunc);
PG_FUNCTION_INFO_V1(hist_finalfunc);
PG_FUNCTION_INFO_V1(hist_serializefunc);
PG_FUNCTION_INFO_V1(hist_deserializefunc);

/* Generate a histogram */
Datum
hist_sfunc(PG_FUNCTION_ARGS) 
{
	MemoryContext aggcontext; 
	bytea 	*state = (PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_P(0));
	Datum   *hist;

	float 	val = PG_GETARG_FLOAT4(1); 
	float 	min = PG_GETARG_FLOAT4(2); 
	float 	max = PG_GETARG_FLOAT4(3); 
	int 	nbuckets = PG_GETARG_INT32(4); 

	int 	bucket = DirectFunctionCall4(width_bucket_float8, val, min, max, nbuckets);  

	if (!AggCheckCallContext(fcinfo, &aggcontext)) {
		/* Cannot be called directly because of internal-type argument */
		elog(ERROR, "hist_sfunc called in non-aggregate context");
	}

	if (min > max) {
		elog(ERROR, "lower bound cannot exceed upper bound");
	}

	if (state == NULL)
    {
        Size arrsize = sizeof(Datum) * (nbuckets + 2);
        state = MemoryContextAllocZero(aggcontext, VARHDRSZ + arrsize);
        SET_VARSIZE(state, VARHDRSZ + arrsize);
    }

    hist = (Datum *) VARDATA(state);
    hist[bucket] = Int32GetDatum(DatumGetInt32(hist[bucket]) + 1);

	PG_RETURN_BYTEA_P(state);
	// PG_RETURN_NULL();
}

/* Generate a histogram */
Datum
hist_sfunc_discrete(PG_FUNCTION_ARGS) 
{
	MemoryContext aggcontext; 
	bytea 	*state = (PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_P(0));
	Datum   *hist;

	Datum 	val = PG_GETARG_DATUM(1);
	Datum 	thresholds = PG_GETARG_DATUM(2);

	// int 	bucket = 1;
	// int 	bucket = DirectFunctionCall2(width_bucket_array, val, PointerGetDatum(thresholds)); 
	// int 	bucket = DirectFunctionCall2(width_bucket_array, val, thresholdss); 
	int 	bucket;
	int 	nbuckets =  DirectFunctionCall2(array_upper, thresholds, 1);

 	// print values
 	//elog(ERROR, "value: %d and array: %d", DatumGetInt32(val), DatumGetInt32(thresholds));

 	// bucket = 1;
 	if (PG_ARGISNULL(1) || PG_ARGISNULL(2))
 		bucket = 1;
 	else
 		bucket = DirectFunctionCall2(width_bucket_array, val, thresholds);

	if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* Cannot be called directly because of internal-type argument */
		elog(ERROR, "hist_sfunc called in non-aggregate context");
	}

	// check if array is sorted  before directfunctionalcall4 

	if (state == NULL)
    {
        Size arrsize = sizeof(Datum) * (nbuckets + 1);
        state = MemoryContextAllocZero(aggcontext, VARHDRSZ + arrsize);
        SET_VARSIZE(state, VARHDRSZ + arrsize);
    }

    hist = (Datum *) VARDATA(state);
    hist[bucket] = Int32GetDatum(DatumGetInt32(hist[bucket]) + 1);

	PG_RETURN_BYTEA_P(state);
	// PG_RETURN_NULL();
}

/* hist_combinefunc(internal, internal) => internal */
Datum
hist_combinefunc(PG_FUNCTION_ARGS)
{
	MemoryContext 	aggcontext;
	bytea 	*result;
	bytea 	*state1 = (PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_P(0));
	bytea 	*state2 = (PG_ARGISNULL(1) ? NULL : PG_GETARG_BYTEA_P(1));

	Datum 	*hist1;
	Datum 	*hist2;
	Datum 	*rhist;
	int 	i;


    if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* Cannot be called directly because of internal-type argument */
		elog(ERROR, "hist_combinefunc called in non-aggregate context");
	}

	// if (VARSIZE(state2) != VARSIZE(state1)) {
	// 	elog(ERROR, "state1 and state2 not same size");
	// }

	if (state2 == NULL) {
		// for (i = 0; i < (VARSIZE(result) - VARHDRSZ) / sizeof(Datum); i++) {
		// 	rhist[i] = hist1[i];
		// }
		PG_RETURN_BYTEA_P(state1);
	}

	else if (state1 == NULL) {
		// for (i = 0; i < (VARSIZE(result) - VARHDRSZ) / sizeof(Datum); i++) {
		// 	rhist[i] = hist2[i];
		// }
		PG_RETURN_BYTEA_P(state2);
	}

	else 
	{
		Size arrsize = VARSIZE(state1) - VARHDRSZ;
	    result = MemoryContextAllocZero(aggcontext, VARHDRSZ + arrsize);
	    SET_VARSIZE(result, VARHDRSZ + arrsize);

	    hist1 = (Datum *) VARDATA(state1);
		hist2 = (Datum *) VARDATA(state2);
		rhist = (Datum *) VARDATA(result);

		for (i = 0; i < (VARSIZE(state1) - VARHDRSZ) / sizeof(Datum); i++) {
			rhist[i] += (Datum) hist1[i];
			// rhist[i] += (Datum) hist1[i] + hist2[i];
		}
		for (i = 0; i < (VARSIZE(state2) - VARHDRSZ) / sizeof(Datum); i++) {
			rhist[i] += (Datum) hist2[i];
		}

		PG_RETURN_BYTEA_P(result);
	}

	// PG_RETURN_BYTEA_P(result);
}

/* hist_serializefunc(internal) => bytea */
Datum
hist_serializefunc(PG_FUNCTION_ARGS)
{
	bytea *state;
	Datum   *hist;
	int i;

	Assert(!PG_ARGISNULL(0));
	state = PG_GETARG_BYTEA_P(0);
	hist = (Datum *) VARDATA(state);

	for (i = 0; i < (VARSIZE(state) - VARHDRSZ) / sizeof(Datum); i++) {
		hist[i] = Int32GetDatum(htonl(DatumGetInt32(hist[i])));
	}

	PG_RETURN_BYTEA_P(state);
}

/* hist_deserializefunc(bytea, internal) => internal */
Datum
hist_deserializefunc(PG_FUNCTION_ARGS)
{
	bytea *state;
	Datum   *hist;
	int i;

	Assert(!PG_ARGISNULL(0));
	state = PG_GETARG_BYTEA_P(0);
	hist = (Datum *) VARDATA(state);

	for (i = 0; i < (VARSIZE(state) - VARHDRSZ) / sizeof(Datum); i++) {
		hist[i] = Int32GetDatum(ntohl(DatumGetInt32(hist[i])));
	}

	PG_RETURN_BYTEA_P(state);
}

/* hist_combinefunc(ArrayType, ArrayType) => ArrayType */
Datum
hist_combinefunc(PG_FUNCTION_ARGS)
{
	MemoryContext 	aggcontext;
	ArrayType 	*state1 = PG_ARGISNULL(0) ? NULL : PG_GETARG_ARRAYTYPE_P(0);
	ArrayType 	*state2 = PG_ARGISNULL(1) ? NULL : PG_GETARG_ARRAYTYPE_P(1);

	if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* Cannot be called directly because of internal-type argument */
		elog(ERROR, "hist_combinefunc called in non-aggregate context");
	}

	if (state2 == NULL)
		PG_RETURN_ARRAYTYPE_P(state1);

	else if (state1 == NULL)
		PG_RETURN_ARRAYTYPE_P(state2);

	else 
	{
		Datum 	*s1, *s2, *result; // Datum array representations of state1, state2, and result

		int     dims[1]; // 1-D array containing number of buckets used to construct result
 		int     lbs[1]; // 1-D array containing the lower bound used to construct result
		int 	ubs; // upper bound used to construct histogram

		/* Lower and upper bounds for state1 and state2 */
		int 	lb1 = DirectFunctionCall2(array_lower, PointerGetDatum(state1), 1);
		int 	lb2 = DirectFunctionCall2(array_lower, PointerGetDatum(state2), 1);
		int 	ub1 = DirectFunctionCall2(array_upper, PointerGetDatum(state1), 1);
		int 	ub2 = DirectFunctionCall2(array_upper, PointerGetDatum(state2), 1); 

		/* State variables */
		Oid    	i_eltype;
	    int16  	i_typlen;
	    bool   	i_typbyval;
	    char   	i_typalign;
	    int 	n;

		/* Get bound extremities */ 
		lbs[0] = (lb1 <= lb2) ? lb1 : lb2;
		ubs = (ub1 >= ub2) ? ub1 : ub2;

		dims[0] = ubs - lbs[0] + 1;
		result = (Datum *) MemoryContextAlloc(aggcontext, sizeof(Datum) * (dims[0]));

		/* Get state1 array element type */
		i_eltype = ARR_ELEMTYPE(state1);
		get_typlenbyvalalign(i_eltype, &i_typlen, &i_typbyval, &i_typalign);

		/* Deconstruct state1 into s1 */
		deconstruct_array(state1, i_eltype, i_typlen, i_typbyval, i_typalign, &s1, NULL, &n);

		/* Get state2 array element type */
		i_eltype = ARR_ELEMTYPE(state2);
		get_typlenbyvalalign(i_eltype, &i_typlen, &i_typbyval, &i_typalign);

		/* Deconstruct state2 into s2 */
		deconstruct_array(state2, i_eltype, i_typlen, i_typbyval, i_typalign, &s2, NULL, &n); 

		/* Initialize result array (which is zero-indexed in C) with zeroes */
		for (int i = 0; i < dims[0] + lbs[0]; i++) {
			result[i] = (Datum) 0;
		}

		/* Add in state1 */
		for (int i = lb1; i <= ub1; i++) {
			result[i] += (Datum) s1[i - lb1];
		}

		/* Add in state2 */
		for (int i = lb2; i <= ub2; i++) {
			result[i] += (Datum) s2[i - lb2];
		}

		PG_RETURN_ARRAYTYPE_P((construct_md_array(result + lbs[0], NULL, 1, dims, lbs, INT4OID, 4, true, 'i')));
	}
}

/* hist_funalfunc(internal, val REAL, MIN REAL, MAX REAL, nbuckets INTEGER) => INTEGER[] */
Datum
hist_finalfunc(PG_FUNCTION_ARGS)
{
	bytea 		*state;
	Datum    	*hist;    
	// ArrayType 	*result;
	int 		dims[1];
	int 		lbs[1];

	//REPLACE ABOVE WITH VARSIZE STUFF

	if (!AggCheckCallContext(fcinfo, NULL))
	{
		/* cannot be called directly because of internal-type argument */
		elog(ERROR, "hist_finalfunc called in non-aggregate context");
	}

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	state = PG_GETARG_BYTEA_P(0);
	hist = (Datum *) VARDATA(state);

	dims[0] = (VARSIZE(state) - VARHDRSZ) / sizeof(Datum);
	lbs[0] = 1;

	//instead of INT4OID try VARHDRSZ
	PG_RETURN_ARRAYTYPE_P(construct_md_array(hist, NULL, 1, dims, lbs, INT4OID, 4, true, 'i'));
}