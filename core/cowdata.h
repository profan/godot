/*************************************************************************/
/*  cowdata.h                                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef COWDATA_H_
#define COWDATA_H_

#include <string.h>

#include "main/bench.h"
#include "core/os/memory.h"
#include "core/safe_refcount.h"

template <class T, int N>
class Vector;
class String;
class CharString;
template <class T, class V>
class VMap;

template <class T, int N=2>
class CowData {
	template <class TV, int TN>
	friend class Vector;
	friend class String;
	friend class CharString;
	template <class TV, class VV>
	friend class VMap;

private:
	mutable T *_ptr;
	T _small_data[N];
	uint32_t _size;

	// internal helpers

	_FORCE_INLINE_ uint32_t *_get_refcount() const {

		if (_ptr == _small_data)
			return NULL;

		return reinterpret_cast<uint32_t *>(_ptr) - 2;
	}

	_FORCE_INLINE_ uint32_t *_get_capacity() const {

		if (_ptr == _small_data)
			return NULL;

		return reinterpret_cast<uint32_t *>(_ptr) - 1;
	}

	_FORCE_INLINE_ uint32_t _get_size() const {
		CRASH_COND(_ptr == NULL);
		return _size;
	}

	_FORCE_INLINE_ T *_get_data() const {
		CRASH_COND(_ptr == NULL);
		return reinterpret_cast<T *>(_ptr);
	}

	_FORCE_INLINE_ size_t _get_alloc_size(size_t p_elements) const {
		//return nearest_power_of_2_templated(p_elements*sizeof(T)+sizeof(SafeRefCount)+sizeof(int));
		return next_power_of_2(p_elements * sizeof(T));
	}

	_FORCE_INLINE_ bool _get_alloc_size_checked(size_t p_elements, size_t *out) const {
#if defined(_add_overflow) && defined(_mul_overflow)
		size_t o;
		size_t p;
		if (_mul_overflow(p_elements, sizeof(T), &o)) {
			*out = 0;
			return false;
		}
		*out = next_power_of_2(o);
		if (_add_overflow(o, static_cast<size_t>(32), &p)) return false; //no longer allocated here
		return true;
#else
		// Speed is more important than correctness here, do the operations unchecked
		// and hope the best
		*out = _get_alloc_size(p_elements);
		return true;
#endif
	}

	void _unref(void *p_data);
	void _ref(const CowData *p_from);
	void _ref(const CowData &p_from);
	void _copy_on_write();

public:
	void operator=(const CowData<T, N> &p_from) { _ref(p_from); }

	_FORCE_INLINE_ T *ptrw() {
		_copy_on_write();
		return (T *)_get_data();
	}

	_FORCE_INLINE_ const T *ptr() const {
		return _get_data();
	}

	_FORCE_INLINE_ int size() const {
		uint32_t size = _get_size();
		return size;
	}

	_FORCE_INLINE_ int capacity() const {
		uint32_t *capacity = _get_capacity();
		if (capacity) {
			return *capacity;
		} else {
			return N;
		}
	}

	_FORCE_INLINE_ void clear() {
		resize(0);
	}
	
	_FORCE_INLINE_ bool empty() const { return _size == 0; }

	_FORCE_INLINE_ void set(int p_index, const T &p_elem) {

		CRASH_BAD_INDEX(p_index, size());
		_copy_on_write();
		_get_data()[p_index] = p_elem;
	}

	_FORCE_INLINE_ T &get_m(int p_index) {

		CRASH_BAD_INDEX(p_index, size());
		_copy_on_write();
		return _get_data()[p_index];
	}

	_FORCE_INLINE_ const T &get(int p_index) const {

		CRASH_BAD_INDEX(p_index, size());

		return _get_data()[p_index];
	}

	Error resize(int p_size);

	_FORCE_INLINE_ void remove(int p_index) {

		ERR_FAIL_INDEX(p_index, size());
		T *p = ptrw();
		int len = size();
		for (int i = p_index; i < len - 1; i++) {

			p[i] = p[i + 1];
		};

		resize(len - 1);
	};

	Error insert(int p_pos, const T &p_val) {

		ERR_FAIL_INDEX_V(p_pos, size() + 1, ERR_INVALID_PARAMETER);
		resize(size() + 1);
		for (int i = (size() - 1); i > p_pos; i--)
			set(i, get(i - 1));
		set(p_pos, p_val);

		return OK;
	};

	int find(const T &p_val, int p_from = 0) const;

	_FORCE_INLINE_ CowData();
	_FORCE_INLINE_ ~CowData();
	_FORCE_INLINE_ CowData(CowData<T, N> &p_from) { _ref(p_from); };
};

template <class T>
class CowData<T, 0> {
	template <class TV, int TN>
	friend class Vector;
	friend class String;
	friend class CharString;
	template <class TV, class VV>
	friend class VMap;

private:
	mutable T *_ptr;
	uint32_t _size;

	// internal helpers

	_FORCE_INLINE_ uint32_t *_get_refcount() const {

		if (!_ptr)
			return NULL;

		return reinterpret_cast<uint32_t *>(_ptr) - 2;
	}

	_FORCE_INLINE_ uint32_t *_get_capacity() const {

		if (!_ptr)
			return NULL;

		return reinterpret_cast<uint32_t *>(_ptr) - 1;
	}

	_FORCE_INLINE_ uint32_t _get_size() const {

		if (!_ptr)
			return 0;

		return _size;
	}

	_FORCE_INLINE_ T *_get_data() const {

		if (!_ptr)
			return NULL;

		return reinterpret_cast<T *>(_ptr);
	}

	_FORCE_INLINE_ size_t _get_alloc_size(size_t p_elements) const {
		//return nearest_power_of_2_templated(p_elements*sizeof(T)+sizeof(SafeRefCount)+sizeof(int));
		return next_power_of_2(p_elements * sizeof(T));
	}

	_FORCE_INLINE_ bool _get_alloc_size_checked(size_t p_elements, size_t *out) const {
#if defined(_add_overflow) && defined(_mul_overflow)
		size_t o;
		size_t p;
		if (_mul_overflow(p_elements, sizeof(T), &o)) {
			*out = 0;
			return false;
		}
		*out = next_power_of_2(o);
		if (_add_overflow(o, static_cast<size_t>(32), &p)) return false; //no longer allocated here
		return true;
#else
		// Speed is more important than correctness here, do the operations unchecked
		// and hope the best
		*out = _get_alloc_size(p_elements);
		return true;
#endif
	}

	void _unref(void *p_data);
	void _ref(const CowData *p_from);
	void _ref(const CowData &p_from);
	void _copy_on_write();

public:
	void operator=(const CowData &p_from) { _ref(p_from); }

	_FORCE_INLINE_ T *ptrw() {
		_copy_on_write();
		return (T *)_get_data();
	}

	_FORCE_INLINE_ const T *ptr() const {
		return _get_data();
	}

	_FORCE_INLINE_ int size() const {
		uint32_t size = _get_size();
		return size;
	}

	_FORCE_INLINE_ int capacity() const {
		uint32_t *capacity = _get_capacity();
		if (capacity) {
			return *capacity;
		} else {
			return 0;
		}
	}

	_FORCE_INLINE_ void clear() {
		resize(0);
	}
	
	_FORCE_INLINE_ bool empty() const { return _size == 0; }

	_FORCE_INLINE_ void set(int p_index, const T &p_elem) {

		CRASH_BAD_INDEX(p_index, size());
		_copy_on_write();
		_get_data()[p_index] = p_elem;
	}

	_FORCE_INLINE_ T &get_m(int p_index) {

		CRASH_BAD_INDEX(p_index, size());
		_copy_on_write();
		return _get_data()[p_index];
	}

	_FORCE_INLINE_ const T &get(int p_index) const {

		CRASH_BAD_INDEX(p_index, size());

		return _get_data()[p_index];
	}

	Error resize(int p_size);

	_FORCE_INLINE_ void remove(int p_index) {

		ERR_FAIL_INDEX(p_index, size());
		T *p = ptrw();
		int len = size();
		for (int i = p_index; i < len - 1; i++) {

			p[i] = p[i + 1];
		};

		resize(len - 1);
	};

	Error insert(int p_pos, const T &p_val) {

		ERR_FAIL_INDEX_V(p_pos, size() + 1, ERR_INVALID_PARAMETER);
		resize(size() + 1);
		for (int i = (size() - 1); i > p_pos; i--)
			set(i, get(i - 1));
		set(p_pos, p_val);

		return OK;
	};

	int find(const T &p_val, int p_from = 0) const;

	_FORCE_INLINE_ CowData();
	_FORCE_INLINE_ ~CowData();
	_FORCE_INLINE_ CowData(CowData &p_from) { _ref(p_from); };
};

template <class T, int N>
void CowData<T, N>::_unref(void *p_data) {

	if (!p_data)
		return;

	uint32_t *refc = _get_refcount();

	if (atomic_decrement(refc) > 0) {
		return; // still in use
	}

	// clean up
	if (!__has_trivial_destructor(T)) {
		uint32_t count = _get_size();
		T *data = _get_data();

		for (uint32_t i = 0; i < count; ++i) {
			// call destructors
			data[i].~T();
		}
	}

	// free mem
	Memory::free_static((uint8_t *)p_data, true);

	// log final size for profiling purposes
	add_cowdata_size(_size);

}

template <class T>
void CowData<T, 0>::_unref(void *p_data) {

	if (!p_data)
		return;

	uint32_t *refc = _get_refcount();

	if (refc != NULL && atomic_decrement(refc) > 0) {
		return; // still in use or small vector
	}

	// clean up
	if (!__has_trivial_destructor(T)) {
		uint32_t count = _get_size();
		T *data = _get_data();

		for (uint32_t i = 0; i < count; ++i) {
			// call destructors
			data[i].~T();
		}
	}

	// free mem, if necessary
	if (refc != NULL) {
		Memory::free_static((uint8_t *)p_data, true);
	}

	// log final size for profiling purposes
	add_cowdata_size(_size);

}

template <class T, int N>
void CowData<T, N>::_copy_on_write() {

	if (!_ptr)
		return;

	uint32_t *refc = _get_refcount();

	if (unlikely(*refc > 1)) {
		/* in use by more than me */
		uint32_t current_size = *_get_capacity();

		uint32_t *mem_new = (uint32_t *)Memory::alloc_static(current_size * sizeof(T), true);
		*(mem_new - 1) = current_size; // capacity
		*(mem_new - 2) = 1; // refcount

		T *_data = (T *)(mem_new);

		// initialize new elements
		if (__has_trivial_copy(T)) {
			memcpy(mem_new, _ptr, _size * sizeof(T));
		} else {
			for (uint32_t i = 0; i < _size; i++) {
				memnew_placement(&_data[i], T(_get_data()[i]));
			}
		}

		_unref(_ptr);
		_ptr = _data;
	}
}

template <class T>
void CowData<T, 0>::_copy_on_write() {

	if (!_ptr)
		return;

	uint32_t *refc = _get_refcount();
	if (refc == NULL) return; // we're small enough that it's still in the small vector segment

	if (unlikely(*refc > 1)) {
		/* in use by more than me */
		uint32_t current_size = *_get_capacity();

		uint32_t *mem_new = (uint32_t *)Memory::alloc_static(current_size * sizeof(T), true);
		*(mem_new - 1) = current_size; // capacity
		*(mem_new - 2) = 1; // refcount

		T *_data = (T *)(mem_new);

		// initialize new elements
		if (__has_trivial_copy(T)) {
			memcpy(mem_new, _ptr, _size * sizeof(T));
		} else {
			for (uint32_t i = 0; i < _size; i++) {
				memnew_placement(&_data[i], T(_get_data()[i]));
			}
		}

		_unref(_ptr);
		_ptr = _data;
	}
}

template <class T>
Error CowData<T, 0>::resize(int p_size) {

	ERR_FAIL_COND_V(p_size < 0, ERR_INVALID_PARAMETER);

	if (p_size == size()) {
		return OK;
	}

	// possibly changing size, copy on write
	_copy_on_write();

	uint32_t alloc_size = p_size * 1.618;
	//ERR_FAIL_COND_V(!_get_alloc_size_checked(p_size, &alloc_size), ERR_OUT_OF_MEMORY);

	if (p_size > capacity()) {

		if (capacity() == 0) {

			// heap alloc from scratch
			uint32_t *new_ptr = (uint32_t *)Memory::alloc_static(alloc_size * sizeof(T), true);
			ERR_FAIL_COND_V(!new_ptr, ERR_OUT_OF_MEMORY);
			*(new_ptr - 1) = 0; // capacity, currently none
			*(new_ptr - 2) = 1; // refcount
			_ptr = (T*)new_ptr;

		} else {

			void *_ptrnew = (T *)Memory::realloc_static(_ptr, alloc_size * sizeof(T), true);
			ERR_FAIL_COND_V(!_ptrnew, ERR_OUT_OF_MEMORY);
			_ptr = (T *)(_ptrnew);

		}

		// construct the newly created elements
		if (!__has_trivial_constructor(T)) {
			T *elems = _get_data();

			for (int i = *_get_capacity(); i < p_size; i++) {
				memnew_placement(&elems[i], T);
			}
		}

		*_get_capacity() = (uint32_t)(alloc_size);
		_size = p_size;

	} else if (p_size > size()) {

		if (!__has_trivial_constructor(T)) {
			T *elems = _get_data();

			for (int i = _get_size(); i < p_size; i++) {
				memnew_placement(&elems[i], T);
			}
		}

		_size = p_size;

	} else if (p_size < size()) {

		if (!__has_trivial_destructor(T)) {
			// deinitialize no longer needed elements
			for (uint32_t i = p_size; i < _get_size(); i++) {
				T *t = &_get_data()[i];
				t->~T();
			}
		}

		_size = p_size;

	}

	return OK;
}

template <class T, int N>
Error CowData<T, N>::resize(int p_size) {

	ERR_FAIL_COND_V(p_size < 0, ERR_INVALID_PARAMETER);

	if (p_size == size()) {
		return OK;
	}

	// possibly changing size, copy on write
	_copy_on_write();

	uint32_t alloc_size = p_size * 1.618;
	//ERR_FAIL_COND_V(!_get_alloc_size_checked(p_size, &alloc_size), ERR_OUT_OF_MEMORY);

	if (p_size > capacity()) {

		if (_ptr == _small_data) {

			// heap alloc from scratch
			uint32_t *new_ptr = (uint32_t *)Memory::alloc_static(alloc_size * sizeof(T), true);
			ERR_FAIL_COND_V(!new_ptr, ERR_OUT_OF_MEMORY);
			*(new_ptr - 1) = 0; // capacity, currently none
			*(new_ptr - 2) = 1; // refcount
			T *data = (T*)new_ptr;

			// initialize new elements
			if (__has_trivial_copy(T)) {
				memcpy(new_ptr, _ptr, _size * sizeof(T));
			} else {
				for (uint32_t i = 0; i < _size; i++) {
					memnew_placement(&data[i], T(_get_data()[i]));
				}
			}

			_ptr = data;

		} else {

			void *_ptrnew = (T *)Memory::realloc_static(_ptr, alloc_size * sizeof(T), true);
			ERR_FAIL_COND_V(!_ptrnew, ERR_OUT_OF_MEMORY);
			_ptr = (T *)(_ptrnew);

		}

		// construct the newly created elements
		if (!__has_trivial_constructor(T)) {
			T *elems = _get_data();

			for (int i = *_get_capacity(); i < p_size; i++) {
				memnew_placement(&elems[i], T);
			}
		}

		*_get_capacity() = (uint32_t)(alloc_size);
		_size = p_size;

	} else if (p_size > size()) {

		if (!__has_trivial_constructor(T)) {
			T *elems = _get_data();

			for (int i = _get_size(); i < p_size; i++) {
				memnew_placement(&elems[i], T);
			}
		}

		_size = p_size;

	} else if (p_size < size()) {

		if (!__has_trivial_destructor(T)) {
			// deinitialize no longer needed elements
			for (uint32_t i = p_size; i < _get_size(); i++) {
				T *t = &_get_data()[i];
				t->~T();
			}
		}

		_size = p_size;

	}

	return OK;
}

template <class T, int N>
int CowData<T, N>::find(const T &p_val, int p_from) const {
	int ret = -1;

	if (p_from < 0 || size() == 0) {
		return ret;
	}

	for (int i = p_from; i < size(); i++) {
		if (get(i) == p_val) {
			ret = i;
			break;
		}
	}

	return ret;
}

template <class T, int N>
void CowData<T, N>::_ref(const CowData<T, N> *p_from) {
	_ref(*p_from);
}

template <class T, int N>
void CowData<T, N>::_ref(const CowData<T, N> &p_from) {

	if (_ptr == p_from._ptr)
		return; // self assign, do nothing.

	_unref(_ptr);
	_ptr = NULL;

	if (!p_from._ptr)
		return; //nothing to do

	uint32_t *other_ref = p_from._get_refcount();
	if (other_ref == NULL) {
		memcpy(_small_data, p_from._small_data, sizeof(_small_data));
		_size = p_from._size;
		_ptr = _small_data;
		return;
	}

	if (atomic_conditional_increment(p_from._get_refcount()) > 0) { // could reference
		_size = p_from._size; // FIXME: is this correct?
		_ptr = p_from._ptr;
	}

}

template <class T>
void CowData<T, 0>::_ref(const CowData &p_from) {

	if (_ptr == p_from._ptr)
		return; // self assign, do nothing.

	_unref(_ptr);
	_ptr = NULL;

	if (!p_from._ptr)
		return; //nothing to do

	if (atomic_conditional_increment(p_from._get_refcount()) > 0) { // could reference
		_size = p_from._size; // FIXME: is this correct?
		_ptr = p_from._ptr;
	}

}

template <class T, int N>
CowData<T, N>::CowData() {
	_size = 0;
	_ptr = NULL;
}

template <class T>
CowData<T, 0>::CowData() {
	_size = 0;
	_ptr = _small_data;
}

template <class T, int N>
CowData<T, N>::~CowData() {

	_unref(_ptr);
	_size = 0;
}

template <class T>
CowData<T, 0>::~CowData() {

	_unref(_ptr);
	_size = 0;
}


#endif /* COW_H_ */
