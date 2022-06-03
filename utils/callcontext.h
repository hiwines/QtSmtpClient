#ifndef CALLCONTEXT_H
#define CALLCONTEXT_H

/// Utility container for Call-Context Data
/// \note Use the following `CALL_CONTEXT` macro to build
///     the context with the expected infos
struct CallContext
{
    const char *file = nullptr; ///< calling file
    int line = 0; ///< line within the calling file
    const char *fnc = nullptr; ///< calling function

    /// Utility Constructors
    inline CallContext(const char *fl, int ln, const char *fn)
        : file(fl), line(ln), fnc(fn) {}
    inline CallContext() = default;
    /// Utility Null-Check
    inline bool isNull() const { return (file == nullptr && fnc == nullptr); }
};

/// Utility Macro to build the Context as Expected
/// \note Use it as function arg: `function(CALL_CONTEXT)`
#ifdef _MSC_VER // MSVC version of the Macro
#define CALL_CONTEXT CallContext(__FILE__, __LINE__, __FUNCTION__)
#else // GCC version of the Macro
#define CALL_CONTEXT CallContext(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

#endif // CALLCONTEXT_H
