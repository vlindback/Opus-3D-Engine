#pragma once

#include <foundation/core/include/assert.hpp>
#include <foundation/core/include/result.hpp>
#include <foundation/memory/include/allocator.hpp>

#include <foundation/simd/include/simd_128.hpp>

#include "hash.hpp"

#include <bit>
#include <concepts>
#include <cstring>
#include <type_traits>

namespace opus3d::foundation
{
	namespace detail
	{
		namespace concepts
		{
			template <typename T>
			concept SwissProbeBase = requires(T v, size_t idx) {
				{ v.on_match(idx) } -> std::convertible_to<bool>;
				{ v.on_empty(idx) };
				{ v.result() };
			};

			template <typename T>
			concept SwissProbeDeleted = requires(T v, size_t idx) {
				{ v.on_deleted(idx) };
			};
		} // namespace concepts

		static constexpr int8_t SWISS_PROBE_CTRL_EMPTY	 = static_cast<int8_t>(0x80);
		static constexpr int8_t SWISS_PROBE_CTRL_DELETED = static_cast<int8_t>(0xFE);

		template <concepts::SwissProbeBase Visitor>
		inline auto swiss_probe(int8_t* ctrl, size_t capacity, size_t startGroup, int8_t h2, Visitor& visit) noexcept
		{
			using namespace simd;

			constexpr size_t GROUP_SIZE = simd128<int8_t>::width;

			simd128<int8_t> match(h2);
			simd128<int8_t> empty(SWISS_PROBE_CTRL_EMPTY);

#ifndef NDEBUG
			size_t probes = 0;
#endif

			size_t group = startGroup;

			for(;;)
			{
				simd128<int8_t> c = simd128<int8_t>::load(ctrl + group);

				// h2 matches, visitor decides
				uint32_t matchMask = simd128<int8_t>::cmpeq(c, match).movemask();
				while(matchMask)
				{
					uint32_t bit = std::countr_zero(matchMask);
					size_t	 idx = (group + bit) & (capacity - 1);

					if(visit.on_match(idx))
					{
						return visit.result();
					}

					matchMask &= matchMask - 1;
				}

				// deleted, visitor decides
				if constexpr(concepts::SwissProbeDeleted<Visitor>)
				{
					simd128<int8_t> deleted(SWISS_PROBE_CTRL_DELETED);
					uint32_t	delMask = simd128<int8_t>::cmpeq(c, deleted).movemask();
					while(delMask)
					{
						uint32_t bit = std::countr_zero(delMask);
						size_t	 idx = (group + bit) & (capacity - 1);

						visit.on_deleted(idx);

						delMask &= delMask - 1;
					}
				}

				// empty, visitor decides
				uint32_t emptyMask = simd128<int8_t>::cmpeq(c, empty).movemask();
				if(emptyMask)
				{
					uint32_t bit = std::countr_zero(emptyMask);
					size_t	 idx = (group + bit) & (capacity - 1);

					visit.on_empty(idx);
					return visit.result();
				}

				// advance probe
				group = (group + GROUP_SIZE) & (capacity - 1);

				DEBUG_ASSERT_MSG(probes++ < capacity, "SwissTable invariant violated: no EMPTY slot");
			}
		}
	} // namespace detail

	// Swiss Table Flat HashMap (SIMD enhanced)
	//
	// NOTE: this class has some hard requirements:
	//
	// Capacity must be clamped >= SIMD minimum size and must be power of 2.
	// EMPTY slots must always exist.

	template <typename Key, typename Value, typename Hash = std::hash<Key>>
		requires HashFor<Hash, Key>
	class FlatHashMap
	{
	public:

		FlatHashMap(memory::Allocator allocator, size_t entries = 16) noexcept;

		~FlatHashMap();

		void clear() noexcept;

		Result<void> insert(const Key& key, Value value) noexcept;

		Result<Value*> find(const Key& key) noexcept;

		bool erase(const Key& key) noexcept;

		void rehash(size_t newCapacity) noexcept;

	private:

		struct Entry
		{
			Key   key;
			Value value;
		};

		struct FindVisitor
		{
			Entry*	   entries;
			const Key& key;
			Value*	   found = nullptr;

			bool on_match(size_t idx) noexcept
			{
				if(entries[idx].key == key)
				{
					found = &entries[idx].value;
					return true; // stop probing
				}
				return false;
			}

			void on_deleted(size_t) noexcept
			{
				// nothing to do
			}

			void on_empty(size_t) noexcept
			{
				// nothing to do
			}

			Value* result() const noexcept { return found; }
		};

		struct EraseVisitor
		{
			Entry*	   entries;
			int8_t*	   ctrl;
			const Key& key;

			bool erased = false;

			bool on_match(size_t idx) noexcept
			{
				if(entries[idx].key == key)
				{
					// Destroy the entry
					std::destroy_at(entries + idx);

					// Mark as DELETED
					ctrl[idx] = CTRL_DELETED;

					erased = true;
					return true; // stop probing
				}
				return false;
			}

			void on_deleted(size_t) noexcept
			{
				// nothing to do
			}

			void on_empty(size_t) noexcept
			{
				// EMPTY means key does not exist, stop probing
				erased = false;
			}

			bool result() const noexcept { return erased; }
		};

		struct InsertVisitor
		{
			Entry*	   entries;
			int8_t*	   ctrl;
			const Key& key;
			Value&	   value;

			size_t firstDeleted = static_cast<size_t>(-1);
			size_t insertIndex  = static_cast<size_t>(-1);
			bool   found	    = false;

			bool on_match(size_t idx) noexcept
			{
				if(entries[idx].key == key)
				{
					entries[idx].value = std::move(value);
					found		   = true;
					return true;
				}
				return false;
			}

			void on_deleted(size_t idx) noexcept
			{
				if(firstDeleted == static_cast<size_t>(-1))
				{
					firstDeleted = idx;
				}
			}

			void on_empty(size_t idx) noexcept { insertIndex = (firstDeleted != static_cast<size_t>(-1)) ? firstDeleted : idx; }

			bool result() const noexcept { return found; }
		};

		struct ProbeSeed
		{
			int8_t h2;
			size_t groupStart;
		};

		static constexpr int8_t CTRL_EMPTY   = detail::SWISS_PROBE_CTRL_EMPTY;
		static constexpr int8_t CTRL_DELETED = detail::SWISS_PROBE_CTRL_DELETED;

		// Prevent future refactors from silently breaking SIMD logic.
		static_assert(CTRL_EMPTY < 0);
		static_assert(CTRL_DELETED < 0);

	private:

		void deallocate() noexcept;

		Result<void> allocate(size_t entries) noexcept;

		size_t storage_block_size(size_t entries) const noexcept;

		// Returns the start of the metadata array:
		int8_t* metadata() noexcept;

		// Returns the start of the Entry array.
		Entry* entries() noexcept;

		ProbeSeed make_probe_seed(const Key& key) const noexcept;

		static inline bool is_full(int8_t ctrl) noexcept { return ctrl >= 0; }

		static inline bool is_empty(int8_t ctrl) noexcept { return ctrl == CTRL_EMPTY; }

		static inline bool is_deleted(int8_t ctrl) noexcept { return ctrl == CTRL_DELETED; }

	private:

		// Must be 16 byte aligned for SIMD compatability.
		std::byte* m_data = nullptr;

		// number of live elements
		size_t m_size = 0;

		// number of total slots
		size_t m_capacity = 0;

		// number of deleted-but-not-empty slots
		size_t m_tombstones = 0;

		Hash	  m_hash = {};
		Allocator m_allocator;

		static constexpr size_t GROUP_SIZE   = simd::simd128<int8_t>::width;
		static constexpr size_t CTRL_ALIGN   = alignof(simd::simd128<int8_t>);
		static constexpr size_t MIN_CAPACITY = GROUP_SIZE;
	};

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	FlatHashMap<Key, Value, Hash>::FlatHashMap(Allocator allocator, size_t entries) noexcept : m_allocator(allocator), m_capacity(0)
	{
		// Capacity is set in allocate.
		// TODO: create a factory method that returns Result cleanly
		if(Result<void> alloc = allocate(entries); !alloc.has_value())
		{
			panic("FlatHashMap: oom", alloc.error());
		}
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	FlatHashMap<Key, Value, Hash>::~FlatHashMap()
	{
		clear();
		deallocate();
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	void FlatHashMap<Key, Value, Hash>::clear() noexcept
	{
		int8_t* ctrl = metadata();
		Entry*	ent  = entries();

		if constexpr(!std::is_trivially_destructible_v<Entry>)
		{
			for(size_t i = 0; i < m_capacity; ++i)
			{
				if(is_full(ctrl[i]))
				{
					ent[i].~Entry();
				}
			}
		}

		// Reset all control bytes to EMPTY (including the over-allocated tail)
		std::memset(ctrl, static_cast<int>(CTRL_EMPTY), m_capacity + GROUP_SIZE);

		m_size	     = 0;
		m_tombstones = 0;
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	Result<void> FlatHashMap<Key, Value, Hash>::insert(const Key& key, Value value) noexcept
	{
		// TODO: write clear message explaing the math here.
		if((m_size + m_tombstones + 1) * 4 >= m_capacity * 3)
		{
			rehash(m_capacity * 2);
		}

		Entry*	ent  = entries();
		int8_t* ctrl = metadata();

		auto [h2, groupStart] = make_probe_seed(key);

		InsertVisitor visitor{ent, ctrl, key, value};

		bool existed = detail::swiss_probe(ctrl, m_capacity, groupStart, h2, visitor);

		if(!existed)
		{
			size_t idx = visitor.insertIndex;

			std::construct_at(ent + idx, key, std::move(value));
			ctrl[idx] = h2;

			if(visitor.firstDeleted != static_cast<size_t>(-1))
			{
				--m_tombstones;
			}

			++m_size;
		}

		return {};
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	Result<Value*> FlatHashMap<Key, Value, Hash>::find(const Key& key) noexcept
	{
		if(m_capacity == 0 || m_size == 0)
		{
			return nullptr;
		}

		auto [h2, groupStart] = make_probe_seed(key);

		FindVisitor visitor{entries(), key};

		return detail::swiss_probe(metadata(), m_capacity, groupStart, h2, visitor);
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	bool FlatHashMap<Key, Value, Hash>::erase(const Key& key) noexcept
	{
		if(m_size == 0)
		{
			return false;
		}

		Entry*	ent  = entries();
		int8_t* ctrl = metadata();

		auto [h2, groupStart] = make_probe_seed(key);

		EraseVisitor visitor{ent, ctrl, key};

		bool erased = detail::swiss_probe(ctrl, m_capacity, groupStart, h2, visitor);

		if(erased)
		{
			--m_size;
			++m_tombstones;
		}

		return erased;
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	void FlatHashMap<Key, Value, Hash>::rehash(size_t newCapacity) noexcept
	{
		DEBUG_ASSERT_MSG(newCapacity >= m_size * 2, "Rehash capacity too small");

		// Allocate new table
		FlatHashMap tmp(m_allocator, newCapacity);

		Entry*	oldEntries = entries();
		int8_t* oldCtrl	   = metadata();

		for(size_t i = 0; i < m_capacity; ++i)
		{
			if(is_full(oldCtrl[i]))
			{
				tmp.insert(std::move(oldEntries[i].key), std::move(oldEntries[i].value));
			}
		}

		// Destroy old entries
		if constexpr(!std::is_trivially_destructible_v<Entry>)
		{
			for(size_t i = 0; i < m_capacity; ++i)
			{
				if(is_full(oldCtrl[i]))
				{
					std::destroy_at(oldEntries + i);
				}
			}
		}

		// Free old memory
		deallocate();

		// Steal tmp's storage
		m_data	     = tmp.m_data;
		m_capacity   = tmp.m_capacity;
		m_size	     = tmp.m_size;
		m_tombstones = 0;

		tmp.m_data = nullptr; // prevent double free

		// Sanity checks for debug.
		DEBUG_ASSERT(m_size <= m_capacity);
		DEBUG_ASSERT(m_tombstones == 0);
		DEBUG_ASSERT(m_capacity >= GROUP_SIZE);
		DEBUG_ASSERT(std::has_single_bit(m_capacity));
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	void FlatHashMap<Key, Value, Hash>::deallocate() noexcept
	{
		m_allocator.deallocate(m_data, storage_block_size(m_capacity), CTRL_ALIGN);
		m_data	   = nullptr;
		m_capacity = 0;
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	Result<void> FlatHashMap<Key, Value, Hash>::allocate(size_t entries) noexcept
	{
		// Ensure we have enough room for the load factor (e.g., max 75% full)
		// load factor of 0.75 (4/3 ratio)
		const size_t required = (entries * 4 + 2) / 3;

		// Entries *MUST* be a power of 2 for the bitwise logic to work.
		// And it must be at least minimum GROUP_SIZE for SIMD.
		size_t pow2Entries = std::bit_ceil(std::max<size_t>(GROUP_SIZE, required));

		const size_t size = storage_block_size(pow2Entries);
		if(Result<void*> alloc = m_allocator.try_allocate(size, CTRL_ALIGN); alloc.has_value())
		{
			m_capacity = pow2Entries;
			m_data	   = std::assume_aligned<CTRL_ALIGN>(static_cast<std::byte*>(alloc.value()));

			// SwissTable requires control bytes initialized to Empty (0x80).
			std::memset(metadata(), static_cast<int>(CTRL_EMPTY), m_capacity + GROUP_SIZE);

			return {};
		}
		else
		{
			return Unexpected(alloc.error());
		}
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	size_t FlatHashMap<Key, Value, Hash>::storage_block_size(size_t entries) const noexcept
	{
		// Calculate the actual storage size required
		// if called with m_capacity returns the actual storage size.

		// Metadata: 1 byte per entry (e.g., Abseil-style control bytes)
		// Plus over-allocation for SIMD instructions.
		const size_t metadataSize = entries + GROUP_SIZE;

		// Entry might require 4, 8, or even 16-byte alignment
		const size_t entryAlign = alignof(Entry);

		// Padding: Distance from end of metadata to the start of the first Entry.
		// (A % B) handles the offset; the outer modulo handles the case where offset is 0.
		const size_t padding = (entryAlign - (metadataSize % entryAlign)) % entryAlign;

		return metadataSize + padding + (entries * sizeof(Entry));
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	int8_t* FlatHashMap<Key, Value, Hash>::metadata() noexcept
	{
		return std::assume_aligned<CTRL_ALIGN>(reinterpret_cast<int8_t*>(m_data));
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	typename FlatHashMap<Key, Value, Hash>::Entry* FlatHashMap<Key, Value, Hash>::entries() noexcept
	{
		const size_t metadataSize = m_capacity + GROUP_SIZE;
		const size_t entryAlign	  = alignof(Entry);
		const size_t padding	  = (entryAlign - (metadataSize % entryAlign)) % entryAlign;

		void* ptr = m_data + metadataSize + padding;

		return std::assume_aligned<alignof(Entry)>(std::launder(reinterpret_cast<Entry*>(ptr)));
	}

	template <typename Key, typename Value, typename Hash>
		requires HashFor<Hash, Key>
	FlatHashMap<Key, Value, Hash>::ProbeSeed FlatHashMap<Key, Value, Hash>::make_probe_seed(const Key& key) const noexcept
	{
		const size_t hash = m_hash(key);
		const int8_t h2	  = static_cast<int8_t>(hash & 0x7F);
		const size_t h1	  = hash >> 7;

		size_t index	  = h1 & (m_capacity - 1);
		size_t groupStart = index & ~(GROUP_SIZE - 1);

		return ProbeSeed{.h2 = h2, .groupStart = groupStart};
	}

} // namespace opus3d::foundation