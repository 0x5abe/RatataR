#ifndef _DYNARRAY_Z_H_
#define _DYNARRAY_Z_H_
#include "Types_Z.h"

#define DYNARRAY_Z_EXP(exp)
#define DYA_SIZEBITS 18
#define DYA_RSVSIZEBITS (32 - DYA_SIZEBITS)
#define DYA_SIZEMAX ((1 << DYA_SIZEBITS))
#define DYA_RSVSIZEMAX ((1 << DYA_RSVSIZEBITS))

template <class T, int Granularity = 8, Bool DeleteObject = TRUE, Bool InitObject = TRUE, int Align = _ALLOCDEFAULTALIGN>
class DynArray_Z {
public:
    T* GetArrayPtr() const {
        return m_ArrayPtr;
    }

    int GetSize() const {
        return m_Size;
    }

    T& Get(int i_Index) {
        DYNARRAY_Z_EXP(i_Index < m_Size);
        return m_ArrayPtr[i_Index];
    }

    const T& Get(int i_Index) const {
        DYNARRAY_Z_EXP(i_Index < m_Size);
        return m_ArrayPtr[i_Index];
    }

    const T& operator[](int i_Index) const {
        DYNARRAY_Z_EXP(i_Index < m_Size);
        return m_ArrayPtr[i_Index];
    }

    T& operator[](int i_Index) {
        DYNARRAY_Z_EXP(i_Index < m_Size);
        return m_ArrayPtr[i_Index];
    }

private:

    U32 m_ReservedSize : 14,
        m_Size : 18;
    T* m_ArrayPtr;
};

typedef DynArray_Z<S32, 32, FALSE, FALSE, 4> S32DA;
typedef DynArray_Z<S16, 32, FALSE, FALSE, 4> S16DA;
typedef DynArray_Z<S8, 32, FALSE, FALSE, 4> S8DA;
typedef DynArray_Z<U32, 32, FALSE, FALSE, 4> U32DA;
typedef DynArray_Z<U16, 32, FALSE, FALSE, 4> U16DA;
typedef DynArray_Z<U8, 32, FALSE, FALSE, 4> U8DA;
typedef DynArray_Z<Bool, 32, FALSE, FALSE, 4> BoolDA;
typedef DynArray_Z<Float, 32, FALSE, FALSE, 4> FloatDA;

#endif
