#include <iostream>
#include <utility>
#include <typeinfo>
#include <type_traits>
#include <string>
#include "static_util.h"

template<typename... Ts>
struct variant_helper;

template<typename F, typename... Ts>
struct variant_helper<F, Ts...> {
	inline static void destroy(size_t id, void * data) 
	{
		if (id == typeid(F).hash_code())
			reinterpret_cast<F*>(data)->~F();
		else
			variant_helper<Ts...>::destroy(id, data);
	}

	inline static void move(size_t old_t, void * old_v, void * new_v)
	{
		if (old_t == typeid(F).hash_code())
			new (new_v) F(std::move(*reinterpret_cast<F*>(old_v)));
		else
			variant_helper<Ts...>::move(old_t, old_v, new_v);       
	}

	inline static void copy(size_t old_t, const void * old_v, void * new_v)
	{
		if (old_t == typeid(F).hash_code())
			new (new_v) F(*reinterpret_cast<const F*>(old_v));
		else
			variant_helper<Ts...>::copy(old_t, old_v, new_v);       
	}   
};

template<> struct variant_helper<>  {
inline static void destroy(size_t id, void * data) { }
inline static void move(size_t old_t, void * old_v, void * new_v) { }
inline static void copy(size_t old_t, const void * old_v, void * new_v) { }
};

template<typename... Ts>
struct variant {
private: 
	static const size_t data_size = static_max<sizeof(Ts)...>::value;
	static const size_t data_align = static_max<alignof(Ts)...>::value;

	using data_t = typename std::aligned_storage<data_size, data_align>::type;

	using helper_t = variant_helper<Ts...>;
	
	static inline size_t invalid_type() {
		return typeid(void).hash_code();
	}

	size_t type_id;
	data_t data;
public: 
	variant() : type_id(invalid_type()) {   }

	variant(const variant<Ts...>& old) : type_id(old.type_id)
	{
		helper_t::copy(old.type_id, &old.data, &data);
	}

	variant(variant<Ts...>&& old) : type_id(old.type_id)
	{
		helper_t::move(old.type_id, &old.data, &data);
	}

	// Serves as both the move and the copy asignment operator.
	variant<Ts...>& operator= (variant<Ts...> old)
	{
		std::swap(type_id, old.type_id);
		std::swap(data, old.data);

		return *this;
	}

	template<typename T>
	void is() {
		return (type_id == typeid(T).hash_code());
	}

	void valid() {
		return (type_id != invalid_type());
	}

	template<typename T, typename... Args>
	void set(Args&&... args)
	{
		// First we destroy the current contents    
		helper_t::destroy(type_id, &data);      
		new (&data) T(std::forward<Args>(args)...);
		type_id = typeid(T).hash_code();
	}

	template<typename T>
	T& get()
	{
		// It is a dynamic_cast-like behaviour
		if (type_id == typeid(T).hash_code())
			return *reinterpret_cast<T*>(&data);
		else
			throw std::bad_cast();
	}   

	~variant() {
		helper_t::destroy(type_id, &data);
	}
};


struct test{
	int * holder;
	test() {
		std::cout << "test()" << std::endl;
		holder = new int();
	}

	test(test&& old) : holder(nullptr) {
		std::cout << "test(test&&)" << std::endl;
		std::swap(holder,old.holder);
	}
	test(const test& old) {
		std::cout << "test(const test&)" << std::endl;
		holder = new int(*old.holder);
	}
	~test()
	{
		std::cout << "~test()" << std::endl;
		delete holder;
	}
};


int main() {
	using my_var = variant<std::string, test>;
	
	my_var d;
	
	d.set<std::string>("First string");
	std::cout << d.get<std::string>() << std::endl;

	d.set<test>();
	*d.get<test>().holder = 42;

	my_var e(std::move(d));
	std::cout << *e.get<test>().holder << std::endl;    

	*e.get<test>().holder = 43;

	d = e;
	
	std::cout << *d.get<test>().holder << std::endl;    
}