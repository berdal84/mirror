#pragma once

#include <mirror_base.h>

#define MIRROR_CLASS(_class, ...)\
public:\
	static mirror::Class* GetClass()\
	{\
		static mirror::Class* s_class = nullptr;\
		if (!s_class)\
		{\
			s_class = new mirror::Class(#_class, typeid(_class).hash_code());\
			mirror::g_classSet.addClass(s_class);\
			char fakePrototype[sizeof(_class)] = {};\
			_class* prototypePtr = reinterpret_cast<_class*>(fakePrototype);\
			_MIRROR_CLASS_CONTENT

#define _MIRROR_CLASS_CONTENT(...)\
			__VA_ARGS__\
		}\
		return s_class;\
	}

#define MIRROR_MEMBER(_memberName)\
	{\
		size_t offset = reinterpret_cast<size_t>(&(prototypePtr->_memberName)) - reinterpret_cast<size_t>(prototypePtr);\
		mirror::TypeDesc* type = mirror::GetTypeDesc(prototypePtr->_memberName);\
		const char* memberName = #_memberName;\
		_MIRROR_MEMBER_CONTENT

#define _MIRROR_MEMBER_CONTENT(...)\
		const char* metaDataString = #__VA_ARGS__##"";\
		mirror::ClassMember* classMember = new mirror::ClassMember(memberName, offset, type, metaDataString);\
		s_class->addMember(classMember);\
	}

#define MIRROR_PARENT(_parentClass)\
	{\
		s_class->addParent(mirror::GetClass<_parentClass>());\
	}


/*#define MIRROR_MEMBER_CSTRING()
#define MIRROR_MEMBER_CFIXEDARRAY()
#define MIRROR_MEMBER_CDYNAMICARRAY()*/