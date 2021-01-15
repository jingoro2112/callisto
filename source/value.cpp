#include "value.h"
#include "c_json_parser.h"

namespace Callisto
{

CCJsonValue CCJsonValue::m_deadValue;
template<> CTPool<CCLinkHash<CCJsonValue>::Node> CCLinkHash<CCJsonValue>::m_linkNodes( 0, 0 );

//------------------------------------------------------------------------------
inline void clearV( CCLinkHash<Value>::Node& N )
{
	clearValue( N.item );
}
template<> CTPool<CCLinkHash<Value>::Node> CCLinkHash<Value>::m_linkNodes( 16, clearV );

//------------------------------------------------------------------------------
inline void clearUnitSpace( CCLinkHash<Value>& space )
{
	space.clear();
}
CTPool<CCLinkHash<Value>> Value::m_unitSpacePool( 4, clearUnitSpace );

//------------------------------------------------------------------------------
static void clearArrayPool( Carray<Value>& array )
{
	array.clear();
}

CTPool<Carray<Value>> Value::m_arrayPool( 4, clearArrayPool );

CTPool<C_str> Value::m_stringPool( 16 );

//------------------------------------------------------------------------------
static inline void clearKV( CCLinkHash<CKeyValue>::Node& N )
{
	clearValue( N.item.key );
	clearValue( N.item.value );
}
template<> CTPool<CCLinkHash<CKeyValue>::Node> CCLinkHash<CKeyValue>::m_linkNodes( 6, clearKV );

//------------------------------------------------------------------------------
static void clearTableSpace( CCLinkHash<CKeyValue>& space )
{
	space.clear();
}
CTPool<CCLinkHash<CKeyValue>> Value::m_tableSpacePool( 4, clearTableSpace );

CTPool<Value> Value::m_valuePool( 32, Value::clear );

CTPool<ValueIterator> Value::m_iteratorPool( 4 );

//------------------------------------------------------------------------------
void clearValueRef( Value& value )
{
	switch( value.type )
	{
		case CTYPE_STRING_ITERATOR: Value::m_iteratorPool.release( value.iterator );
		case CTYPE_STRING: Value::m_stringPool.release( value.string ); return;

		case CTYPE_TABLE_ITERATOR: Value::m_iteratorPool.release( value.iterator );
		case CTYPE_TABLE: Value::m_tableSpacePool.release( value.tableSpace ); return;

		case CTYPE_ARRAY_ITERATOR: Value::m_iteratorPool.release( value.iterator );
		case CTYPE_ARRAY: Value::m_arrayPool.release( value.array ); return;
						  
		case CTYPE_UNIT: Value::m_unitSpacePool.release( value.unitSpace ); return;
	}
}

//------------------------------------------------------------------------------
void valueMirrorEx( Value* target, Value* source )
{
	switch( source->type )
	{
		case CTYPE_UNIT:
			Value::m_unitSpacePool.getReference( target->unitSpace = source->unitSpace );
			return;

		case CTYPE_STRING_ITERATOR:
			Value::m_iteratorPool.getReference( target->iterator = source->iterator );
		case CTYPE_STRING:
			Value::m_stringPool.getReference( target->string = source->string );
			return;
			
		case CTYPE_TABLE_ITERATOR:
			Value::m_iteratorPool.getReference( target->iterator = source->iterator );
		case CTYPE_TABLE:
			Value::m_tableSpacePool.getReference( target->tableSpace = source->tableSpace );
			return;
			
		case CTYPE_ARRAY_ITERATOR:
			Value::m_iteratorPool.getReference( target->iterator = source->iterator );
		case CTYPE_ARRAY:
			Value::m_arrayPool.getReference( target->array = source->array );
			return;
	}
}

//------------------------------------------------------------------------------
void deepCopyEx( Value* target, Value* source )
{
	switch( target->type )
	{
		case CTYPE_STRING:
		{
			target->allocString();
			*target->string = *source->string;
			break;
		}

		case CTYPE_UNIT:
		{
			target->allocUnit();

			CCLinkHash<Value>::Iterator iter( *source->unitSpace );
			for( Value* V = iter.getFirst(); V; V = iter.getNext() )
			{
				Value* N = target->unitSpace->add( iter.getCurrentKey() );
				deepCopy( N, V );
			}
			break;
		}

		case CTYPE_TABLE:
		{
			target->allocTable();
			CCLinkHash<CKeyValue>::Iterator iter( *source->tableSpace );
			for( CKeyValue* K = iter.getFirst(); K; K = iter.getNext() )
			{
				CKeyValue* N = target->tableSpace->add( iter.getCurrentKey() );
				deepCopy( &N->value, &K->value );
				deepCopy( &N->key, &K->key );
			}
			break;
		}

		case CTYPE_ARRAY:
		{
			target->allocArray();
			for( unsigned int i=0; i<source->array->count(); ++i )
			{
				deepCopy( &(*target->array)[i], &(*source->array)[i] );
			}
			break;
		}
	}
}

}