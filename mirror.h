#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <set>
#include <string.h>
#include <type_traits>
#include <typeinfo>

#include <mirror_types.h>

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
		mirror::ClassMember classMember(memberName, offset, type, metaDataString);\
		s_class->addMember(classMember);\
	}

#define MIRROR_PARENT(_parentClass)\
	{\
		s_class->addParent(mirror::GetClass<_parentClass>());\
	}


/*#define MIRROR_MEMBER_CSTRING()
#define MIRROR_MEMBER_CFIXEDARRAY()
#define MIRROR_MEMBER_CDYNAMICARRAY()*/

namespace mirror
{
	class Class;
	class TypeDesc;

	class TypeDesc
	{
	public:
		TypeDesc(Type _type) : m_type(_type) {}

		// NOTE(Remi|2020/04/26): virtual specifier is not needed, but is added to allow the debugger to show inherited types
		virtual Type getType() const { return m_type; }

	private:
		Type m_type = Type_none;
	};

	class PointerTypeDesc : public TypeDesc
	{
	public:
		PointerTypeDesc(TypeDesc* _subType);

		TypeDesc* getSubType() const { return m_subType; }

	private:
		TypeDesc* m_subType;
	};

	struct ClassMember
	{
		friend class Class;

		ClassMember(const char* _name, size_t _offset, TypeDesc* _type, const char* _metaDataString);

		const char* getName() const;
		size_t getOffset() const;
		TypeDesc* getType() const;
		void* getInstanceMemberPointer(void* _classInstancePointer) const;

		struct MetaData
		{
			MetaData(const char* _name, const char* _data);

			const char* getName() const;

			bool asBool() const;
			int asInt() const;
			float asFloat() const;
			const char* asString() const;

		private:
			std::string m_name;
			std::string m_data;
		};

		MetaData* getMetaData(const char* _key) const;

	private:
		std::string m_name;
		size_t m_offset;
		TypeDesc* m_type;
		std::unordered_map<uint32_t, MetaData> m_metaData;
	};


	class Class : public TypeDesc
	{
	public:
		Class(const char* _name, size_t _typeHash);

		const std::vector<ClassMember>& getMembers() const { return m_members; }
		const char* getName() const { return m_name.c_str(); }
		size_t getTypeHash() const { return m_typeHash; }

		void addMember(const ClassMember& _member);
		void addParent(Class* _parent);

	private:
		std::set<Class*> m_parents;
		std::set<Class*> m_children;
		std::vector<ClassMember> m_members;
		size_t m_typeHash;
		std::string m_name;
	};

	
	class ClassSet
	{
	public:
		~ClassSet();

		Class* findClassByName(const char* _className);
		Class* findClassByTypeHash(size_t _classTypeHash);

		void addClass(Class* _class);
		void removeClass(Class* _class);

		const std::set<Class*>& GetClasses() const;

	private:
		std::set<Class*> m_classes;
		std::unordered_map<uint32_t, Class*> m_classesByNameHash;
		std::unordered_map<size_t, Class*> m_classesByTypeHash;
	};

	template <typename T, typename IsPointer = void, typename IsEnum = void>
	struct TypeDescGetter
	{
		static TypeDesc* Get()
		{
			return T::GetClass();
		}
	};

	template <typename T>
	struct TypeDescGetter<T, std::enable_if_t<std::is_pointer<T>::value>>
	{
		static TypeDesc* Get()
		{
			static PointerTypeDesc s_pointerTypeDesc(TypeDescGetter<std::remove_pointer<T>::type>::Get()); return &s_pointerTypeDesc;
		}
	};

	template <typename T>
	struct TypeDescGetter<T, void, std::enable_if_t<std::is_enum<T>::value>>
	{
		static TypeDesc* Get()
		{
			switch (sizeof(T))
			{
			case 1: return TypeDescGetter<int8_t>::Get();
			case 2: return TypeDescGetter<int16_t>::Get();
			case 4: return TypeDescGetter<int32_t>::Get();
			case 8: return TypeDescGetter<int64_t>::Get();
			}
			return nullptr;
		}
	};

	template <> struct TypeDescGetter<void> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_void); return &s_typeDesc; } };
	template <> struct TypeDescGetter<bool> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_bool); return &s_typeDesc; } };
	template <> struct TypeDescGetter<char> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_char); return &s_typeDesc; } };
	template <> struct TypeDescGetter<int8_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_int8); return &s_typeDesc; } };
	template <> struct TypeDescGetter<int16_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_int16); return &s_typeDesc; } };
	template <> struct TypeDescGetter<int32_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_int32); return &s_typeDesc; } };
	template <> struct TypeDescGetter<int64_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_int64); return &s_typeDesc; } };
	template <> struct TypeDescGetter<uint8_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_uint8); return &s_typeDesc; } };
	template <> struct TypeDescGetter<uint16_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_uint16); return &s_typeDesc; } };
	template <> struct TypeDescGetter<uint32_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_uint32); return &s_typeDesc; } };
	template <> struct TypeDescGetter<uint64_t> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_uint64); return &s_typeDesc; } };
	template <> struct TypeDescGetter<float> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_float); return &s_typeDesc; } };
	template <> struct TypeDescGetter<double> { static TypeDesc* Get() { static TypeDesc s_typeDesc = TypeDesc(Type_double); return &s_typeDesc; } };

	template <typename T>
	TypeDesc* GetTypeDesc(T) { return TypeDescGetter<T>::Get(); }

	template <typename T>
	Class* GetClass()
	{ 
		TypeDesc* typeDesc = TypeDescGetter<T>::Get();
		if (typeDesc->getType() == Type_Class)
		{
			return static_cast<Class*>(typeDesc);
		}
		return nullptr;
	}

	uint32_t Hash32(const void* _data, size_t _size);
	uint32_t HashCString(const char* _str);

	extern ClassSet	g_classSet;
}
