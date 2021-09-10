#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/Utils/BoolPointer.h>

namespace FE
{
    class Job;

    class JobTree
    {
        JobTree* m_Sibling;
        // BoolPointer is used here to make sizeof(JobTree) = 16 bytes, not 17 for better alignment
        // in contiguous arrays or pools where these trees will be stored.
        BoolPointer<JobTree> m_ChildCancelledPair;

    public:
        FE_STRUCT_RTTI(JobTree, "0E61A069-4E63-44D4-9084-DE314AFE8DA0");

        inline JobTree()
            : m_Sibling(nullptr)
            , m_ChildCancelledPair(nullptr, false)
        {
        }

        [[nodiscard]] inline bool IsCancelled() const
        {
            return m_ChildCancelledPair.GetBool();
        }

        inline void Cancel()
        {
            m_ChildCancelledPair.SetBool(true);
        }

        inline void AddChild(JobTree* child)
        {
            if (m_ChildCancelledPair.GetPointer())
            {
                child->m_Sibling = m_ChildCancelledPair.GetPointer();
            }
            m_ChildCancelledPair.SetPointer(child);
        }

        [[nodiscard]] inline JobTree* GetSibling() const
        {
            return m_Sibling;
        }

        [[nodiscard]] inline JobTree* GetFirstChild() const
        {
            return m_ChildCancelledPair.GetPointer();
        }
    };
} // namespace FE
