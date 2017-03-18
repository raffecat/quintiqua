#pragma once

class QSGObject
{
public:
	QSGObject(void) : m_refs(0) {}
	virtual ~QSGObject(void) {};

	// Reference counting
public:
	inline void retain() { ++m_refs; }
	inline void release() { if (!--m_refs) delete this; }
private:
	int m_refs;
};

template <typename T>
class ref_ptr
{
public:
	ref_ptr() : p(0) {}
	ref_ptr(T* raw) : p(raw) {
		if (p) p->retain();
	}
	ref_ptr(const ref_ptr<T>& rp) : p(rp.p) {
		if (p) p->retain();
	}
	~ref_ptr() {
		if (p) p->release();
		p=0;
	}
	const ref_ptr<T>& operator = (T* raw) {
		if (p) p->release();
		p = raw;
		if (p) p->retain();
		return *this;
	}
	const ref_ptr<T>& operator = (const ref_ptr<T>& rp) {
		if (p) p->release();
		p = rp.p;
		if (p) p->retain();
		return *this;
	}
	T* operator -> () { return p; }
	T& operator * () { return *p; }
	operator T* () { return p; }
	operator bool () { return !!p; }
protected:
	T* p;
};
