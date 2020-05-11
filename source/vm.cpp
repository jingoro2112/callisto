#include "vm.h"
#include "arch.h"
#include "utf8.h"
#include "array.h"

#include <stdio.h>

#include "object_tpool.h"

int g_Callisto_lastErr = CE_NoError;

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

template<> CObjectTPool<CLinkHash<CLinkHash<UnitDefinition>>::Node> CLinkHash<CLinkHash<UnitDefinition>>::m_linkNodes( 0 );
template<> CObjectTPool<CLinkHash<UnitDefinition>::Node> CLinkHash<UnitDefinition>::m_linkNodes( 6 );
template<> CObjectTPool<CLinkHash<Cstr>::Node> CLinkHash<Cstr>::m_linkNodes( 16 );
template<> CObjectTPool<CLinkHash<ChildUnit>::Node> CLinkHash<ChildUnit>::m_linkNodes( 16 );

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
		--begin; // begining of code
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
	CLinkHash<Value>* useNameSpace = 0;
	int operand1;
	int operand2;

	exec->state = Callisto_ThreadState::Running; // yes I am
	exec->warning = 0;
	
	UnitDefinition* unitDefinition = exec->frame->unitContainer.u;
	CLinkHash<Value>* localSpace = exec->frame->unitContainer.unitSpace;

	for(;;)
	{
		if ( exec->warning )
		{
			if ( exec->warning & 0x80 )
			{
				exec->warning = 0;
				if ( exec->state == Callisto_ThreadState::Waiting )
				{
					popOne( exec );
					exec->numberOfArgs = 0;
//					goto DoYield;
				}
			}

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

					callisto->options.warningCallback( exec->warning, formatSnippet.c_str(), line, col, exec->threadId );
				}
				else
				{
					callisto->options.warningCallback( exec->warning, 0, 0, 0, exec->threadId );
				}
			}

			if ( callisto->options.warningsFatal )
			{
				return g_Callisto_lastErr = exec->warning;
			}

			exec->warning = CE_NoError;
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

				clearValue( exec->object );

				for( ; s>=0; --s ) // iterator is not in a predictable location, walk backward until we find one
				{
					if ( (tempValue = (exec->stack + s))->type > 2 )
					{
						if ( tempValue->type == CTYPE_FRAME ) // can't search past the current frame
						{
							exec->warning = CE_IteratorNotFound;
							break;
						}

						continue;
					}

					valueMirror( &exec->object, tempValue );
					
					CLinkHash<UnitDefinition>* table = callisto->typeFunctions.get( exec->object.type );
					if ( !table )
					{
						exec->warning = CE_FunctionTableNotFound;
						break;
					}

					if ( !(unitDefinition = table->get(key)) || !unitDefinition->function )
					{
						exec->warning = CE_FunctionTableEntryNotFound;
						unitDefinition = exec->frame->unitContainer.u;
						break;
					}

					goto callUnitCFunction;
				}

				if ( s == 0 )
				{
					exec->warning = CE_IteratorNotFound;
				}

				popNum( exec, exec->numberOfArgs );
				getNextStackEntry( exec );

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
					exec->warning = CE_UnitNotFound;
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
							exec->warning |= CE_HandleNotFound; // in case the caller called 'wait'
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
				}
				else // code offset
				{
					useNameSpace = Value::m_unitSpacePool.get();
doUnitCall:
					D_EXECUTE(Log( "O_CallLocalThenGlobal found callisto unit [%d] args @[%d]", exec->numberOfArgs, unitDefinition->textOffset));

					tempValue = getNextStackEntry( exec ); // create a call-frame
					tempValue->frame = Callisto_ExecutionContext::m_framePool.get();
					tempValue->type32 = CTYPE_FRAME;
					
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

/*
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
*/

			case O_CallFromUnitSpace:
			{
				exec->numberOfArgs = *T++;
				Value* unit = (exec->stack + (exec->stackPointer - (exec->numberOfArgs + 1)));
				
				if ( (unit = (unit->type == CTYPE_REFERENCE) ? unit->v : unit)->type != CTYPE_UNIT )
				{
					exec->warning = CE_TriedToCallNonUnit;
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
				if ( exec->frame->returnVector & 0xFF000000 ) // unit construction
				{
					popOne( exec ); // never care about the return value

					if ( (int)(exec->frame->returnVector >> 24) < exec->frame->unitContainer.u->numberOfParents )
					{
						goto nextParent;
					}

					// otherwise construction complete, place it on the stack
					
					T = (exec->frame->returnVector & 0x00FFFFFF) + Tbase;

					popNum( exec, exec->frame->numberOfArguments ); // pop args, leaving frame

					tempValue = exec->stack + (exec->stackPointer - 1);
					clearValueOnly( *tempValue );
					valueMove( tempValue, &exec->frame->unitContainer );
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
					tempValue = exec->stack + (exec->stackPointer - (2 + exec->frame->numberOfArguments));
					clearValueOnly( *tempValue );
					valueMove( tempValue, exec->stack + (exec->stackPointer - 1) );

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
					exec->warning = CE_FirstThreadArgumentMustBeUnit;
					getNextStackEntry( exec ); // null return
					continue;
				}

				// allocate a new thread
				Callisto_ExecutionContext* newExec = getExecutionContext( callisto );

				// set up it's call frame
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
					exec->warning = CE_ImportMustBeString;
					continue;
				}

				// the import process will almost always cause the text
				// string to re-alloc, have to re-base the instruction pointer
				unsigned int preserveOffset = (unsigned int)(T - Tbase);

				int offset = loadEx( callisto, tempValue->string->c_str(), tempValue->string->size() );
				
				if ( offset )
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
					if ( (offset = Callisto_parse( tempValue->string->c_str(),
												   tempValue->string->size(),
												   &out,
												   &size,
												   debug)) )
					{
						popNum( exec, exec->numberOfArgs );
						continue;
					}

					popNum( exec, exec->numberOfArgs );

					offset = loadEx( callisto, out, size );

					delete[] out;
				}
				
				Tbase = callisto->text.c_str();
				T = Tbase + preserveOffset;

				if ( offset > 0 )
				{
					unitDefinition = callisto->rootUnit.u;
					unitDefinition->textOffset = offset;
					Value::m_unitSpacePool.getReference( useNameSpace = callisto->globalNameSpace );

					exec->numberOfArgs = 0;

					goto doUnitCall;
				}
				else
				{
					getNextStackEntry( exec );
					exec->warning = CE_ImportLoadError;
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
				tempValue->type32 = CTYPE_FRAME;

				if ( !(tempValue->frame->unitContainer.u = callisto->unitDefinitions.get(key)) )
				{
					exec->warning = CE_UnitNotFound;
					
					Callisto_ExecutionContext::m_framePool.release( tempValue->frame );
					popNum( exec, exec->numberOfArgs );
					clearValue( *(exec->stack + (exec->stackPointer - 1)) );
					continue;
				}

				tempValue->frame->unitContainer.allocUnit();
				localSpace = tempValue->frame->unitContainer.unitSpace;
				
				tempValue->frame->numberOfArguments = exec->numberOfArgs;
				tempValue->frame->returnVector = (int)(T - Tbase); // also sets "parent" to zero

				tempValue->frame->next = exec->frame;
				exec->frame = tempValue->frame;

nextParent:
				
				// start executing parents in the chain

				int onParent = exec->frame->returnVector >> 24;
				unitDefinition = callisto->unitDefinitions.get( exec->frame->unitContainer.u->parentList[onParent++] );
				exec->frame->returnVector = (exec->frame->returnVector & 0x00FFFFFF) | (onParent << 24);
				
				if ( !unitDefinition )
				{
					exec->warning = CE_ParentNotFound;

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
						exec->warning = CE_IteratorNotFound; break;
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
						exec->warning = CE_IteratorNotFound;
						T += ntohl(*(unsigned int *)T);
						break;
					}
				}
				
				continue;
			}

			case O_ShiftOneDown:
			{
				tempValue = exec->stack + (exec->stackPointer - 2);
				clearValueOnly( *tempValue );
				valueMove( tempValue, exec->stack + (exec->stackPointer - 1) );
				popOne( exec );
				continue;
			}
			
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
		default: return ret.format( "<UNRECOGNIZED TYPE %d>", type ); // eek
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

		default: return ret = "<UNRECOGNIZED TYPE>"; // eek
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

	char newId;
	while( C->threads.get(newId = C->threadIdGenerator++) );
	
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
		E->warning = CE_AssignedValueToStack;
		return;
	}

	to = to->v;

	if ( to->flags & VFLAG_CONST )
	{
		E->warning = CE_ValueIsConst;
		return;
	}

	if ( from->type == CTYPE_REFERENCE )
	{
		from = from->v;
		
		D_ASSIGN(Log("offstack to offstack type [%d]", from->type));

		clearValueOnly( *to );
		valueMirror( to, from );
	}
	else
	{
		D_ASSIGN(Log("assign from stack->offstack, copy data from type [%d]", from->type));

		// can move the value off the stack, by definition it will not
		// be used directly from there
		clearValueOnly( *to );
		valueMove( to, from );
	}
}

//------------------------------------------------------------------------------
void doAddAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

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
			E->warning = CE_IncompatibleDataTypes;
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
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_STRING )
	{
		to->flags &= ~VFLAG_HASH_COMPUTED;

		if ( to->type == CTYPE_STRING )
		{
			*to->string += *from->string;
			D_OPERATOR(Log("addAssign <5> [%s]", to->string->c_str()));
		}
		else
		{
			D_OPERATOR(Log("doAddAssign fail<3> [%d]", to->type));
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		D_OPERATOR(Log("doAddAssign fail<5>"));
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doSubtractAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

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
			E->warning = CE_IncompatibleDataTypes;
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
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doModAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		to->i64 %= from->i64;
		D_OPERATOR(Log("modAssign <1> %lld", to->i64));
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doDivideAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

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
			E->warning = CE_IncompatibleDataTypes;
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
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doMultiplyAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

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
			E->warning = CE_IncompatibleDataTypes;
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
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doCompareEQ( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	int result;

	if ( (from->type & BIT_COMPARE_DIRECT) && (to->type & BIT_COMPARE_DIRECT) )
	{
		result = from->i64 == to->i64;
	}
	else if ( (from->type == CTYPE_NULL) || (to->type == CTYPE_NULL) )
	{
		result = from->type == to->type;
	}
	else if ( (from->type & BIT_CAN_HASH) && (to->type & BIT_CAN_HASH) )
	{
		result = from->getHash() == to->getHash();
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
		return;
	}

	clearValueOnly( *destination );
	destination->type32 = CTYPE_INT;
	destination->i64 = result;
}

//------------------------------------------------------------------------------
void doCompareGT( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

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
		E->warning = CE_IncompatibleDataTypes;
		return;
	}

	clearValueOnly( *destination );
	destination->type32 = CTYPE_INT;
	destination->i64 = result;
}

//------------------------------------------------------------------------------
void doCompareLT( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

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
		E->warning = CE_IncompatibleDataTypes;
		clearValue( *destination );
		return;
	}

	clearValueOnly( *destination );
	destination->type32 = CTYPE_INT;
	destination->i64 = result;
}

//------------------------------------------------------------------------------
void doLogicalAnd( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	int result = (from->i64 && to->i64) ? 1 : 0;

	if ( (from->type & BIT_CAN_LOGIC) && (to->type & BIT_CAN_LOGIC) )
	{
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doLogicalOr( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	int result = (from->i64 || to->i64) ? 1 : 0;
	
	if ( (from->type & BIT_CAN_LOGIC) && (to->type & BIT_CAN_LOGIC) )
	{
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValueOnly( *destination );
		E->warning = CE_IncompatibleDataTypes;
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

		clearValueOnly( *destination );
		destination->type32 = CTYPE_REFERENCE;
		destination->v = member ? member : source->unitSpace->add( key );
	}
	else // must be a type access
	{
		clearValueOnly( E->object );
		valueMirror( &E->object, source );
		
		CLinkHash<UnitDefinition>* table = E->callisto->typeFunctions.get( source->type );

		clearValueOnly(*destination);

		if ( !table )
		{
			destination->type32 = CTYPE_NULL;
			destination->i64 = 0;
			E->warning = CE_FunctionTableEntryNotFound;
			return;
		}

		if ( !(destination->u = table->get(key)) )
		{
			destination->type32 = CTYPE_NULL;
			destination->i64 = 0;
			E->warning = CE_FunctionTableNotFound;
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
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
		D_OPERATOR(Log("negate<1> [%lld]", destination->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		double result = -value->d;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_FLOAT;
		destination->d = result;
		D_OPERATOR(Log("negate<2> [%g]", destination->d));
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
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

		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
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

		clearValueOnly( *destination );
		destination->type32 = CTYPE_FLOAT;
		destination->d = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
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
		
		clearValueOnly( *destination );
		destination->type32 = CTYPE_STRING;
		destination->string = result;
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		Cstr* result = Value::m_stringPool.get();
		result->format( "%g", value->d );
		
		clearValueOnly( *destination );
		destination->type32 = CTYPE_STRING;
		destination->string = result;
	}
	else if ( value->type != CTYPE_STRING )
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doSwitch( Callisto_ExecutionContext* E, const char** T )
{
	Value* value = E->stack + (E->stackPointer - 1);
	if ( value->type == CTYPE_REFERENCE )
	{
		value = value->v;
	}

	const char* jumpTable = *T + ntohl(*(int *)*T); // offset to table
	
	unsigned int cases = ntohl(*(unsigned int *)jumpTable);
	jumpTable += 4;
	
	char switchFlags = *jumpTable++;
	uint64_t hash;

	if ( switchFlags & SWITCH_HAS_NULL )
	{
		if ( value->type == CTYPE_NULL )
		{
			popOne( E );
			goto doSwitchJump;
		}

		jumpTable += 4;
	}

	hash = htobe64( value->getHash() );
	popOne( E );

	for( unsigned int c = 0; c<cases; ++c, jumpTable += 12 )
	{
		if ( hash == *(uint64_t *)jumpTable )
		{
			jumpTable += 8;
			int vector = ntohl(*(int *)jumpTable);
			*T = jumpTable + vector;
			return;
		}
	}

	if ( switchFlags & SWITCH_HAS_DEFAULT )
	{
doSwitchJump:
		int vector = ntohl(*(int *)jumpTable);
		*T = jumpTable + vector;
	}
	else
	{
		*T += 4;
		*T += ntohl(*(int *)*T);
	}
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
			clearValueOnly( *destination );
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
			clearValueOnly( *destination );
			destination->type32 = CTYPE_TABLE_ITERATOR;
			destination->iterator = Value::m_iteratorPool.get();
			destination->iterator->iterator.iterate( *(destination->tableSpace = tableSpace) );
			break;
		}

		case CTYPE_ARRAY:
		{
			Carray<Value>* array;
			Value::m_arrayPool.getReference( array = value->array );
			clearValueOnly( *destination );
			destination->type32 = CTYPE_ARRAY_ITERATOR;
			destination->iterator = Value::m_iteratorPool.get();
			destination->iterator->index = 0;
			destination->array = array;
			break;
		}

		default:
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
}

//------------------------------------------------------------------------------
void doBinaryAddition( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 + from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d + from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else if ( to->type == CTYPE_STRING )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%s%lld", to->string->c_str(), from->i64 );
			clearValueOnly( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 + from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = (int64_t)result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d + from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else if ( to->type == CTYPE_STRING )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%s%g", to->string->c_str(), from->d );
			clearValueOnly( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_STRING )
	{
		if ( to->type == CTYPE_STRING )
		{
			Cstr* result = Value::m_stringPool.get();
			*result = *to->string + *from->string;

			clearValueOnly( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else if ( to->type32 == CTYPE_INT )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%lld%s", to->i64, from->string->c_str() );

			clearValueOnly( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else if ( to->type32 == CTYPE_FLOAT )
		{
			Cstr* result = Value::m_stringPool.get();
			result->format( "%g%s", to->d, from->string->c_str() );

			clearValueOnly( *destination );
			destination->type32 = CTYPE_STRING;
			destination->string = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinarySubtraction( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 - from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d - from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 - from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d - from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryMultiplication( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;
	
	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 * from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doBinaryMultiplication<1> [%lld]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d * from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<2> [%g]", result));
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 * from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<3> [%g]", result));
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d * from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doBinaryMultiplication<4> [%g]", result));
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryDivision( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT )
	{
		if ( to->type == CTYPE_INT )
		{
			int64_t result = to->i64 / from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d / from->i64;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else if ( from->type == CTYPE_FLOAT )
	{
		if ( to->type == CTYPE_INT )
		{
			double result = to->i64 / from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = (int64_t)result;
		}
		else if ( to->type == CTYPE_FLOAT )
		{
			double result = to->d / from->d;
			clearValueOnly( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
		}
		else
		{
			clearValue( *destination );
			E->warning = CE_IncompatibleDataTypes;
		}
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryMod( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 % from->i64;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseRightShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 >> from->i64;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseLeftShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( from->type == CTYPE_INT && to->type == CTYPE_INT )
	{
		int64_t result = to->i64 << from->i64;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;
	
	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 | from->i64;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseAND( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 & from->i64;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doBinaryBitwiseXOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;

	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		int64_t result = to->i64 ^ from->i64;
		clearValueOnly( *destination );
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 |= from->i64;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doXORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 ^= from->i64;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doANDAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 &= from->i64;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doRightShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 >>= from->i64;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void doLeftShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 )
{
	Value* from = E->stack + (E->stackPointer - operand1);
	Value* to = E->stack + (E->stackPointer - operand2);
	Value* destination = to;
	from = (from->type == CTYPE_REFERENCE) ? from->v : from;
	to = (to->type == CTYPE_REFERENCE) ? to->v : to;

	if ( (from->type == CTYPE_INT) && (to->type == CTYPE_INT) )
	{
		to->i64 <<= from->i64;
	}
	else
	{
		clearValue( *destination );
		E->warning = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
void addArrayElement( Callisto_ExecutionContext* E, unsigned int index )
{
	Value* from = getStackValue( E, 1 );
	Value* to = getStackValue( E, 2 );

	if ( to->type != CTYPE_ARRAY )
	{
		clearValueOnly( *to );
		to->allocArray();
	}

	Value& toAdd = ((*to->array)[index]);
	clearValueOnly( toAdd );
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
		E->warning = CE_UnhashableDataType;
		clearValue( *value ); // null it out
	}
	else
	{
		if ( table->type != CTYPE_TABLE )
		{
			clearValueOnly( *table );
			table->allocTable();
		}

		unsigned int hash = (unsigned int)key->getHash();
		CKeyValue* KV = table->tableSpace->get( hash );
		if ( !KV )
		{
			KV = table->tableSpace->add( hash );
		}
		else
		{
			clearValueOnly( KV->value );
			clearValueOnly( KV->key );
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
			clearValueOnly( *destination );
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
			clearValueOnly( *destination );
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
		CKeyValue *KV = table->tableSpace->get( (unsigned int)key->getHash() );
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
	CLinkHash<Value>::Iterator iter( *C->globalNameSpace );
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
