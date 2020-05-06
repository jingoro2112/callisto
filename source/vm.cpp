#include "vm.h"
#include "arch.h"
#include "utf8.h"
#include "array.h"

#include <stdio.h>

#include "object_tpool.h"

int g_Callisto_lastErr = CE_NoError;
int g_Callisto_warn = CE_NoError;

#define D_EXECUTE(a) //a
#define D_DUMPSTACK(a) //a
#define D_DUMPSYMBOLS(a) //a

#define D_ITERATE(a) //a
#define D_GET_VALUE(a) //a
#define D_COMPARE(a) //a
#define D_ASSIGN(a) //a
#define D_CLEAR_VALUE(a) //a
#define D_ADD_TABLE_ELEMENT(a) //a
#define D_COPY(a) //a
#define D_MIRROR(a) //a
#define D_DEREFERENCE(a) //a
#define D_MEMBERACCESS(a) //a

CObjectTPool<ExecutionFrame> Callisto_ExecutionContext::m_framePool( 4, ExecutionFrame::clear );

template<> CObjectTPool<CHashTable<CHashTable<UnitDefinition>>::Node> CHashTable<CHashTable<UnitDefinition>>::m_hashNodes( 0 );

template<> CObjectTPool<CHashTable<UnitDefinition>::Node> CHashTable<UnitDefinition>::m_hashNodes( 6 );
template<> CObjectTPool<CHashTable<Cstr>::Node> CHashTable<Cstr>::m_hashNodes( 32 );
template<> CObjectTPool<CHashTable<ChildUnit>::Node> CHashTable<ChildUnit>::m_hashNodes( 32 );

//------------------------------------------------------------------------------
void lineFromCode( Callisto_Context* context, unsigned int base, unsigned int pos,
				   Cstr* rawSnippet =0, Cstr* formatSnippet =0, unsigned int* line =0, unsigned int* col =0 )
{
	unsigned int p = base;
	unsigned int adjustedPos = base + pos;
	unsigned int l = 0;
	
	for( l = 0; p<context->text.size() && p<adjustedPos; ++p )
	{
		if ( context->text[p] == '\n' )
		{
			l++;
		}
	}

	unsigned int end = (base + pos) + 1;
	for( ; (end < context->text.size()) && (context->text[end] != '\n'); ++end );

	unsigned int begin = base + pos;
	for( ; (begin > base) && (context->text[begin] != '\n') ; --begin );

	if ( context->text[begin] != '\n' )
	{
		// begining of code
		--begin;
	}

	unsigned int c = (pos + base) - begin;

	if ( c == 0 )
	{
		l++;
	}
	
	if ( col )
	{
		*col = c;
	}

	Cstr snippet( context->text.c_str() + begin + 1, end - begin );
	if ( snippet.size() > 0 && snippet[snippet.size() - 1] == '\n' )
	{
		snippet.shave(1);
	}

	if ( rawSnippet )
	{
		(*rawSnippet) = snippet;
	}

	if ( line )
	{
		*line = l;
	}

	if ( formatSnippet )
	{
		bool charFound = false;
		formatSnippet->format( "%06d: %s\n        ", l, snippet.c_str() );
		unsigned int carrot=0;
		for( ; carrot<c; ++carrot )
		{
			if ( !isspace(snippet[carrot]) )
			{
				charFound = true;
			}
			
			if ( snippet[carrot] == '\t' )
			{
				formatSnippet->append("        ");
			}
			else
			{
				formatSnippet->append(' ');
			}
		}

		if ( !charFound )
		{
			for( ;carrot < snippet.size(); ++carrot )
			{
				if ( !isspace(snippet[carrot]) )
				{
					break;
				}
				else if (snippet[carrot] == '\t')
				{
					formatSnippet->append("        ");
				}
				else
				{
					formatSnippet->append(' ');
				}
			}
		}
		
		formatSnippet->append('^');
		formatSnippet->append('\n');
	}
}

//------------------------------------------------------------------------------
static inline void clearExe( CLinkHash<Callisto_ExecutionContext>::Node& E )
{
	E.item.clear();
}
template<> CObjectTPool<CLinkHash<Callisto_ExecutionContext>::Node> CLinkHash<Callisto_ExecutionContext>::m_linkNodes( 4, clearExe );

//------------------------------------------------------------------------------
int execute( Callisto_ExecutionContext *exec )
{
	const char* Tbase = exec->callisto->text.c_str();
	const char* T = Tbase + exec->textOffsetToResume;
	ChildUnit* child = 0;
	unsigned int key = 0;
	unsigned int sourcePos = 0;
	Value* tempValue = 0;
	Callisto_Context* callisto = exec->callisto;
	CHashTable<Value>* useNameSpace = 0;
	int operand1;
	int operand2;
	
	exec->state = Callisto_ThreadState::Running; // yes I am

	UnitDefinition* unitDefinition = exec->frame->unitContainer.u;
	CHashTable<Value>* localSpace = exec->frame->unitContainer.unitSpace;
		
	for(;;)
	{
		if ( g_Callisto_warn != CE_NoError )
		{
			if ( callisto->options.warningCallback )
			{
				if ( sourcePos )
				{
					Cstr formatSnippet;
					unsigned int offset = (unsigned int)(T - Tbase);
					unsigned int topAccumulator = 0;
					unsigned int line;
					unsigned int col;
					for( TextSection* debug = callisto->textSections.getFirst();
						 debug;
						 debug = callisto->textSections.getNext() )
					{
						if ( offset >= debug->textOffsetTop )
						{
							topAccumulator += debug->textOffsetTop;
							continue;
						}
						
						lineFromCode( callisto, debug->sourceOffset + topAccumulator, sourcePos, 0, &formatSnippet, &line, &col );
						break;
					}

					callisto->options.warningCallback( g_Callisto_warn, formatSnippet.c_str(), line, col, exec->threadId );
				}
				else
				{
					callisto->options.warningCallback( g_Callisto_warn, 0, 0, 0, exec->threadId );
				}
			}

			if ( callisto->options.warningsFatal )
			{
				return g_Callisto_lastErr = g_Callisto_warn;
			}

			g_Callisto_warn = CE_NoError;
		}
		
		D_EXECUTE(Log("[%s] stack[%d] >>", formatOpcode( (char)*T ).c_str(), exec->stackPointer));
		D_DUMPSYMBOLS( dumpSymbols(callisto) );
		D_DUMPSTACK(dumpStack(exec));
		D_DUMPSTACK(Log("----------------<<<<<<<"));

 		switch( *T++ )
		{
			case O_CallIteratorAccessor:
			{
				exec->numberOfArgs = *T++;
				key = ntohl( *(unsigned int *)T );
				T += 4;
				
				int s = exec->stackPointer - (exec->numberOfArgs + 1);

				for( ; s>=0; --s )
				{
					if ( (tempValue = (exec->stack + s))->type > 2 )
					{
						continue;
					}

					clearValue( exec->object );
					valueMirror( &exec->object, tempValue );
					
					CHashTable<UnitDefinition>* table = callisto->typeFunctions.get( exec->object.type );
					if ( !table )
					{
						g_Callisto_warn = CE_FunctionTableNotFound;
						popNum( exec, exec->numberOfArgs );
						getNextStackEntry( exec );
						break;
					}

					if ( !(unitDefinition = table->get(key)) || !unitDefinition->function )
					{
						g_Callisto_warn = CE_FunctionTableEntryNotFound;
						unitDefinition = exec->frame->unitContainer.u;
						popNum( exec, exec->numberOfArgs );
						getNextStackEntry( exec );
						break;
					}

					goto callUnitCFunction;
				}

				if ( s == 0 )
				{
					g_Callisto_warn = CE_IteratorNotFound;
				}
				
				continue;
			}

			case O_CallGlobalOnly: // called with :: to skip local resolution
			{
				exec->numberOfArgs = *T++;
				key = ntohl( *(unsigned int *)T );
				T += 4;
				goto globalUnitCheck;
			}

			case O_CallLocalThenGlobal:
			{
				exec->numberOfArgs = *T++;
				key = ntohl( *(unsigned int *)T );
				T += 4;

				D_EXECUTE(Log( "O_CallLocalThenGlobal looking for [%s]", formatSymbol(callisto,key).c_str()));

				child = unitDefinition->childUnits.get( key ); // try current unit
				if ( child )
				{
					unitDefinition = child->child;
					goto memberCheck;
				}
				else
globalUnitCheck:
				if ( !(unitDefinition = callisto->unitDefinitions.get(key)) ) // unit name?
				{
					if ( (tempValue = localSpace->get(key)) ) // calling a variable then?
					{
						tempValue = (tempValue->type == CTYPE_REFERENCE) ? tempValue->v : tempValue;
						if ( tempValue->type == CTYPE_UNIT )
						{
							if ( (unitDefinition = tempValue->u)->function )
							{
								goto callUnitCFunction;
							}

							Value::m_unitSpacePool.getReference( useNameSpace = tempValue->unitSpace );
							goto doUnitCall;
						}
					}

					unitDefinition = exec->frame->unitContainer.u;
					g_Callisto_warn = CE_UnitNotFound;
					popNum( exec, exec->numberOfArgs );
					getNextStackEntry( exec );
					continue;
				}
				else
memberCheck:
				if ( unitDefinition->member )
				{
					Value::m_unitSpacePool.getReference( useNameSpace = localSpace );
					goto doUnitCall; // skip adding new space
				}


				if ( unitDefinition->function ) // C function?
				{
callUnitCFunction:
					exec->Args = exec->stack + (exec->stackPointer - exec->numberOfArgs);
					exec->userData = unitDefinition->userData;

					// 12       <- ptr
					// 11 [arg]
					// 10 [arg] <- frame

					// 10       <- ptr <- frame

					Callisto_Handle ret = unitDefinition->function( exec );

					if ( !exec->numberOfArgs )
					{
						getNextStackEntry( exec );
					}
					else 
					{
						if ( exec->numberOfArgs > 1 )
						{
							popNum( exec, exec->numberOfArgs - 1);
						}

						clearValue( *(exec->stack + (exec->stackPointer - 1)) );
					}

					// at this point the stack has a null on top, need
					// to move a value into it?

					if ( ret )
					{
						if ( ret == Callisto_THIS_HANDLE )
						{
							valueMirror( (exec->stack + (exec->stackPointer - 1)), &exec->object ); 
						}
						else if ( !(tempValue = callisto->globalNameSpace->get(ret)) )
						{
							g_Callisto_warn = CE_HandleNotFound;
						}
						else
						{
							valueMove( exec->stack + (exec->stackPointer - 1), tempValue );
							callisto->globalNameSpace->remove( ret );
						}
					}

					clearValue( exec->object );

					// otherwise there is nothing to do, a null is already there

					unitDefinition = exec->frame->unitContainer.u;

					// were we yielded in the c call?
					if ( exec->state == Callisto_ThreadState::Waiting )
					{
						popOne( exec ); // yield resume will be the return value
						exec->numberOfArgs = 0;
						goto DoYield;
					}
				}
				else // code offset
				{
					useNameSpace = Value::m_unitSpacePool.get();
doUnitCall:
					D_EXECUTE(Log( "O_CallLocalThenGlobal found callisto unit [%d] args @[%d]", exec->numberOfArgs, unitDefinition->textOffset));

					tempValue = getNextStackEntry( exec ); // create a call-frame
					tempValue->frame = Callisto_ExecutionContext::m_framePool.get();
					
					tempValue->frame->onParent = 0;
					tempValue->frame->unitContainer.u = unitDefinition;
					tempValue->frame->unitContainer.type32 = CTYPE_UNIT;
					localSpace = (tempValue->frame->unitContainer.unitSpace = useNameSpace);

					tempValue->frame->returnVector = (int)(T - Tbase);
					T = Tbase + unitDefinition->textOffset;

					tempValue->frame->numberOfArguments = exec->numberOfArgs;

					tempValue->frame->next = exec->frame;
					exec->frame = tempValue->frame;
				}

				continue;
			}

			case O_Yield:
			{
				exec->numberOfArgs = *T++;
DoYield:
				exec->Args = exec->stack + (exec->stackPointer - exec->numberOfArgs);
				exec->textOffsetToResume = (unsigned int)(T - Tbase);
				exec->state = Callisto_ThreadState::Waiting;

				// on resume a value will be on top of the stack

				return CE_NoError;
			}

			case O_CallFromUnitSpace:
			{
				exec->numberOfArgs = *T++;
				Value* unit = (exec->stack + (exec->stackPointer - (exec->numberOfArgs + 1)));
				
				if ( (unit = (unit->type == CTYPE_REFERENCE) ? unit->v : unit)->type != CTYPE_UNIT )
				{
					g_Callisto_warn = CE_TriedToCallNonUnit;
					popNum( exec, exec->numberOfArgs );
					getNextStackEntry( exec );
					continue;
				}
				
				unitDefinition = unit->u;
				if ( unitDefinition->function )
				{
					goto callUnitCFunction;
				}
				
				useNameSpace = unit->unitSpace;
				unit->type32 = CTYPE_NULL;
				unit->i64 = 0;
				
				goto doUnitCall;
			}

			case O_Return:
			{
				if ( exec->frame->onParent ) // unit construction
				{
					popOne( exec ); // never care about the return value

					if ( exec->frame->onParent < exec->frame->unitContainer.u->numberOfParents )
					{
						goto nextParent;
					}

					// otherwise construction complete, place it on the stack
					
					T = exec->frame->returnVector + Tbase;

					popNum( exec, exec->frame->numberOfArguments ); // pop args, leaving frame

					valueMove( exec->stack + (exec->stackPointer - 1), &exec->frame->unitContainer );
				}
				else // normal return
				{
					if ( (T = exec->frame->returnVector + Tbase) == Tbase )
					{
						exec->state = Callisto_ThreadState::Reap;
						return CE_NoError;
					}

					// move the return value to the bottom of the stack
					// and pop off everything else
					valueMove( exec->stack + (exec->stackPointer - (2 + exec->frame->numberOfArguments)),
							   exec->stack + (exec->stackPointer - 1) );

					popNum( exec, 1 + exec->frame->numberOfArguments );

					Value *R = exec->stack + (exec->stackPointer - 1);
					if ( R->type == CTYPE_REFERENCE )
					{
						Value *RV = R->v;
						D_EXECUTE(Log("required deep-copy for return"));

						R->ref1 = R->v->ref1;
						if ( !((R->type32 = RV->type32) & BIT_PASS_BY_VALUE) )
						{
							deepCopyEx( R, RV );
						}
					}
				}

				ExecutionFrame* F = exec->frame->next;
				Callisto_ExecutionContext::m_framePool.release( exec->frame );
				exec->frame = F;

				unitDefinition = exec->frame->unitContainer.u;
				localSpace = exec->frame->unitContainer.unitSpace;

				continue;
			}


			case O_Thread:
			{
				exec->numberOfArgs = *T++;

				tempValue = exec->stack + (exec->stackPointer - exec->numberOfArgs);
				if ( tempValue->type == CTYPE_REFERENCE )
				{
					tempValue = tempValue->v;
				}

				if ( (tempValue->type != CTYPE_UNIT) || (tempValue->u->function) )
				{
					popNum( exec, exec->numberOfArgs ); // clean up
					g_Callisto_warn = CE_FirstThreadArgumentMustBeUnit;
					getNextStackEntry( exec ); // null return
					continue;
				}

				// allocate a new thread
				Callisto_ExecutionContext* newExec = getExecutionContext( callisto );

				// set up it's call frame
				newExec->frame->onParent = 0;
				newExec->frame->unitContainer.u = tempValue->u;
				newExec->frame->unitContainer.unitSpace = Value::m_unitSpacePool.get();
				newExec->frame->unitContainer.type32 = CTYPE_UNIT;
				newExec->frame->returnVector = 0; // return when done
				newExec->textOffsetToResume = tempValue->u->textOffset;
				newExec->frame->numberOfArguments = exec->numberOfArgs - 1;

				for( int i=0; i<newExec->frame->numberOfArguments ; ++i )
				{
					valueMove( getNextStackEntry(newExec), exec->stack + (exec->stackPointer - i) );
				}

				// guaranteeed to be at least one arg, pop off all but
				// one and change it to the int thread return value
				popNum( exec, newExec->frame->numberOfArguments );
				
				tempValue = exec->stack + (exec->stackPointer - 1);
				clearValue( *tempValue );
				tempValue->type32 = CTYPE_INT;
				tempValue->i64 = newExec->threadId;

				continue;
			}

			case O_Import:
			{
				exec->numberOfArgs = *T++;
				tempValue = exec->stack + (exec->stackPointer - exec->numberOfArgs);

				// first arg must be a string
				tempValue = (tempValue->type == CTYPE_REFERENCE) ? tempValue->v : tempValue;

				if ( tempValue->type != CTYPE_STRING )
				{
					popNum( exec, exec->numberOfArgs );
					g_Callisto_warn = CE_ImportMustBeString;
					continue;
				}

				// the import process will almost always cause the text
				// string to re-alloc, have to re-base the instruction pointer
				unsigned int preserveOffset = (unsigned int)(T - Tbase);

				g_Callisto_warn = loadEx( callisto, tempValue->string->c_str(), tempValue->string->size() );
				
				if ( g_Callisto_warn )
				{
					bool debug = false;
					if ( exec->numberOfArgs > 1 )
					{
						Value* V = exec->stack + (exec->stackPointer - (exec->numberOfArgs - 1));

						V = (V->type == CTYPE_REFERENCE) ? V->v : V;

						debug = (V->type & BIT_CAN_LOGIC) && V->i64;
					}

					char* out = 0;
					unsigned int size = 0;
					if ( (g_Callisto_warn = Callisto_parse( tempValue->string->c_str(),
															tempValue->string->size(),
															&out,
															&size,
															debug)) )
					{
						popNum( exec, exec->numberOfArgs );
						continue;
					}

					popNum( exec, exec->numberOfArgs );

					g_Callisto_warn = loadEx( callisto, out, size );

					delete[] out;
				}
				
				Tbase = callisto->text.c_str();
				T = Tbase + preserveOffset;

				if ( g_Callisto_warn > 0 )
				{
					unitDefinition = callisto->rootUnit.u;
					unitDefinition->textOffset = g_Callisto_warn;
					g_Callisto_warn = 0;
					Value::m_unitSpacePool.getReference( useNameSpace = callisto->globalNameSpace );

					exec->numberOfArgs = 0;

					goto doUnitCall;
				}
				else
				{
					g_Callisto_warn = CE_ImportLoadError;
				}
				
				continue;
			}

			case O_ClaimCopyArgument:
			{
				char number = *T++;
				key = ntohl(*(unsigned int *)T );
				T += 4;

				if ( !(tempValue = localSpace->get(key)) )
				{
					D_EXECUTE(Log("adding to space"));
					tempValue = localSpace->add( key );
				}
				else
				{
					clearValue( *tempValue );
				}

				if ( number < exec->frame->numberOfArguments )
				{
					D_EXECUTE(Log("Claiming COPY arg #%d [0x%08X]", (int)number, key ));
					deepCopy( tempValue, exec->stack + (exec->stackPointer - ((1 + exec->frame->numberOfArguments) - number)) );
				}
				
				continue;
			}

			case O_ClaimArgument:
			{
				char number = *T++;
				key = ntohl(*(unsigned int *)T);
				T += 4;

				if ( !(tempValue = localSpace->get(key)) )
				{
					D_EXECUTE(Log("adding to space"));
					tempValue = localSpace->add( key );
				}
				else
				{
					clearValue( *tempValue );
				}

				if ( number < exec->frame->numberOfArguments )
				{
					D_EXECUTE(Log("Claiming arg #%d [0x%08X]", (int)number, key ));
					valueMirror( tempValue, exec->stack + (exec->stackPointer - ((1 + exec->frame->numberOfArguments) - number)) );
				}

				continue;
			}
						
			case O_LoadThis:
			{
				tempValue = getNextStackEntry( exec );
				tempValue->type32 = CTYPE_REFERENCE;
				tempValue->v = &exec->frame->unitContainer;
				continue;	
			}

			case O_LiteralInt8:
			{
				D_EXECUTE(Log("O_LiteralInt8 [%d][0x%02X][0x%016llX]", (int)*T, (unsigned char)*T, (int64_t)(int8_t)*T ));
				tempValue = getNextStackEntry( exec );
				tempValue->type32 = CTYPE_INT;
				tempValue->i64 = (int64_t)(int8_t)*T++;

				D_EXECUTE(Log("out [%lld][0x%016llX]", tempValue->i64, tempValue->i64 ));
				continue;
			}
			
			case O_LiteralInt16:
			{
				tempValue = getNextStackEntry( exec );
				tempValue->type32 = CTYPE_INT;
				tempValue->i64 = (int64_t)(int16_t)ntohs( *(int16_t *)T );
				T += 2;
				continue;
			}
			
			case O_LiteralInt32:
			{
				tempValue = getNextStackEntry( exec );
				tempValue->type32 = CTYPE_INT;
				tempValue->i64 = (int64_t)(int32_t)ntohl( *(int32_t *)T );
				T += 4;
				continue;
			}

			case O_LiteralInt64:
			{
				tempValue = getNextStackEntry( exec );
				tempValue->type32 = CTYPE_INT;
				tempValue->i64 = ntohll( *(int64_t *)T );
				T += 8;
				continue;
			}

			case O_LiteralFloat:
			{
				tempValue = getNextStackEntry( exec );
				tempValue->type32 = CTYPE_FLOAT;
				tempValue->i64 = ntohll( *(int64_t *)T );
				T += 8;
				continue;
			}

			case O_LiteralString:
			{
				tempValue = getNextStackEntry( exec );
				tempValue->allocString();
				readStringFromCodeStream( *tempValue->string, &T );
				continue;
			}

			case O_LoadLabelGlobal:
			{
				key = ntohl( *(unsigned int *)T );
				T += 4;
				tempValue = callisto->globalNameSpace->get( key );
				if ( !tempValue )
				{
					goto tryLoadingUnit;
				}
				
				goto LabelToStack;
			}

			case O_LoadLabel:
			{
				key = ntohl( *(unsigned int *)T );
				T += 4;
				if ( !(tempValue = localSpace->get(key)) )
				{
tryLoadingUnit:
					// is it a unit name?
					UnitDefinition* unit = callisto->unitDefinitions.get( key );
					if ( unit )
					{
						tempValue = getNextStackEntry( exec );
						tempValue->allocUnit();
						tempValue->u = unit;
						continue;
					}
					else
					{
						tempValue = localSpace->add( key );
					}
				}
LabelToStack:
				Value* stackReference = getNextStackEntry( exec );
				stackReference->type32 = CTYPE_REFERENCE;
				if ( tempValue->type != CTYPE_REFERENCE )
				{
					stackReference->v = tempValue;
				}
				else
				{
					stackReference->v = tempValue->v;
				}

				continue;
			}

			case O_CompareGE:
			{
				operand1 = *T++;
				operand2 = *T++;
				doCompareLT( exec, operand1, operand2 );
				exec->stack[ exec->stackPointer - operand2 ].i64 ^= 0x1;
				continue;
			}
			
			case O_CompareLE:
			{
				operand1 = *T++;
				operand2 = *T++;
				doCompareGT( exec, operand1, operand2 );
				exec->stack[ exec->stackPointer - operand2 ].i64 ^= 0x1;
				continue;
			}
			
			case O_BZ:
			{
				tempValue = exec->stack + (exec->stackPointer - 1);
				if ( (tempValue->type == CTYPE_REFERENCE) ? tempValue->v->i64 : tempValue->i64 )
				{
					D_EXECUTE(Log("O_BZ: NOT branching"));
					T += 4; // not zero, do not branch
				}
				else
				{
					D_EXECUTE(Log("O_BZ: branching"));
					T += (int)ntohl(*(int *)T);
				}
				
				popOne( exec );
				continue;
			}

			case O_BZ16:
			{
				tempValue = exec->stack + (exec->stackPointer - 1);
				if ( (tempValue->type == CTYPE_REFERENCE) ? tempValue->v->i64 : tempValue->i64 )
				{
					D_EXECUTE(Log("O_BZ16: NOT branching"));
					T += 2; // not zero, do not branch
				}
				else
				{
					D_EXECUTE(Log("O_BZ16: branching"));
					T += (int16_t)ntohs( *(int16_t *)T );
				}

				popOne( exec );
				continue;
			}

			case O_BZ8:
			{
				tempValue = exec->stack + (exec->stackPointer - 1);
				if ( (tempValue->type == CTYPE_REFERENCE) ? tempValue->v->i64 : tempValue->i64 )
				{
					D_EXECUTE(Log("O_BZ8: NOT branching"));
					++T;
				}
				else
				{
					D_EXECUTE(Log("O_BZ8: branching"));
					T += *T;
				}

				popOne( exec );
				continue;
			}

			case O_BNZ:
			{
				tempValue = exec->stack + (exec->stackPointer - 1);
				if ( (tempValue->type == CTYPE_REFERENCE) ? tempValue->v->i64 : tempValue->i64 )
				{
					D_EXECUTE(Log("O_BNZ: branching"));
					T += (int)ntohl(*(int *)T); 
				}
				else
				{
					D_EXECUTE(Log("O_BNZ: not branching"));
					T += 4; // zero, do not branch
				}

				popOne( exec );
				continue;
			}

			case O_BNZ16:
			{
				tempValue = exec->stack + (exec->stackPointer - 1);
				if ( (tempValue->type == CTYPE_REFERENCE) ? tempValue->v->i64 : tempValue->i64 )
				{
					D_EXECUTE(Log("O_BNZ16: branching"));
					T += (int16_t)ntohs( *(int16_t *)T );
				}
				else
				{
					D_EXECUTE(Log("O_BNZ16: NOT branching"));
					T += 2; // not zero, do not branch
				}

				popOne( exec );
				continue;
			}

			case O_BNZ8:
			{
				tempValue = exec->stack + (exec->stackPointer - 1);
				if ( (tempValue->type == CTYPE_REFERENCE) ? tempValue->v->i64 : tempValue->i64 )
				{
					D_EXECUTE(Log("O_BNZ8: branching"));
					T += *T;
				}
				else
				{
					D_EXECUTE(Log("O_BNZ8: NOT branching"));
					++T;
				}

				popOne( exec );
				continue;
			}

			case O_CopyValue:
			{
				tempValue = exec->stack + (exec->stackPointer - *T++);
				if ( tempValue->type == CTYPE_REFERENCE )
				{
					Value* from = tempValue->v;
					clearValue( *tempValue );
					deepCopy( tempValue, from );
				}
				continue;
			}

			case O_NewUnit:
			{
				exec->numberOfArgs = *T++;
				key = ntohl(*(unsigned int *)T);
				T += 4;

				tempValue = getNextStackEntry( exec ); // create a call-frame
				tempValue->frame = Callisto_ExecutionContext::m_framePool.get();

				if ( !(tempValue->frame->unitContainer.u = callisto->unitDefinitions.get(key)) )
				{
					g_Callisto_warn = CE_UnitNotFound;
					
					Callisto_ExecutionContext::m_framePool.release( tempValue->frame );
					popNum( exec, exec->numberOfArgs );
					clearValue( *(exec->stack + (exec->stackPointer - 1)) );
					continue;
				}

				tempValue->frame->unitContainer.allocUnit();
				localSpace = tempValue->frame->unitContainer.unitSpace;
				
				tempValue->frame->onParent = 0;
				tempValue->frame->numberOfArguments = exec->numberOfArgs;
				tempValue->frame->returnVector = (int)(T - Tbase);

				tempValue->frame->next = exec->frame;
				exec->frame = tempValue->frame;

nextParent:
				// start executing parents in the chain
				unitDefinition = callisto->unitDefinitions.get( exec->frame->unitContainer.u->parentList[exec->frame->onParent++] );
				if ( !unitDefinition )
				{
					g_Callisto_warn = CE_ParentNotFound;

					ExecutionFrame* F = exec->frame->next;
					Callisto_ExecutionContext::m_framePool.release( exec->frame );
					exec->frame = F;

					unitDefinition = exec->frame->unitContainer.u;
					localSpace = exec->frame->unitContainer.unitSpace;
					
					popNum( exec, exec->numberOfArgs );
					clearValue( *(exec->stack + (exec->stackPointer - 1)) );
					continue;
				}

				T = Tbase + unitDefinition->textOffset;

				// and the calls will now begin, and when return is
				// called, just the constructed unit will remain
				
				continue;
			}

			case O_IteratorGetNextKeyValueOrBranch:
			{
				unsigned int keyKey = ntohl( *(unsigned int *)T );
				T += 4;

				unsigned int valueKey = ntohl( *(unsigned int *)T );
				T += 4;

				if ( !(tempValue = localSpace->get(valueKey)) )
				{
					tempValue = localSpace->add( valueKey );
				}
				else
				{
					clearValue( *tempValue );
				}

				Value* tempKey;
				if ( !(tempKey = localSpace->get(keyKey)) )
				{
					tempKey = localSpace->add( keyKey );
				}
				else
				{
					clearValue( *tempKey );
				}

				Value* V = exec->stack + (exec->stackPointer - 1);
				switch( V->type )
				{
					case CTYPE_ARRAY_ITERATOR:
					{
						if ( V->iterator->index < V->array->count() )
						{
							tempKey->type32 = CTYPE_INT;
							tempKey->i64 = V->iterator->index;
							valueMirror( tempValue, &((*V->array)[(unsigned int)(V->iterator->index++)]) );
							T += 4;
						}
						else
						{
							T += ntohl( *(unsigned int *)T );
						}

						break;
					}

					case CTYPE_TABLE_ITERATOR:
					{
						CKeyValue* KV = V->iterator->iterator.getNext();
						if ( KV )
						{
							valueMirror( tempKey, &KV->key );
							valueMirror( tempValue, &KV->value );
							T += 4;
						}
						else
						{
							T += ntohl( *(unsigned int *)T );
						}
						break;
					}

					case CTYPE_STRING_ITERATOR:
					{
						if ( V->iterator->index < V->string->size() )
						{
							tempKey->type32 = CTYPE_INT;
							tempKey->i64 = V->iterator->index;
							tempValue->type32 = CTYPE_INT;
							tempValue->i64 = *V->string->c_str( (unsigned int)V->iterator->index++ );
							T += 4;
						}
						else
						{
							T += ntohl( *(unsigned int *)T );
						}

						break;
					}

					default: 
					{
						T += ntohl(*(unsigned int *)T);
						g_Callisto_warn = CE_IteratorNotFound; break;
					}
				}

				continue;
			}

			case O_IteratorGetNextValueOrBranch:
			{
				unsigned int key = ntohl( *(unsigned int *)T );
				T += 4;

				if ( !(tempValue = localSpace->get(key)) )
				{
					tempValue = localSpace->add( key );
				}
				else
				{
					clearValue( *tempValue );
				}
							   
				Value* V = exec->stack + (exec->stackPointer - 1);
				switch( V->type )
				{
					case CTYPE_ARRAY_ITERATOR:
					{
						if ( V->iterator->index < V->array->count() )
						{
							valueMirror( tempValue, &((*V->array)[(unsigned int)(V->iterator->index++)]) );
							T += 4;
						}
						else
						{
							T += ntohl( *(unsigned int *)T );
						}
						
						break;
					}
					
					case CTYPE_TABLE_ITERATOR:
					{
						CKeyValue* KV = V->iterator->iterator.getNext();
						if ( KV )
						{
							valueMirror( tempValue, &KV->value );
							T += 4;
						}
						else
						{
							T += ntohl( *(unsigned int *)T );
						}
						break;
					}
					
					case CTYPE_STRING_ITERATOR:
					{
						if ( V->iterator->index < V->string->size() )
						{
							tempValue->type32 = CTYPE_INT;
							tempValue->i64 = *V->string->c_str( (unsigned int)V->iterator->index++ );
							T += 4;
						}
						else
						{
							T += ntohl( *(unsigned int *)T );
						}

						break;
					}

					default: 
					{
						// iterating an unknown type, guess.. branch out?
						g_Callisto_warn = CE_IteratorNotFound;
						T += ntohl(*(unsigned int *)T);
						break;
					}
				}
				
				continue;
			}

			case O_ShiftOneDown:
				valueMove( exec->stack + (exec->stackPointer - 2), exec->stack + (exec->stackPointer - 1) );
				popOne( exec );
				continue;
			
			case O_PopTwo: popOne( exec ); // intentional fall-through
			case O_PopOne: popOne( exec ); continue;

			case O_NOP: continue;
			case O_PushBlankTable: { getNextStackEntry(exec)->allocTable(); continue; }
			case O_PushBlankArray: { getNextStackEntry(exec)->allocArray(); continue; }
			case O_PopNum: { char n = *T++; popNum( exec, n ); continue; }
			case O_Jump: { int vector = ntohl( *(int *)T ); T += vector; continue; }
			case O_Assign: { operand1 = *T++; doAssign( exec, operand1, *T++ ); continue; }
			case O_AssignAndPop: { operand1 = *T++; doAssign( exec, operand1, *T++ ); popOne( exec ); continue; }
			case O_CompareEQ: { operand1 = *T++; doCompareEQ( exec, operand1, *T++ ); continue; }
			case O_CompareNE: { operand1 = *T++; operand2 = *T++; doCompareEQ( exec, operand1, operand2 ); exec->stack[ exec->stackPointer - operand2 ].i64 ^= 0x1; continue; }
			case O_CompareGT: { operand1 = *T++; doCompareGT( exec, operand1, *T++ ); continue; }
			case O_CompareLT: { operand1 = *T++; doCompareLT( exec, operand1, *T++ ); continue; }
			case O_MemberAccess: { key = ntohl( *(unsigned int *)T ); T += 4; doMemberAccess( exec, key ); continue; }
			case O_LiteralNull: getNextStackEntry( exec ); continue;
			case O_LogicalAnd: { operand1 = *T++; doLogicalAnd( exec, operand1, *T++ ); continue; }
			case O_LogicalOr: { operand1 = *T++; doLogicalOr( exec, operand1, *T++ ); continue; }
			case O_LogicalNot: doLogicalNot( exec, *T++ ); continue;
			case O_PostIncrement: doPostIncrement(exec, *T++); continue;
			case O_PostDecrement: doPostDecrement(exec, *T++); continue;
			case O_PreIncrement: doPreIncrement( exec, *T++ ); continue;
			case O_PreDecrement: doPreDecrement( exec, *T++); continue;
			case O_Negate: doNegate( exec, *T++ ); continue;
			case O_BinaryAdditionAndPop: { operand1 = *T++; doBinaryAddition( exec, operand1, *T++ ); popOne( exec ); continue; }
			case O_BinaryAddition: { operand1 = *T++; doBinaryAddition( exec, operand1, *T++ ); continue; }
			case O_BinarySubtractionAndPop: { operand1 = *T++; doBinarySubtraction( exec, operand1, *T++ ); popOne( exec ); continue; }
			case O_BinarySubtraction: { operand1 = *T++; doBinarySubtraction( exec, operand1, *T++ ); continue; }
			case O_BinaryMultiplicationAndPop: { operand1 = *T++; doBinaryMultiplication( exec, operand1, *T++ ); popOne( exec ); continue; }
			case O_BinaryMultiplication: { operand1 = *T++; doBinaryMultiplication( exec, operand1, *T++ ); continue; }
			case O_BinaryDivisionAndPop: { operand1 = *T++; doBinaryDivision( exec, operand1, *T++ ); popOne( exec ); continue; }
			case O_BinaryDivision: { operand1 = *T++; doBinaryDivision( exec, operand1, *T++ ); continue; }
			case O_BinaryModAndPop: { operand1 = *T++; doBinaryMod( exec, operand1, *T++ ); popOne( exec ); continue; }
			case O_BinaryMod: { operand1 = *T++; doBinaryMod( exec, operand1, *T++ ); continue; }
			case O_AddAssign: { operand1 = *T++; doAddAssign( exec, operand1, *T++ ); continue; }
			case O_SubtractAssign: { operand1 = *T++; doSubtractAssign( exec, operand1, *T++ ); continue; }
			case O_MultiplyAssign: { operand1 = *T++; doMultiplyAssign( exec, operand1, *T++ ); continue; }
			case O_DivideAssign: { operand1 = *T++; doDivideAssign( exec, operand1, *T++ ); continue; }
			case O_ModAssign: { operand1 = *T++; doModAssign( exec, operand1, *T++ ); continue; }
			case O_BitwiseOR: { operand1 = *T++; doBinaryBitwiseOR( exec, operand1, *T++ ); continue; }
			case O_BitwiseAND: { operand1 = *T++; doBinaryBitwiseAND( exec, operand1, *T++ ); continue; }
			case O_BitwiseNOT: doBitwiseNot( exec, *T++ ); continue;
			case O_BitwiseXOR: { operand1 = *T++; doBinaryBitwiseXOR( exec, operand1, *T++ ); continue; }
			case O_BitwiseRightShift: { operand1 = *T++; doBinaryBitwiseRightShift( exec, operand1, *T++ ); continue; }
			case O_BitwiseLeftShift: { operand1 = *T++; doBinaryBitwiseLeftShift( exec, operand1, *T++ ); continue; }
			case O_ORAssign: { operand1 = *T++; doORAssign( exec, operand1, *T++ ); continue; }
			case O_ANDAssign: { operand1 = *T++; doANDAssign( exec, operand1, *T++ ); continue; }
			case O_XORAssign: { operand1 = *T++; doXORAssign( exec, operand1, *T++ ); continue; }
			case O_RightShiftAssign: { operand1 = *T++; doRightShiftAssign( exec, operand1, *T++ ); continue; }
			case O_LeftShiftAssign: { operand1 = *T++; doLeftShiftAssign( exec, operand1, *T++ ); continue; }
			case O_AddTableElement: addTableElement(exec); continue;
			case O_AddArrayElement:	{ unsigned int index = ntohl(*(unsigned int *)T); T += 4; addArrayElement( exec, index ); continue; }
			case O_DereferenceFromTable: dereferenceFromTable( exec ); continue;
			case O_PushIterator: doPushIterator(exec); continue;
			case O_CoerceToInt: doCoerceToInt(exec, *T++); continue;
			case O_CoerceToString: doCoerceToString(exec, *T++); continue;
			case O_CoerceToFloat: doCoerceToFloat(exec, *T++); continue;
			case O_Switch: doSwitch(exec, &T); continue;

			case O_SourceCodeLineRef:
			{
				sourcePos = ntohl( *(unsigned int *)T );
				T += 4;

				if ( callisto->options.traceCallback )
				{
					unsigned int offset = (unsigned int)(T - Tbase);
					unsigned int topAccumulator = 0;
					for( TextSection* debug = callisto->textSections.getFirst(); debug; debug = callisto->textSections.getNext() )
					{
						if ( offset >= debug->textOffsetTop )
						{
							topAccumulator += debug->textOffsetTop;
							continue;
						}

						Cstr formatSnippet;
						unsigned int line;
						unsigned int col;
						lineFromCode( callisto, debug->sourceOffset + topAccumulator, sourcePos, 0, &formatSnippet, &line, &col );
						callisto->options.traceCallback( formatSnippet.c_str(), line, col, exec->threadId );
						break;
					}
				}

				continue;
			}

			default: return g_Callisto_lastErr = CE_UnrecognizedByteCode; // truly an "in the weeds" problem
		}
	}
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
		case CTYPE_ARRAY: return ret = "CTYPE_ARRAY";
		case CTYPE_REFERENCE: return ret = "CTYPE_REFERENCE";
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
		case CTYPE_INT: return ret.format( "%lld", value.i64 );
		case CTYPE_FLOAT: return ret.format( "%g", value.d );
		case CTYPE_TABLE: return ret.format( "TABLE[%d]", value.tableSpace->count() );
		case CTYPE_UNIT: return ret.format( "UNIT[%p:%p]", value.u, value.unitSpace );
		case CTYPE_STRING: return ret = *value.string;
		case CTYPE_ARRAY: return ret.format( "ARRAY[%d]", value.array->count());
		case CTYPE_REFERENCE:
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
Callisto_ExecutionContext* getExecutionContext( Callisto_Context* C, const long threadId )
{
	if ( threadId )
	{
		return C->threads.get( threadId );
	}

	long newId = 1;
	while( C->threads.get( newId = InterlockedIncrement(&(C->threadIdGenerator)) ) );
	
	Callisto_ExecutionContext *exec = C->threads.get( newId );

	if ( !exec )
	{
		exec = C->threads.add( newId );

		exec->stackPointer = 0;
		exec->stackSize = 32;
		exec->stack = new Value[exec->stackSize];

		exec->frame->returnVector = 0;
		exec->frame->unitContainer.type32 = CTYPE_UNIT;
		exec->frame->unitContainer.u = C->rootUnit.u;
		Value::m_unitSpacePool.getReference( exec->frame->unitContainer.unitSpace = C->globalNameSpace );
		exec->frame->onParent = 0;
		
		exec->threadId = newId;
		exec->callisto = C;
		
		exec->state = Callisto_ThreadState::Runnable;
	}

	return exec;
}

//------------------------------------------------------------------------------
void readStringFromCodeStream( Cstr& string, const char** T )
{
	unsigned int len;
	len = ntohl(*(unsigned int *)*T );
	*T += 4;
	if ( len )
	{
		string.set( *T, len );
		*T += len;
	}
	else
	{
		string.clear();
	}
}

//------------------------------------------------------------------------------
void doAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	
	if ( to->type != CTYPE_REFERENCE )
	{
		D_ASSIGN(Log("Assigning to the stack is undefined"));
		g_Callisto_warn = CE_AssignedValueToStack;
		return;
	}

	to = to->v;

	if ( to->flags & VFLAG_CONST )
	{
		g_Callisto_warn = CE_ValueIsConst;
		return;
	}

	if ( from->type == CTYPE_REFERENCE )
	{
		from = from->v;
		
		D_ASSIGN(Log("offstack to offstack type [%d]", from->type));

		clearValue( *to );
		valueMirror( to, from );
	}
	else
	{
		D_ASSIGN(Log("assign from stack->offstack, copy data from type [%d]", from->type));

		// can move the value off the stack, by definition it will not
		// be used directly from there

		valueMove( to, from );
	}
}

//------------------------------------------------------------------------------
void doAddAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_STRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			*to->string += *from->string;
			D_OPERATOR(Log("addAssign <5> [%s]", to->string->c_str()));
		}
		else
		{
			D_OPERATOR(Log("doAddAssign fail<3> [%d]", to->type));
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		D_OPERATOR(Log("doAddAssign fail<5>"));
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doSubtractAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double d = to->i64 + from->d;
			to->d = d;
			to->type32 = CTYPE_FLOAT;
			
			D_OPERATOR(Log("subtractAssign <3> %lld", to->i64));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			to->d -= from->d;
			D_OPERATOR(Log("subtractAssign <4> %g", to->i64));
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doModAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
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
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doDivideAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doMultiplyAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
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
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doCompareEQ( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result;

	if ( from->type == CTYPE_NULL )
	{
		result = to->type == CTYPE_NULL;
	}
	else if ( (from->type & BIT_CAN_HASH) && (to->type & BIT_CAN_HASH) )
	{
		result = from->getHash() == to->getHash();
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
		return;
	}

	clearValue( *destination );
	destination->type32 = CTYPE_INT;
	destination->i64 = result;
}

//------------------------------------------------------------------------------
void doCompareGT( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result;

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
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
		return;
	}

	clearValue( *destination );
	destination->type32 = CTYPE_INT;
	destination->i64 = result;
}

//------------------------------------------------------------------------------
void doCompareLT( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result;

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
		g_Callisto_warn = CE_IncompatibleDataTypes;
		clearValue( *destination );
		return;
	}

	clearValue( *destination );
	destination->type32 = CTYPE_INT;
	destination->i64 = result;
}

//------------------------------------------------------------------------------
void doLogicalAnd( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result = (from->i64 && to->i64) ? 1 : 0;
	clearValue( *destination );

	if ( (from->type & BIT_CAN_LOGIC) && (to->type & BIT_CAN_LOGIC) )
	{
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doLogicalOr( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	int result = (from->i64 || to->i64) ? 1 : 0;
	clearValue( *destination );
	
	if ( (from->type & BIT_CAN_LOGIC) && (to->type & BIT_CAN_LOGIC) )
	{
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doMemberAccess( Callisto_ExecutionContext* E, const unsigned int key )
{
	D_EXECUTE(Log( "Member access of [%s]", formatSymbol(E->callisto, key).c_str()));

	Value* source = E->stack + (E->stackPointer - 1);
	Value* destination = source;
	if ( source->type == CTYPE_REFERENCE )
	{
		source = source->v;
	}

	if ( source->type == CTYPE_UNIT )
	{
		Value* member = source->unitSpace->get( key );
		if ( !member )
		{
			ChildUnit *C = source->u->childUnits.get( key );
			if ( C )
			{
				clearValue( *destination );
				destination->type32 = CTYPE_UNIT;
				if ( (destination->u = C->child)->member )
				{	
					Value::m_unitSpacePool.getReference( destination->unitSpace = source->unitSpace );
				}
				else
				{
					Value::m_unitSpacePool.getReference( destination->unitSpace = E->frame->unitContainer.unitSpace );
				}
				return;
			}
		}

		clearValue( *destination );
		destination->type32 = CTYPE_REFERENCE;
		destination->v = member ? member : source->unitSpace->add( key );
	}
	else // must be a type access
	{
		clearValue( E->object );
		valueMirror( &E->object, source );
		
		CHashTable<UnitDefinition>* table = E->callisto->typeFunctions.get( source->type );

		clearValue( *destination );

		if ( !table )
		{
			g_Callisto_warn = CE_FunctionTableEntryNotFound;
			return;
		}

		if ( !(destination->u = table->get(key)) )
		{
			g_Callisto_warn = CE_FunctionTableNotFound;
			return;
		}

		destination->type32 = CTYPE_UNIT;
		destination->unitSpace = 0;
	}
}

//------------------------------------------------------------------------------
void doNegate( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_INT )
	{
		int64_t result = -value->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
		D_OPERATOR(Log("negate<1> [%lld]", destination->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		double result = -value->d;
		clearValue( *destination );
		destination->type32 = CTYPE_FLOAT;
		destination->d = result;
		D_OPERATOR(Log("negate<2> [%g]", destination->d));
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doCoerceToInt( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type & BIT_CAN_HASH )
	{
		int64_t result = value->i64;
		
		if ( value->type == CTYPE_FLOAT )
		{
			result = (int64_t)value->d;
		}
		else if ( value->type == CTYPE_STRING )
		{
			result = strtoull( *value->string, 0, 10 );
		}

		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doCoerceToFloat( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type & BIT_CAN_HASH )
	{
		double result = value->d;

		if ( value->type == CTYPE_INT )
		{
			result = (double)value->i64;
		}
		else if ( value->type == CTYPE_STRING )
		{
			result = strtod( *value->string, 0 );
		}

		clearValue( *destination );
		destination->type32 = CTYPE_FLOAT;
		destination->d = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doCoerceToString( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type == CTYPE_REFERENCE )
	{
		value = value->v;
	}

	if ( value->type == CTYPE_INT )
	{
		Cstr* result = Value::m_stringPool.get();
		result->format( "%lld", value->i64 );
		
		clearValue( *destination );
		destination->type32 = CTYPE_STRING;
		destination->string = result;
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		Cstr* result = Value::m_stringPool.get();
		result->format( "%g", value->d );
		
		clearValue( *destination );
		destination->type32 = CTYPE_STRING;
		destination->string = result;
	}
	else if ( value->type != CTYPE_STRING )
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doSwitch( Callisto_ExecutionContext* E, const char** T )
{
	Value* value = E->stack + (E->stackPointer - 1);
	uint64_t hash = (value->type == CTYPE_REFERENCE) ? value->v->getHash() : value->getHash();

	const char* jumpTable = *T + ntohl(*(int *)*T);
	*T += 4; // offset to table
	*T += ntohl(*(int *)*T); // jump to entry
	
	unsigned int cases = ntohl(*(unsigned int *)jumpTable);
	jumpTable += 4;
	
	char defaultExists = *jumpTable++;

	unsigned char c = 0;
	for( ; c<cases; c++ )
	{
		if ( hash == ntohl(*(unsigned int *)jumpTable) )
		{
			jumpTable += 4;
			int vector = ntohl(*(int *)jumpTable);
			*T = jumpTable + vector;
			break;
		}

		jumpTable += 8;
	}

	if ( (c >= cases) && defaultExists )
	{
		// default is always packed last, just back up and jump to it
		jumpTable -= 4;
		int vector = ntohl(*(int *)jumpTable);
		*T = jumpTable + vector;
	}

	popOne( E );
}

//------------------------------------------------------------------------------
void doPushIterator( Callisto_ExecutionContext* E )
{
	Value* value = E->stack + (E->stackPointer - 1);
	Value* destination = value;
	if ( value->type == CTYPE_REFERENCE )
	{
		value = value->v;
	}

	switch( value->type )
	{
		case CTYPE_STRING:
		{
			Cstr* string;
			Value::m_stringPool.getReference( string = value->string );
			clearValue( *destination );
			destination->type32 = CTYPE_STRING_ITERATOR;
			destination->iterator = Value::m_iteratorPool.get();
			destination->iterator->index = 0;
			destination->string = string;
			break;
		}

		case CTYPE_TABLE:
		{
			CLinkHash<CKeyValue>* tableSpace;
			Value::m_tableSpacePool.getReference( tableSpace = value->tableSpace );
			clearValue( *destination );
			destination->type32 = CTYPE_TABLE_ITERATOR;
			destination->iterator = Value::m_iteratorPool.get();
			destination->iterator->iterator.iterate( *(destination->tableSpace = tableSpace) );
			break;
		}

		case CTYPE_ARRAY:
		{
			Carray<Value>* array;
			Value::m_arrayPool.getReference( array = value->array );
			clearValue( *destination );
			destination->type32 = CTYPE_ARRAY_ITERATOR;
			destination->iterator = Value::m_iteratorPool.get();
			destination->iterator->index = 0;
			destination->array = array;
			break;
		}

		default:
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
}

//------------------------------------------------------------------------------
void doBinaryAddition( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 + from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d + from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else if ( to->type == CTYPE_STRING )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%s%lld", to->string->c_str(), from->i64 );
			clearValue( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 + from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = (int64_t)result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d + from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else if ( to->type == CTYPE_STRING )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%s%g", to->string->c_str(), from->d );
			clearValue( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_STRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			Cstr* result = Value::m_stringPool.get();
			*result = *to->string + *from->string;

			clearValue( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else if ( to->type32 == CTYPE_INT )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%lld%s", to->i64, from->string->c_str() );

			clearValue( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else if ( to->type32 == CTYPE_FLOAT )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%g%s", to->d, from->string->c_str() );

			clearValue( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinarySubtraction( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 - from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d - from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 - from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d - from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryMultiplication( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	
	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 * from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doBinaryMultiplication<1> [%lld]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d * from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<2> [%g]", result));
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 * from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<3> [%g]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d * from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<4> [%g]", result));
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryDivision( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 / from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d / from->i64;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 / from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = (int64_t)result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d / from->d;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			g_Callisto_warn = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryMod( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 % from->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseRightShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 >> from->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseLeftShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 << from->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	
	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 | from->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseAND( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 & from->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseXOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 ^ from->i64;
		clearValue( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 |= from->i64;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doXORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 ^= from->i64;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doANDAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 &= from->i64;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doRightShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 >>= from->i64;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doLeftShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	BINARY_ARG_SETUP;
	destination = destination;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 <<= from->i64;
	}
	else
	{
		clearValue( *destination );
		g_Callisto_warn = CE_IncompatibleDataTypes;
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
		to->allocArray();
	}

	Value& toAdd = ((*to->array)[index]);
	clearValue( toAdd );
	valueMirror( &toAdd, from );

	popOne( E );
}

//------------------------------------------------------------------------------
void addTableElement( Callisto_ExecutionContext* E )
{
	Value* value = getStackValue( E, 1 );
	Value* key = getStackValue( E, 2 );
	Value* table = getStackValue( E, 3 );

	D_ADD_TABLE_ELEMENT(Log("Stack before---------"));
	D_ADD_TABLE_ELEMENT(dumpStack(E));
	D_ADD_TABLE_ELEMENT(Log("---------------------"));
	
	if ( !(key->type & BIT_CAN_HASH) )
	{
		g_Callisto_warn = CE_UnhashableDataType;
		clearValue( *value ); // null it out
	}
	else
	{
		if ( table->type != CTYPE_TABLE )
		{
			clearValue( *table );
			table->allocTable();
		}

		int hash = key->getHash();
		CKeyValue* KV = table->tableSpace->get( hash );
		if ( !KV )
		{
			KV = table->tableSpace->add( hash );
		}
		else
		{
			clearValue( KV->value );
			clearValue( KV->key );
		}

		valueMirror( &KV->value, value );
		valueMirror( &KV->key, key );

		D_ADD_TABLE_ELEMENT(Log("Stack after----------"));
		D_ADD_TABLE_ELEMENT(dumpStack(E));
		D_ADD_TABLE_ELEMENT(Log("---------------------"));
	}
	
	popOne( E );
	popOne( E );
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
			clearValue( *destination );
			destination->type32 = CTYPE_REFERENCE;
			destination->v = &((*table->array)[(unsigned int)key->i64]);
		}
		else
		{
			clearValue( *destination );
		}
	}
	else if ( table->type == CTYPE_STRING )
	{
		if ( key->type == CTYPE_INT )
		{
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = *table->string->c_str( (unsigned int)key->i64 );
		}
		else
		{
			clearValue( *destination );
		}
	}
	else if ( key->type & BIT_CAN_HASH )
	{
		CKeyValue *KV = table->tableSpace->get( key->getHash() );
		clearValue( *destination );
		if ( KV )
		{
			destination->type32 = CTYPE_REFERENCE;
			destination->v = &KV->value;
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

//------------------------------------------------------------------------------
bool isValidLabel( Cstr& token, bool doubleColonOkay )
{
	if ( !token.size() || (!isalpha(token[0]) && token[0] != '_') ) // non-zero size and start with alpha or '_' ?
	{
		return false;
	}

	for( unsigned int i=0; c_reserved[i].size(); i++ )
	{
		if ( token == c_reserved[i] )
		{
			return false;
		}
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
void dumpStack( Callisto_ExecutionContext* E )
{
	for( int i=0; i<E->stackPointer; i++ )
	{
		if ( E->stack[i].type == CTYPE_REFERENCE )
		{
			D_DUMPSTACK(Log( "%d: [ref]->%s [%s]\n", i, formatType(E->stack[i].v->type).c_str(), formatValue(*E->stack[i].v).c_str() ));
		}
		else
		{
			D_DUMPSTACK(Log( "%d: %s [%s]\n", i,  formatType(E->stack[i].type).c_str(), formatValue(E->stack[i]).c_str() ));
		}
	}
}

//------------------------------------------------------------------------------
void dumpSymbols( Callisto_Context* C )
{
	CHashTable<Value>::Iterator iter( *C->globalNameSpace );
	for( Value* V = iter.getFirst(); V; V = iter.getNext() )
	{
		if ( V->type == CTYPE_REFERENCE )
		{
			D_DUMPSTACK(Log( "%p->%p: %d:[%s]\n", V, V->v, V->v->type, formatValue(*V->v).c_str() ));
		}
		else
		{
			D_DUMPSTACK(Log( "%p: %d:[%s]\n", V, V->type, formatValue(*V).c_str() ));
		}
	}
}
