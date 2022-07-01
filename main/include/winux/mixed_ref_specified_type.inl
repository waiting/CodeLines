#pragma once

// Mixed 引用指定的类型
//template < MixedType _mt > struct RefSpecifiedType { };
/*
template <> struct RefSpecifiedType<mt> {\
    Mixed * _mx;\
    RefSpecifiedType( Mixed const * mx ) : _mx(const_cast<Mixed*>(mx))\
    { if ( _mx->_type != mt ) throw MixedError( MixedError::meUnexpectedType, TypeString(_mx->_type) + " can't reference " #mt ); }\
    operator ty & () { return _mx->##memb; }\
    operator ty const & () const { return _mx->##memb; }\
};
*/

#define REF_SPECIFIED_TYPE(mt,ty,memb,funcname)\
    ty&ref##funcname(){if(this->_type!=mt)throw MixedError(MixedError::meUnexpectedType,"ref"#funcname"(): "+TypeString(this->_type)+" can not be referenced as a "#mt);return memb;}\
    ty const&ref##funcname()const{if(this->_type!=mt)throw MixedError(MixedError::meUnexpectedType,"ref"#funcname"(): "+TypeString(this->_type)+" can not be referenced as a "#mt);return memb;}


REF_SPECIFIED_TYPE(MT_BOOLEAN, bool, _boolVal, Bool)
REF_SPECIFIED_TYPE(MT_BYTE, byte, _btVal, Byte)
REF_SPECIFIED_TYPE(MT_SHORT, short, _shVal, Short)
REF_SPECIFIED_TYPE(MT_USHORT, ushort, _ushVal, UShort)
REF_SPECIFIED_TYPE(MT_INT, int, _iVal, Int)
REF_SPECIFIED_TYPE(MT_UINT, uint, _uiVal, UInt)
REF_SPECIFIED_TYPE(MT_LONG, long, _lVal, Long)
REF_SPECIFIED_TYPE(MT_ULONG, ulong, _ulVal, ULong)
REF_SPECIFIED_TYPE(MT_FLOAT, float, _fltVal, Float)
REF_SPECIFIED_TYPE(MT_INT64, int64, _i64Val, Int64)
REF_SPECIFIED_TYPE(MT_UINT64, uint64, _ui64Val, UInt64)
REF_SPECIFIED_TYPE(MT_DOUBLE, double, _dblVal, Double)
REF_SPECIFIED_TYPE(MT_ANSI, AnsiString, *_pStr, Ansi)
REF_SPECIFIED_TYPE(MT_UNICODE, UnicodeString, *_pWStr, Unicode)
REF_SPECIFIED_TYPE(MT_ARRAY, MixedArray, *_pArr, Array)
REF_SPECIFIED_TYPE(MT_COLLECTION, MixedMixedMap, *_pMap, Collection)
REF_SPECIFIED_TYPE(MT_BINARY, Buffer, *_pBuf, Buffer)
