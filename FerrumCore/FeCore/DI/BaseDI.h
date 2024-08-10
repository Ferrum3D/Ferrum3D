#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE::DI
{
    //! \brief Represents a dependency injection result code.
    enum class ResultCode : int32_t
    {
        Success = 0,
        Abort = -1,                                  //!< Operation aborted.
        InvalidOperation = -2,                       //!< Operation was invalid.
        NotFound = -3,                               //!< Some of the required services were not found.
        CircularDependency = -4,                     //!< Service activation failed because of circular dependencies.
        UnknownError = DefaultErrorCode<ResultCode>, //!< Unknown error.
    };

    StringSlice GetResultDesc(ResultCode code);


    //! \brief Specifies the lifetime of an injectable service.
    enum class Lifetime : uint32_t
    {
        Singleton, //!< Specifies that only a single instance of the service will be created.
        Thread,    //!< Specifies that an instance of the service will be created for each thread.
        Transient, //!< Specifies that an instance of the service will be created for each call to IServiceProvider::Resolve.

        Count,
    };


    //! \brief Base interface for dependency injection containers.
    class IServiceProvider
    {
    public:
        FE_RTTI_Class(IServiceProvider, "89A29040-31BC-411D-8522-92D7D2696C16");

        virtual ~IServiceProvider() = default;

        virtual ResultCode Resolve(UUID registrationID, Memory::RefCountedObjectBase** ppResult) = 0;

        template<class T>
        inline Result<T*, ResultCode> Resolve()
        {
            Memory::RefCountedObjectBase* pResult = nullptr;
            const ResultCode code = Resolve(fe_typeid<T>(), &pResult);
            if (code == ResultCode::Success)
                return static_cast<T*>(pResult);

            return Err(code);
        }
    };
} // namespace FE::DI
