#include "util.h"
#include "arch.h"

#include <stdio.h>

#define D_ITERATE(a) //a
#define D_GET_VALUE(a) //a
#define D_COMPARE(a) //a
#define D_ASSIGN(a) //a
#define D_OPERATOR(a) //a
#define D_ARG_SETUP(a) //a
#define D_CLEAR_VALUE(a) //a
#define D_ADD_TABLE_ELEMENT(a) //a
#define D_COPY(a) //a
#define D_DEEPCOPY(a) //a
#define D_MIRROR(a) //a
#define D_DEREFERENCE(a) //a
#define D_MEMBERACCESS(a) //a

//------------------------------------------------------------------------------
bool isReserved( Cstr& token )
{
	for( unsigned int i=0; c_reserved[i].token.size(); i++ )
	{
		if ( token == c_reserved[i].token )
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
const Cstr& formatType( int type )
{
	static Cstr ret;
	switch( type )
	{
		case CTYPE_NULL: return ret = "CTYPE_NULL";
		case CTYPE_INT: return ret = "CTYPE_INT";
		case CTYPE_FLOAT: return ret = "CTYPE_FLOAT";
		case CTYPE_TABLE: return ret = "CTYPE_TABLE";
		case CTYPE_UNIT: return ret = "CTYPE_UNIT";
		case CTYPE_STRING: return ret = "CTYPE_STRING";
		case CTYPE_WSTRING: return ret = "CTYPE_WSTRING";
		case CTYPE_ARRAY: return ret = "CTYPE_ARRAY";
		case CTYPE_OFFSTACK_REFERENCE: return ret = "CTYPE_OFFSTACK_REFERENCE";
		case CTYPE_ARRAY_ITERATOR: return ret = "CTYPE_ARRAY_ITERATOR";
		case CTYPE_HASH_ITERATOR: return ret = "CTYPE_HASH_ITERATOR";
		case CTYPE_STRING_ITERATOR: return ret = "CTYPE_STRING_ITERATOR";
		case CTYPE_WSTRING_ITERATOR: return ret = "CTYPE_WSTRING_ITERATOR";
		default: return ret.format( "<UNRECOGNIZED TYPE %d>", type );
	}
}

//------------------------------------------------------------------------------
const Cstr& formatSymbol( Callisto_Context* C, unsigned int key )
{
	static Cstr ret;
	Cstr* symbol = C->symbols.get( key );
	ret.format( "0x%08X:%s", key, symbol ? symbol->c_str() : "<null>" );
	return ret;
}

//------------------------------------------------------------------------------
const Cstr& formatValue( Value const& value )
{
	static Cstr ret;
	switch( value.type )
	{
		case CTYPE_NULL: return ret.format("NULL");
		case CTYPE_INT: return ret.format( "%lld",  value.i64 );
		case CTYPE_FLOAT: return ret.format( "%g", value.d );
		case CTYPE_TABLE: return ret.format( "TABLE" );
		case CTYPE_UNIT: return ret.format( "UNIT %p:%p", value.u, value.refLocalSpace->getItem() );
		case CTYPE_STRING: return ret = value.refCstr->item;
		case CTYPE_WSTRING: return ret = Cstr(value.refWstr->item);
		case CTYPE_ARRAY: return ret.format( "ARRAY %d", value.refArray->item.count());
		case CTYPE_ARRAY_ITERATOR: return ret = "CTYPE_ARRAY_ITERATOR";
		case CTYPE_HASH_ITERATOR: return ret = "CTYPE_HASH_ITERATOR";
		case CTYPE_STRING_ITERATOR: return ret = "CTYPE_STRING_ITERATOR";
		case CTYPE_WSTRING_ITERATOR: return ret = "CTYPE_WSTRING_ITERATOR";
		case CTYPE_OFFSTACK_REFERENCE:
		{
			if (value.v == &value)
			{
				return (ret = "CIRCULAR REFERENCE");
			}
			else
			{
				return formatValue( *value.v );
			}
		}

		default: return ret = "<UNRECOGNIZED TYPE>";
	}
}

//------------------------------------------------------------------------------
const Cstr& formatOpcode( int opcode )
{
	static Cstr ret;
	int i;
	for( i=0; c_opcodes[i].opcode != 0; ++i )
	{
		if ( c_opcodes[i].opcode == opcode )
		{
			break;
		}
	}

	if ( c_opcodes[i].opcode == 0 )
	{
		ret.format( "UNKNOWN_OPCODE <%d>", opcode );
	}
	else
	{
		ret = c_opcodes[i].name;
	}
	
	return ret;
}

//------------------------------------------------------------------------------
void dumpStack( Callisto_ExecutionContext* E )
{
	for( int i=0; i<E->stackPointer; i++ )
	{
		if ( E->stack[i].type == CTYPE_OFFSTACK_REFERENCE )
		{
			Log( "%d: [ref]->%s [%s]\n", i, formatType(E->stack[i].v->type).c_str(), formatValue(*E->stack[i].v).c_str() );
		}
		else
		{
			Log( "%d: %s [%s]\n", i,  formatType(E->stack[i].type).c_str(), formatValue(E->stack[i]).c_str() );
		}
	}
}

//------------------------------------------------------------------------------
void dumpSymbols( Callisto_Context* CC )
{
	for( Value* V = CC->global.refLocalSpace->item.getFirst(); V; V = CC->global.refLocalSpace->item.getNext() )
	{
		if ( V->type == CTYPE_OFFSTACK_REFERENCE )
		{
			Log( "%p->%p: %d:[%s]\n", V, V->v, V->v->type, formatValue(*V->v).c_str() );
		}
		else
		{
			Log( "%p: %d:[%s]\n", V, V->type, formatValue(*V).c_str() );
		}
	}
}

//------------------------------------------------------------------------------
void clearContext( Callisto_Context* C )
{
	C->text.clear();
	C->err = CE_NoError;
	clearValue( C->global );
	C->globalUnit.childUnits.clear();
	C->unitList.clear();
	C->threads.clear();
	C->symbols.clear();
}

//------------------------------------------------------------------------------
void getLiteralFromCodeStream( Callisto_ExecutionContext* E, Value& Value )
{
	// fetch a value from the stack
	Value.type = *E->callisto->text.c_str(E->pos++);

	switch( Value.type )
	{
		default:
		case CTYPE_NULL:
		case CTYPE_TABLE:
		{
			break;
		}

		case CTYPE_INT:
		case CTYPE_FLOAT:
		{
			memcpy( (char *)&Value.i64, E->callisto->text.c_str(E->pos), 8 );
			E->pos += 8;
			Value.i64 = ntohll(Value.i64);
			break;
		}

		case CTYPE_STRING:
		{
			Value.refCstr = new RefCountedVariable<Cstr>();
			readStringFromCodeStream( E, Value.refCstr->item );
			break;
		}

		case CTYPE_WSTRING:
		{
			Value.refWstr = new RefCountedVariable<Wstr>();
			readWStringFromCodeStream( E, Value.refWstr->item );
			break;
		}

	}

	D_GET_VALUE(Log("getLiteralFromCodeStream %s:[%s]\n", formatType(Value.type).c_str(), formatValue(Value).c_str()));
}

//------------------------------------------------------------------------------
void readStringFromCodeStream( Callisto_ExecutionContext* E, Cstr& string )
{
	unsigned int len;
	len = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
	E->pos += 4;
	if ( len )
	{
		string.set( E->callisto->text.c_str(E->pos), len );
		E->pos += len;
	}
	else
	{
		string.clear();
	}
}

//------------------------------------------------------------------------------
void readWStringFromCodeStream( Callisto_ExecutionContext* E, Wstr& string )
{
	unsigned int len;
	len = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
	E->pos += 4;
	if ( len )
	{
		string.set( (wchar_t *)E->callisto->text.c_str(E->pos), len/sizeof(wchar_t) );
		E->pos += len;
	}
	else
	{
		string.clear();
	}
}

//------------------------------------------------------------------------------
void putValue( Cstr& code, Value& Value )
{
	switch( Value.type )
	{
		default:
		case CTYPE_NULL:
		{
			code += (char)O_LiteralNull;
			break;
		}
		
		case CTYPE_INT:
		{
			if ( (Value.i64 <= 127) && (Value.i64 >= -128) )
			{
				code += (char)O_LiteralInt8;
				code += (char)Value.i64;
			}
			else if ( (Value.i64 <= 32767) && (Value.i64 >= -32768) )
			{
				code += (char)O_LiteralInt16;
				int16_t be = htons( (int16_t)Value.i64 );
				code.append( (char*)&be, 2 );
			}
			else if ( (Value.i64 <= 2147483647) && (Value.i64 >= -(int64_t)2147483648) )
			{
				code += (char)O_LiteralInt32;
				int32_t be = htonl( (int32_t)Value.i64 );
				code.append( (char*)&be, 4 );
			}
			else
			{
				code += (char)O_Literal;
				code += (char)CTYPE_INT;
				int64_t be = htonll(Value.i64);
				code.append( (char*)&be, 8 );
			}
			break;
		}
		
		case CTYPE_FLOAT:
		{
			code += (char)O_Literal;
			code += (char)CTYPE_FLOAT;
			int64_t be = ntohll(Value.i64);
			code.append( (char*)&be, 8 );
			break;
		}

		case CTYPE_STRING:
		{
			code += (char)O_Literal;
			code += (char)CTYPE_STRING;
			putString( code, Value.refCstr->item );
			break;
		}

		case CTYPE_WSTRING:
		{
			code += (char)O_Literal;
			code += (char)CTYPE_WSTRING;
			putWString( code, Value.refWstr->item );
			break;
		}
	}
}

//------------------------------------------------------------------------------
void putString( Cstr& code, Cstr& string )
{
	unsigned int len = string.size();
	unsigned int hlen = htonl(len);
	code.append( (char*)&hlen, 4 );
	if ( len )
	{
		code.append( string.c_str(), len );
	}
}

//------------------------------------------------------------------------------
void putWString( Cstr& code, Wstr& string )
{
	unsigned int len = string.size() * sizeof(wchar_t);
	unsigned int hlen = htonl(len);
	code.append( (char*)&hlen, 4 );
	if ( len )
	{
		code.append( (char *)string.c_str(), len );
	}
}

//------------------------------------------------------------------------------
void popOne( Callisto_ExecutionContext* E )
{
	clearValue( E->stack[--E->stackPointer] );
}

//------------------------------------------------------------------------------
void popNum( Callisto_ExecutionContext* E, const int number )
{
	for( int i=0; i<number; i++ )
	{
		clearValue( E->stack[--E->stackPointer] );
	}
}

//------------------------------------------------------------------------------
int doAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	
	if ( to->type != CTYPE_OFFSTACK_REFERENCE )
	{
		// assigning to the stack is undefined
		Log("Assigning to the stack is undefined");
		return g_Callisto_lastErr = CE_AssignedValueToStack;
	}

	to = to->v;

	if ( from->type == CTYPE_OFFSTACK_REFERENCE )
	{
		from = from->v;
		
		D_ASSIGN(Log("offstack to offstack type [%d]", from->type));
		
		valueMirror( to, from );
	}
	else
	{
		D_ASSIGN(Log("assign from stack->offstack, copy data from type [%d]", from->type));

		// can move the value off the stack, by definition it will not
		// be used directly from there

		clearValue( *to );

		memcpy( (char *)to, (char *)from, sizeof(Value) );
		memset( (char *)from, 0, sizeof(Value) );
	}

	return 0;
}

//------------------------------------------------------------------------------
inline Value* getStackValue( Callisto_ExecutionContext* E, int offset )
{
	Value* value = E->stack + (E->stackPointer - offset);
	return (value->type == CTYPE_OFFSTACK_REFERENCE) ? value->v : value;
}

#define BINARY_ARG_SETUP \
Value* from = E->stack + (E->stackPointer - operand1); \
Value* to = E->stack + (E->stackPointer - operand2); \
Value* destination = to; \
if ( from->type == CTYPE_OFFSTACK_REFERENCE ) { D_ARG_SETUP(Log("from is ref")); from = from->v; } \
if ( to->type == CTYPE_OFFSTACK_REFERENCE ) { D_ARG_SETUP(Log("to is ref")); to = to->v; } \
D_ARG_SETUP(Log("Binary arg setup from operand1[%d] to operand2[%d]", E->stackPointer - operand2, E->stackPointer - operand1)); \

#define UNARY_ARG_SETUP \
Value* value = E->stack + (E->stackPointer - operand); \
Value* destination = value; \
if ( value->type == CTYPE_OFFSTACK_REFERENCE ) value = value->v; \
D_ARG_SETUP(Log("Unary arg setup from operand[%d]", E->stackPointer - operand)); \

//------------------------------------------------------------------------------
int doAddAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;
	
	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 += from->i64;
			D_OPERATOR(Log("addAssign <1> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d += from->i64;
			D_OPERATOR(Log("addAssign <2> %g", to->d));
		}
		else
		{
			D_OPERATOR(Log("doAddAssign fail<1>"));
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 += (int64_t)from->d;
			D_OPERATOR(Log("addAssign <3> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d += from->d;
			D_OPERATOR(Log("addAssign <4> %g", to->i64));
		}
		else
		{
			D_OPERATOR(Log("doAddAssign fail<2>"));
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_STRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			to->refCstr->item += from->refCstr->item;
			D_OPERATOR(Log("addAssign <5> [%s]", to->refCstr->item.c_str()));
		}
		else if ( to->type == CTYPE_WSTRING )
		{
			to->refWstr->item += Wstr(from->refCstr->item);
			D_OPERATOR(Log("addAssign <6> [%S]", to->refWstr->item.c_str()));
		}
		else
		{
			D_OPERATOR(Log("doAddAssign fail<3> [%d]", to->type));
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_WSTRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			to->refCstr->item += Cstr(from->refWstr->item);
			D_OPERATOR(Log("addAssign <7> [%s]", to->refCstr->item.c_str()));
		}
		else if ( to->type == CTYPE_WSTRING )
		{
			to->refWstr->item += from->refWstr->item;
			D_OPERATOR(Log("addAssign <6> [%S]", to->refWstr->item.c_str()));
		}
		else
		{
			D_OPERATOR(Log("doAddAssign fail<4>"));
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		D_OPERATOR(Log("doAddAssign fail<5>"));
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doSubtractAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 -= from->i64;
			D_OPERATOR(Log("subtractAssign <1> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d -= from->i64;
			D_OPERATOR(Log("subtractAssign <2> %g", to->d));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double d = to->i64 + from->d;
			to->d = d;
			to->type = CTYPE_FLOAT;
			
			D_OPERATOR(Log("subtractAssign <3> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d -= from->d;
			D_OPERATOR(Log("subtractAssign <4> %g", to->i64));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doModAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		to->i64 %= from->i64;
		D_OPERATOR(Log("modAssign <1> %lld", to->i64));
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doDivideAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 /= from->i64;
			D_OPERATOR(Log("divideAssign <1> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d /= from->i64;
			D_OPERATOR(Log("divideAssign <2> %g", to->d));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 /= (int64_t)from->d;
			D_OPERATOR(Log("divideAssign <3> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d /= from->d;
			D_OPERATOR(Log("divideAssign <4> %g", to->i64));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doMultiplyAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 *= from->i64;
			D_OPERATOR(Log("multiplyAssign <1> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d *= from->i64;
			D_OPERATOR(Log("multiplyAssign <2> %g", to->d));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			to->i64 *= (int64_t)from->d;
			D_OPERATOR(Log("multiplyAssign <3> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d *= from->d;
			D_OPERATOR(Log("multiplyAssign <4> %g", to->i64));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCompareEQ( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result = -1;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		result = (from->i64 == to->i64) ? 1 : 0;
	}
	else if ( from->type == CTYPE_NULL )
	{
		result = (to->type == CTYPE_NULL) ? 1 : 0;
	}
	else if ( to->type == CTYPE_NULL )
	{
		result = (from->type == CTYPE_NULL) ? 1 : 0;
	}
	else if ( from->type == CTYPE_FLOAT && to->type == CTYPE_FLOAT )
	{
		result = (from->d == to->d) ? 1 : 0;
	}
	else if ( from->type == CTYPE_INT && to->type == CTYPE_FLOAT )
	{
		result = (from->i64 == to->d) ? 1 : 0;
	}
	else if ( from->type == CTYPE_FLOAT && to->type == CTYPE_INT )
	{
		result = (from->d == to->i64) ? 1 : 0;
	}
	else if ( from->type == CTYPE_STRING && to->type == CTYPE_STRING )
	{
		result = from->refCstr->item == to->refCstr->item;
	}
	else if ( from->type == CTYPE_WSTRING && to->type == CTYPE_WSTRING )
	{
		result = from->refWstr->item == to->refWstr->item;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	clearValue( *destination );
	destination->type = CTYPE_INT;
	destination->i64 = result;

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCompareGT( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result = -1;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		result = (to->i64 > from->i64) ? 1 : 0;
	}
	else if ( from->type == CTYPE_FLOAT && to->type == CTYPE_FLOAT )
	{
		result = (to->d > from->d) ? 1 : 0;
	}
	else if ( from->type == CTYPE_INT && to->type == CTYPE_FLOAT )
	{
		result = (to->d > from->i64) ? 1 : 0;
	}
	else if ( from->type == CTYPE_FLOAT && to->type == CTYPE_INT )
	{
		result = (to->i64 > from->d) ? 1 : 0;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	clearValue( *destination );
	destination->type = CTYPE_INT;
	destination->i64 = result;

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCompareLT( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result = -1;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		result = (to->i64 < from->i64) ? 1 : 0;
	}
	else if ( from->type == CTYPE_FLOAT && to->type == CTYPE_FLOAT )
	{
		result = (to->d < from->d) ? 1 : 0;
	}
	else if ( from->type == CTYPE_INT && to->type == CTYPE_FLOAT )
	{
		result = (to->d < from->i64) ? 1 : 0;
	}
	else if ( from->type == CTYPE_FLOAT && to->type == CTYPE_INT )
	{
		result = (to->i64 < from->d) ? 1 : 0;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	clearValue( *destination );
	destination->type = CTYPE_INT;
	destination->i64 = result;

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doLogicalAnd( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( (from->type == CTYPE_INT
		  || from->type == CTYPE_FLOAT
		  || from->type == CTYPE_NULL)
		 &&
		 (to->type == CTYPE_INT
		  || to->type == CTYPE_FLOAT
		  || to->type == CTYPE_NULL)
	   )
	{
		int result = (from->i64 && to->i64) ? 1 : 0;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		return CE_NoError;
	}

	return g_Callisto_lastErr = CE_IncompatibleDataTypes;
}

//------------------------------------------------------------------------------
int doLogicalOr( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( (from->type == CTYPE_INT
		  || from->type == CTYPE_FLOAT
		  || from->type == CTYPE_NULL)
		 &&
		 (to->type == CTYPE_INT
		  || to->type == CTYPE_FLOAT
		  || to->type == CTYPE_NULL)
	   )
	{
		int result = (from->i64 || to->i64) ? 1 : 0;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		return CE_NoError;
	}

	return g_Callisto_lastErr = CE_IncompatibleDataTypes;
}

//------------------------------------------------------------------------------
int doMemberAccess( Callisto_ExecutionContext* E, const unsigned int key )
{
	Value* source = E->stack + (E->stackPointer - 1);
	Value* destination = source;
	if ( source->type == CTYPE_OFFSTACK_REFERENCE )
	{
		source = source->v;
	}

	if ( source->type != CTYPE_UNIT )
	{
		if ( destination->type != CTYPE_OFFSTACK_REFERENCE )
		{
			return g_Callisto_lastErr = CE_CannotOperateOnLiteral;
		}

		CHashTable<MetaTableEntry>* table = E->callisto->globalUnit.metaTableByType.get( source->type );
		if ( !table )
		{
			return g_Callisto_lastErr = CE_MemberNotFound;
		}

		destination->metaTable = table->get( key );
		if ( !destination->metaTable )
		{
			return g_Callisto_lastErr = CE_MemberNotFound;
		}
	}
	else
	{
		Value* member = source->refLocalSpace->item.get( key );
		if ( !member )
		{
			ChildUnit* c = source->u->childUnits.get( key );
			if ( c )
			{
				D_MEMBERACCESS(Log("Member not found, but child unit [0x%08X] matched", key));

				if ( !c->child )
				{
					c->child = E->callisto->unitList.get( c->key );
					if ( !c->child )
					{
						D_MEMBERACCESS(Log("cached unit pointer", key));
						return g_Callisto_lastErr = CE_MemberNotFound;
					}
					else
					{
						D_MEMBERACCESS(Log("unit pointer cached valid"));
					}
				}

				RefCountedVariable<CLinkHash<Value>>* r = source->refLocalSpace->getReference();

				clearValue( *destination );

				destination->type = CTYPE_UNIT;
				destination->u = c->child;
				destination->refLocalSpace = r;

				return CE_NoError;
			}
			else
			{
				D_MEMBERACCESS(Log("Member not found and no child matched [0x%08X], creating", key));
				member = source->refLocalSpace->item.add( key );
			}
		}

		clearValue( *destination );
		destination->type = CTYPE_OFFSTACK_REFERENCE;
		destination->v = member;
	}
	
	return CE_NoError;
}

//------------------------------------------------------------------------------
int doLogicalNot( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;

	if ( (value->type == CTYPE_INT
		  || value->type == CTYPE_FLOAT
		  || value->type == CTYPE_NULL) )
	{
		int result = value->i64 ? 0 : 1;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		return CE_NoError;
	}

	return g_Callisto_lastErr = CE_IncompatibleDataTypes;
}

//------------------------------------------------------------------------------
int doBitwiseNot( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;

	if ( value->type == CTYPE_INT )
	{
		int64_t result = ~value->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		return CE_NoError;
	}

	return g_Callisto_lastErr = CE_IncompatibleDataTypes;
}

//------------------------------------------------------------------------------
int doPostIncrement( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type != CTYPE_OFFSTACK_REFERENCE )
	{
		D_ARG_SETUP(Log("OFFSTACK"));
		return g_Callisto_lastErr = CE_StackedPostOperatorsUnsupported;
	}

	value = value->v;
	D_ARG_SETUP(Log("Unary arg setup from operand[%d]", E->stackPointer - operand));

	if ( value->type == CTYPE_INT )
	{
		int64_t result = value->i64++;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		D_OPERATOR(Log("doPostIncrement<1> [%lld]", destination->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		double result = value->d++;
		clearValue( *destination );
		destination->type = CTYPE_FLOAT;
		destination->d = result;
		D_OPERATOR(Log("doPostIncrement<2> [%g]", destination->d));
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}
	
	return CE_NoError;
}

//------------------------------------------------------------------------------
int doPostDecrement( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type != CTYPE_OFFSTACK_REFERENCE )
	{
		D_ARG_SETUP(Log("OFFSTACK"));
		return g_Callisto_lastErr = CE_StackedPostOperatorsUnsupported;
	}

	value = value->v;
	D_ARG_SETUP(Log("Unary arg setup from operand[%d]", E->stackPointer - operand));

	if ( value->type == CTYPE_INT )
	{
		int64_t result = value->i64--;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		D_OPERATOR(Log("doPostDecrement<1> [%lld]", destination->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		double result = value->d--;
		clearValue( *destination );
		destination->type = CTYPE_FLOAT;
		destination->d = result;
		D_OPERATOR(Log("doPostDecrement<2> [%g]", destination->d));
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doPreIncrement( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;

	destination = destination;
	
	if ( value->type == CTYPE_INT )
	{
		++value->i64;
		D_OPERATOR(Log("doPreIncrement<1> [%lld]", value->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		++value->d;
		D_OPERATOR(Log("doPreIncrement<2> [%g]", value->d));
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doPreDecrement( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;

	destination = destination;

	if ( value->type == CTYPE_INT )
	{
		--value->i64;
		D_OPERATOR(Log("doPreDecrement<1> [%lld]", value->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		--value->d;
		D_OPERATOR(Log("doPreDecrement<2> [%g]", value->d));
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doNegate( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_OFFSTACK_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_INT )
	{
		int64_t result = -value->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
		D_OPERATOR(Log("negate<1> [%lld]", destination->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		double result = -value->d;
		clearValue( *destination );
		destination->type = CTYPE_FLOAT;
		destination->d = result;
		D_OPERATOR(Log("negate<2> [%g]", destination->d));
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCopyValue( Callisto_ExecutionContext* E, const int operand )
{
	D_COPY(Log("copy in [%d]---------", operand));
	D_COPY(dumpStack(E));
	D_COPY(Log("---------------------"));

	valueDeepCopy( E->stack + (E->stackPointer - operand), getStackValue(E, operand) );

	D_COPY(Log("copy out-------------"));
	D_COPY(dumpStack(E));
	D_COPY(Log("---------------------"));

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCoerceToInt( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_OFFSTACK_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_FLOAT )
	{
		int64_t result = (int64_t)value->d;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else if ( value->type == CTYPE_STRING )
	{
		int64_t result = strtoull( value->refCstr->item, 0, 10 );
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else if ( value->type == CTYPE_WSTRING )
	{
		int64_t result = wcstoull( value->refWstr->item, 0, 10 );
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else if ( value->type != CTYPE_INT )
	{
		clearValue( *destination );
	}
	
	return CE_NoError;
	
}

//------------------------------------------------------------------------------
int doCoerceToFloat( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_OFFSTACK_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_INT )
	{
		double result = (double)value->i64;
		clearValue( *destination );
		destination->type = CTYPE_FLOAT;
		destination->d = result;
	}
	else if ( value->type == CTYPE_STRING )
	{
		double result = strtod( value->refCstr->item, 0 );
		clearValue( *destination );
		destination->type = CTYPE_FLOAT;
		destination->d = result;
	}
	else if ( value->type == CTYPE_WSTRING )
	{
		double result = wcstod( value->refWstr->item, 0 );
		clearValue( *destination );
		destination->type = CTYPE_FLOAT;
		destination->d = result;
	}
	else if ( value->type != CTYPE_FLOAT )
	{
		clearValue( *destination );
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCoerceToString( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_OFFSTACK_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_INT )
	{
		RefCountedVariable<Cstr>* result = new RefCountedVariable<Cstr>();
		result->item.format( "%lld", value->i64 );
		
		clearValue( *destination );
		destination->type = CTYPE_STRING;
		destination->refCstr = result;
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		RefCountedVariable<Cstr>* result = new RefCountedVariable<Cstr>();
		result->item.format( "%g", value->d );
		
		clearValue( *destination );
		destination->type = CTYPE_STRING;
		destination->refCstr = result;
	}
	else if ( value->type == CTYPE_WSTRING )
	{
		RefCountedVariable<Cstr>* result = new RefCountedVariable<Cstr>();
		result->item = value->refWstr->item;

		clearValue( *destination );
		destination->type = CTYPE_STRING;
		destination->refCstr = result;
	}
	else if ( value->type != CTYPE_STRING )
	{
		clearValue( *destination );
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doCoerceToWString( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_OFFSTACK_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_INT )
	{
		RefCountedVariable<Wstr>* result = new RefCountedVariable<Wstr>();
		result->item.format( L"%lld", value->i64 );

		clearValue( *destination );
		destination->type = CTYPE_WSTRING;
		destination->refWstr = result;
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		RefCountedVariable<Wstr>* result = new RefCountedVariable<Wstr>();
		result->item.format( L"%g", value->d );

		clearValue( *destination );
		destination->type = CTYPE_WSTRING;
		destination->refWstr = result;
	}
	else if ( value->type == CTYPE_STRING )
	{
		RefCountedVariable<Wstr>* result = new RefCountedVariable<Wstr>();
		result->item = value->refCstr->item;

		clearValue( *destination );
		destination->type = CTYPE_WSTRING;
		destination->refWstr = result;
	}
	else if ( value->type != CTYPE_WSTRING )
	{
		clearValue( *destination );
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doSwitch( Callisto_ExecutionContext* E )
{
	Value* value = E->stack + (E->stackPointer - 1);
	unsigned int hash = (value->type == CTYPE_OFFSTACK_REFERENCE) ? value->v->getHash() : value->getHash();

	int offset = E->pos + ntohl(*(int *)E->callisto->text.c_str( E->pos )); // get the jump table location
	E->pos += 4;
	E->pos += ntohl(*(int *)E->callisto->text.c_str( E->pos )); // preload the break target

	const char* jumpTable = E->callisto->text.c_str( offset );
	const char* base = jumpTable - offset;
	
	unsigned int cases = ntohl(*(unsigned int *)jumpTable);
	jumpTable += 4;
	char defaultExists = *jumpTable;
	++jumpTable;

	unsigned char c = 0;
	for( ; c<cases; c++ )
	{
		if ( hash == ntohl(*(unsigned int *)jumpTable) )
		{
			jumpTable += 4;
			E->pos = (unsigned int)(ntohl(*(int *)jumpTable) + (jumpTable - base));
			break;
		}

		jumpTable += 8;
	}

	if ( (c >= cases) && defaultExists )
	{
		// default is always packed last, just back up and jump to it
		jumpTable -= 4;
		E->pos = (unsigned int)(ntohl(*(int *)jumpTable) + (jumpTable - base));
	}

	popOne( E );

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doPushIterator( Callisto_ExecutionContext* E )
{
	Value* value = E->stack + (E->stackPointer - 1);
	Value* destination = value;
	if ( value->type == CTYPE_OFFSTACK_REFERENCE )
	{
		value = value->v;
	}

	switch( value->type )
	{
		case CTYPE_STRING:
		{
			RefCountedVariable<Cstr>* ref = value->refCstr->getReference();
			clearValue( *destination );
			destination->type = CTYPE_STRING_ITERATOR;
			destination->i64 = 0;
			destination->refCstr = ref;
			break;
		}

		case CTYPE_WSTRING:
		{
			RefCountedVariable<Wstr>* ref = value->refWstr->getReference();
			clearValue( *destination );
			destination->type = CTYPE_WSTRING_ITERATOR;
			destination->i64 = 0;
			destination->refWstr = ref;
			break;
		}

		case CTYPE_TABLE:
		{
			RefCountedVariable<CLinkHash<Value>>* ref = value->refLocalSpace->getReference();
			clearValue( *destination );
			destination->type = CTYPE_HASH_ITERATOR;
			destination->hashIterator = new CLinkHash<Value>::Iterator(ref->item);
			destination->refLocalSpace = ref;
			break;
		}

		case CTYPE_ARRAY:
		{
			RefCountedVariable<Carray>* ref = value->refArray->getReference();
			clearValue( *destination );
			destination->type = CTYPE_ARRAY_ITERATOR;
			destination->i64 = 0;
			destination->refArray = ref;
			break;
		}

		default:
		{
			return g_Callisto_lastErr = CE_TypeCannotBeIterated;
		}
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doIteratorGetNextKeyValueOrBranch( Callisto_ExecutionContext* E )
{
	unsigned int keyKey = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
	E->pos += 4;
	unsigned int valueKey = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
	E->pos += 4;

	D_ITERATE(Log("key[%s] value[%s]", formatSymbol(E->callisto, keyKey).c_str(),formatSymbol(E->callisto, valueKey).c_str()));

	Value* key = E->currentUnit->refLocalSpace->item.get( keyKey );
	if ( !key )
	{
		D_ITERATE(Log("<1> first encountered key [%s] adding to space", formatSymbol(E->callisto, keyKey).c_str()));
		key = E->currentUnit->refLocalSpace->item.add( keyKey );
	}
	else
	{
		D_ITERATE(Log("<1> loading key [%s]", formatSymbol(E->callisto, keyKey).c_str()));
		clearValue( *key );
	}

	Value* val = E->currentUnit->refLocalSpace->item.get( valueKey );
	if ( !val )
	{
		D_ITERATE(Log("<1> first encountered val [%s] adding to space", formatSymbol(E->callisto, valueKey).c_str()));
		val = E->currentUnit->refLocalSpace->item.add( valueKey );
	}
	else
	{
		D_ITERATE(Log("<1> loading val [%s]", formatSymbol(E->callisto, valueKey).c_str()));
		clearValue( *val );
	}
	
	Value* list = E->stack + (E->stackPointer - 1);
	if ( list->type == CTYPE_OFFSTACK_REFERENCE )
	{
		list = list->v;
	}

	switch( list->type )
	{
		case CTYPE_ARRAY_ITERATOR:
		{
			if ( list->i64 < list->refArray->item.count() )
			{
				valueMirror( val, &(list->refArray->item[(unsigned int)list->i64]) );
				key->type = CTYPE_INT;
				key->i64 = list->i64++;
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}
		
		case CTYPE_HASH_ITERATOR:
		{
			Value* V = list->hashIterator->getNext();
			if ( V )
			{
				valueMirror( val, V );
				key->type = CTYPE_INT;
				key->i64 = list->hashIterator->getCurrentKey();
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}
		
		case CTYPE_STRING_ITERATOR:
		{
			if ( list->i64 < list->refCstr->item.size() )
			{
				val->type = CTYPE_INT;
				val->i64 = *list->refCstr->item.c_str( (unsigned int)list->i64 );
				key->type = CTYPE_INT;
				key->i64 = list->i64++;
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}
		
		case CTYPE_WSTRING_ITERATOR:
		{
			if ( list->i64 < list->refWstr->item.size() )
			{
				val->type = CTYPE_INT;
				val->i64 = *list->refWstr->item.c_str( (unsigned int)list->i64 );
				key->type = CTYPE_INT;
				key->i64 = list->i64++;
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}

		default:
		{
			return g_Callisto_lastErr = CE_TypeCannotBeIterated;
		}
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doIteratorGetNextValueOrBranch( Callisto_ExecutionContext* E )
{
	unsigned int valueKey = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
	E->pos += 4;

	Value* val = E->currentUnit->refLocalSpace->item.get( valueKey );
	if ( !val )
	{
		D_ITERATE(Log("<2> first encountered val [%s] adding to space", formatSymbol(E->callisto, valueKey).c_str()));
		val = E->currentUnit->refLocalSpace->item.add( valueKey );
	}
	else
	{
		D_ITERATE(Log("<2> loading val [%s]", formatSymbol(E->callisto, valueKey).c_str()));
		clearValue( *val );
	}

	Value* list = E->stack + (E->stackPointer - 1);
	if ( list->type == CTYPE_OFFSTACK_REFERENCE )
	{
		list = list->v;
	}

	switch( list->type )
	{
		case CTYPE_ARRAY_ITERATOR:
		{
			if ( list->i64 < list->refArray->item.count() )
			{
				valueMirror( val, &(list->refArray->item[(unsigned int)list->i64]) );
				++list->i64;
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}

		case CTYPE_HASH_ITERATOR:
		{
			Value* V = list->hashIterator->getNext();
			if ( V )
			{
				valueMirror( val, V );
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}

		case CTYPE_STRING_ITERATOR:
		{
			if ( list->i64 < list->refCstr->item.size() )
			{
				val->type = CTYPE_INT;
				val->i64 = *list->refCstr->item.c_str( (unsigned int)list->i64++ );
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}

		case CTYPE_WSTRING_ITERATOR:
		{
			if ( list->i64 < list->refWstr->item.size() )
			{
				val->type = CTYPE_INT;
				val->i64 = *list->refWstr->item.c_str( (unsigned int)list->i64++ );
				E->pos += 4; // skip
			}
			else
			{
				E->pos += ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos )); // break
			}
			break;
		}

		default:
		{
			return g_Callisto_lastErr = CE_TypeCannotBeIterated;
		}
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryAddition( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 + from->i64;
			clearValue( *destination );
			destination->type = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doBinaryAddition<1> [%lld]", destination->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d + from->i64;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryAddition<2> [%g]", destination->d));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 + from->d;
			clearValue( *destination );
			destination->type = CTYPE_INT;
			destination->i64 = (int64_t)result;
			D_OPERATOR(Log("doBinaryAddition<3> [%lld]", destination->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d + from->d;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryAddition<4> [%g", destination->d));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_STRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			RefCountedVariable<Cstr>* result = new RefCountedVariable<Cstr>();
			result->item = to->refCstr->item + from->refCstr->item;
			
			clearValue( *destination );
			destination->type = CTYPE_STRING;
			destination->refCstr = result;
			D_OPERATOR(Log("doBinaryAddition<5> [%s]", result->item.c_str()));
		}
		else if ( to->type == CTYPE_WSTRING )
		{
			RefCountedVariable<Wstr>* result = new RefCountedVariable<Wstr>();
			result->item = to->refWstr->item + Wstr(from->refCstr->item);

			clearValue( *destination );
			destination->type = CTYPE_WSTRING;
			destination->refWstr = result;
			D_OPERATOR(Log("doBinaryAddition<6> [%S]", result->item.c_str()));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_WSTRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			RefCountedVariable<Cstr>* result = new RefCountedVariable<Cstr>();
			result->item = to->refCstr->item + Cstr(from->refWstr->item);

			clearValue( *destination );
			destination->type = CTYPE_STRING;
			destination->refCstr = result;
			D_OPERATOR(Log("doBinaryAddition<5> [%s]", result->item.c_str()));
		}
		else if ( to->type == CTYPE_WSTRING )
		{
			RefCountedVariable<Wstr>* result = new RefCountedVariable<Wstr>();
			result->item = to->refWstr->item + from->refWstr->item;

			clearValue( *destination );
			destination->type = CTYPE_WSTRING;
			destination->refWstr = result;
			D_OPERATOR(Log("doBinaryAddition<6> [%S]", result->item.c_str()));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinarySubtraction( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 - from->i64;
			clearValue( *destination );
			destination->type = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doBinarySubtraction<1> [%lld]", destination->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d - from->i64;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinarySubtraction<2> [%g]", result));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 - from->d;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinarySubtraction<3> [%g]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d - from->d;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinarySubtraction<4> [%g]", result));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryMultiplication( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	
	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 * from->i64;
			clearValue( *destination );
			destination->type = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doBinaryMultiplication<1> [%lld]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d * from->i64;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<2> [%g]", result));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 * from->d;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<3> [%g]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d * from->d;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<4> [%g]", result));
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryDivision( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 / from->i64;
			clearValue( *destination );
			destination->type = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d / from->i64;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 / from->d;
			clearValue( *destination );
			destination->type = CTYPE_INT;
			destination->i64 = (int64_t)result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d / from->d;
			clearValue( *destination );
			destination->type = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			return g_Callisto_lastErr = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryMod( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 % from->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryBitwiseRightShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 >> from->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryBitwiseLeftShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 << from->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryBitwiseOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	
	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 | from->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}
	
	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryBitwiseAND( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 & from->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doBinaryBitwiseXOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 ^ from->i64;
		clearValue( *destination );
		destination->type = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 |= from->i64;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doXORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 ^= from->i64;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doANDAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 &= from->i64;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doRightShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 >>= from->i64;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int doLeftShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 <<= from->i64;
	}
	else
	{
		return g_Callisto_lastErr = CE_IncompatibleDataTypes;
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
void ensureStack( Callisto_ExecutionContext* E, int to )
{
	if ( to < E->stackSize )
	{
		return; // ooookay
	}

	// grow by 50% until it's large enough
	int oldStackSize = E->stackSize;
	Value* old = E->stack;

	do
	{
		E->stackSize = ((E->stackSize * 3) / 2) + 1;
	} while( E->stackSize < to );
	
//	printf( ">>>>>>>>>>>>>>>>>>>>>>>>>>> growing stack to [%d] from[%d]\n", E->stackSize, to);
	
	E->stack = new Value[ E->stackSize ];

	memcpy( E->stack, old, oldStackSize * sizeof(Value) ); // copy all pointers and everything
	memset( old, 0, oldStackSize * sizeof(Value) ); // .. so make sure destructors are harmless 
	delete[] old;
}

//------------------------------------------------------------------------------
void valueSwap( Value* v1, Value* v2 )
{
	int t = v1->type;
	v1->type = v2->type;
	v2->type = t;

	int64_t t1 = v1->ref1;
	v1->ref1 = v2->ref1;
	v2->ref1 = t1;

	void* t2 = v1->ref2;
	v1->ref2 = v2->ref2;
	v2->ref2 = t2;
}

//------------------------------------------------------------------------------
void valueMove( Value* target, Value* source )
{
	clearValue( *target );
	target->type = source->type;
	target->ref1 = source->ref1;
	target->ref2 = source->ref2;

	source->type = CTYPE_NULL;
	source->ref1 = 0;
}

//------------------------------------------------------------------------------
void valueDeepCopy( Value* target, Value* source )
{
	clearValue( *target );
	Value* from = source;
	if (from->type == CTYPE_OFFSTACK_REFERENCE)
	{
		from = from->v;
	}
	
	switch( (target->type = from->type) )
	{
		case CTYPE_NULL:
			D_DEEPCOPY(Log("deep copy <null>"));
			break;

		case CTYPE_INT:
		case CTYPE_FLOAT:
		{
			D_DEEPCOPY(Log("deep copy value [%lld]:[%g]", source->i64, source->d));
			target->ref1 = from->ref1;
			break;
		}

		case CTYPE_STRING:
		{
			target->refCstr = new RefCountedVariable<Cstr>();
			target->refCstr->item = from->refCstr->item;
			D_DEEPCOPY(Log("deep copy string [%s]", target->refCstr->item.c_str()));
			break;
		}
		
		case CTYPE_WSTRING:
		{
			target->refWstr = new RefCountedVariable<Wstr>();
			target->refWstr->item = from->refWstr->item;
			D_DEEPCOPY(Log("deep copy wide string [%S]", target->refWstr->item.c_str()));
			break;
		}

		case CTYPE_UNIT:
		case CTYPE_TABLE:
		{
			D_DEEPCOPY(Log("deep copy <table>"));
			
			target->u = from->u;
			target->refLocalSpace = new RefCountedVariable<CLinkHash<Value>>();
			for( Value* V = from->refLocalSpace->item.getFirst(); V; V = from->refLocalSpace->item.getNext() )
			{
				D_DEEPCOPY(Log("key[0x%08X]", from->refLocalSpace->item.getCurrentKey() ));

				Value* T = target->refLocalSpace->item.add( from->refLocalSpace->item.getCurrentKey() );
				valueDeepCopy( T, V );
			}
			
			break;
		}

		case CTYPE_ARRAY:
		{
			D_DEEPCOPY(Log("deep copy <array>"));

			target->refArray = new RefCountedVariable<Carray>();
			for( unsigned int i=0; i<from->refArray->item.count(); ++i )
			{
				D_DEEPCOPY(Log("index[%d]", i));
				valueDeepCopy( &target->refArray->item[i], &from->refArray->item[i] );
			}

			break;
		}

		case CTYPE_OFFSTACK_REFERENCE:
		{
			target->v = source->v;
			break;
		}
		
		default:
		{
			D_DEEPCOPY(Log("type[%s] unhandled", formatType(target->type).c_str()));
			break;
		}
	}
}

//------------------------------------------------------------------------------
void addArrayElement( Callisto_ExecutionContext* E, unsigned int index )
{
	Value* from = getStackValue( E, 1 );
	Value* to = getStackValue( E, 2 );

	if ( to->type != CTYPE_ARRAY )
	{
		clearValue( *to );
		to->type = CTYPE_ARRAY;
		to->refArray = new RefCountedVariable<Carray>();
	}

	valueMirror( &(to->refArray->item[index]), from );

	popOne( E );
}

//------------------------------------------------------------------------------
int addTableElement( Callisto_ExecutionContext* E )
{
	Value* value = getStackValue( E, 1 );
	Value* key = getStackValue( E, 2 );
	Value* table = getStackValue( E, 3 );

	D_ADD_TABLE_ELEMENT(Log("Stack before---------"));
	D_ADD_TABLE_ELEMENT(dumpStack(E));
	D_ADD_TABLE_ELEMENT(Log("---------------------"));
	
	if ( !(key->type & BIT_CAN_HASH) )
	{
		return g_Callisto_lastErr = CE_UnhashableDataType;
	}
	
	if ( table->type != CTYPE_TABLE )
	{
		clearValue( *table );
		table->type = CTYPE_TABLE;
		table->refLocalSpace = new RefCountedVariable<CLinkHash<Value>>();
	}

	unsigned int hash = key->getHash();
	Value* V = table->refLocalSpace->item.get( hash );
	if ( !V )
	{
		V = table->refLocalSpace->item.add( hash );
	}

	valueMirror( V, value );
	
	popOne( E );
	popOne( E );

	D_ADD_TABLE_ELEMENT(Log("Stack after----------"));
	D_ADD_TABLE_ELEMENT(dumpStack(E));
	D_ADD_TABLE_ELEMENT(Log("---------------------"));

	return CE_NoError;
}

//------------------------------------------------------------------------------
void dereferenceFromTable( Callisto_ExecutionContext* E )
{
	D_DEREFERENCE(Log("Derefernece stack IN--------"));
	D_DEREFERENCE(dumpStack(E));
	D_DEREFERENCE(Log("----------------------------"));
	
	Value* key = getStackValue( E, 1 );
	Value* table = getStackValue( E, 2 );
	Value* destination = &( E->stack[E->stackPointer - 2] );

	// if it's not a table, make it one, dereferencing it morphs it
	if ( (table->type != CTYPE_ARRAY) && (table->type != CTYPE_TABLE) )
	{
		D_DEREFERENCE(Log("trying to dereference a non table/array"));
		popOne(E);
		return;
	}
	
	if ( table->type == CTYPE_ARRAY )
	{
		if ( key->type == CTYPE_INT )
		{
			Value* V = &(table->refArray->item[(unsigned int)key->i64]);
			clearValue( *destination );
			destination->type = CTYPE_OFFSTACK_REFERENCE;
			destination->v = V;
		}
		else
		{
			clearValue( *destination );
		}
	}
	else if ( key->type & BIT_CAN_HASH )
	{
		Value *V = table->refLocalSpace->item.get( key->getHash() );
		clearValue( *destination );
		if ( V )
		{
			destination->type = CTYPE_OFFSTACK_REFERENCE;
			destination->v = V;
		}
	}
	else
	{
		clearValue( *destination );
	}

	popOne( E );

	D_DEREFERENCE(Log("Derefrence stack OUT-------"));
	D_DEREFERENCE(dumpStack(E));
	D_DEREFERENCE(Log("----------------------------"));
}

