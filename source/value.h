#ifndef VALUE_H
#define VALUE_H
/*------------------------------------------------------------------------------*/

#include "../include/callisto.h"

#include "hash.h"
#include "linkhash.h"
#include "object_tpool.h"
#include "array.h"
#include "str.h"

//------------------------------------------------------------------------------
enum ValueFlags
{
	VFLAG_HASH_COMPUTED = 1<<0,
	VFLAG_CONST = 1<<1,
};

//------------------------------------------------------------------------------
enum ValueTypeBits
{
	// first 4 bits are consumed by the type

	// for quick run-time checks
	BIT_CAN_HASH = 1<<4,
	BIT_CAN_LOGIC = 1<<5,
	BIT_PASS_BY_VALUE = 1<<6,
	BIT_COMPARE_DIRECT = 1<<7,

	// TODO: theoretically these can be OR'ed in for direct assign when the
	// hash is known to be pre-computed, but check endian-ness before
	// comitting to the optimization
	BIT_VFLAG_HASH_COMPUTED = (VFLAG_HASH_COMPUTED)<<24,
	BIT_VFLAG_CONT = (VFLAG_CONST)<<24,
};

//------------------------------------------------------------------------------
enum ValueType
{
	CTYPE_ARRAY_ITERATOR = 0,
	CTYPE_TABLE_ITERATOR = 1,
	CTYPE_STRING_ITERATOR = 2, // so <=2 is an iterator

	CTYPE_UNIT     = 3,
	CTYPE_TABLE    = 4,
	CTYPE_ARRAY    = 5,
	CTYPE_STRING   = 6 | BIT_CAN_HASH, // so switch on "by ref" can be optimal

	CTYPE_NULL     = 7 | BIT_CAN_LOGIC | BIT_PASS_BY_VALUE,
	CTYPE_INT      = 8 | BIT_CAN_HASH | BIT_CAN_LOGIC | BIT_PASS_BY_VALUE | BIT_COMPARE_DIRECT,
	CTYPE_FLOAT    = 9 | BIT_CAN_HASH | BIT_CAN_LOGIC | BIT_PASS_BY_VALUE | BIT_COMPARE_DIRECT,

	CTYPE_FRAME    = 10 | BIT_PASS_BY_VALUE,
	// 11 reserved
	// 12 reserved
	// 13 reserved
	// 14 reserved

	CTYPE_REFERENCE = 15 | BIT_PASS_BY_VALUE,
};

struct Value;
struct ExecutionFrame;
struct CKeyValue;
struct UnitDefinition;

inline void clearValueFunc( Value& V );

void clearValueRef( Value& Value );
#define clearValue(V) { if ( !((V).type & BIT_PASS_BY_VALUE) ) { clearValueRef(V); } (V).type32 = CTYPE_NULL; (V).i64 = 0; } 
#define clearValueOnly(V) { if ( !((V).type & BIT_PASS_BY_VALUE) ) { clearValueRef(V); } } 

//------------------------------------------------------------------------------
struct ValueIterator
{
	int64_t index;
	CLinkHash<CKeyValue>::Iterator iterator;
};

#pragma pack(4) // on a 64-bit system this is a 20 byte struct, 32-bit is 16
//------------------------------------------------------------------------------
struct Value
{
	union // contains only pointers, so 'ref2' can be a void* class
	{
		Carray<Value>* array;
		Cstr* string;
		CLinkHash<CKeyValue>* tableSpace; // for units or tables
		CLinkHash<Value>* unitSpace; // for units or tables
		ExecutionFrame* frame;
		void* hashIterator; // hang iterators from here

		void* ref2; // must be the size of the largest member for quick equates (which might be only 32 bits, this union has pointers only)
	};

	union // contains explicit 64-bit values, so on 32-bit machines void* is not large enough for ref1
	{
		// if this is a value, this is where it is stored
		int64_t i64; 
		uint64_t precomputedHash;
		double d;
		Value* v; // if this is a reference, this points to the actual value
		UnitDefinition* u; // if this is a CTYPE_UNIT, this is a pointer to the Unit, and it may have valid children
		ValueIterator* iterator;
		
		int64_t ref1; // must be the size of the largest member for quick equates
	};

	union 
	{
		ValueType namedType;
		int32_t type32; // let the entire struct be set in one go
		
		struct
		{
			unsigned char type; // must always be first so a set to type32 sets it
			unsigned char flags;
			unsigned char reserved[2]; // reserved for alignment reasons
		};
	};

	uint64_t getHash()
	{
		if ( (flags & VFLAG_HASH_COMPUTED) || (type & BIT_PASS_BY_VALUE) )
		{
//			return precomputedHash;
		}

		flags |= VFLAG_HASH_COMPUTED;
		
		if ( type == CTYPE_STRING )
		{
			precomputedHash = hash64( string->c_str(), string->size() );
		}

		return precomputedHash;
	}

	void allocArray() { type32 = CTYPE_ARRAY; array = m_arrayPool.get(); array->setClearFunction(clearValueFunc); }
	void allocString() { type32 = CTYPE_STRING; string = m_stringPool.get(); }
	void allocTable() { type32 = CTYPE_TABLE; tableSpace = m_tableSpacePool.get(); }
	void allocUnit() { type32 = CTYPE_UNIT; unitSpace = m_unitSpacePool.get(); }

	static CObjectTPool<Carray<Value>> m_arrayPool;
	static CObjectTPool<Cstr> m_stringPool;
	static CObjectTPool<CLinkHash<CKeyValue>> m_tableSpacePool;
	static CObjectTPool<Value> m_valuePool;
	static CObjectTPool<ValueIterator> m_iteratorPool;

	static CObjectTPool<CLinkHash<Value>> m_unitSpacePool;

	static void clear( Value& V ) { clearValue(V); }
	
	Value() : ref1(0), type32(CTYPE_NULL) {}
};
#pragma pack()

//------------------------------------------------------------------------------
struct ChildUnit
{
	unsigned int key;
	UnitDefinition* child;
};

const int c_maxUnitInheritance = 8;

//------------------------------------------------------------------------------
struct UnitDefinition
{
	unsigned int parentList[c_maxUnitInheritance]; // what parents to run when this unit is instantiated
	int numberOfParents;
	bool member;

	unsigned int textOffset; // where in the loaded block this unit resides, if it is callisto native

	Callisto_CallbackFunction function; // what to call if this is a C function palceholder
	void* userData;

	CLinkHash<ChildUnit> childUnits; // list of units this unit is parent to

	UnitDefinition() { member = false; }
};

//------------------------------------------------------------------------------
struct CKeyValue
{
	Value value;
	Value key; // loaded on demand, rarely needed
};

void valueMirrorEx( Value* target, Value* source );

//------------------------------------------------------------------------------
inline void valueMirror( Value* target, Value* source )
{
	target->ref1 = source->ref1;
	if ( !((target->type32 = source->type32) & BIT_PASS_BY_VALUE) )
	{
		valueMirrorEx( target, source );
	}
}

//------------------------------------------------------------------------------
inline bool recycleValue( Value& Value, int type )
{
	if ( Value.type == type )
	{
		return true;
	}

	clearValue( Value );
	return false;
}

//------------------------------------------------------------------------------
inline void valueMove( Value* target, Value* source )
{
	target->type32 = source->type;
	target->ref1 = source->ref1;
	target->ref2 = source->ref2;

	source->type32 = CTYPE_NULL;
	source->ref1 = 0;
}

//------------------------------------------------------------------------------
inline void valueSwap( Value* v1, Value* v2 )
{
	char t = v1->type;
	v1->type32 = v2->type;
	v2->type32 = t;

	uint64_t t1 = v1->ref1;
	v1->ref1 = v2->ref1;
	v2->ref1 = t1;

	void* t2 = v1->ref2;
	v1->ref2 = v2->ref2;
	v2->ref2 = t2;
}

//------------------------------------------------------------------------------
void deepCopyEx( Value* target, Value* source );

//------------------------------------------------------------------------------
inline void deepCopy( Value* target, Value* source )
{
	Value* from = (source->type == CTYPE_REFERENCE) ? source->v : source;
	target->ref1 = from->ref1;

	if ( !((target->type32 = from->type) & BIT_PASS_BY_VALUE) )
	{
		deepCopyEx( target, from );
	}
}

//------------------------------------------------------------------------------
inline void clearValueFunc( Value& V )
{
	clearValue( V );
}

#endif
