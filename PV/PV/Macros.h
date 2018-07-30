#pragma once



#pragma region Macros

#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return { f }; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

#define allocnullptr nullptr
#define PV_VK_RUN(VK_OPERATION)																													\
do { VkResult res = VK_OPERATION;																												\
	if (res != VK_SUCCESS)																														\
	{																																			\
		std::string msg;																														\
		msg.append("ERROR, VULKAN Operation: " #VK_OPERATION).append(" in file ").append(__FILE__).append(" at line ").append(std::to_string(__LINE__)).append(" failed with code: ").append(std::to_string(res));	\
		throw std::runtime_error(msg);																											\
	}																																			\
} while (0);

#define PV_ASSERT(expression, message)																											\
do {																																			\
	if (!(expression))																															\
	{																																			\
		std::string assertMessage = "PV_ASSERT: ";																			\
		assertMessage.append((message)).append("\nIn File ").append(__FILE__).append(" at line ").append(std::to_string(__LINE__));									\
		throw std::runtime_error(assertMessage);																								\
	}																																			\
} while (0)

#pragma endregion


