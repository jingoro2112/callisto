#include "../include/callisto.h"
#include "vm.h"

#define _USE_MATH_DEFINES
#include <math.h>

//------------------------------------------------------------------------------
Callisto_Handle Callisto_pow( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 2 )
	{
		const Value *V1 = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		const Value *V2 = (E->Args[1].type == CTYPE_REFERENCE) ? E->Args[1].v : E->Args + 1;

		return Callisto_setValue( E, pow( (V1->type == CTYPE_FLOAT) ? V1->d : V1->i64,
										  (V2->type == CTYPE_FLOAT) ? V2->d : V2->i64) );
	}

	return 0;
}

Callisto_Handle Callisto_sin( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, sin( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_cos( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, cos( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_tan( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, tan( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_sinh( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, sinh( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_cosh( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, cosh( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_tanh( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, tanh( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_asin( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, asin( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_acos( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, acos( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_atan( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, atan( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_atan2( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 2 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		const Value *V2 = (E->Args[1].type == CTYPE_REFERENCE) ? E->Args[1].v : E->Args + 1;
		return Callisto_setValue( E, atan2( (V->type == CTYPE_FLOAT) ? V->d : V->i64,
											   (V2->type == CTYPE_FLOAT) ? V2->d : V2->i64));
	}
	return 0;
}

Callisto_Handle Callisto_exp( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, atan( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_log( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, atan( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_log10( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, log10( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_sqrt( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, sqrt( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_ceil( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, ceil( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_floor( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, floor( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_fabs( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, fabs( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_trunc( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 1 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		return Callisto_setValue( E, trunc( (V->type == CTYPE_FLOAT) ? V->d : V->i64) );
	}
	return 0;
}

Callisto_Handle Callisto_ldexp( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 2 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		const Value *V2 = (E->Args[1].type == CTYPE_REFERENCE) ? E->Args[1].v : E->Args + 1;
		return Callisto_setValue( E, ldexp( (V->type == CTYPE_FLOAT) ? V->d : V->i64,
											  (int)((V2->type == CTYPE_FLOAT) ? V2->d : V2->i64)) );
	}
	return 0;
}

Callisto_Handle Callisto_fmod( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs == 2 )
	{
		const Value *V = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
		const Value *V2 = (E->Args[1].type == CTYPE_REFERENCE) ? E->Args[1].v : E->Args + 1;
		return Callisto_setValue( E, fmod( (V->type == CTYPE_FLOAT) ? V->d : V->i64,
											  (V2->type == CTYPE_FLOAT) ? V2->d : V2->i64));
	}
	return 0;
}

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry mathFuncs[]=
{
	{ "math::pow", Callisto_pow, 0 },
	{ "math::sin", Callisto_sin, 0 },
	{ "math::cos", Callisto_cos, 0 },
	{ "math::tan", Callisto_tan, 0 },
	{ "math::sinh", Callisto_sinh, 0 },
	{ "math::cosh", Callisto_cosh, 0 },
	{ "math::tanh", Callisto_tanh, 0 },
	{ "math::asin", Callisto_asin, 0 },
	{ "math::acos", Callisto_acos, 0 },
	{ "math::atan", Callisto_atan, 0 },
	{ "math::atan2", Callisto_atan2, 0 },
	{ "math::exp", Callisto_exp, 0 },
	{ "math::log", Callisto_log, 0 },
	{ "math::log10", Callisto_log10, 0 },
	{ "math::pow", Callisto_pow, 0 },
	{ "math::sqrt", Callisto_sqrt, 0 },
	{ "math::ceil", Callisto_ceil, 0 },
	{ "math::floor", Callisto_floor, 0 },
	{ "math::fabs", Callisto_fabs, 0 },
	{ "math::ldexp", Callisto_ldexp, 0 },
	{ "math::trunc", Callisto_trunc, 0 },
	{ "math::fmod", Callisto_fmod, 0 },
//	{ "math::frexp", Callisto_frexp, 0 },
//	{ "math::modf", Callisto_modf, 0 },
//	{ "math::rad2deg", Callisto_rad2deg, 0 },
//	{ "math::deg2rad", Callisto_deg2rad, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importMath( Callisto_Context *C )
{
	Callisto_importFunctionList( C, mathFuncs, sizeof(mathFuncs) / sizeof(Callisto_FunctionEntry) );

	Callisto_importConstant( C, "math::e", Callisto_setValue(C, M_E) );
	Callisto_importConstant( C, "math::log2e", Callisto_setValue(C, M_LOG2E) );
	Callisto_importConstant( C, "math::log10e", Callisto_setValue(C, M_LOG10E) );
	Callisto_importConstant( C, "math::ln2", Callisto_setValue(C, M_LN2) );
	Callisto_importConstant( C, "math::ln10", Callisto_setValue(C, M_LN10) );
	Callisto_importConstant( C, "math::pi", Callisto_setValue(C, M_PI) );
	Callisto_importConstant( C, "math::pi_2", Callisto_setValue(C, M_PI_2) );
	Callisto_importConstant( C, "math::pi_4", Callisto_setValue(C, M_PI_4) );
	Callisto_importConstant( C, "math::_1pi", Callisto_setValue(C, M_1_PI) );
	Callisto_importConstant( C, "math::_2pi", Callisto_setValue(C, M_2_PI) );
	Callisto_importConstant( C, "math::_2sqrtpi", Callisto_setValue(C, M_2_SQRTPI) );
	Callisto_importConstant( C, "math::sqrt2", Callisto_setValue(C, M_SQRT2) );
	Callisto_importConstant( C, "math::sqrt1_2", Callisto_setValue(C, M_SQRT1_2) );
}
