#include "util.h"
#include "asciidump.hpp"
#include "arch.h"

#include <ctype.h>

#define D_JUMP(a) //a
#define D_ENUM(a) //a
#define D_CALLISTO_PARSE(a) //a
#define D_PARSE(a) //a
#define D_PARSE_EXPRESSION(a) //a
#define D_LINK(a) //a
#define D_ADD_SYMBOL(a) //a
#define D_RESOLVE(a) //a
#define D_FORLOOP(a) //a
#define D_FOREACHLOOP(a) //a
#define D_PARSE_UNIT(a) //a
#define D_PARSE_ARRAY(a) //a
#define D_PARSE_TABLE(a) //a
#define D_IFCHAIN(a) // a
#define D_PARSE_NEW(a) //a
#define D_SWITCH(a) //a

extern CLog Log;
unsigned int Compilation::jumpIndex = 1;

//------------------------------------------------------------------------------
void CCloggingCallback( const char* msg )
{
	printf( "%s\n", msg ); // %s important so '%' characters are not interpretted by the printf
}

bool processSwitchContexts( Compilation& target, UnitEntry& unit );
bool parseBlock( Compilation& target, UnitEntry& unit, SwitchContext *sc =0, Cstr* preload =0 );
bool link( Compilation& target );
bool parseExpression( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value );
bool parseConstExpression( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value );

unsigned int flatHash = 0;
CLinkHash<unsigned int> hashMap;

//------------------------------------------------------------------------------
unsigned int hash( Cstr& name, Compilation& target, bool alias =true )
{
	unsigned int key = hashStr( name );
/*
	if ( alias )
	{
		if ( !hashMap.get(key) )
		{
			*hashMap.add(key) = ++flatHash;
		}
		key = *hashMap.get(key);
	}
*/	
	if ( !target.symbolTable.get( key ) )
	{
		*target.symbolTable.add( key ) = name;
	}
	
	return key;
}

//------------------------------------------------------------------------------
char getParents( Cstr* parentBlock, Compilation& target, UnitEntry& unit, CLinkHash<bool>& parentsSeen )
{
	char parentCount = 0;

	CLinkList<Cstr>::Iterator iter( unit.parents );
	CLinkList<UnitEntry>::Iterator uiter( target.units );
	
	for( Cstr* parent = iter.getFirst(); parent; parent = iter.getNext() )
	{
		for( UnitEntry *U = uiter.getFirst(); U; U = uiter.getNext() )
		{
			if ( U->name == *parent )
			{
				parentCount += getParents( parentBlock, target, *U, parentsSeen );
			}
		}

		unsigned int key = hash( *parent, target );
		
		if ( !parentsSeen.get(key) )
		{
			parentsSeen.add( key );
			
			D_CALLISTO_PARSE(Log("Parent [%s] found", parent->c_str()));
			++parentCount;
			key = htonl(key);
			parentBlock->append( (char*)&key, 4 );
		}
	}

	return parentCount;
}

//------------------------------------------------------------------------------
int Callisto_parse( const char* data, const int size, char** out, unsigned int* outLen )
{
	if ( O_LAST > 128 )
	{
		Log("OPCODE ERROR [%d]", O_LAST);
		return -1000;
	}
		
	Cstr output;
	
	Log.setCallback( CCloggingCallback );

	Compilation comp;
	
	comp.source.set( data, size );
	comp.pos = 0;
	comp.output = &output;
	comp.units.clear();
	comp.currentUnit = 0;
	
	UnitEntry *E = comp.units.add();
	E->name = "_";
	E->baseOffset = 0;

	comp.space = &comp.defaultSpace;

	bool ret = parseBlock( comp, *E );

	if ( ret )
	{
		E->program += (char)O_LiteralNull;
		E->program += (char)O_Return;
		ret = processSwitchContexts( comp, *E );
	}

	if ( !ret
		 || !link( comp )
		 || comp.err.size() )
	{
		int onChar = 0;
		int onLine = 1;
		Cstr line;

		for( unsigned int p = 0; p<comp.source.size() && comp.source[p] != '\n'; p++ ) { line += comp.source[p]; }

		for( unsigned int i=0; i<comp.pos; i++ )
		{
			if ( comp.source[i] == '\n' )
			{
				onLine++;
				onChar = 0;

				line.clear();
				for( unsigned int p = i+1; p<comp.source.size() && comp.source[p] != '\n'; p++ ) { line += comp.source[p]; }
			}
			else
			{
				onChar++;
			}
		}

		printf( "line:%d\n", onLine );
		printf( "%s\n", comp.err.c_str() );
		printf( "-------------------------------------------------------------------------------------------\n");
		printf( "%-5d %s\n", onLine, line.c_str() );

		for( int i=0; i<onChar; i++ )
		{
			printf(" ");
		}

		printf( "     ^\n" );
		printf( "-------------------------------------------------------------------------------------------\n");

		return -1;
	}

	CodeHeader header;
	header.version = CALLISTO_VERSION;
	header.unitCount = comp.units.count() - 1;
	header.firstUnitBlock = output.size();

	D_CALLISTO_PARSE(Log( "complete version[0x%04X] units[%d] block[0x%08X]\n", CALLISTO_VERSION, header.unitCount, header.firstUnitBlock ));

	header.symbols = 0;
	for( Cstr *S = comp.symbolTable.getFirst(); S; S = comp.symbolTable.getNext() )
	{
		header.symbols++;
		unsigned int key = htonl((unsigned int)comp.symbolTable.getCurrentKey());
		output.append( (char *)&key, 4 );
		output += (char)S->size();
		output.append( S->c_str(), S->size() );
	}

	// populate children
	CLinkList<UnitEntry>::Iterator uiter1( comp.units );
	CLinkList<UnitEntry>::Iterator uiter2( comp.units );
	uiter1.getFirst();
	for( UnitEntry *parent = uiter1.getNext(); parent; parent = uiter1.getNext() )
	{
		uiter2.getFirst();
		for( UnitEntry *child = uiter2.getNext(); child; child = uiter2.getNext() )
		{
			if ( parent == child )
			{
				continue;
			}

			for( Cstr *P = child->parents.getFirst(); P; P = child->parents.getNext() )
			{
				if ( *P != parent->name )
				{
					continue;
				}

				D_CALLISTO_PARSE(Log("child unit name[%s] uqname[%s]", child->name.c_str(), child->unqualifiedName.c_str()));

				unsigned int key = hash( child->unqualifiedName, comp );
				if ( !parent->childAliases.get(key) )
				{
					*(parent->childAliases.add(key)) = hash( child->name, comp );
				}
			}
		}
	}

	comp.units.getFirst();
	for( UnitEntry *U = comp.units.getNext(); U; U = comp.units.getNext() )
	{
		D_CALLISTO_PARSE(Log("Unit [%s]", U->name.c_str()));

		putString( output, U->name );
		unsigned int h = htonl( U->nameHash );
		output.append( (char*)&h, 4 );
		h = htonl( U->baseOffset );
		output.append( (char *)&h, 4 );

		Cstr parentBlock;
		CLinkHash<bool> parentsSeen;
		char parents = getParents( &parentBlock, comp, *U, parentsSeen );
		output += parents;
		if ( parents )
		{
			output.append( parentBlock, parentBlock.size() );
		}

		unsigned int children = htonl(U->childAliases.count());
		output.append( (char*)&children, 4 );
		
		for( unsigned int *realKey = U->childAliases.getFirst(); realKey; realKey = U->childAliases.getNext() )
		{
			unsigned int key = htonl((unsigned int)U->childAliases.getCurrentKey());
			output.append( (char*)&key, 4 );
			key = htonl(*realKey);
			output.append( (char*)&key, 4 );
		}
	}

	for( UnitNew *N = comp.newUnitCalls.getFirst(); N; N = comp.newUnitCalls.getNext() )
	{
		D_CALLISTO_PARSE(Log("new fromSpace[%s] unitName[%s]", N->fromSpace.c_str(), N->unitName.c_str()));
		bool found = false;
		for( UnitEntry *U = comp.units.getFirst(); U; U = comp.units.getNext() )
		{
			D_CALLISTO_PARSE(Log("Checking [%s]", U->name.c_str()));
			if ( U->name == N->unitName )
			{
				found = true;

				for( Cstr *P = U->parents.getFirst(); P; P = U->parents.getNext() )
				{
					if ( *P == N->fromSpace )
					{
						printf( "illegal: new unit [%s] includes itself as a parent [%s]\n", N->unitName.c_str(), N->fromSpace.c_str() );
						return -1;
					}
				}
			}
		}

		if ( !found )
		{
			printf( "Unit [%s] not found for new\n", N->unitName.c_str() );
			return -1;
		}
	}

	header.CRC = hash( &header, sizeof(CodeHeader) - 4 );

	memcpy( output.p_str(), &header, sizeof(CodeHeader) );

	D_CALLISTO_PARSE(asciiDump( output, output.size() ));
	
	*outLen = output.release( out );

	return 0;
}

//------------------------------------------------------------------------------
Cstr& addNamespaceTo( Compilation& comp, Cstr& label )
{
	Cstr out;
	for( Cstr *S = comp.space->getFirst(); S; S = comp.space->getNext() )
	{
		out += *S;
		out += "::";
	}
	out += label;
	label = out;

	return label;
}

//------------------------------------------------------------------------------
bool isValidLabel( Cstr& token, bool doubleColonOkay =false )
{
	if ( !token.size() || (!isalpha(token[0]) && token[0] != '_') ) // non-zero size and start with alpha or '_' ?
	{
		return false;
	}

	if ( isReserved(token) ) // reserved word? no go
	{
		return false;
	}
		 
	for( unsigned int i=1; i<token.size(); i++ ) // entire token alphanumeric or '_'?
	{
		if ( doubleColonOkay && (token[i] == ':') )
		{
			++i;
			if ( i >= token.size() || token[i] != ':' )
			{
				return false;
			}
		}
		else if ( !isalnum(token[i]) && token[i] != '_' )
		{
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
void addSymbolPlaceholder( Compilation& comp, Cstr& code, CLinkList<LinkEntry>& symbols, Cstr& token, bool addNameSpace, unsigned int enumKey )
{
	LinkEntry *L = symbols.addTail();

	L->plainTarget = token;
	L->target = token;
	L->enumKey = enumKey;

	if ( addNameSpace )
	{
		addNamespaceTo( comp, L->target );
	}
	else
	{
		L->target = token;
	}

	L->offset = code.size();

	D_ADD_SYMBOL(Log("symbol placeholder plain[%s] target[%s]", L->plainTarget.c_str(), L->target.c_str()));

	code += "\01\02\03\04";
}

//------------------------------------------------------------------------------
bool loadAsEnum( Compilation& comp, ExpressionContext& context, Cstr& token )
{
	if ( !token.find( "::" ) )
	{
		return false;
	}

	context.code += (char)O_LoadEnumValue;

	D_ENUM(Log("replaced enum literal [%s]", token.c_str()));
	
	addSymbolPlaceholder( comp, context.code, context.symbols, token, true, hash(token, comp) );
	
	return true;
}

//------------------------------------------------------------------------------
bool loadLabel( Compilation& comp,
				ExpressionContext& context,
				Cstr& token,
				Value& value,
				bool lvalue )
{
	if ( value.type != CTYPE_NULL ) // its a literal
	{
		if ( lvalue )
		{
			comp.err = "must be Lvalue";
			return false;
		}
		
		D_PARSE_EXPRESSION(Log("pushing literal %s", formatValue(value).c_str()));
		putValue( context.code, value );
	}
	else if ( (token == "null") || (token == "NULL") )
	{
		if ( lvalue )
		{
			comp.err = "must be Lvalue";
			return false;
		}
		
		context.code += (char)O_LiteralNull;
	}
	else if ( token == "true" )
	{
		if ( lvalue )
		{
			comp.err = "must be Lvalue";
			return false;
		}

		context.code += (char)O_LiteralInt8;
		context.code += (char)1;
	}
	else if ( token == "false" )
	{
		if ( lvalue )
		{
			comp.err = "must be Lvalue";
			return false;
		}

		context.code += (char)O_LiteralInt8;
		context.code += (char)0;
	}
	else if ( !loadAsEnum(comp, context, token) )
	{
		if ( !isValidLabel(token) )
		{
			comp.err.format( "Invalid label [%s]", token.c_str());
			return false;
		}
		
		D_PARSE_EXPRESSION(Log("pushing load label [%s]", token.c_str()));

		context.code += (char)O_LoadLabel;
		unsigned int key = hash( token, comp );

		if ( comp.currentUnit )
		{
			Cstr *M = comp.currentUnit->argumentTokenMapping.get( key );
			if ( M )
			{
				key = hash( *M, comp );
			}
		}

		key = htonl(key);
		context.code.append( (char *)&key, 4 );
	}

	return true;
}

//------------------------------------------------------------------------------
void addUnitKey( Compilation& comp, Cstr& code, Cstr& token, bool addNameSpace )
{
	Cstr label = token;
	if ( addNameSpace )
	{
		addNamespaceTo( comp, label );
	}

	unsigned int key = hash( label, comp );
	D_ADD_SYMBOL(Log("adding unit key [%s]:[0x%08X]", label.c_str(), key));
	key = htonl(key);
	code.append( (char *)&key, 4 );
}

//------------------------------------------------------------------------------
int doOperation( ExpressionContext& context, const Operation** operations, bool* stackUsage, int stackLocation, int position, int size )
{

// a = 40 * 30 - 20 + 10;

// -> 2
	
// [11]     <- stack pointer
// [10] 10
//    +     [0]   2
//  [9] 20
//    -		[1]   1
//  [8] 30
//    *		[2]   0
//  [7] 40
//    =     [3]   3
//  [6]  a

//	(sp - (1 + position))
//	(sp - (2 + position))


// -> 1
	
// [11]     <- stack pointer
// [10] 10
//    +     [0]   2
//  [9] 20
//    -		[1]   1
//  [8] 30         
//    *		[2]   0 -- NULL --
//  [7] 1200
//    =     [3]   3
//  [6]  a

//	(sp - (1 + position))
//	(sp - (2 + position))

	D_RESOLVE(Log("stack[%d] position[%d]", stackLocation, position));

	if ( operations[position]->operation == 0 )
	{
		int operand = stackLocation;
		for( ; !operations[operand]; ++operand );
		return operand + 1;
	}

	context.code += operations[position]->operation;
	
	if ( operations[position]->binary )
	{
		int operand1 = stackLocation;
		for( ; stackUsage[operand1] && operand1 < size ; ++operand1 ); // walk down the stack until the next valid one is found
		stackUsage[operand1] = true;
		
		int operand2 = stackLocation + 1;
		for( ; stackUsage[operand2] && operand2 < size ; ++operand2 ); // walk down the stack until the next valid one is found

		// adjust for actual offset
		++operand1;
		++operand2;
		
		context.code += (char)(operand1);
		context.code += (char)(operand2);

		D_RESOLVE(Log("added binary for %d @ [%d]->[%d]", position, operand1, operand2));

		return operand2;
	}
	else
	{
		int operand = stackLocation;
		for( ; !operations[operand]; ++operand );

		++operand;
		
		context.code += (char)(operand);
		
		D_RESOLVE(Log("added unary for [%d] @%d", position, operand));

		return operand;
	}
}

//------------------------------------------------------------------------------
void resolveExpressionOperations( Compilation& comp, ExpressionContext& context )
{
	if ( context.oper.getHead()->count() > 126 )
	{
		Log( "Cannot have more than 126 operations in single expression" );
		_exit(1);
	}

	int operations = context.oper.getHead()->count();
	const Operation* operation[126];
	int stackLocation[126];
	bool stackUsage[126];
	int valuesToPop = 0;

	int i=0;
	for( const Operation** entry = context.oper.getHead()->getFirst(); entry ; entry = context.oper.getHead()->getNext() )
	{
		if ( *entry )
		{
			stackLocation[i] = valuesToPop;
			operation[i] = *entry;

			if ( (*entry)->binary )
			{
				++valuesToPop;
			}

			++i;
		}
	}
	context.oper.getHead()->clear();

	for( int o=0; o<operations; ++o )
	{
		stackUsage[o] = false;
		D_RESOLVE(Log("%d/%d @%d [%d:'%s']", o + 2, operations + 2, stackLocation[o], operation[o]->operation, operation[o]->token));
	}

	int finalResult = valuesToPop;
	
	for( i=1; i<=c_highestPrecedence; ++i )
	{
		bool log = false;
		
		// RIGHT TO LEFT
		for( int o=0; o<operations; ++o )
		{
			if ( operation[o] && (operation[o]->precedence == i) && !operation[o]->leftToRight ) 
			{
				D_RESOLVE(Log("right to left: pos[%d] [%d:'%s']", o, operation[o]->operation, operation[o]->token));
				finalResult = doOperation( context, operation, stackUsage, stackLocation[o], o, operations );
				log = true;
			}

			for( int v=0; log && v<valuesToPop; ++v )
			{
				D_RESOLVE(Log("stack[%d] %s", v, stackUsage[v] ? "used":"unused"));
			}

			log = false;
		}

		// LEFT TO RIGHT
		for( int o=operations - 1; o>=0; --o )
		{
			if ( operation[o] && (operation[o]->precedence == i) && operation[o]->leftToRight )
			{
				D_RESOLVE(Log("left to right: pos[%d] [%d:'%s']", o, operation[o]->operation, operation[o]->token));
				finalResult = doOperation( context, operation, stackUsage, stackLocation[o], o, operations );
				log = true;
			}

			for( int v=0; log && v<valuesToPop; ++v )
			{
				D_RESOLVE(Log("stack[%d] %s", v, stackUsage[v] ? "used":"unused"));
			}

			log = false;
		}

	}

	D_RESOLVE(Log("Final Rest[%d]", finalResult));
	finalResult = finalResult;

	if ( valuesToPop == 1 )
	{
		context.code += (char)O_PopOne;
	}
	else if ( valuesToPop > 0 )
	{
		context.code += (char)O_PopNum;
		context.code += (char)valuesToPop;
	}

	D_RESOLVE(Log("and popping off [%d] dead values", valuesToPop));
}

//------------------------------------------------------------------------------
bool parseNew( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value )
{
	Cstr label;
	for(;;)
	{
		if ( !isValidLabel(token) )
		{
			comp.err.format( "Invalid label [%s]\n", token.c_str() );
			return false;
		}

		for( Cstr* S = comp.space->getFirst(); S; S = comp.space->getNext() )
		{
			if ( *S == token )
			{
				comp.err.format( "cannot instantiate child from parent [%s]", S->c_str() );
				return false;
			}
		}

		label += token;
		D_PARSE_NEW(Log("parseNew building [%s]", label.c_str()));
		
		if ( !getToken( comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		if ( token == "::" ) // qualify it some more
		{
			label += "::";

			if ( !getToken( comp, token, value) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}

			continue;
		}

		if ( token != "(" )
		{
			comp.err = "Expected '(' or '::'";
			return false;
		}

		break;
	}

	UnitNew *N = comp.newUnitCalls.add();
	N->fromSpace = comp.space->getTail() ? *comp.space->getTail() : "";
	N->unitName = label;
	D_PARSE_NEW(Log("UnitNew adding from[%s] unit[%s]", N->fromSpace.c_str(), N->unitName.c_str()));

	int args = 0;
	for(;;)
	{
		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( token == ")" )
		{
			break;
		}

		context.oper.addHead();
		if ( !parseExpression(comp, context, token, value) )
		{
			return false;
		}
		context.oper.popHead();

		++args;
		if ( token == "," )
		{
			continue;
		}

		if ( token == ")" )
		{
			break;
		}

		comp.err.format( "unexpected token '%s'", token.c_str() );
		return false;
	}

	context.code += (char)O_NewUnit;
	context.code += (char)args;
	
	addUnitKey( comp, context.code, label, false );

	if ( !context.oper.getHead()->count() )
	{
		comp.err.format( "new unit must be assigned" );
		return false;
	}

	if ( context.oper.getHead()->count() != 1 )
	{
		comp.err.format( "new unit cannot be part of mult-part expression" );
		return false;
	}

	if ( (context.oper.getHead()->count() != 1)
		|| ((*context.oper.getHead()->getHead())->operation != O_Assign) )
	{
		comp.err.format( "new unit must only be assigned ('=') to variable" );
		return false;
	}

	// by definition this can only be equated
	resolveExpressionOperations( comp, context );

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool parseTable( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value )
{
	for(;;)
	{
		if ( token != ":" )
		{
			comp.err.format( "expected ':'" );
			return false;
		}

		if ( !getToken( comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		context.oper.addHead();
		if ( !parseExpression(comp, context, token, value) ) 
		{
			return false;
		}
		context.oper.popHead();

		context.code += (char)O_AddTableElement;
		D_PARSE_TABLE(Log("Pushing key/value pair"));

		if ( token == "]" )
		{
			D_PARSE_TABLE(Log("ending table"));
			break;
		}

		if ( token == "," )
		{
			if ( !getToken( comp, token, value) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}

			context.oper.addHead();
			if ( !parseExpression(comp, context, token, value) ) 
			{
				return false;
			}
			context.oper.popHead();

			D_PARSE_TABLE(Log("continuing"));

			continue;
		}

		comp.err.format( "expected ',' or ']'" );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool parseArray( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value )
{
	unsigned int elementNumber = 0;
	for(;;)
	{
		D_PARSE_ARRAY(Log("Pushing element [%d]", elementNumber));
		context.code += (char)O_AddArrayElement;
		unsigned int e = htonl( elementNumber );
		context.code.append( (char *)&e, 4 );
		++elementNumber;

		if ( token == "]" )
		{
			D_PARSE_ARRAY(Log("ending array"));
			break;
		}
							
		if ( !getToken( comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		context.oper.addHead();
		if ( !parseExpression(comp, context, token, value) ) 
		{
			return false;
		}
		context.oper.popHead();

		if ( token == "," || token == "]" )
		{
			continue;
		}

		comp.err.format( "expected ',' or ']'" );
		return false;
	}
	
	return true;
}

//------------------------------------------------------------------------------
bool parseNewTable( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value )
{
	if ( (context.oper.getHead()->count() != 1)
		 || ((*context.oper.getHead()->getHead())->operation != O_Assign) )
	{
		comp.err = "Table/Array construction can only be assigned ('=') to a variable";
		return false;
	}

	D_PARSE_EXPRESSION(Log("declaration table/array IN"));

	if ( !getToken( comp, token, value) )
	{
		comp.err.format( "unexpected EOF" );
		return false;
	}

	if ( token == "]" )
	{
		if ( !getToken( comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		if ( token != ";" )
		{
			comp.pos = comp.lastPos;
			comp.err = "expected ';'";
			return false;
		}

		D_PARSE_EXPRESSION(Log("creating blank array from []"));

		context.code += (char)O_PushBlankArray;
	}
	else
	{
		unsigned int constructOpcodeOffset = context.code.size();
		context.code += '\01';

		D_PARSE_EXPRESSION(Log("pushed null to become table"));

		context.oper.addHead();
		if ( !parseExpression(comp, context, token, value) ) 
		{
			return false;
		}
		context.oper.popHead();

		D_PARSE_EXPRESSION(Log("parsed key"));

		if ( token == ":" )
		{
			D_PARSE_EXPRESSION(Log("Parsing a TABLE"));

			context.code.p_str()[ constructOpcodeOffset ] = O_PushBlankTable;

			if ( !parseTable( comp, context, token, value) )
			{
				return false;
			}
		}
		else if ( token == "," || token == "]" )
		{
			D_PARSE_EXPRESSION(Log("Parsing an ARRAY"));

			context.code.p_str()[ constructOpcodeOffset ] = O_PushBlankArray;

			if ( !parseArray( comp, context, token, value) )
			{
				return false;
			}
		}
		else
		{
			comp.err.format( "unexpected token [%s]", token.c_str() );
			return false;
		}

		if ( !getToken( comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		if ( token != ";" )
		{
			comp.pos = comp.lastPos;
			comp.err = "expected ';'";
			return false;
		}
	}

	// make sure the assignment happens
	D_PARSE_EXPRESSION(Log("<3> resolving table/array construction" ));
	resolveExpressionOperations( comp, context );

	return true;
}

//------------------------------------------------------------------------------
bool parseExpression( Compilation& comp, ExpressionContext& context, Cstr& token, Value& value )
{
	D_PARSE_EXPRESSION(Log("Parsing expression [%s]: %s", token.c_str(), formatValue(value).c_str()));

	// okay we have a token which right now is just a lable. need to
	// figure out what to DO with that label, so find an operation

	Cstr nextToken;
	Value nextValue;

	bool labelPushed = false;
	bool unitPushed = false;
	bool globalName = false;

	if ( value.type != CTYPE_NULL )
	{
		token = "";
	}

	if ( token == "[" ) // declaring an array or table
	{
		if ( !parseNewTable(comp, context, nextToken, nextValue) )
		{
			return false;
		}
		
		goto parseExpressionSuccess;
	}
	
	// check for unary operators
	for( int i=0; c_operations[i].token; i++ )
	{
		if ( token == c_operations[i].token
			 && !c_operations[i].binary
			 && !c_operations[i].matchAsCast
			 && !c_operations[i].postFix
			 && (value.type == CTYPE_NULL) )
		{
			D_PARSE_EXPRESSION(Log("<1> Adding operation [%d:'%s'] to tail", c_operations[i].operation, c_operations[i].token));

			if ( !getToken( comp, nextToken, nextValue) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}

			bool skipParse = false;
			if ( nextValue.type != CTYPE_NULL )
			{
				switch( c_operations[i].operation )
				{
					case O_Negate:
					{
						if ( nextValue.type == CTYPE_INT )
						{
							if ( nextToken == "9223372036854775808" )
							{
								// specific case of "most negative number"
								// cannot be properly negated, so brute
								// force it
								nextValue.i64 = 0x8000000000000000LL;
							}
							else
							{
								nextValue.i64 = -nextValue.i64;
							}
						}
						else if ( nextValue.type == CTYPE_FLOAT )
						{
							nextValue.d = -nextValue.d;
						}
						else
						{
							comp.err = "Negate operator on illegal literal type";
							return false;
						}

						putValue( context.code, nextValue );
						skipParse = true;
						break;
					}
					
					case O_PreIncrement:
					{
						if ( nextValue.type == CTYPE_INT )
						{
							++nextValue.i64;
						}
						else if ( nextValue.type == CTYPE_FLOAT )
						{
							++nextValue.d;
						}
						else
						{
							comp.err = "Pre-increment operator on illegal literal type";
							return false;
						}

						putValue( context.code, nextValue );
						skipParse = true;
						break;
					}
					
					case O_PreDecrement:
					{
						if ( nextValue.type == CTYPE_INT )
						{
							--nextValue.i64;
						}
						else if ( nextValue.type == CTYPE_FLOAT )
						{
							--nextValue.d;
						}
						else
						{
							comp.err = "Pre-decrement operator on illegal literal type";
							return false;
						}

						putValue( context.code, nextValue );
						skipParse = true;
						break;
					}
					
					case O_LogicalNot:
					{
						int64_t i;
						if ( (nextValue.type == CTYPE_INT) || (nextValue.type == CTYPE_FLOAT) )
						{
							i = !nextValue.i64;
						}
						else if ( nextValue.type == CTYPE_NULL )
						{
							i = 1;
						}
						else if ( nextValue.type == CTYPE_STRING )
						{
							i = nextValue.refCstr->item.size() ? 0 : 1;
						}
						else if ( nextValue.type == CTYPE_WSTRING )
						{
							i = nextValue.refWstr->item.size() ? 0 : 1;
						}
						else
						{
							comp.err = "Logical Not on illegal literal type";
							return false;
						}

						if ( i )
						{
							context.code += (char)O_LiteralInt8;
							context.code += (char)1;
						}
						else
						{
							context.code += (char)O_LiteralInt8;
							context.code += (char)0;
						}

						skipParse = true;
						break;
					}
					
					case O_BitwiseNOT:
					{
						if ( nextValue.type != CTYPE_INT )
						{
							comp.err = "Bitwise Not on illegal literal type";
							return false;
						}
						
						context.code += (char)O_Literal;
						context.code += (char)CTYPE_INT;
						nextValue.i64 = ~nextValue.i64;
						context.code.append( (char*)&nextValue.i64, 8 );
						skipParse = true;
						break;
					}
				}
			}
			
			if ( skipParse )
			{
				*context.oper.getHead()->addHead() = c_operations;
				labelPushed = true;
				break;
			}
			else
			{
				*context.oper.getHead()->addHead() = c_operations + i;

				if ( !parseExpression(comp, context, nextToken, nextValue) ) 
				{
					return false;
				}

				goto parseExpressionSuccess;
			}
		}
	}

	// okay need to get the next token so we have context on the
	// current one, having exhausted the existing possiblilities
	
	if ( !getToken( comp, nextToken, nextValue) )
	{
		comp.err.format( "unexpected EOF" );
		return false;
	}

	if ( nextValue.type != CTYPE_NULL )
	{
		nextToken = "";
	}
	
	if ( token == "::" ) // absolute location
	{
		D_PARSE_EXPRESSION(Log("Pre-qualified '::' in parse"));
		
		if ( nextValue.type != CTYPE_NULL )
		{
			comp.err.format( "unexpected literal" );
			return false;
		}

		if ( !isValidLabel(nextToken) )
		{
			comp.err.format( "Invalid label [%s]\n", nextToken.c_str() );
			return false;
		}
		
		globalName = true;

		// do not qualify the label
		token = nextToken;
		value = nextValue;

		if ( !getToken( comp, nextToken, nextValue) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}
	}
	
	if ( token == "(" )
	{
		D_PARSE_EXPRESSION(Log("parse SUB expression IN"));

		for( int i=0; c_operations[i].token; i++ )
		{
			if ( (nextToken == c_operations[i].token) && c_operations[i].matchAsCast )
			{
				if ( !getToken( comp, nextToken, nextValue) )
				{
					comp.err.format( "unexpected EOF" );
					return false;
				}

				if ( nextToken != ")" )
				{
					comp.err = "expected ')'";
					return false;
				}
				
				*context.oper.getHead()->addHead() = c_operations + i;
				D_PARSE_EXPRESSION(Log("adding cast operation [%d:'%s'] to tail", c_operations[i].operation, c_operations[i].token));

				if ( !getToken( comp, nextToken, nextValue) )
				{
					comp.err.format( "unexpected EOF" );
					return false;
				}

				if ( !parseExpression(comp, context, nextToken, nextValue) ) 
				{
					return false;
				}

				goto parseExpressionSuccess;
			}
		}

		// parse sub-expression before proceeding
		context.oper.addHead();
		if ( !parseExpression(comp, context, nextToken, nextValue) ) 
		{
			return false;
		}
		context.oper.popHead();

		if ( nextToken != ")" )
		{
			comp.err.format( "expected ')'" );
			return false;
		}

		D_PARSE_EXPRESSION(Log("parse SUB expression OUT"));

		if ( !getToken( comp, nextToken, nextValue) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		labelPushed = true;
	}
	else if ( token == "new" )
	{
		if ( !parseNew( comp, context, nextToken, nextValue ) )
		{
			return false;
		}
		
		goto parseExpressionSuccess;
	}
	
	if ( nextToken == "." ) // label qualifier?
	{
		if ( !labelPushed && !loadLabel(comp, context, token, value, true) ) // loads from current context
		{
			return false;
		}

		D_PARSE_EXPRESSION(Log("<1> pushed [%s] as unit context", token.c_str()));

		do
		{
			if ( !getToken(comp, nextToken, nextValue) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			if ( !isValidLabel(nextToken) )
			{
				comp.err.format( "unexpected token [%s], expected unit name", nextToken.c_str());
				return false;
			}

			context.code += (char)O_MemberAccess;

			unsigned int key = htonl(hash( nextToken, comp ));
			context.code.append( (char *)&key, 4 );
			
			D_PARSE_EXPRESSION(Log("loaded [%s]", nextToken.c_str()));
			
			if ( !getToken(comp, nextToken, nextValue) )
			{
				comp.err = "unexpected EOF";
				return false;
			}
			
		} while( nextToken == '.' );
		
		unitPushed = true;
		labelPushed = true;
	}
	else if ( nextToken == "::" ) // absolute location
	{
		globalName = true;

		while ( nextToken == "::" )
		{
			token += nextToken;

			if ( !getToken(comp, nextToken, nextValue) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}
			
			if ( !isValidLabel(nextToken) )
			{
				comp.err.format( "invalid label [%s] following '::' qualifier", nextToken.c_str() );
				return false;
			}
			
			token += nextToken;
			
			if ( !getToken(comp, nextToken, nextValue) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}
		}
	}

	if ( nextToken == "[" )
	{
		D_PARSE_EXPRESSION(Log("dereference operator IN"));

		// put the label on the stack
		if ( !labelPushed && !loadLabel(comp, context, token, value, true) )
		{
			return false;
		}

		// parse the next block as a full operation
		if ( !getToken(comp, nextToken, nextValue) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		// parse sub-expression before proceeding
		context.oper.addHead();
		if ( !parseExpression(comp, context, nextToken, nextValue) ) 
		{
			return false;
		}
		context.oper.popHead();

		if ( nextToken != "]" )
		{
			comp.err.format( "expected ']'" );
			return false;
		}	

		context.code += (char)O_DereferenceFromTable;

		// this will leave one entry on the stack which IS an Lvalue
		labelPushed = true;

		if ( !getToken(comp, nextToken, nextValue) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}
	}
	else if ( nextToken == "(" ) // call this label as a function? alrighty
	{
/*
		global  unitPushed
		c_func();       F          F       LG      looked up in global unitList by c_func
		A();            F          F       LG      looked up in global unitList by A
		A::B();         T          F       GO      looked up in global unitList by A::B (symbol)

		a = new A();
		a.B();          F          T       U       reference stack

		unit A()
		{
		B();        F          F       LG      look up local then check	global
		c_func();   F          F       LG      look up local then check	global
		C();        F          F       LG      look up local then check	global
		::C();      T          F       GO      looked up in global unitList by A::B (symbol)
		::c_func(); T          F       GO      looked up in global unitList by A::B (symbol)
		A::B();     T          F       GO      looked up in global unitList by A::B (symbol)

*/
		if ( !isValidLabel(token, true) )
		{
			comp.err.format( "Invalid function name [%s]", token.c_str() );
			return false;
		}

		// this implies global context though
		D_PARSE_EXPRESSION(Log("calling[%s] globalName[%s] unitPushed[%s]", token.c_str(), globalName ? "TRUE":"FALSE" , unitPushed ? "TRUE":"FALSE" ));

		int args = 0;
		for(;;)
		{
			if ( !getToken(comp, nextToken, nextValue) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			if ( nextToken == ")" )
			{
				break;
			}

			context.oper.addHead();
			if ( !parseExpression(comp, context, nextToken, nextValue) )
			{
				return false;
			}
			context.oper.popHead();

			++args;
			if ( nextToken == "," )
			{
				continue;
			}

			if ( nextToken == ")" )
			{
				break;
			}

			comp.err.format( "unexpected token '%s'", nextToken.c_str() );
			return false;
		}

		if ( globalName )
		{
			D_PARSE_EXPRESSION(Log("function GO[%s]", token.c_str()));

			context.code += (char)O_CallGlobalOnly;
			context.code += (char)args;
			addSymbolPlaceholder( comp, context.code, context.symbols, token, true, 0 );
		}
		else if ( unitPushed )
		{
			D_PARSE_EXPRESSION(Log("function U[%s]", token.c_str()));

			context.code += (char)O_CallFromUnitSpace;
			context.code += (char)args;
		}
		else
		{
			D_PARSE_EXPRESSION(Log("function LG[%s]", token.c_str()));

			context.code += (char)O_CallLocalThenGlobal;
			context.code += (char)args;

			unsigned int key = htonl(hash( token, comp ));
			context.code.append( (char *)&key, 4 );
		}

		// for end-of-expression-chain tokens resolve them
		//		if ( context.oper.getHead()->count() )
		//		{
		//			D_PARSE_EXPRESSION(Log("<3> resolving expressions" ));
		//			resolveExpressionOperations( comp, context );
		//		}

		labelPushed = true;

		if ( !getToken(comp, nextToken, nextValue) )
		{
			comp.err = "unexpected EOF";
			return false;
		}
	}

	// check for 'end' conditions
	if ( nextToken == ";" || nextToken == "," || nextToken == "]" || nextToken == ")" || nextToken == ":" )
	{
		D_PARSE_EXPRESSION(Log("<2> end of expression '%s'", nextToken.c_str()));

		if ( !labelPushed && !loadLabel(comp, context, token, value, false) )
		{
			return false;
		}

		// for end-of-expression-chain tokens resolve them
		if ( context.oper.getHead()->count() )
		{
			D_PARSE_EXPRESSION(Log("<2> resolving expressions" ));
			resolveExpressionOperations( comp, context );
		}

		goto parseExpressionSuccess;
	}

	// check for post operators
	for( int i=0; c_operations[i].token; i++ )
	{
		if ( nextToken == c_operations[i].token
			 && ((c_operations[i].binary || c_operations[i].postFix) && (nextValue.type == CTYPE_NULL))
			 && !c_operations[i].matchAsCast )
		{
			*context.oper.getHead()->addHead() = c_operations + i;
			D_PARSE_EXPRESSION(Log("<2> Adding operation [%d:'%s'] to tail", c_operations[i].operation, c_operations[i].token));

			if ( c_operations[i].postFix )
			{
				if ( !parseExpression(comp, context, token, value) )
				{
					return false;
				}

				nextToken = token;
				nextValue = value;
			}
			else
			{
				if ( !labelPushed && !loadLabel( comp, context, token, value, c_operations[i].lvalue ) )
				{
					return false;
				}

				if ( !getToken(comp, nextToken, nextValue) )
				{
					comp.err.format( "unexpected EOF" );
					return false;
				}

				if ( !parseExpression(comp, context, nextToken, nextValue) ) 
				{
					return false;
				}
			}

			goto parseExpressionSuccess;
		}
	}


	// otherwise just .. don't know what to do with the token we just saw
	Log("Unexpected token [%s]", nextToken.c_str() );
	return false;

parseExpressionSuccess:
	
	token = nextToken;
	return true;
}

//------------------------------------------------------------------------------
void addJumpLocation( UnitEntry& unit, int id, bool canOptimize =false )
{
	D_JUMP(Log("Adding jump away id [%d]@[%d]", id, unit.program.size()));

	CodePointer* link = unit.jumpLinkSymbol.add();
	link->offset = unit.program.size();
	link->canOptimize = canOptimize;
	link->targetIndex = id;
	link->added = false;
	unit.program += "\01\02\03\04"; // placeholder
}

//------------------------------------------------------------------------------
void addJumpTarget( UnitEntry& unit, int id )
{
	D_JUMP(Log("Adding jump to [%d]@[%d]", id, unit.program.size()));

	*unit.jumpIndexes.add( id ) = unit.program.size();
}

//------------------------------------------------------------------------------
bool parseIfChain( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	D_IFCHAIN(Log("Into if chain"));
	
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}
	
	if ( token != "(" )
	{
		comp.err = "expected '('";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	context.oper.addHead();
	if ( !parseExpression(comp, context, token, value) )
	{
		return false;
	}
	context.oper.popHead();

	if ( token != ")" )
	{
		comp.err = "expected ')'";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	int conditionFalseMarker = Compilation::jumpIndex++;

	unit.program += (char)O_BZ;
	addJumpLocation( unit, conditionFalseMarker );

	D_IFCHAIN(Log("if ( $$ ) false condition [%d] dropped", conditionFalseMarker));

	if ( token == "{" )
	{
		*unit.bracketContext.addTail() = B_BLOCK;
		if ( !parseBlock(comp, unit) )
		{
			return false;
		}
	}
	else if ( !parseBlock(comp, unit, 0, &token) )
	{
		return false;
	}

	D_IFCHAIN(Log("if (  ) { $$ }"));

	if ( !getToken(comp, token, value) )
	{
		D_IFCHAIN(Log("ifchain EOF"));
		return true; // EOF here is okay
	}

	if ( token == "else" )
	{
		// not done, jump from here PAST The next logic to the end point
		int conditionTrueMarker = Compilation::jumpIndex++;

		unit.program += (char)O_Jump;
		addJumpLocation( unit, conditionTrueMarker );

		D_IFCHAIN(Log("else case detected, set up jump PAST it as [%d] and dropped false marker[%d] location here", conditionTrueMarker, conditionFalseMarker));

		addJumpTarget( unit, conditionFalseMarker ); // this is where you end up "if (false)" .. check the else

		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( token == "if" )
		{
			D_IFCHAIN(Log("else IF detected, parsing into another if case"));

			if ( !parseIfChain(comp, unit, context, token, value) )
			{
				return false;
			}
		}
		else if ( token == "{" )
		{
			*unit.bracketContext.addTail() = B_BLOCK;
			
			if ( !parseBlock(comp, unit) )
			{
				return false;
			}

			getToken( comp, token, value ); // return expects next token to be pre-loaded
		}
		else
		{
			if ( !parseBlock(comp, unit, 0, &token) )
			{
				return false;
			}
			
			getToken( comp, token, value ); // return expects next token to be pre-loaded
		}
		
		addJumpTarget( unit, conditionTrueMarker );
		D_IFCHAIN(Log("true target location[%d] dropped", conditionTrueMarker));
	}
	else
	{
		addJumpTarget( unit, conditionFalseMarker ); // we're done, mark this as the jump target for "if ( false )"
		D_IFCHAIN(Log("no else, false marker[%d] dropped, and returning", conditionFalseMarker));
	}

	D_IFCHAIN(Log("exiting if parse loop with[%s]", token.c_str()));

	return true;
}

//------------------------------------------------------------------------------
bool parseWhileLoop( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "(" )
	{
		comp.err = "expected '('";
		return false;
	}

	LoopingStructure* loop = unit.loopTracking.addHead();
	loop->continueTarget = Compilation::jumpIndex++;
	loop->breakTarget = Compilation::jumpIndex++;

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	addJumpTarget( unit, loop->continueTarget );

	context.oper.addHead();
	if ( !parseExpression(comp, context, token, value) )
	{
		return false;
	}
	context.oper.popHead();

	unit.program += (char)O_BZ;
	addJumpLocation( unit, loop->breakTarget );

	if ( token != ")" )
	{
		comp.err = "expected ')'";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	D_PARSE(Log("Parsing while body [%s]", token.c_str()));

	if ( token == "{" )
	{
		*unit.bracketContext.addTail() = B_BLOCK;
		if ( !parseBlock(comp, unit) )
		{
			return false;
		}
	}
	else if ( !parseBlock(comp, unit, 0, &token) )
	{
		return false;
	}

	unit.program += (char)O_Jump;
	addJumpLocation( unit, loop->continueTarget );
	addJumpTarget( unit, loop->breakTarget );

	unit.loopTracking.popHead();

	return true;
}

//------------------------------------------------------------------------------
bool parseDoLoop( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	LoopingStructure* loop = unit.loopTracking.addHead();
	loop->continueTarget = Compilation::jumpIndex++;
	loop->breakTarget = Compilation::jumpIndex++;

	addJumpTarget( unit, loop->continueTarget );
	
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token == "{" )
	{
		*unit.bracketContext.addTail() = B_BLOCK;
		if ( !parseBlock(comp, unit) )
		{
			return false;
		}
	}
	else if ( !parseBlock(comp, unit, 0, &token) )
	{
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "while" )
	{
		comp.err = "expected 'while'";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "(" )
	{
		comp.err = "expected '('";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	context.oper.addHead();
	if ( !parseExpression(comp, context, token, value) )
	{
		return false;
	}
	context.oper.popHead();

	if ( token != ")" )
	{
		comp.err = "expected ')'";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != ";" )
	{
		comp.pos = comp.lastPos;
		comp.err = "expected ';'";
		return false;
	}

	unit.program += (char)O_BNZ;
	addJumpLocation( unit, loop->continueTarget );
	addJumpTarget( unit, loop->breakTarget );

	unit.loopTracking.popHead();

	return true;
}

//------------------------------------------------------------------------------
bool parseForEachLoop( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "(" )
	{
		comp.err.format( "expected '('" );
		return false;
	}
	
	D_FOREACHLOOP(Log("'foreach ('"));

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( !isValidLabel(token) )
	{
		comp.err.format( "[%s] not a valid label", token.c_str() );
		return false;
	}

	unsigned int valueKey = hash( token, comp );

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	D_FOREACHLOOP(Log("itemLabel [%s]", itemLabel.c_str()));

	unsigned int keyKey = 0;
	if ( token == "," )
	{
		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( !isValidLabel(token) )
		{
			comp.err.format( "[%s] not a valid label", token.c_str() );
			return false;
		}

		keyKey = hash( token, comp );

		D_FOREACHLOOP(Log("keyLabel [%s]", token.c_str()));
		 
		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( token != ":" )
		{
			comp.err = "expected ':'";
			return false;
		}
	}
	else if ( token != ":" )
	{	
		comp.err.format( "unexpected token [%s]", token.c_str() );
		return false;
	}
	else
	{
		D_FOREACHLOOP(Log("no keyLabel"));
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	context.oper.addHead();
	if ( !parseExpression(comp, context, token, value) )
	{
		return false;
	}
	context.oper.popHead();

	// replace the table with an iterator FOR the table
	unit.program += (char)O_PushIterator;
	
	LoopingStructure* loop = unit.loopTracking.addHead();
	loop->continueTarget = Compilation::jumpIndex++;
	loop->breakTarget = Compilation::jumpIndex++;

	addJumpTarget( unit, loop->continueTarget );

	if ( !keyKey )
	{
		unit.program += (char)O_IteratorGetNextValueOrBranch;
		unsigned int k = htonl(valueKey);
		unit.program.append( (char *)&k, 4 );
	}
	else
	{
		unit.program += (char)O_IteratorGetNextKeyValueOrBranch;
		unsigned int k = htonl(keyKey);
		unit.program.append( (char *)&k, 4 );
		k = htonl(valueKey);
		unit.program.append( (char *)&k, 4 );
	}

	addJumpLocation( unit, loop->breakTarget );

	if ( token != ")" )
	{
		comp.err = "expected ')'";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token == "{" )
	{
		*unit.bracketContext.addTail() = B_BLOCK;
		if ( !parseBlock(comp, unit) )
		{
			return false;
		}
	}
	else if ( !parseBlock(comp, unit, 0, &token) )
	{
		return false;
	}

	unit.program += (char)O_Jump;
	addJumpLocation( unit, loop->continueTarget );
	
	addJumpTarget( unit, loop->breakTarget );

	unit.program += (char)O_PopOne; // ignore return value from setup expression

	unit.loopTracking.popHead();

	return true;
}

//------------------------------------------------------------------------------
bool parseForLoop( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "(" )
	{
		comp.err.format( "expected '('" );
		return false;
	}

	D_FORLOOP(Log("'for ('"));

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != ";" )
	{
		for(;;)
		{
			context.oper.addHead();
			if ( !parseExpression(comp, context, token, value) )
			{
				return false;
			}
			context.oper.popHead();

			unit.program += (char)O_PopOne; // ignore return value from setup expression

			if ( token == "," )
			{
				if ( !getToken(comp, token, value) )
				{
					comp.err = "unexpected EOF";
					return false;
				}
			}
			else
			{
				break;
			}
		}
	}

	D_FORLOOP(Log("'for ( $$ ;'"));

	if ( token != ";" )
	{
		comp.err.format( "expected ';'" );
		return false;
	}

	LoopingStructure* loop = unit.loopTracking.addHead();
	loop->continueTarget = Compilation::jumpIndex++;
	loop->breakTarget = Compilation::jumpIndex++;

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	//------- TOP OF LOOP
	addJumpTarget( unit, loop->continueTarget );

	if ( token != ";" )
	{
		context.oper.addHead();
		if ( !parseExpression(comp, context, token, value) )
		{
			return false;
		}
		context.oper.popHead();
		
		unit.program += (char)O_BZ; //--------- BRANCH OUT?
		addJumpLocation( unit, loop->breakTarget );
	}
	
	if ( token != ";" )
	{
		comp.err.format( "expected ';'" );
		return false;
	}

	D_FORLOOP(Log("'for (  ; $$ ;'"));

	//------------ 
	// parse the end-of-loop logic, but keep it temporary
	// so it can be plugged into the end of the loop, the
	// symbols must be re-located so make a temp table for them
	// too
	Cstr after;
	CLinkList<LinkEntry> afterSymbols;
	ExpressionContext afterContext( after, afterSymbols );

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != ")" )
	{
		for(;;)
		{
			context.oper.addHead();
			if ( !parseExpression(comp, afterContext, token, value) )
			{
				return false;
			}
			context.oper.popHead();
			
			after += (char)O_PopOne; // ignore result
			if ( token == "," )
			{
				if ( !getToken(comp, token, value) )
				{
					comp.err = "unexpected EOF";
					return false;
				}
			}
			else
			{
				break;
			}
		}
	}
	//------------

	if ( token != ")" )
	{
		comp.err.format( "expected ')'" );
		return false;
	}

	D_FORLOOP(Log("'for (  ;  ; $$ )'"));

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token == "{" )
	{
		*unit.bracketContext.addTail() = B_BLOCK;
		if ( !parseBlock(comp, unit) )
		{
			return false;
		}
	}
	else if ( !parseBlock(comp, unit, 0, &token) )
	{
		return false;
	}

	//-------- LOOP BODY

	D_FORLOOP(Log("'for (  ;  ;  ) { $$ }'"));

	// now plug in the "after" condition
	for( LinkEntry *link = afterSymbols.getFirst(); link; link = afterSymbols.getNext() )
	{
		LinkEntry *L = unit.unitSym.addTail();
		*L = *link;
		L->offset += unit.program.size();
	}
	unit.program += after;

	unit.program += (char)O_Jump;

	addJumpLocation( unit, loop->continueTarget );
	addJumpTarget( unit, loop->breakTarget );

	unit.loopTracking.popHead();
	
	return true;
}

//------------------------------------------------------------------------------
bool parseSwitch( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	D_SWITCH(Log("Parsing 'switch'"));
	
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "(" )
	{
		comp.err.format( "expected '('" );
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	context.oper.addHead();
	if ( !parseExpression(comp, context, token, value) )
	{
		return false;
	}
	context.oper.popHead();

	SwitchContext* sc = unit.switchContexts.add();

	unit.program += (char)O_Switch; // take the value on the stack, pull the jump vector that is placed next and jump for the selection logic

	sc->tableLocationId = Compilation::jumpIndex++;
	addJumpLocation( unit, sc->tableLocationId );

	sc->hasDefault = false;

	LoopingStructure* loop = unit.loopTracking.addHead();
	loop->continueTarget = 0;
	loop->breakTarget = Compilation::jumpIndex++;
	addJumpLocation( unit, loop->breakTarget );
	sc->breakTarget = loop->breakTarget;

	if ( token != ")" )
	{
		comp.err.format( "expected ')'" );
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "{" )
	{
		comp.err.format( "expected '{'" );
		return false;
	}

	*unit.bracketContext.addTail() = B_BLOCK;

	if ( !parseBlock(comp, unit, sc) )
	{
		return false;
	}

	addJumpTarget( unit, loop->breakTarget );

	unit.loopTracking.popHead();

	return true;
}
		
//------------------------------------------------------------------------------
bool parseEnum( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( !isValidLabel(token) )
	{
		comp.err.format( "invalid enum label [%s]", token.c_str());
		return false;
	}

	Cstr label = token;

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token != "{" )
	{
		comp.err = "expected '{'";
		return false;
	}

	Cstr thisSpace;
	if ( comp.space->count() )
	{
		for( Cstr* S = comp.space->getFirst(); S; S = comp.space->getNext() )
		{
			thisSpace += *S + "::";
		}
	}

	thisSpace += label + "::";

	D_ENUM(Log("This space [%s]", thisSpace.c_str()));

	for( int runningValue = 0 ; ; ++runningValue )
	{
		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( token == "}" )
		{
			break;
		}
		
		if ( !isValidLabel(token) )
		{
			comp.err.format( "invalid enum label [%s]", token.c_str());
			return false;
		}

		Cstr valueName = thisSpace;
		valueName += token;
		unsigned int key = hash( valueName, comp );

		if ( comp.enumValues.get( key ) )
		{
			comp.err.format( "repeated enumeration identifier [%s]", valueName.c_str() );
			return false;
		}
		
		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if( token == "," )
		{
			D_ENUM(Log("<1> [%s:0x%08X] = %lld", valueName.c_str(), key, runningValue));

			*comp.enumValues.add( key ) = runningValue;
		}
		else if ( token == "=" )
		{
			if ( !getToken(comp, token, value) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			bool negate = false;
			if ( token == "-" )
			{
				negate = true;
				if ( !getToken(comp, token, value) )
				{
					comp.err = "unexpected EOF";
					return false;
				}
			}
			
			if ( value.type != CTYPE_INT )
			{
				comp.err.format( "expected integer initializer" );
				return false;
			}

			if ( negate )
			{
				value.i64 = -value.i64;
			}
			
			D_ENUM(Log("<2> [%s:0x%08X] = %lld", valueName.c_str(), key, value.i64));

			*comp.enumValues.add( key ) = (int)value.i64;
			runningValue = (int)value.i64;

			if ( !getToken(comp, token, value) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			if ( token == "}" )
			{
				break;
			}
			else if ( token != "," )
			{
				comp.err.format( "unexpected token [%s]", token.c_str());
				return false;
			}
		}
		else if ( token == "}" )
		{
			D_ENUM(Log("<3> [%s:0x%08X] = %d", valueName.c_str(), key, runningValue));
			*comp.enumValues.add( key ) = (int)value.i64;
			break;
		}
		else
		{
			comp.err.format( "Unexpected token [%s]", token.c_str() );
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
bool parseImport( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	
	return true;
}

//------------------------------------------------------------------------------
bool parseUnit( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value )
{
	if ( !getToken(comp, token, value) )
	{
		comp.err.format( "unexpected EOF" );
		return false;
	}

	D_PARSE_UNIT(Log("parsing unit [%s]", token.c_str()));

	Cstr space = token;

	CLinkList<Cstr> tempSpace;

	for(;;)
	{
		if ( !isValidLabel(token) )
		{
			comp.err.format( "Invalid label [%s]\n", token.c_str() );
			return false;
		}

		if ( !getToken(comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		if ( token == "(" )
		{
			break;
		}

		if ( token == "::" )
		{
			*tempSpace.addTail() = space;
		}
		else
		{
			comp.err.format( "unexpected token [%s]", token.c_str() );
			return false;
		}

		if ( !getToken(comp, token, value) )
		{
			comp.err.format( "unexpected EOF" );
			return false;
		}

		space = token;
	}

	CLinkList<Cstr>* oldSpace = comp.space;
	if ( tempSpace.count() )
	{
		comp.space = &tempSpace;
	}

	UnitEntry *oldUnit = comp.currentUnit;
	comp.currentUnit = comp.units.addTail();
	comp.currentUnit->baseOffset = 0;

	Cstr thisSpace;
	for( Cstr* S = comp.space->getFirst(); S; S = comp.space->getNext() )
	{
		thisSpace += *S;
		*comp.currentUnit->parents.addTail() = thisSpace;
		D_PARSE_UNIT(Log("parent [%s]", thisSpace.c_str()));
		thisSpace += "::";
	}
	thisSpace.shave( 2 );

	Cstr parentSpace = thisSpace;
	if ( thisSpace.size() )
	{
		thisSpace += "::" + space;
	}
	else
	{
		thisSpace = space;
	}
	
	D_PARSE(Log("parentSpace[%s] thisSpace[%s] unit[%s]", parentSpace.c_str(), thisSpace.c_str(), space.c_str()));

	comp.currentUnit->unitSym.clear();
	comp.currentUnit->name = thisSpace;
	comp.currentUnit->nameHash = hash( thisSpace, comp );
	
	for( unsigned int q = comp.currentUnit->name.size() - 1; q > 0 ; --q )
	{
		if ( comp.currentUnit->name[q] == ':' )
		{
			comp.currentUnit->unqualifiedName.set( comp.currentUnit->name.c_str(q+1) );
			break;
		}

		if ( q == 1 )
		{
			comp.currentUnit->unqualifiedName = comp.currentUnit->name;
		}
	}

	for( UnitEntry *entry = comp.units.getFirst(); entry; entry = comp.units.getNext() )
	{
		if ( (entry->name == thisSpace) && (entry != comp.currentUnit) )
		{
			comp.err.format( "unit [%s] previously defined", thisSpace.c_str() );
			return false;
		}
	}
	
	*comp.space->addTail() = space;

	D_PARSE(Log("unit[%s] starting", comp.currentUnit->name.c_str()));

	char argNumber = 0;
	bool copyKeywordSeen = false;
	for(;;)
	{
		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( token == ")" )
		{
			if ( copyKeywordSeen )
			{
				comp.err = "bare 'copy' not valid";
				return false;
			}
			
			break; // end of variables, cool
		}

		if ( token == "," && (argNumber > 0) )
		{
			if ( copyKeywordSeen )
			{
				comp.err = "bare 'copy' not valid";
				return false;
			}

			continue;
		}

		if ( (token == "copy") && (value.type == CTYPE_NULL) )
		{
			copyKeywordSeen = true;
			continue;
		}
		
		if ( !isValidLabel(token) )
		{
			comp.err.format( "invalid argument label [%s]", token.c_str() );
			return false;
		}

		if ( copyKeywordSeen )
		{
			D_PARSE_UNIT(Log("COPY arg claim[%s]", token.c_str()));
			copyKeywordSeen = false;
			comp.currentUnit->program += (char)O_ClaimCopyArgument;
		}
		else
		{
			D_PARSE_UNIT(Log("arg claim[%s]", token.c_str()));
			comp.currentUnit->program += (char)O_ClaimArgument;
		}

		unsigned int key = hash( token, comp );
		if ( comp.currentUnit->argumentTokenMapping.get(key) )
		{
			comp.err.format( "duplicate argument [%s]", token.c_str() );
			return false;
		}
		
		*comp.currentUnit->argumentTokenMapping.add(key) = token;
					
		comp.currentUnit->program += (char)argNumber++;
		unsigned int mkey = htonl(hash( token, comp ));
		comp.currentUnit->program.append( (char *)&mkey, 4 );
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err.format( "unexpected EOF", token.c_str() );
		return false;
	}

	if ( token == ":" )
	{
		Cstr label;

		for(;;)
		{
			if ( !getToken(comp, token, value) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}
			
			if ( !isValidLabel(token) )
			{
				comp.err.format( "invalid label [%s]", token.c_str() );
				return false;
			}

			label += token;

			if ( !getToken(comp, token, value) )
			{
				comp.err.format( "unexpected EOF" );
				return false;
			}

			if ( token == "{" )
			{
				*comp.currentUnit->parents.addTail() = label;
				D_PARSE_UNIT(Log("<1> Adding parent [%s]", label.c_str()));
				break;
			}
			else if ( token == "::" )
			{
				label += "::";
			}
			else if ( token == "," )
			{
				*comp.currentUnit->parents.addTail() = label;
				D_PARSE_UNIT(Log("<2> Adding parent [%s]", label.c_str()));
				label.clear();
			}
			else
			{
				comp.err.format( "unexpected token [%s]", token.c_str() );
				return false;
			}
		}
	}
	else if ( token != "{" )
	{
		comp.err.format( "unexpected token [%s], expected '{' or ':'", token.c_str() );
		return false;
	}

	*comp.currentUnit->bracketContext.addTail() = B_UNIT;

	if ( !parseBlock(comp, *comp.currentUnit) ) // inside a unit, parse it
	{
		return false;
	}

	comp.currentUnit->bracketContext.popTail();

	if ( !processSwitchContexts( comp, *comp.currentUnit ) )
	{
		return false;
	}

	comp.space->popTail();
	comp.space = oldSpace;

	comp.currentUnit = oldUnit;
	
	return true;
}

//------------------------------------------------------------------------------
bool processSwitchContexts( Compilation& target, UnitEntry& unit )
{
	for( SwitchContext *sc = unit.switchContexts.getFirst(); sc; sc = unit.switchContexts.getNext() )
	{
		addJumpTarget( unit, sc->tableLocationId );

		unsigned int count = sc->caseLocations.count();
		
		D_SWITCH(Log("Switch Context break[%d] tableLocation[%d] entries[%d]", sc->breakTarget, sc->tableLocationId, count));
		count = htonl(count);
		unit.program.append( (char *)&count, 4 );
		unit.program += sc->hasDefault ? (char)1 : (char)0;
		
		SwitchCase *defaultCase = 0;
		unsigned int hash = 0;
		for( SwitchCase *C = sc->caseLocations.getFirst(); C; C = sc->caseLocations.getNext() )
		{
			if ( C->defaultCase )
			{
				defaultCase = C;
			}
			else
			{
				hash = htonl(C->val.getHash());
				unit.program.append( (char *)&hash, 4 );
				addJumpLocation( unit, C->jumpIndex );
				
				D_SWITCH(Log("val[%s] jumpIndex[%d] enum[%s] default[%s]", formatValue(C->val).c_str(), C->jumpIndex, C->enumName.c_str(), C->defaultCase ? "T":"F"));
			}
		}

		if ( defaultCase )
		{
			hash = htonl(defaultCase->val.getHash());
			unit.program.append( (char *)&hash, 4 );
			addJumpLocation( unit, defaultCase->jumpIndex );
		}
	}

	return true;
}

//------------------------------------------------------------------------------
bool parseSwitchBodyTerm( Compilation& comp, UnitEntry& unit, ExpressionContext& context, Cstr& token, Value& value, SwitchContext* sc, bool& tokenFetched )
{
	SwitchCase* C = 0;
	
	if ( token == "default" )
	{
		if ( !sc )
		{
			comp.err = "'default' keyword outside of switch context";
			return false;
		}

		sc->hasDefault = true;

		C = sc->caseLocations.add();
		C->jumpIndex = Compilation::jumpIndex++;
		C->defaultCase = true;

		if ( !getToken(comp, token, value) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		D_SWITCH(Log("'default' case parsed"));
	}
	else // case
	{
		if ( !sc )
		{
			comp.err = "'case' keyword outside of switch context";
			return false;
		}

		C = sc->caseLocations.add();
		C->jumpIndex = Compilation::jumpIndex++;
		C->defaultCase = false;

		if ( !getToken(comp, token, C->val) )
		{
			comp.err = "unexpected EOF";
			return false;
		}

		if ( (token != "null") && (C->val.type == CTYPE_NULL) )
		{
			if ( !isValidLabel(token) )
			{
				comp.err.format( "invalid constant expression [%s]", token.c_str() );
				return false;
			}

			C->enumName = token;

			for(;;)
			{
				if ( !getToken(comp, token, value) )
				{
					comp.err = "unexpected EOF";
					return false;
				}

				if ( token == ":" )
				{
					break;
				}
				else if ( token != "::" )
				{
					comp.err.format( "invalid constant expression" );
					return false;
				}

				C->enumName += "::";

				if ( !getToken(comp, token, value) )
				{
					comp.err = "unexpected EOF";
					return false;
				}

				if ( !isValidLabel(token) )
				{
					comp.err.format( "invalid label [%s]", token.c_str() );
					return false;
				}

				C->enumName += token;
			}

			D_SWITCH(Log("case built enum value [%s]", C->enumName.c_str()));
		}
		else
		{
			D_SWITCH(Log("case built [%s]", formatValue(C->val).c_str()));

			if ( !getToken(comp, token, value) )
			{
				comp.err = "unexpected EOF";
				return false;
			}
		}
	}

	for( SwitchCase* check = sc->caseLocations.getFirst(); check; check = sc->caseLocations.getNext() )
	{
		if ( C == check )
		{
			continue;
		}
		else if ( C->defaultCase )
		{
			if ( check->defaultCase )
			{
				comp.err = "duplicate 'default' case";
				return false;
			}
		}
		else if ( check->defaultCase )
		{
			continue;
		}
		else if ( C->enumName.size() )
		{
			if ( C->enumName == check->enumName )
			{
				comp.err.format( "Duplicate case [%s]", check->enumName.c_str() );
				return false;
			}
		}
		else if ( C->val.type == CTYPE_NULL )
		{
			if ( check->val.type == CTYPE_NULL )
			{
				comp.err.format( "Duplicate 'null' case" );
				return false;
			}
		}
		else if ( !(C->val.type & BIT_CAN_HASH) )
		{
			comp.err.format( "type cannot be a 'switch' target" );
			return false;
		}
		else if ( C->val.type == CTYPE_INT
				  && check->val.type == CTYPE_INT
				  && C->val.i64 == check->val.i64 )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_FLOAT
				  && check->val.type == CTYPE_INT
				  && C->val.d == check->val.i64 )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_FLOAT
				  && check->val.type == CTYPE_FLOAT
				  && C->val.d == check->val.d )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_INT
				  && check->val.type == CTYPE_INT
				  && C->val.i64 == check->val.i64 )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_STRING
				  && check->val.type == CTYPE_STRING
				  && C->val.refCstr->item == check->val.refCstr->item )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_WSTRING
				  && check->val.type == CTYPE_STRING
				  && C->val.refWstr->item == check->val.refCstr->item )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_WSTRING
				  && check->val.type == CTYPE_WSTRING
				  && C->val.refWstr->item == check->val.refWstr->item )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
		else if ( C->val.type == CTYPE_STRING
				  && check->val.type == CTYPE_WSTRING
				  && C->val.refCstr->item == check->val.refWstr->item )
		{
			comp.err.format( "Duplicate case [%s]", formatValue(C->val).c_str());
			return false;
		}
	}

	if ( token != ":" )
	{
		comp.err = "expected ':'";
		return false;
	}

	if ( !getToken(comp, token, value) )
	{
		comp.err = "unexpected EOF";
		return false;
	}

	if ( token == "{" )
	{
		*unit.bracketContext.addTail() = B_CASE;
	}
	else
	{
		tokenFetched = true;
	}

	addJumpTarget( unit, C->jumpIndex );
	return true;
}

//------------------------------------------------------------------------------
bool parseBlock( Compilation& comp, UnitEntry& unit, SwitchContext* sc, Cstr* preload )
{
	D_PARSE(Log("parsing block: unit[%d] loop[%d] switch[%p] single[%s]", comp.space->count(), unit.loopTracking.count(), sc, preload ? preload->c_str() :"F"));
	
	Cstr token = preload ? *preload : "";
	Value value;
	ExpressionContext context( unit.program, unit.unitSym );
	bool tokenFetched = false;
	for(;;)
	{
		if ( !tokenFetched && !preload )
		{
			if ( !getToken(comp, token, value) || !token.size() )
			{
				break;
			}
		}

		tokenFetched = false;

		if ( token == "default" || token == "case" )
		{
			if ( !parseSwitchBodyTerm(comp, unit, context, token, value, sc, tokenFetched) )
			{
				return false;
			}
		}
		else if ( token == ";" )
		{
			D_PARSE(Log("null statement"));
		}
		else if ( token == "return" )
		{
			if ( !getToken(comp, token, value) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			context.oper.addHead();
			if ( token == ";" )
			{
				unit.program += (char)O_LiteralNull;
			}
			else if ( !parseExpression(comp, context, token, value) )
			{
				return false;
			}
			context.oper.popHead();

			unit.program += (char)O_Return;
		}
		else if ( token == "if" )
		{
			if ( !parseIfChain( comp, unit, context, token, value ) )
			{
				return false;
			}

			tokenFetched = true;
		}
		else if ( token == "break" )
		{
			unit.program += (char)O_Jump;
			
			if ( unit.loopTracking.count() )
			{
				addJumpLocation( unit, unit.loopTracking.getHead()->breakTarget );
			}
			else
			{
				comp.err = "no loop matching 'break'";
				return false;
			}

			if ( !getToken(comp, token, value) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			if ( token != ";" )
			{
				comp.err = "expected '}'";
				return false;
			}
		}
		else if ( token == "continue" )
		{
			if ( !unit.loopTracking.count() )
			{
				comp.err = "no loop matching 'continue'";
				return false;
			}

			if ( unit.loopTracking.getHead()->continueTarget )
			{
				unit.program += (char)O_Jump;
				addJumpLocation( unit, unit.loopTracking.getHead()->continueTarget );
			}
			else
			{
				comp.err = "no loop to continue to";
				return false;
			}

			if ( !getToken(comp, token, value) )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			if ( token != ";" )
			{
				comp.err = "expected '}'";
				return false;
			}
		}
		else if ( token == "while" )
		{
			if ( !parseWhileLoop( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "do" )
		{
			if ( !parseDoLoop( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "for" )
		{
			if ( !parseForLoop( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "switch" )
		{
			if ( !parseSwitch( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "foreach" )
		{
			if ( !parseForEachLoop( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "}" )
		{
			if ( !unit.bracketContext.count() )
			{
				comp.err = "Unexpected '}'";
				return false;
			}

			int context = *unit.bracketContext.getTail();
			unit.bracketContext.popTail();

			if ( context == B_BLOCK )
			{
				if ( unit.loopTracking.count() )
				{
					D_PARSE(Log("End of loop parse"));
				}

				break;
			}
			else if ( context == B_CASE )
			{
				D_SWITCH(Log("End of case"));
			}
			else if ( context == B_UNIT )
			{
				D_PARSE(Log("End of unit, adding implicit: return null;"));
				unit.program += (char)O_LiteralNull;
				unit.program += (char)O_Return;
				break;
			}
			else
			{
				comp.err = "Unexpected '}'";
				return false;
			}
		}
		else if ( token == "unit" )
		{
			if ( !parseUnit(comp, unit, context, token, value) )
			{
				return false;
			}
		}
		else if ( token == "enum" )
		{
			if ( !parseEnum( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "import" )
		{
			if ( !parseImport( comp, unit, context, token, value ) )
			{
				return false;
			}
		}
		else if ( token == "" )
		{
			if (  unit.bracketContext.count() )
			{
				comp.err = "unexpected EOF";
				return false;
			}

			break; // EOF
		}
		else
		{
			context.oper.addHead();
			if ( !parseExpression(comp, context, token, value) )
			{
				return false;
			}
			context.oper.popHead();

			if ( token != ';' )
			{
				comp.pos = comp.lastPos;
				comp.err = "expected ';'";
				return false;
			}

			unit.program += (char)O_PopOne; // all expressions result in a value but we are done with this block so pop it
		}

		if ( preload  )
		{
			break;
		}
	}

	D_PARSE(Log("OUT unit[%d] loop[%d] ", comp.space->count(), unit.loopTracking.count()));
	
	return true;
}

//------------------------------------------------------------------------------
bool link( Compilation& target )
{
	D_LINK(Log("parsed units[%d] values[%d]", target.units.count(), target.values.count()));

	CodeHeader f;
	memset( &f, 0, sizeof(CodeHeader) );
	target.output->set( (char *)&f, sizeof(CodeHeader) );

	CLinkList<UnitEntry>::Iterator unitIter1( target.units );
	CLinkList<UnitEntry>::Iterator unitIter2( target.units );

	// stack the code together
	for( UnitEntry* E = unitIter1.getFirst(); E; E = unitIter1.getNext() )
	{
		E->baseOffset = target.output->size(); // entry point for unit
		target.output->append( E->program.c_str(), E->program.size() );

		D_LINK(Log("unit[%s] %d @%d", E->name.c_str(), E->program.size(), E->baseOffset ));

		for( CodePointer* symbolLocation = E->jumpLinkSymbol.getFirst(); symbolLocation; symbolLocation = E->jumpLinkSymbol.getNext() )
		{
			int *offset = E->jumpIndexes.get( symbolLocation->targetIndex );
			if ( !offset )
			{
				Log("index %d not found", symbolLocation->targetIndex );
				return false;
			}

			int relative = (*offset + E->baseOffset) - (E->baseOffset + symbolLocation->offset);// + symbolLocation->offset;

//			TODO-  something like the following can shorten all the
//			32-bit relative jumps to 8/16 versions but shifts the
//			ENTIRE byte image around so has to be approached carefully
//			(since both sources and target of every move _MIGHT_ need
//			to be recomputed if any one of them change, particularly
//			intersting in the case of a jump/target being beofre AND
//			after a move. approach I want to take is to pre-compute all
//			moves before installing any of them so nothing has to be
//			re-written
/*
			if ( symbolLocation->canOptimize )
			{
				restart = true;
				symbolLocation->canOptimize = false;
				symbolLocation->added = true;

				char* opcode = target.output->c_str( (E->baseOffset + symbolLocation->offset) - 1 );

				if ( (relative >= -127) && (relative <= 128)
				{
					D_LINK(Log("Optimizing 8-bit opcode[%d]", (int)opcode));
					if ( relative > 0 )
					{
						relative -= 3;
					}

					*target.output->c_str( E->baseOffset + symbolLocation->offset ) = relative;
					target.output->shift( E->baseOffset + symbolLocation->offset + 4, E->baseOffset + symbolLocation->offset + 1 );

					for( CodePointer* reLocate = E->jumpLinkSymbol.getFirst(); reLocate; reLocate = E->jumpLinkSymbol.getNext() )
					{
						if ( re
					}


					switch ( *opcode )
					{
						case O_BZ: *opcode = (char)O_BZ8; break;
						case O_BNZ: *opcode = (char)O_BNZ8; break;
						case O_Jump: *opcode = (char)O_Jump8; break;
						default: target.err.format( "unrecognized opcode 0x%02",(unsigned char)*opcode); return false;
					}
				}
			}
			else if ( !symbolLocation->added )
*/
			*(int *)target.output->c_str( E->baseOffset + symbolLocation->offset ) = htonl( relative );
			D_LINK(Log("jump vector[%d] [%d]->[%d] relative[%d]", symbolLocation->targetIndex, E->baseOffset + symbolLocation->offset, *offset + E->baseOffset, relative));
		}
	}
	
	// fill in the missing symbols
	for( UnitEntry* E = target.units.getFirst(); E; E = target.units.getNext() )
	{
		for( LinkEntry* L = E->unitSym.getFirst(); L; L = E->unitSym.getNext() )
		{
			if ( L->enumKey )
			{
				continue;
			}
			
			// default to target base (global)
			*(unsigned int *)target.output->c_str( E->baseOffset + L->offset ) = htonl(hash( L->plainTarget, target));
			D_LINK(Log("value link default[%s] @%d", L->plainTarget.c_str(), E->baseOffset + L->offset ));

			for( UnitEntry* E2 = unitIter2.getFirst(); E2; E2 = unitIter2.getNext() )
			{
				if ( L->target == E2->name )
				{
					D_LINK(Log("unit link [%s] [0x%08X] @%d", E2->name.c_str(), hash(E2->name, target), E->baseOffset + L->offset ));

					// unless a more specific one is found
					*(unsigned int *)target.output->c_str( E->baseOffset + L->offset ) = htonl(hash( E2->name, target ));

					
					break;
				}
			}
		}
	}
	
	// fill in the missing enums
	for( UnitEntry* E = target.units.getFirst(); E; E = target.units.getNext() )
	{
		for( LinkEntry* L = E->unitSym.getFirst(); L; L = E->unitSym.getNext() )
		{
			if ( !L->enumKey )
			{
				continue;
			}

			int* V = target.enumValues.get( L->enumKey );
			if ( !V )
			{
				V = target.enumValues.get( hash(L->target, target) );
				if ( !V )
				{
					target.err.format( "Enum [%s] not defined", L->target.c_str() );
					return false;
				}
			}
			
			*(int *)target.output->c_str( E->baseOffset + L->offset ) = htonl(*V);
		}
	}
	
	return true;
}
