#ifndef MACROS_H
#define MACROS_H

/// QString => C-String Conversion
#define cstr(string) string.toUtf8().constData()



/// Utility Upcasting Interface
/// \warning OBSOLETE
#define UPCAST_UTILITY_INTERFACE \
    template <typename UpcastedType> const UpcastedType* upcast() const \
        { return dynamic_cast<const UpcastedType*>(this); } \
    template <typename UpcastedType> UpcastedType* upcast() \
        { return dynamic_cast<UpcastedType*>(this); }

/// Templated Static-Cast Interface
#define STATIC_CAST_UTILITY_INTERFACE \
    template <typename CastedType> const CastedType* staticCast() const \
        { return static_cast<const CastedType*>(this); } \
    template <typename CastedType> CastedType* staticCast() \
        { return static_cast<CastedType*>(this); }

/// Templated Dynamic-Cast Interface
#define DYNAMIC_CAST_UTILITY_INTERFACE \
    template <typename CastedType> const CastedType* dynamicCast() const \
        { return dynamic_cast<const CastedType*>(this); } \
    template <typename CastedType> CastedType* dynamicCast() \
        { return dynamic_cast<CastedType*>(this); }



/// Utility Macro to create a "PrivateData *d" pointer
/// \example Use it like: ```
///     class MyClass {
///         PRIVATE_DATA_PTR(d);
///         // now you have a MyClass::PrivateData struct type to define
///         // and a MyClass::d pointer to use in your instance
///     };
///
/// ```
#define PRIVATE_DATA_PTR(__ptrName) \
public: \
    struct PrivateData; \
private: \
    PrivateData *__ptrName = nullptr;



/// Utility QDataStream operators for custom types
/// \note Encoding/Decoding of the custom __ValType is performed casting
///     it to and from the given __RawType
#define UTILS_QDATASTREAM_OPERATORS(__ValType, __RawType) \
    inline QDataStream& operator<< (QDataStream &s, const __ValType &v) \
        { s << static_cast<__RawType>(v); return s; } \
    inline QDataStream& operator>> (QDataStream &s, __ValType &v) \
        { __RawType d; s >> d; v = static_cast<__ValType>(d); return s; }

#endif // MACROS_H
