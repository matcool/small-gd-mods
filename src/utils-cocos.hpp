#pragma once

template <typename T>
struct CCArrayIterator {
public:
	CCArrayIterator(T* p) : m_ptr(p) {}
	T* m_ptr;

	T& operator*() { return *m_ptr; }
	T* operator->() { return m_ptr; }

	auto& operator++() {
		++m_ptr;
		return *this;
	}

	friend bool operator== (const CCArrayIterator<T>& a, const CCArrayIterator<T>& b) { return a.m_ptr == b.m_ptr; };
	friend bool operator!= (const CCArrayIterator<T>& a, const CCArrayIterator<T>& b) { return a.m_ptr != b.m_ptr; };   
};

template <typename T>
class AwesomeArray {
public:
	AwesomeArray(cocos2d::CCArray* arr) : m_arr(arr) {}
	cocos2d::CCArray* m_arr;
	auto begin() { return CCArrayIterator<T*>(reinterpret_cast<T**>(m_arr->data->arr)); }
	auto end() { return CCArrayIterator<T*>(reinterpret_cast<T**>(m_arr->data->arr) + m_arr->count()); }

	auto size() const { return m_arr->count(); }
	T* operator[](size_t index) { return reinterpret_cast<T*>(m_arr->objectAtIndex(index)); }
};
