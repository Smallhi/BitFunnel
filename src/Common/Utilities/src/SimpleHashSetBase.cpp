#include <cstring>               // For memset().

#include "BitFunnel/Allocators/IAllocator.h"
#include "BitFunnel/Utilities/StreamUtilities.h"
#include "LoggerInterfaces/Logging.h"
#include "SimpleHashSetBase.h"

namespace BitFunnel
{
    SimpleHashSetBase::SimpleHashSetBase(unsigned capacity, unsigned maxProbes)
        : m_allocator(nullptr),
          m_keys(nullptr),
          m_maxProbes(maxProbes)
    {
        ResizeKeyBuffer(capacity);
    }


    SimpleHashSetBase::SimpleHashSetBase(unsigned capacity,
                                         Allocators::IAllocator& allocator,
                                         unsigned maxProbes)
        : m_allocator(&allocator),
          m_keys(nullptr),
          m_maxProbes(maxProbes)
    {
        ResizeKeyBuffer(capacity);
    }


    SimpleHashSetBase::~SimpleHashSetBase()
    {
        if (m_allocator != nullptr)
        {
            // Destruction is not thread safe.
            // cast is to remove volatile.
            m_allocator->Deallocate(const_cast<uint64_t*>(m_keys));
        }
        else
        {
            delete [] m_keys;
        }

        // DESIGN NOTE: Do not delete m_allocator. Is is not owned by this class.
    }


    SimpleHashSetBase::SimpleHashSetBase(std::istream& input)
        : m_allocator(nullptr),
          m_keys(nullptr)
    {
        m_capacity = StreamUtilities::ReadField<uint32_t>(input);
        m_maxProbes = m_capacity;

        ResizeKeyBuffer(m_capacity);

        // Construction is not thread safe.
        // const_cast is to get rid of volatile attribute.
        StreamUtilities::ReadBytes(input, const_cast<uint64_t*>(m_keys),
                                   m_slotCount * sizeof(uint64_t));
    }


    void SimpleHashSetBase::Write(std::ostream& output) const
    {
        StreamUtilities::WriteField<uint32_t>(output, m_capacity);

        // File writing is not thread safe.
        // const_cast is to get rid of volatile attribute.
        StreamUtilities::WriteBytes(output,
            reinterpret_cast<const char*>(const_cast<uint64_t*>(m_keys)),
                                          m_slotCount * sizeof(uint64_t));
    }


    unsigned SimpleHashSetBase::GetNextSlot(unsigned slot) const
    {
        for (++slot; slot < m_slotCount; ++slot)
        {
            if (IsFilledSlot(slot))
            {
                break;
            }
        }

        return slot;
    }


    bool SimpleHashSetBase::IsValidSlot(unsigned slot) const
    {
        return (slot < m_slotCount);
    }


    bool SimpleHashSetBase::IsFilledSlot(unsigned slot) const
    {
        return IsFilledSlot(slot, m_capacity, m_keys);
    }


    bool SimpleHashSetBase::IsFilledSlot(unsigned slot, unsigned capacity,
                                         volatile uint64_t* keys)
    {
        return (slot < capacity) ^ (keys[slot] == 0);
    }


    uint64_t SimpleHashSetBase::GetKey(unsigned slot) const
    {
        LogAssertB(IsValidSlot(slot), "GetKey on invalid slot.\n");
        LogAssertB(IsFilledSlot(slot), "GetKey on empty slot.\n");

        return m_keys[slot];
    }


    bool SimpleHashSetBase::TryFindSlot(uint64_t key, unsigned& slotOut,
                                        bool& foundKey) const
    {
        if (key == 0)
        {
            // Special case.
            // key == 0 is used to encode empty slots. The value for key == 0
            // is stored at slot == m_capacity.
            slotOut = m_capacity;
            foundKey = HasKeyZero();
            return true;
        }
        else
        {
            // Warning: Edits to the hash to slot mapping will necessitate
            // regeneration of the TermTables.
            unsigned slot = key % m_capacity;
            for (unsigned i = 0; i < m_maxProbes; ++i)
            {
                if (m_keys[slot] == key)
                {
                    // We found the key. Return the corresponding value.
                    slotOut = slot;
                    foundKey = true;
                    return true;
                }
                else if (m_keys[slot] == 0)
                {
                    // We encountered an empty slot. This means the key is
                    // not in the table. Return this slot.
                    slotOut = slot;
                    foundKey = false;
                    return true;
                }

                // The original operation slot = (slot + 1) % m_capacity was
                // determined to be less performant since it does a 32 bit
                // divide.
                slot++;
                if (slot >= m_capacity)
                {
                    slot = 0;
                }
            }
        }

        foundKey = false;
        return false;
    }


    uint64_t* SimpleHashSetBase::ResizeKeyBuffer(unsigned capacity)
    {
        // Ensure that requested capacity is greater than zero. The current
        // implementation relies on a non-zero capacity for the modulus
        // operation in SimpleHashSetBase::TryFindSlot(). Also
        // SimpleHashTable::Rehash() doubles the capacity, and therefore
        // requires a non-zero capacity in order to actually grow the table.
        LogAssertB(capacity > 0, "Negative capacity.\n");
        m_capacity = capacity;

        // The table has m_capacity slots, plus one extra to handle the special
        // case where key == 0.
        m_slotCount = m_capacity + 1;

        // Resizing is not thread safe.
        // const_cast is to remove volatile.
        uint64_t* oldBuffer = const_cast<uint64_t*>(m_keys);
        if (m_allocator != nullptr)
        {
            m_keys =
                reinterpret_cast<uint64_t*>(
                    m_allocator->Allocate(sizeof(uint64_t) * m_slotCount));
        }
        else
        {
            m_keys = new uint64_t[m_slotCount];
        }

        memset(const_cast<uint64_t*>(m_keys),
               0, sizeof(uint64_t) * m_slotCount);

        // Initialize this zero key slot to a non-zero value to indicate it is empty.
        m_keys[m_capacity] = c_invalidKeyZero;
        return oldBuffer;
    }


    bool SimpleHashSetBase::HasKeyZero() const
    {
        return m_keys[m_capacity] == 0;
    }


    uint64_t SimpleHashSetBase::GetKeyForEmptySlot(uint64_t key) const
    {
        return key == 0 ? c_invalidKeyZero : 0;
    }


    //*************************************************************************
    //
    // SimpleHashBase::EnumeratorObjectBase
    //
    //*************************************************************************
    SimpleHashSetBase::EnumeratorObjectBase::EnumeratorObjectBase(
        const SimpleHashSetBase& hashSet)
        : m_current(c_currentBeforeStart),
          m_set(hashSet)
    {
    }

    bool SimpleHashSetBase::EnumeratorObjectBase::MoveNext()
    {
        LogAssertB(m_set.IsValidSlot(m_current) ||
                   m_current == c_currentBeforeStart,
                   "");

        m_current = m_set.GetNextSlot(m_current);

        return m_set.IsValidSlot(m_current);
    }

    void SimpleHashSetBase::EnumeratorObjectBase::Reset()
    {
        m_current = c_currentBeforeStart;
    }
}
