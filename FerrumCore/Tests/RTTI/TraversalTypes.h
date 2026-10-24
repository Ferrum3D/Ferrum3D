#pragma once
#include <Core/IO/Assets.h>
#include <Core/Math/Vector3.h>
#include <Core/RTTI/Any.h>
#include <Core/RTTI/RTTI.h>
#include <festd/string.h>
#include <festd/variant.h>
#include <festd/vector.h>

namespace FE::Rtti::Tests
{
    struct TraversalLeaf final
    {
        int32_t m_value = 0;
        IO::Link<TraversalLeaf> m_link;

        FE_RTTI_Reflect("34C5E845-2A05-4CDD-A1BD-0807B53A6002");
    };


    struct TraversalObject : Memory::RefCountedObjectBase
    {
        FE_RTTI("84A0D693-C79F-4D75-AC93-A7F11D9275CC");
        FE_RTTI_Reflect();

        int32_t m_baseValue = 0;

    protected:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };


    struct TraversalDerivedObject final : TraversalObject
    {
        FE_RTTI("D87017AF-63B4-40CD-9A53-769E4362BA74");
        FE_RTTI_Reflect();

        festd::vector<TraversalLeaf> m_leaves;
    };


    struct TraversalRoot final
    {
        festd::vector<TraversalLeaf> m_leaves;
        TraversalLeaf m_fixed[2];
        festd::variant<int32_t, TraversalLeaf> m_choice;
        Rc<TraversalObject> m_object;
        Any m_any;
        festd::inline_vector<uint32_t, 4> m_numbers;
        festd::string m_name;
        Vector3 m_offset;
        FE_SKIP_SERIALIZING IO::Link<TraversalLeaf> m_runtimeLink;
        FE_SKIP_SERIALIZING int32_t m_runtimeValue = 0;

        FE_RTTI_Reflect("E8C5C67E-5BA4-421E-999E-CB7619C8AC77");
    };
} // namespace FE::Rtti::Tests
