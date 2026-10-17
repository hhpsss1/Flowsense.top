#pragma once

#include "../datatypes/utl_hash.h"
#include "utils/memory.h"
#include <core/hooks/modules.h>

struct schema_class_field_t
{
    const char* m_name;
    char pad0[8];
    int m_offset;
    int m_unk;
    void* pad1;
};

class c_schema_class_binding_base
{
public:
    short get_fields_size( )
    {
        return *address_t{ this }.add( 0x24 ).as<short*>( );
    }

    const char* get_name( )
    {
        return *address_t{ this }.add( 0x8 ).as<const char**>( );
    }

    schema_class_field_t* get_fields( )
    {
        return *address_t{ this }.add( 0x30 ).as<schema_class_field_t**>( );
    }
};
class c_schema_type_scope;

class CSchemaType
{
public:
    bool GetSizes(int* pOutSize, uint8_t* unkPtr)
    {
        return g_memory->call_virtual<bool>(this, 3U, pOutSize, unkPtr);
    }

public:
    bool GetSize(int* out_size)
    {
        uint8_t smh = 0;
        return GetSizes(out_size, &smh);
    }

public:
    void* pVtable; // 0x0000
    const char* szName; // 0x0008

    c_schema_type_scope* pSystemTypeScope; // 0x0010
    uint8_t nTypeCategory; // ETypeCategory 0x0018
    uint8_t nAatomicCategory; // EAtomicCategory 0x0019
};
struct SchemaMetadataEntryData_t;

struct SchemaClassFieldData_t
{
    const char* szName; // 0x0000
    CSchemaType* pSchemaType; // 0x0008
    std::uint32_t nSingleInheritanceOffset; // 0x0010
    std::int32_t nMetadataSize; // 0x0014
    SchemaMetadataEntryData_t* pMetaData; // 0x0018
};


enum schema_class_flags_t
{
    SCHEMA_CLASS_HAS_VIRTUAL_MEMBERS = 1,
    SCHEMA_CLASS_IS_ABSTRACT = 2,
    SCHEMA_CLASS_HAS_TRIVIAL_CONSTRUCTOR = 4,
    SCHEMA_CLASS_HAS_TRIVIAL_DESTRUCTOR = 8,
    SCHEMA_CLASS_TEMP_HACK_HAS_NOSCHEMA_MEMBERS = 16,
    SCHEMA_CLASS_TEMP_HACK_HAS_CONSTRUCTOR_LIKE_METHODS = 32,
    SCHEMA_CLASS_TEMP_HACK_HAS_DESTRUCTOR_LIKE_METHODS = 64,
    SCHEMA_CLASS_IS_NOSCHEMA_CLASS = 128,
};

enum EAtomicCategory
{
    Atomic_Basic,
    Atomic_T,
    Atomic_CollectionOfT,
    Atomic_TT,
    Atomic_I,
    Atomic_None
};

class c_schema_class_info_data;
class c_schema_system_type_scope;

class c_schema_enumerator_info_data
{
public:
    const char* m_name;

    union
    {
        unsigned char m_value_char;
        unsigned short m_value_short;
        unsigned int m_value_int;
        unsigned long long m_value;
    };

    char pad_001[0x10];
};

class c_schema_enum_binding
{
public:
    virtual const char* GetBindingName() = 0;
    virtual c_schema_class_info_data* AsClassBinding() = 0;
    virtual c_schema_enum_binding* AsEnumBinding() = 0;
    virtual const char* GetBinaryName() = 0;
    virtual const char* GetProjectName() = 0;
public:
    char* binding_name_; // 0x0008
    char* dll_name_; // 0x0010
    std::int8_t align_; // 0x0018
    char pad_001[0x3];
    std::int16_t size_; // 0x001C
    std::int16_t flags_; // 0x001E
    c_schema_enumerator_info_data* enum_info_;
    char pad_002[0x8];
    c_schema_system_type_scope* type_scope_; // 0x0030
    char pad_003[0x8];
    std::int32_t i_unk1_; // 0x0040
};

class c_schema_type
{
public:
    c_schema_type* get_ref_class() const
    {
        if (type_category != 1)
            return nullptr;

        auto ptr = schema_type_;
        while (ptr && ptr->type_category == 1)
            ptr = ptr->schema_type_;

        return ptr;
    }

    uintptr_t* _vtable; // 0x0000
    const char* name_; // 0x0008

    c_schema_system_type_scope* type_scope_; // 0x0010
    uint8_t type_category; // ETypeCategory 0x0018
    uint8_t atomic_category; // EAtomicCategory 0x0019
    struct array_t
    {
        uint32_t array_size;
        uint32_t unknown;
        c_schema_type* element_type_;
    };

    struct atomic_t
    { // same goes for CollectionOfT
        uint64_t gap[2];
        c_schema_type* template_typename;
    };

    struct atomic_tt
    {
        uint64_t gap[2];
        c_schema_type* templates[2];
    };

    struct atomic_i
    {
        uint64_t gap[2];
        uint64_t integer;
    };

    // this union depends on CSchema implementation, all members above
    // is from base class ( c_schema_type )
    union // 0x020
    {
        c_schema_type* schema_type_;
        c_schema_class_info_data* class_info;
        c_schema_enum_binding* enum_binding_;
        array_t array_;
        atomic_t atomic_t_;
        atomic_tt atomic_tt_;
        atomic_i atomic_i_;
    };
};

class c_schema_base_class_field
{
public:
    c_schema_base_class_field* self;
    const char* name;
    c_schema_type* type;
    uint32_t single_inheritance_offset;
};

class c_schema_field
{
public:
    const char* name; // 0x0000
    c_schema_type* type; // 0x0008
    uint32_t single_inheritance_offset; // 0x0010
    uint32_t metadata_size; // 0x0014
    void* metadata; // 0x0018 SchemaMetadataEntryData_t
};

class c_schema_base_class_field_data
{
public:
    int64_t offset;
    c_schema_base_class_field* field;
};
class SchemaClassFieldData_t;
class c_schema_class_info_data
{
public:
    c_schema_class_info_data* self; // 0x0000
    const char* name; // 0x0008
    const char* module_name; // 0x0010
    int size; // 0x0018
    std::int16_t fields_size; // 0x001C
    std::int16_t static_fields_size; // 0x001E
    std::int16_t static_metadata_size; // 0x0020
    std::uint8_t align_of; // 0x0022
    std::uint8_t has_base_class; // 0x0023
    std::int16_t total_class_size; // 0x0024 // @note: @og: if there no derived or base class then it will be 1 otherwise derived class size + 1.
    std::int16_t derived_class_size; // 0x0026
    SchemaClassFieldData_t* fields; // 0x0028
    void* static_fields; // 0x0030 / SchemaStaticFieldData_t
    c_schema_base_class_field_data* base_class; // 0x0038
    void* field_metadata_overrides; // 0x0040 SchemaFieldMetadataOverrideSetData_t
    void* static_metadata; // 0x0048 SchemaMetadataEntryData_t
    c_schema_system_type_scope* type_scope; // 0x0050 
    c_schema_type* shema_type; // 0x0058
    schema_class_flags_t class_flags : 8; // 0x0060
    std::uint32_t sequence; // 0x0064 // @note: @og: idk
    void* fn; // 0x0068

};
class c_schema_declared_class
{
private:
    void* pVtable; // 0x0000
public:
    const char* szName; // 0x0008
    char* szDescription; // 0x0010

    int m_nSize; // 0x0018
    std::int16_t nFieldSize; // 0x001C
    std::int16_t nStaticSize; // 0x001E
    std::int16_t nMetadataSize; // 0x0020

    std::uint8_t nAlignOf; // 0x0022
    std::uint8_t nBaseClassesCount; // 0x0023
    char pad2[0x4]; // 0x0024
    SchemaClassFieldData_t* pFields; // 0x0028
    char pad3[0x8]; // 0x0030
    c_schema_class_info_data* pBaseClasses; // 0x0038
    char pad4[0x28]; // 0x0040
};


class c_schema_declared_class_entry
{
public:
    char pad_001[0x10];
    c_schema_declared_class* declarated_class;
};

class c_schema_type_scope
{
public:
    void* vfptr;
    char m_name[256];
    char pad_002[0x470];
    uint16_t declared_classes_count;
    char pad_003[0x6];
    c_schema_declared_class_entry* declared_classes;

    c_schema_class_binding_base* find_declared_class( const char* name )
    {
        c_schema_class_binding_base* declared_class{ nullptr };
        g_memory->call_virtual<void>( this, 2, &declared_class, name );
        return declared_class;
    }


    void get_declarated_classes(std::vector<c_schema_declared_class*>& declarated_classes_)
    {
        if (!declared_classes)
            return;

        for (int i = 0; i < declared_classes_count; ++i)
        {
            declarated_classes_.push_back(declared_classes[i].declarated_class);
        }
    }

    c_utl_ts_hash<c_schema_class_binding_base*, 256, unsigned int> get_class_bind_table( )
    {
        return *address_t{ this }.add( 0x0570 ).as<c_utl_ts_hash<c_schema_class_binding_base*, 256, unsigned int>*>( );
    }
};

class c_schema_system
{
public:
    c_schema_type_scope* find_type_scope_for_module( const char* name )
    {
        return g_memory->call_virtual<c_schema_type_scope*>( this, 13, name, nullptr );
    }
};
