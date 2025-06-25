#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Threading/SharedSpinLock.h>

namespace FE::DI
{
    struct ServiceRegistry;
    struct ServiceRegistryBuilder;


    //! @brief Represents a dependency injection result code.
    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kAbort = -1,                                   //!< Operation aborted.
        kInvalidOperation = -2,                        //!< Operation was invalid.
        kNotFound = -3,                                //!< Some of the required services were not found.
        kCircularDependency = -4,                      //!< Service activation failed because of circular dependencies.
        kUnknownError = kDefaultErrorCode<ResultCode>, //!< Unknown error.
    };

    const char* GetResultDesc(ResultCode code);


    //! @brief Specifies the lifetime of an injectable service.
    enum class Lifetime : uint32_t
    {
        kSingleton, //!< Specifies that only a single instance of the service will be created.
        kThread,    //!< Specifies that an instance of the service will be created for each thread.
        kTransient, //!< Specifies that an instance of the service will be created for each call to IServiceProvider::Resolve.

        kCount,
    };


    //! @brief Base interface for dependency injection containers.
    struct IServiceProvider
    {
        FE_RTTI_Class(IServiceProvider, "89A29040-31BC-411D-8522-92D7D2696C16");

        virtual ~IServiceProvider() = default;

        virtual ResultCode Resolve(UUID registrationID, Memory::RefCountedObjectBase** ppResult) = 0;

        template<class T>
        festd::expected<T*, ResultCode> Resolve()
        {
            if constexpr (std::is_same_v<T, IServiceProvider>)
            {
                return this;
            }
            else
            {
                Memory::RefCountedObjectBase* pResult = nullptr;
                const ResultCode code = Resolve(fe_typeid<T>(), &pResult);
                if (code == ResultCode::kSuccess)
                    return static_cast<T*>(pResult);

                return festd::unexpected(code);
            }
        }

        template<class T>
        T* ResolveRequired()
        {
            if constexpr (std::is_same_v<T, IServiceProvider>)
                return this;
            else
                return Resolve<T>().value();
        }
    };
} // namespace FE::DI
