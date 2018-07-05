#pragma once

#include <stdint.h>

template <class T>
struct ArrayView {

	ArrayView(char * begin, char * end)
		: m_begin(reinterpret_cast<T *>(begin))
		, m_end(reinterpret_cast<T *>(end))
	{ }

	T & operator[](int64_t index) {
#ifdef DEBUG
		assert(m_begin != nullptr && index >= 0 &&
			index < (m_end - m_begin));
#endif
		return m_begin[index];
	}

	int64_t size() const { return m_end - m_begin; }
	int64_t memory_size() const {
		return sizeof(T) * size();
	}
	// ... More methods, const operator[], begin, end, etc
private:
	T * m_begin = nullptr;
	T * m_end = nullptr;
};


template <class ... Args>
struct MultiArray 
{

private:
	static constexpr int s_num_arrays = sizeof...(Args);
	std::array<uint32_t, s_num_arrays> m_offsets;
	std::unique_ptr<char[]> m_memory;
public: 
	MultiArray(const std::array<uint32_t, s_num_arrays> & sizes)
	{
		constexpr std::array<uint32_t, s_num_arrays> type_sizes =
		{ sizeof(Args)... };

		uint32_t partial_sum = 0;
		for (std::size_t i = 0; i != s_num_arrays; ++i) {
			partial_sum += sizes[i] * type_sizes[i];
			m_offsets[i] = partial_sum;
		}

		m_memory.reset(new char[totalMemory()]);
	}
	MultiArray(const MultiArray & other)
		: m_offsets(other.m_offsets)
		, m_memory(new char[other.totalMemory()])
	{
		std::memcpy(m_memory.get(), other.m_memory.get(), totalMemory());
	}
	MultiArray& operator=(const MultiArray & other) {
		if (this == &other) {
			return *this;
		}
		if (totalMemory() != other.totalMemory()) {
			m_memory.reset(new char[other.totalMemory()]);
		}
		m_offsets = other.m_offsets;
		std::memcpy(m_memory.get(), other.m_memory.get(), totalMemory());
		return *this;
	}


	int32_t totalMemory() const {
		return m_offsets.back();
	}

	template <uint32_t ItemIndex, typename ItemType>
	ArrayView<ItemType> getView()
	{
		return ArrayView<ItemType>(m_memory.get() + (ItemIndex == 0 ? 0 : m_offsets[ItemIndex - 1]),
			m_memory.get() + m_offsets[ItemIndex]);
	}
};

