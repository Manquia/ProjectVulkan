#pragma once


#include <array>
#include <numeric>


template <size_t arg1, size_t ... others>
struct static_max;

template <size_t arg>
struct static_max<arg>
{
	static const size_t value = arg;
};

template <size_t arg1, size_t arg2, size_t ... others>
struct static_max<arg1, arg2, others...>
{
	static const size_t value = arg1 >= arg2 ? static_max<arg1, others...>::value :
		static_max<arg2, others...>::value;
};


template <size_t ... others>
struct static_sum;

template <size_t arg>
struct static_sum<arg>
{
	static const size_t value = arg;
};

template <size_t arg1, size_t ... others>
struct static_sum<arg1, others...>
{
	static const size_t value = arg1 + static_sum<others...>::value;
};



template <size_t ... others>
struct static_offsets;

template <size_t arg>
struct static_offsets<arg>
{
	static const size_t index = std::numeric_limits<size_t>::max();
	static const size_t value = arg;
};

template <size_t arg1, size_t ... others>
struct static_offsets<arg1, others...>
{
	static const size_t index = static_offsets<others...>::index - 1;
	static const size_t value = arg1 + static_offsets<others...>::value;
	//static const std::array<size_t, sizeof(> = {};
};