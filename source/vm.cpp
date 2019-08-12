#include "util.h"
#include "arch.h"

#include <stdio.h>

#define D_EXECUTE(a) //a
#define D_CALLUNIT(a) //a
#define D_DUMPSTACK(a) //a
#define D_DUMPSYMBOLS(a) //a

//------------------------------------------------------------------------------
inline int callUnit( Callisto_ExecutionContext *context, const int offset, const int numberOfArgs )
{
	int floor = context->stackPointer - numberOfArgs;

	D_EXECUTE(Log("floor[%d] stackPtr[%d] args[%d]", floor, context->stackPointer, numberOfArgs));

	int returnVector = context->pos;

	// 12       <- ptr
	// 11 [arg]
	// 10 [arg] <- floor

	// 10       <- ptr/floor

	D_CALLUNIT(Log("Calling unit @[%d] args[%d] floor[%d]", offset, numberOfArgs, floor));
							 
	if ( execute(context, offset, floor) )
	{
		return g_Callisto_lastErr;
	}

	D_CALLUNIT(Log("<execute> returned"));

	// all arguments consumed/no arguments given
	// 11       <- ptr
	// 10 [ret] <- floor

	context->pos = returnVector;

	return CE_NoError;
}

//------------------------------------------------------------------------------
inline int callCFunc( Callisto_ExecutionContext *context, Unit* unit, const int numberOfArgs )
{
	context->numberOfArgs = numberOfArgs;

	// 12       <- ptr
	// 11 [arg]
	// 10 [arg] <- frame

	// 10       <- ptr <- frame

	context->Args = context->stack + (context->stackPointer - context->numberOfArgs);
	context->userData = unit->userData;

	// 12       <- ptr
	// 11 [arg]
	// 10 [arg] <- frame

	// 10       <- ptr <- frame

	Callisto_Handle ret = unit->function( context );
	if ( ret )
	{
		Value *retVal = context->currentUnit->refLocalSpace->item.get( ret );
		if ( retVal )
		{
			if ( numberOfArgs )
			{
				valueMove( context->stack + (context->stackPointer - numberOfArgs), retVal );
				popNum( context, numberOfArgs - 1);
			}
			else
			{
				getNextStackEntry( context );
				valueMove( context->stack + (context->stackPointer - 1), retVal );
			}

			context->currentUnit->refLocalSpace->item.remove( ret );
		}
		else
		{
			return g_Callisto_lastErr = CE_HandleNotFound;
		}
	}
	else if ( numberOfArgs )
	{
		popNum( context, numberOfArgs - 1 ); // no return value but args, pop and clear
		clearValue( context->stack[context->stackPointer - 1] ); // clear the top 'arg' make it the null return
	}
	else
	{
		getNextStackEntry( context );
	}

	// 13       <- ptr
	// 12 [arg]
	// 11 [arg]
	// 10 [ret] <- frame

	// 11       <- ptr
	// 10 [ret] <- frame


	return CE_NoError;
}

//------------------------------------------------------------------------------
int execute( Callisto_Context *C, int thread, const unsigned int offset, const int stackFloor )
{
	Callisto_ExecutionContext *context = C->threads.get( thread );
	if ( !context )
	{
		return CE_ThreadNotFound;
	}
	return execute( context, offset, stackFloor );
}

//------------------------------------------------------------------------------
inline int callGlobal( Callisto_ExecutionContext *context, Unit* unit, int numberOfArgs )
{
	Value instance;
	instance.type = CTYPE_UNIT;
	instance.u = unit;
	instance.refLocalSpace = new RefCountedVariable<CLinkHash<Value>>(); // create space for local data

	// 12       <- ptr
	// 11 [arg]
	// 10 [arg]

	// 10       <- ptr

	// swap out current context for the new one
	Value* oldUnit = context->currentUnit;
	context->currentUnit = &instance;

	if ( callUnit(context, instance.u->textOffset, numberOfArgs) )
	{
		return g_Callisto_lastErr;
	}

	// 13				<- ptr
	// 12 [ret]
	// 11 [arg2]
	// 10 [arg1]

	// 11				<- ptr
	// 10 [ret]

	context->currentUnit = oldUnit;

	return CE_NoError;
}

//------------------------------------------------------------------------------
int execute( Callisto_ExecutionContext *context, const unsigned int offset, const int stackFloor )
{
	D_EXECUTE(Log("<execute> stack[%d] floor[%d]", context->stackPointer, stackFloor));

	context->pos = offset;
	Cstr* text = &context->callisto->text;
	int returnStackSize = (context->stackPointer - stackFloor) + 1;

	for(;;)
	{
		D_EXECUTE(Log("pos[%d] [%s] stack[%d] >>",
					  context->pos,
					  formatOpcode( (int)*text->c_str(context->pos) ).c_str(),
					  context->stackPointer));
		D_DUMPSYMBOLS( dumpSymbols(context->callisto) );
		D_DUMPSTACK(dumpStack(context));
		D_DUMPSTACK(Log("----------------<<<<<<<"));

		switch( *text->c_str(context->pos++) )
		{
			case O_CallLocalThenGlobal:
			{
				int numberOfArgs = *text->c_str(context->pos++);
				unsigned int key = ntohl(*(unsigned int *)text->c_str(context->pos));
				context->pos += 4;

				D_EXECUTE(Log( "O_CallLocalThenGlobal looking for [%s]", formatSymbol(context->callisto,key).c_str()));

				ChildUnit* C = context->searchChildren->get( key );

				if ( !C )
				{
					C = context->callisto->globalUnit.childUnits.get( key );

					if ( !C )
					{
						return g_Callisto_lastErr = CE_UnitNotFound;
					}
				}

				if ( !C->child )
				{
					C->child = context->callisto->unitList.get( C->key );
					if ( !C->child )
					{
						return g_Callisto_lastErr = CE_MemberNotFound;
					}
				}

				// whatever context is currently loaded is being trusted
				if ( C->child->function ) // C function
				{
					if ( callCFunc( context, C->child, numberOfArgs ) )
					{
						return g_Callisto_lastErr;
					}
				}
				else // code offset
				{

					D_EXECUTE(Log( "O_CallLocalThenGlobal found callisto unit [%d] args @[%d]", numberOfArgs, C->child->textOffset));

					CHashTable<ChildUnit>* oldChildren = context->searchChildren;
					context->searchChildren = &C->child->childUnits;

					if ( callGlobal(context, C->child, numberOfArgs) )
					{
						return g_Callisto_lastErr;
					}

					context->searchChildren = oldChildren;

					if ( numberOfArgs > 0 ) 
					{
						// 13       <- ptr
						// 12 [arg]
						// 11 [arg]
						// 10 [ret] <- floor

						D_CALLUNIT(Log("cleaning up args[%d]", numberOfArgs));

						valueSwap( context->stack + (context->stackPointer - 1),
								   context->stack + (context->stackPointer - (1 + numberOfArgs)) );

						popNum( context, numberOfArgs );
					}
				}

				break;
			}

			case O_CallGlobalOnly:
			{
				int numberOfArgs = *text->c_str(context->pos++);
				unsigned int key = ntohl(*(unsigned int *)text->c_str(context->pos));
				context->pos += 4;

				Unit* unit = context->callisto->unitList.get( key );
				if ( !unit )
				{
					Cstr* symbol = context->callisto->symbols.get( key );
					context->callisto->err.format( "symbol[%s]:0x%08X not found", symbol ? symbol->c_str() : "<null>", key );
					return g_Callisto_lastErr = CE_UnitNotFound;
				}
				
				if ( unit->function ) // C function
				{
					if ( callCFunc( context, unit, numberOfArgs ) )
					{
						return g_Callisto_lastErr;
					}
				}
				else // code offset
				{
					if ( !callGlobal( context, unit, numberOfArgs ) )
					{
						return g_Callisto_lastErr;
					}

					if ( numberOfArgs ) 
					{
						// 13       <- ptr
						// 12 [arg]
						// 11 [arg]
						// 10 [ret] <- floor

						D_CALLUNIT(Log("cleaning up args[%d] <2>", numberOfArgs));

						valueSwap( context->stack + (context->stackPointer - 1),
								   context->stack + (context->stackPointer - (1 + numberOfArgs)) );

						popNum( context, numberOfArgs );
					}
				}

				break;
			}

			case O_CallFromUnitSpace:
			{
				int numberOfArgs = *text->c_str(context->pos++);

				D_EXECUTE(Log( "calling from unit space [%d] args", numberOfArgs));

				// 13		   <- ptr
				// 12 [arg]
				// 11 [arg]
				// 10 [UNIT/META]

				// 11          <- ptr
				// 10 [UNIT/META]

				Value* callOn = context->stack + (context->stackPointer - (numberOfArgs + 1));

				if ( callOn->type == CTYPE_OFFSTACK_REFERENCE )
				{
					if (!callOn->metaTable)
					{
						return g_Callisto_lastErr = CE_MemberNotFound;
					}

					context->numberOfArgs = numberOfArgs + 1;
					context->Args = callOn - 1;
					context->userData = callOn->metaTable->userData;

					getNextStackEntry( context );

					// 13       <- ptr
					// 12 [ret]
					// 11 [arg]
					// 10 [arg]
					//  9 [UNIT]

					// 11		<- ptr
					// 10 [ret]
					//  9 [UNIT]

					Callisto_Handle ret = callOn->metaTable->function( context );

					if ( ret )
					{
						Value *retVal = context->currentUnit->refLocalSpace->item.get( ret );
						if ( retVal )
						{
							valueMove( context->stack + (context->stackPointer - 1), retVal );
							context->currentUnit->refLocalSpace->item.remove( ret );
						}
						else
						{
							return g_Callisto_lastErr = CE_HandleNotFound;
						}
					}
				}
				else if ( callOn->type == CTYPE_UNIT )
				{
					Value* oldUnit = context->currentUnit;
					context->currentUnit = callOn;
				
					if ( callUnit(context, callOn->u->textOffset, numberOfArgs) )
					{
						return g_Callisto_lastErr;
					}

					context->currentUnit = oldUnit;

					// 13       <- ptr
					// 12 [ret]
					// 11 [arg]
					// 10 [arg]
					//  9 [UNIT]

					// 11		<- ptr
					// 10 [ret]
					//  9 [UNIT]
				}
				else
				{
					return g_Callisto_lastErr = CE_CannotOperateOnLiteral;
				}

				D_CALLUNIT(Log("cleaning up args[%d]", numberOfArgs));

				valueSwap( context->stack + (context->stackPointer - 1),
						   context->stack + (context->stackPointer - (2 + numberOfArgs)) );

				popNum( context, numberOfArgs + 1 );

				D_EXECUTE(Log( "Swapped and cleaned up [%d] slots stack[%d]", numberOfArgs + 1, context->stackPointer));

				break;
			}

			case O_Return:
			{
				// if a return happens in the middle of a logic block
				// with a bunch of values on the stack make sure they
				// are all cleaned up
				if ( returnStackSize != (context->stackPointer - stackFloor) )
				{
					// needs cleanup
					D_EXECUTE(Log("Cleaning up for return ret[%d] != offset[%d]", returnStackSize, context->stackPointer - stackFloor));

					// the assumption is the return value has been
					// pushed to the top of the stack, so swap
					// it to where it should be and pop off the balance

					// 14       <- ptr
					// 13 [ret]
					// 12 [..]
					// 11 [..] // args/vals/don't care it's not supposed to be there
					// 10 [..]
					//  9 [..]  <- floor
					
					valueSwap( context->stack + (context->stackPointer - 1),
							   context->stack + stackFloor );

					D_EXECUTE(Log("popping [%d] values", (context->stackPointer - stackFloor) - returnStackSize));

					popNum( context, (context->stackPointer - stackFloor) - returnStackSize );

					// 10       <- ptr
					//  9 [ret] <- floor
				}
				else
				{
					D_EXECUTE(Log("no cleanup required"));
				}

				if ( context->stack[context->stackPointer - 1].type == CTYPE_OFFSTACK_REFERENCE )
				{
					D_EXECUTE(Log("required deep-copy for return"));
					Value *V = context->stack[context->stackPointer - 1].v;
					valueDeepCopy( context->stack + (context->stackPointer - 1), V );
				}
				
				return CE_NoError;
			}

			case O_ClaimCopyArgument:
			{
				char number = *text->c_str( context->pos++ );
				unsigned int key = ntohl(*(unsigned int *)text->c_str( context->pos ));
				context->pos += 4;

				if ( context->stackPointer > stackFloor )
				{
					D_EXECUTE(Log("Claiming COPY arg#%d [0x%08X] ptr[%d] > floor[%d]", (int)number, key, context->stackPointer, stackFloor ));

					Value* V = context->currentUnit->refLocalSpace->item.get( key );
					if ( !V )
					{
						D_EXECUTE(Log("adding to space"));
						V = context->currentUnit->refLocalSpace->item.add( key );
					}

					Value *arg = context->stack + (stackFloor + number);
					if ( arg->type != CTYPE_OFFSTACK_REFERENCE
						 || arg->v != V )
					{
						valueDeepCopy( V, context->stack + (stackFloor + number) );
					}
				}
				else
				{
					D_EXECUTE(Log("Claim COPY for [0x%08X] but floor reached ptr[%d] > floor[%d]", key, context->stackPointer, stackFloor ));
				}
				
				break;
			}

			case O_ClaimArgument:
			{
				char number = *text->c_str( context->pos++ );
				unsigned int key = ntohl(*(unsigned int *)text->c_str( context->pos ));
				context->pos += 4;

				if ( context->stackPointer > stackFloor )
				{
					Value* V = context->currentUnit->refLocalSpace->item.get( key );
					if ( !V )
					{
						D_EXECUTE(Log("adding to space"));
						V = context->currentUnit->refLocalSpace->item.add( key );
					}

					// 13         <- ptr
					// 12 [arg]
					// 11 [arg]
					// 10 [arg]   <- floor

					D_EXECUTE(Log("Claiming arg#%d [0x%08X] ptr[%d] > floor[%d]", (int)number, key, context->stackPointer, stackFloor ));

					Value *arg = context->stack + (stackFloor + number);
					if ( arg->type != CTYPE_OFFSTACK_REFERENCE
						 || arg->v != V )
					{
						valueMirror( V, context->stack + (stackFloor + number) );
					}
				}
				else
				{
					D_EXECUTE(Log("Claim for [0x%08X] but floor reached ptr[%d] > floor[%d]", key, context->stackPointer, stackFloor ));
				}

				break;
			}
						
			case O_Literal:
			{
				getLiteralFromCodeStream( context, *getNextStackEntry(context) );
				break;
			}

			case O_LoadEnumValue:
			{
				Value* N = getNextStackEntry( context );
				N->type = CTYPE_INT;
				int i = (ntohl(*(int *)text->c_str(context->pos)));
				N->i64 = (int64_t)i;
				context->pos += 4;
				break;
			}

			case O_LiteralNull:
			{
				getNextStackEntry( context );
				break;
			}

			case O_LiteralInt8:
			{
				Value* N = getNextStackEntry( context );
				N->type = CTYPE_INT;
				N->i64 = (int64_t)(int8_t)*text->c_str( context->pos++ );
				break;
			}
			
			case O_LiteralInt16:
			{
				Value* N = getNextStackEntry( context );
				N->type = CTYPE_INT;
				N->i64 = (int64_t)(int16_t)ntohs(*(int16_t *)text->c_str( context->pos ));
				context->pos += 2;
				break;
			}

			case O_LiteralInt32:
			{
				Value* N = getNextStackEntry( context );
				N->type = CTYPE_INT;
				N->i64 = (int64_t)(int32_t)ntohl( *(int32_t *)text->c_str(context->pos) );
				context->pos += 4;
				break;
			}

			case O_PushBlankTable:
			{
				Value* N = getNextStackEntry( context );
				N->type = CTYPE_TABLE;
				N->refLocalSpace = new RefCountedVariable<CLinkHash<Value>>();
				break;
			}

			case O_PushBlankArray:
			{
				Value* N = getNextStackEntry( context );
				N->type = CTYPE_ARRAY;
				N->refArray = new RefCountedVariable<Carray>();
				break;
			}

			case O_PopOne:
			{
				popOne( context );
				break;
			}

			case O_PopNum:
			{
				popNum( context, *text->c_str(context->pos++) );
				break;
			}
			
			case O_LoadLabel:
			{
				unsigned int key = ntohl(*(unsigned int *)text->c_str( context->pos ));
				context->pos += 4;
				Value* V = context->currentUnit->refLocalSpace->item.get( key );
				if ( !V )
				{
					D_EXECUTE(Log("first encountered label 0x%08X [%s] adding to space", key, formatSymbol(context->callisto, key).c_str()));
					V = context->currentUnit->refLocalSpace->item.add( key );
				}
				else
				{
					D_EXECUTE(Log("loading label 0x%08X [%s]", key, formatSymbol(context->callisto, key).c_str()));
				}

				Value* stackReference = getNextStackEntry( context );
				stackReference->type = CTYPE_OFFSTACK_REFERENCE;
				if ( V->type != CTYPE_OFFSTACK_REFERENCE )
				{
					stackReference->v = V;
				}
				else
				{
					stackReference->v = V->v;
				}
				break;
			}
			
			case O_Jump:
			{
				D_EXECUTE(Log("jumping to: [%d]", *(int *)text->c_str(context->pos)));
				context->pos += ntohl(*(int *)text->c_str( context->pos ));
				break;
			}

			case O_Assign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doAssign( context, operand1, operand2 );
				break;
			}

			case O_CompareEQ:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doCompareEQ( context, operand1, operand2 );
				break;
			}

			case O_CompareNE:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doCompareEQ( context, operand1, operand2 );
				context->stack[ context->stackPointer - operand2 ].i64 ^= 0x1;
				break;
			}

			case O_CompareGT:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doCompareGT( context, operand1, operand2 );
				break;
			}

			case O_CompareLT:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doCompareLT( context, operand1, operand2 );
				break;
			}

			case O_CompareGE:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doCompareLT( context, operand1, operand2 );
				context->stack[ context->stackPointer - operand2 ].i64 ^= 0x1;
				break;
			}
			
			case O_CompareLE:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				doCompareGT( context, operand1, operand2 );
				context->stack[ context->stackPointer - operand2 ].i64 ^= 0x1;
				break;
			}
			
			case O_BZ:
			{
				Value *V = context->stack + (context->stackPointer - 1);
				if ( V->type == CTYPE_OFFSTACK_REFERENCE )
				{
					V = V->v;
				}
				
				if ( V->i64 )
				{
					D_EXECUTE(Log("O_BZ: NOT branching"));
					context->pos += 4; // not zero, do not branch
				}
				else
				{
					D_EXECUTE(Log("O_BZ: branching"));
					context->pos += ntohl(*(int *)text->c_str( context->pos )); // NOT zero, branch
				}

				popOne( context );
				break;
			}

			case O_BNZ:
			{
				Value *V = context->stack + (context->stackPointer - 1);
				if ( V->type == CTYPE_OFFSTACK_REFERENCE )
				{
					V = V->v;
				}
				
				if ( V->i64 )
				{
					D_EXECUTE(Log("O_BNZ: branching"));
					context->pos += ntohl(*(int *)text->c_str( context->pos )); // NOT zero, branch
				}
				else
				{
					D_EXECUTE(Log("O_BNZ: not branching"));
					context->pos += 4; // zero, do not branch
				}

				popOne( context );
				break;
			}

			case O_LogicalAnd:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doLogicalAnd( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_LogicalOr:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doLogicalOr( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_MemberAccess:
			{
				unsigned int key = ntohl(*(unsigned int *)text->c_str( context->pos ));
				context->pos += 4;
				
				if ( doMemberAccess( context, key ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_LogicalNot:
			{
				if ( doLogicalNot( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_PostIncrement:
			{
				if ( doPostIncrement( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_PostDecrement:
			{
				if ( doPostDecrement( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_PreIncrement:
			{
				if ( doPreIncrement( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_Negate:
			{
				if( doNegate( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_PreDecrement:
			{
				if ( doPreDecrement( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_CopyValue:
			{
				if ( doCopyValue( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_BinaryAddition:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryAddition( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_BinarySubtraction:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinarySubtraction( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_BinaryMultiplication:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryMultiplication( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_BinaryDivision:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryDivision( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_AddAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doAddAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_SubtractAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doSubtractAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_ModAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doModAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_MultiplyAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doMultiplyAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_DivideAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doDivideAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_BitwiseOR:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryBitwiseOR( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_BitwiseAND:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryBitwiseAND( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_BitwiseNOT:
			{
				if ( doBitwiseNot( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_BitwiseXOR:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryBitwiseXOR( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_BitwiseRightShift:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryBitwiseRightShift( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_BitwiseLeftShift:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryBitwiseLeftShift( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_ORAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doORAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_ANDAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doANDAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_XORAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doXORAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_RightShiftAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doRightShiftAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_LeftShiftAssign:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doLeftShiftAssign( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_BinaryMod:
			{
				int operand1 = *text->c_str( context->pos++ );
				int operand2 = *text->c_str( context->pos++ );
				if ( doBinaryMod( context, operand1, operand2 ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
				
			case O_NewUnit:
			{
				int numberOfArgs = *text->c_str( context->pos++ );
				unsigned int key = ntohl(*(unsigned int *)text->c_str( context->pos ));
				context->pos += 4;

				Value instance;

				D_EXECUTE(Log("NewUnit [0x%08X] being passed[%d] arguments, added instance stack[%d]", key, numberOfArgs, context->stackPointer));

				instance.type = CTYPE_UNIT;
				instance.u = context->callisto->unitList.get( key ); // point to template
				
				if ( !instance.u )
				{
					Cstr* symbol = context->callisto->symbols.get( key );
					context->callisto->err.format( "unit template [%s]:0x%08X not found", symbol ? symbol->c_str() : "<null>", key );
					return CE_UnitNotFound;
				}

				instance.refLocalSpace = new RefCountedVariable<CLinkHash<Value>>(); // create space for local data

				// 12       <- ptr
				// 11 [arg]
				// 10 [arg]

				// 10       <- ptr

				// swap out current context for the new one

				// swap out current context for the new one
				Value* oldUnit = context->currentUnit;
				context->currentUnit = &instance;
				
				// for each parent
				for( int i=0; i<instance.u->numberOfParents; i++ )
				{
					Unit* parent = context->callisto->unitList.get( instance.u->parentList[i] );
					if ( !parent  )
					{
						Cstr* symbol = context->callisto->symbols.get( instance.u->parentList[i] );
						context->callisto->err.format( "unit parent [%s]:0x%08X not found", symbol ? symbol->c_str() : "<null>", instance.u->parentList[i] );
						return CE_ParentNotFound;
					}

					CHashTable<ChildUnit>* oldChildren = context->searchChildren;
					context->searchChildren = &parent->childUnits;
		
					D_EXECUTE(Log( "Calling parent unit [0x%08X] [%d] args", instance.u->parentList[i], numberOfArgs));
					if ( callUnit(context, parent->textOffset, numberOfArgs) )
					{
						return g_Callisto_lastErr;
					}

					context->searchChildren = oldChildren;

					// 13				<- ptr
					// 12 [ret]
					// 11 [arg2]
					// 10 [arg1]

					// 11				<- ptr
					// 10 [ret]
 
					popOne( context ); // pop return off
				}

				// 12				<- ptr
				// 11 [arg2]
				// 10 [arg1]

				// 10				<- ptr

				CHashTable<ChildUnit>* oldChildren = context->searchChildren;
				context->searchChildren = &instance.u->childUnits;

				D_EXECUTE(Log("newing unit unit [0x%08X] [%d] args", key, numberOfArgs));
				if ( callUnit(context, instance.u->textOffset, numberOfArgs) )
				{
					return g_Callisto_lastErr;
				}

				context->searchChildren = oldChildren;

				// 13				<- ptr
				// 12 [ret]
				// 11 [arg2]
				// 10 [arg1]

				// 11				<- ptr
				// 10 [ret]

				popOne( context ); // pop return off

				// 12				<- ptr
				// 11 [arg2]
				// 10 [arg1]

				// 10				<- ptr

				if ( numberOfArgs > 0 )
				{
					popNum( context, numberOfArgs );
				}

				valueMove( getNextStackEntry(context), &instance );
				
				context->currentUnit = oldUnit;

				D_EXECUTE(Log("Unit created and placed at stack [%d]", context->stackPointer - 1));

				// 11				<- ptr
				// 10 [instance]
				
				break;
			}

			case O_AddArrayElement:
			{
				unsigned int index = ntohl(*(unsigned int *)text->c_str( context->pos ));
				context->pos += 4;
				addArrayElement( context, index );
				break;
			}

			case O_AddTableElement:
			{
				if ( addTableElement(context) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_DereferenceFromTable:
			{
				dereferenceFromTable( context );
				break;
			}

			case O_PushIterator:
			{
				if ( doPushIterator( context ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_IteratorGetNextKeyValueOrBranch:
			{
				if ( doIteratorGetNextKeyValueOrBranch( context ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_IteratorGetNextValueOrBranch:
			{
				if ( doIteratorGetNextValueOrBranch( context ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
							
			case O_CoerceToInt:
			{
				if( doCoerceToInt( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_CoerceToString:
			{
				if( doCoerceToString( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}
			
			case O_CoerceToWideString:
			{
				if( doCoerceToWString( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_CoerceToFloat:
			{
				if( doCoerceToFloat( context, *text->c_str(context->pos++) ) )
				{
					return g_Callisto_lastErr;
				}
				break;
			}

			case O_Switch:
			{
				if ( doSwitch( context ) )
				{
					return g_Callisto_lastErr;
				}

				break;
			}
			default:
			{
				return g_Callisto_lastErr = CE_UnrecognizedByteCode;
			}
		}
	}

	context->callisto->err.format( "data stream ended unexpectedly");
	return CE_DataErr;
}
