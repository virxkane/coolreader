/** \file lvfastref.h
	\brief Fast smart pointer with reference counting

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.
	See LICENSE file for details.
*/

#ifndef __LVFASTREF_H_INCLUDED__
#define __LVFASTREF_H_INCLUDED__

/// Fast smart pointer with reference counting
/**
	Stores pointer to object and reference counter.
	Imitates usual pointer behavior, but deletes object
	when there are no more references on it.
	On copy, increases reference counter.
	On destroy, decreases reference counter; deletes object if counter became 0.
	T should implement AddRef(), Release() and getRefCount() methods.
	\param T class of stored object
 */
template <class T> class LVFastRef
{
private:
	T * _ptr;
	inline void Release()
	{
		if ( _ptr ) {
			if ( _ptr->Release()==0 ) {
				delete _ptr;
			}
			_ptr=NULL;
		}
	}
public:
	/// Default constructor.
	/** Initializes pointer to NULL */
	LVFastRef() : _ptr(NULL) { }

	/// Constructor by object pointer.
	/** Initializes pointer to given value
	\param ptr is a pointer to object
	 */
	explicit LVFastRef( T * ptr ) {
		_ptr = ptr;
		if ( _ptr )
			_ptr->AddRef();
	}

	/// Copy constructor.
	/** Creates copy of object pointer. Increments reference counter instead of real copy.
	\param ref is reference to copy
	 */
	LVFastRef( const LVFastRef & ref )
	{
		_ptr = ref._ptr;
		if ( _ptr )
			_ptr->AddRef();
	}

	/// Destructor.
	/** Decrements reference counter; deletes object if counter became 0. */
	~LVFastRef() { Release(); }

	/// Clears pointer.
	/** Sets object pointer to NULL. */
	void Clear() { Release(); }

	/// Copy operator.
	/** Duplicates a pointer from specified reference.
	Increments counter instead of copying of object.
	\param ref is reference to copy
	 */
	LVFastRef & operator = ( const LVFastRef & ref )
	{
		if ( _ptr ) {
			if ( _ptr==ref._ptr )
				return *this;
			Release();
		}
		if ( ref._ptr ) {
			_ptr = ref._ptr;
			_ptr->AddRef();
		}
		return *this;
	}

	/// Object pointer assignment operator.
	/** Sets object pointer to the specified value.
	Reference counter is being initialized to 1.
	\param obj pointer to object
	 */
	LVFastRef & operator = ( T * obj )
	{
		if ( _ptr ) {
			if ( _ptr==obj )
				return *this;
			Release();
		}
		if ( obj ) {
			_ptr = obj;
			_ptr->AddRef();
		}
		return *this;
	}

	/// Returns stored pointer to object.
	/** Imitates usual pointer behavior.
	Usual way to access object fields.
	 */
	T * operator -> () const { return _ptr; }

	/// Dereferences pointer to object.
	/** Imitates usual pointer behavior. */
	T & operator * () const { return *_ptr; }

	/// To check reference counter value.
	/** It might be useful in some cases.
	\return reference counter value.
	 */
	int getRefCount() const { return _ptr->getRefCount(); }

	/// Returns stored pointer to object.
	/** Usual way to get pointer value.
	\return stored pointer to object.
	 */
	T * get() const { return _ptr; }

	/// Checks whether pointer is NULL or not.
	/** \return true if pointer is NULL.
	\sa isNull() */
	bool operator ! () const { return !_ptr; }

	/// Checks whether pointer is NULL or not.
	/** \return true if pointer is NULL.
	\sa operator !()
	 */
	bool isNull() const { return (_ptr == NULL); }
};

#endif	// __LVFASTREF_H_INCLUDED__
