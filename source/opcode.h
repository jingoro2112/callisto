#ifndef CALLISTO_OPCODE_H
#define CALLISTO_OPCODE_H
/*------------------------------------------------------------------------------*/

namespace Callisto
{

//------------------------------------------------------------------------------
enum OpcodeType
{
	O_NOP =0,

	O_CallFromUnitSpace = 1,
	O_CallLocalThenGlobal,
	O_CallGlobalOnly,
	O_CallIteratorAccessor,

	O_Return, // end of a unit

	O_Sleep, // sleep for n milliseconds
	O_Yield, // yield one cycle
	O_Wait, // wait for inner function to finish, if this is a c function, spawn a new thread to process it
	O_WaitGlobalOnly,

	O_Thread, // create another thread and start executing it
	O_ThreadGlobalOnly, // create another thread and start executing it

	O_ClaimArgument, // [as label] define a label which can be referred to within a unit as a passed arg value
	O_ClaimCopyArgument, // [as label] copy the value so it is NOT a reference

	O_LoadLabel, // [label key:] push this label onto the stack
	O_LoadLabelGlobal, // [label key:] push this label onto the stack from global space only

	O_LoadEnumValue,
	O_LoadThis,
	O_LiteralNull, // push a literal NULL onto stack
	O_LiteralInt8, // push an int based on 8-bit source
	O_LiteralInt16, // push an int based on 16-bit source
	O_LiteralInt32, // push an int based on 32-bit source
	O_LiteralInt64,
	O_LiteralFloat,
	O_LiteralString,

	O_PushBlankTable,
	O_PushBlankArray,

	O_PopOne, // pop exactly one value from stack
	O_PopTwo, // pop exactly two values
	O_PopNum, // pop next 0-255 stack entries off
	O_ShiftOneDown, // pop and move top of stack down one, clobbering value below it

	O_Assign,
	O_AssignAndPop,

	// pop 2, compare and leave true or false appropriately
	O_CompareEQ, 
	O_CompareNE, 
	O_CompareGE,
	O_CompareLE,
	O_CompareGT,
	O_CompareLT,

	// 8, 16 and 32 bit versions
	O_BZ, // pop and branch if value was any kind of FALSE
	O_BZ8, // pop and branch if value was any kind of FALSE
	O_BZ16, // pop and branch if value was any kind of FALSE
	O_BNZ, // pop and branch if value was any kind of TRUE
	O_BNZ8, // pop and branch if value was any kind of TRUE
	O_BNZ16, // pop and branch if value was any kind of TRUE
	O_Jump, // jump to following int
	O_Jump8, // jump to following int
	O_Jump16, // jump to following int

	O_LogicalAnd, // &&
	O_LogicalOr, // ||
	O_LogicalNot, // !

	O_PostIncrement, // a++
	O_PostDecrement, // a--
	O_PreIncrement, // ++a
	O_PreDecrement, // ++a
	O_Negate, // !a

	O_MemberAccess, // a.b

	O_CopyValue,

	O_BinaryAddition,
	O_BinarySubtraction,
	O_BinaryMultiplication,
	O_BinaryDivision,
	O_BinaryMod,

	O_BinaryAdditionAndPop,
	O_BinarySubtractionAndPop,
	O_BinaryMultiplicationAndPop,
	O_BinaryDivisionAndPop,
	O_BinaryModAndPop,

	O_AddAssign,
	O_SubtractAssign,
	O_ModAssign,
	O_MultiplyAssign,
	O_DivideAssign,
	O_RightShiftAssign,
	O_LeftShiftAssign,

	O_BitwiseOR,
	O_BitwiseAND,
	O_BitwiseNOT,
	O_BitwiseXOR,
	O_ORAssign,
	O_ANDAssign,
	O_XORAssign,
	O_BitwiseRightShift,
	O_BitwiseLeftShift,

	O_NewUnit,
	O_LoadFromContext,

	O_AddArrayElement, // add element at index to array
	O_AddTableElement,
	O_DereferenceFromTable,
	O_PushIterator,

	O_IteratorGetNextKeyValueOrBranch,
	O_IteratorGetNextValueOrBranch,

	//	O_IteratorGetNextItemOrBranch,

	O_CoerceToInt,
	O_CoerceToString,
	O_CoerceToWideString,
	O_CoerceToFloat,

	O_Switch, // switch on the value currently on the stack with the table pointed to with the next offset

	O_SourceCodeLineRef,
	O_Import,

	O_LAST,
};

//------------------------------------------------------------------------------
struct OpcodeString
{
	char opcode;
	C_str name;
};

//------------------------------------------------------------------------------
const OpcodeString c_opcodes[] =
{
	{ O_CallFromUnitSpace, "O_CallFromUnitSpace" },
	{ O_CallLocalThenGlobal, "O_CallLocalThenGlobal" },
	{ O_CallGlobalOnly, "O_CallGlobalOnly" },
	{ O_CallIteratorAccessor, "O_CallIteratorAccessor" },
	{ O_Return, "O_Return" },
	{ O_Yield, "O_Yield" },
	{ O_Wait, "O_Wait" },
	{ O_WaitGlobalOnly, "O_WaitGlobalOnly" },
	{ O_Thread, "O_Thread" },
	{ O_ThreadGlobalOnly, "O_ThreadGlobalOnly" },
	{ O_Import, "O_Import" },
	{ O_ClaimArgument, "O_ClaimArgument" },
	{ O_ClaimCopyArgument, "O_ClaimCopyArgument" },
	{ O_LoadLabel, "O_LoadLabel" },
	{ O_LoadEnumValue, "O_LoadEnumValue" },
	{ O_LoadThis, "O_LoadThis" },
	{ O_LiteralNull, "O_LiteralNull" },
	{ O_LiteralInt8, "O_LiteralInt8" },
	{ O_LiteralInt16, "O_LiteralInt16" },
	{ O_LiteralInt32, "O_LiteralInt32" },
	{ O_LiteralInt64, "O_LiteralInt64" },
	{ O_LiteralFloat, "O_LiteralFloat" },
	{ O_LiteralString, "O_LiteralString" },
	{ O_PushBlankTable, "O_PushBlankTable" },
	{ O_PushBlankArray, "O_PushBlankArray" },
	{ O_PopOne, "O_PopOne" },
	{ O_PopTwo, "O_PopTwo" },
	{ O_PopNum, "O_PopNum" },
	{ O_ShiftOneDown, "O_ShiftOneDown" },
	{ O_Jump, "O_Jump" },
	{ O_Assign, "O_Assign" },
	{ O_AssignAndPop, "O_AssignAndPop" },
	{ O_CompareEQ, "O_CompareEQ" },
	{ O_CompareNE, "O_CompareNE" },
	{ O_CompareGE, "O_CompareGE" },
	{ O_CompareLE, "O_CompareLE" },
	{ O_CompareGT, "O_CompareGT" },
	{ O_CompareLT, "O_CompareLT" },
	{ O_BNZ, "O_BNZ" },
	{ O_BZ, "O_BZ" },
	{ O_LogicalAnd, "O_LogicalAnd" },
	{ O_LogicalOr, "O_LogicalOr" },
	{ O_LogicalNot, "O_LogicalNot" },
	{ O_PostIncrement, "O_PostIncrement" },
	{ O_PostDecrement, "O_PostDecrement" },
	{ O_PreIncrement, "O_PreIncrement" },
	{ O_PreDecrement, "O_PreDecrement" },
	{ O_Negate, "O_Negate" },
	{ O_MemberAccess, "O_MemberAccess" },
	{ O_CopyValue, "O_CopyValue" },
	{ O_BinaryAddition, "O_BinaryAddition" },
	{ O_BinarySubtraction, "O_BinarySubtraction" },
	{ O_BinaryMultiplication, "O_BinaryMultiplication" },
	{ O_BinaryDivision, "O_BinaryDivision" },
	{ O_BinaryMod, "O_BinaryMod" },
	{ O_BinaryAdditionAndPop, "O_BinaryAdditionAndPop" },
	{ O_BinarySubtractionAndPop, "O_BinarySubtractionAndPop" },
	{ O_BinaryMultiplicationAndPop, "O_BinaryMultiplicationAndPop" },
	{ O_BinaryDivisionAndPop, "O_BinaryDivisionAndPop" },
	{ O_BinaryModAndPop, "O_BinaryModAndPop" },
	{ O_AddAssign, "O_AddAssign" },
	{ O_SubtractAssign, "O_SubtractAssign" },
	{ O_ModAssign, "O_ModAssign" },
	{ O_MultiplyAssign, "O_MultiplyAssign" },
	{ O_DivideAssign, "O_DivideAssign" },
	{ O_NewUnit, "O_NewUnit" },
	{ O_LoadFromContext, "O_LoadFromContext" },
	{ O_AddArrayElement, "O_AddArrayElement" },
	{ O_AddTableElement, "O_AddTableElement" },
	{ O_DereferenceFromTable, "O_DereferenceFromTable" },
	{ O_PushIterator, "O_PushIterator" },
	{ O_IteratorGetNextKeyValueOrBranch, "O_IteratorGetNextKeyValueOrBranch" },
	{ O_IteratorGetNextValueOrBranch, "O_IteratorGetNextValueOrBranch" },
	{ O_CoerceToInt, "O_CoerceToInt" },
	{ O_CoerceToString, "O_CoerceToString" },
	{ O_CoerceToWideString, "O_CoerceToWideString" },
	{ O_CoerceToFloat, "O_CoerceToFloat" },
	{ O_Switch, "O_Switch" },
	{ O_SourceCodeLineRef, "O_SourceCodeLineRef" },

	{ 0, "UNKNOWN_OPCODE" },
};

//------------------------------------------------------------------------------
const C_str c_reserved[] =
{
	{ "break" },
	{ "case" },
	{ "const" },
	{ "continue" },
	{ "copy" },
	{ "default" },
	{ "do" },
	{ "else" },
	{ "enum" },
	{ "false" },
	{ "float" },
	{ "for" },
	{ "foreach" },
	{ "if" },
	{ "import" },
	{ "int" },
	{ "new" },
	{ "null" },
	{ "NULL" },
	{ "return" },
	{ "string" },
	{ "switch" },
	{ "this" },
	{ "thread" },
	{ "true" },
	{ "unit" },
	{ "while" },
	{ "yield" },
	{ "wait" },

	{ "" },
};

}

#endif