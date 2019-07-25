/*************************************************************************/
/*  vector.h                                                             */
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

#ifndef VECTOR_H
#define VECTOR_H

/**
 * @class Vector
 * @author Juan Linietsky
 * Vector container. Regular Vector Container. Use with care and for smaller arrays when possible. Use PoolVector for large arrays.
*/

#include "core/cowdata.h"
#include "core/error_macros.h"
#include "core/os/memory.h"
#include "core/sort_array.h"

template <class T>
class VectorWriteProxy {
public:
	_FORCE_INLINE_ T &operator[](int p_index) {
		CRASH_BAD_INDEX(p_index, ((Vector<T> *)(this))->_cowdata.size());

		return ((Vector<T> *)(this))->_cowdata.ptrw()[p_index];
	}
};

template <class T>
class Vector {
	friend class VectorWriteProxy<T>;

public:
	VectorWriteProxy<T> write;

private:
	CowData<T> _cowdata;
	int _size;

public:
	bool push_back(const T &p_elem);

	void remove(int p_index) {

		ERR_FAIL_INDEX(p_index, size());
		T *p = _cowdata.ptrw();
		int len = size();
		for (int i = p_index; i < len - 1; i++) {

			p[i] = p[i + 1];
		};

		if (!__has_trivial_destructor(T)) {
			(&p[_size])->~T();
		}
		_size--;

	}

	void erase(const T &p_val) {
		int idx = find(p_val);
		if (idx >= 0) remove(idx);
	};
	void invert();

	_FORCE_INLINE_ T *ptrw() { return _cowdata.ptrw(); }
	_FORCE_INLINE_ const T *ptr() const { return _cowdata.ptr(); }
	_FORCE_INLINE_ void clear() {
		_size = 0;
		resize(0);
	}
	_FORCE_INLINE_ bool empty() const { return _size == 0; }

	_FORCE_INLINE_ T get(int p_index) { return _cowdata.get(p_index); }
	_FORCE_INLINE_ const T get(int p_index) const { return _cowdata.get(p_index); }
	_FORCE_INLINE_ void set(int p_index, const T &p_elem) { _cowdata.set(p_index, p_elem); }
	_FORCE_INLINE_ int capacity() const { return _cowdata.size(); }
	_FORCE_INLINE_ int size() const { return _size; }
	Error resize(int p_size) {

		if (p_size > capacity()) {
			_size = p_size;
			Error err = _cowdata.resize(p_size);
			return err;
		} else if (p_size < size()) {
			for (int i = p_size; i < size(); ++i) {
				if (!__has_trivial_destructor(T)) {
					(&_cowdata.ptrw()[i])->~T();
				}
			}
			_size = p_size;
		} else if (p_size > size()) {
			for (int i = size(); i < p_size; ++i) {
				if (!__has_trivial_constructor(T)) {
					memnew_placement(&_cowdata.ptrw()[i], T);
				}
			}
			_size = p_size;
		}

		return Error::OK;

	}
	Error reserve(int p_size) {

		if (p_size < _size) {
			_size = p_size;
		}

		if (p_size > capacity()) {
			return _cowdata.resize(p_size);
		} else if (p_size < size()) {
			for (int i = size(); i < p_size; ++i) {
				if (!__has_trivial_destructor(T)) {
					(&_cowdata.ptrw()[i])->~T();
				}
			}
		} else if (p_size > size()) {
			for (int i = size(); i < p_size; ++i) {
				if (!__has_trivial_constructor(T)) {
					memnew_placement(&(_cowdata.ptrw()[i]), T);
				}
			}
		}
		
		return Error::OK;

	}
	_FORCE_INLINE_ const T &operator[](int p_index) const { return _cowdata.get(p_index); }
	Error insert(int p_pos, const T &p_val) {
		
		ERR_FAIL_INDEX_V(p_pos, size() + 1, ERR_INVALID_PARAMETER);
		reserve(size() + 1);
		for (int i = (size() - 1); i > p_pos; i--)
			set(i, get(i - 1));
		set(p_pos, p_val);

		_size++;

		return OK;

	}
	int find(const T &p_val, int p_from = 0) const { return _cowdata.find(p_val, p_from); }

	void append_array(const Vector<T> &p_other);

	template <class C>
	void sort_custom() {

		int len = _cowdata.size();
		if (len == 0)
			return;

		T *data = ptrw();
		SortArray<T, C> sorter;
		sorter.sort(data, len);
	}

	void sort() {

		sort_custom<_DefaultComparator<T> >();
	}

	void ordered_insert(const T &p_val) {
		int i;
		for (i = 0; i < size(); i++) {

			if (p_val < operator[](i)) {
				break;
			};
		};
		insert(i, p_val);
	}

	_FORCE_INLINE_ Vector() : _size(0) {}
	_FORCE_INLINE_ Vector(const Vector &p_from) { 
		_size = p_from._size;
		_cowdata._ref(p_from._cowdata);
	}
	inline Vector &operator=(const Vector &p_from) {
		_size = p_from._size;
		_cowdata._ref(p_from._cowdata);
		return *this;
	}

	_FORCE_INLINE_ ~Vector() {}
};

template <class T>
void Vector<T>::invert() {

	for (int i = 0; i < size() / 2; i++) {
		T *p = ptrw();
		SWAP(p[i], p[size() - i]);
	}
}

template <class T>
void Vector<T>::append_array(const Vector<T> &p_other) {

	const int ds = p_other.size();
	if (ds == 0) return;

	const int bs = size();
	if (bs + ds > capacity()) {
		reserve(bs + ds);
	}

	for (int i = 0; i < ds; ++i) {
		ptrw()[bs + i] = p_other[i];
		_size++;
	}
	
}

template <class T>
bool Vector<T>::push_back(const T &p_elem) {

	if (size() == capacity()) {
		Error err = reserve(size() + 1);
		ERR_FAIL_COND_V(err, true);
	}

	if (!__has_trivial_constructor(T)) {
		memnew_placement(&(_cowdata.ptrw()[size()]), T);
	}

	set(size(), p_elem);
	_size++;

	return false;
}

#endif
