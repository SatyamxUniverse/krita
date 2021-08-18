/*
 *  SPDX-FileCopyrightText: 2021 Tanmay Chavan <earendil01tc@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DATABUFFER_H
#define DATABUFFER_H

#include "QtCore/qbytearray.h"

#include <stdlib.h>

/*
 * This file has been taken directly from Qt internal codebase, as it is
 * required for the improved clipping algorithm and it lied behind the Qt
 * API.
 */

QT_BEGIN_NAMESPACE

template <typename Type> class QDataBuffer
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    Q_DISABLE_COPY_MOVE(QDataBuffer)
#else
    Q_DISABLE_COPY(QDataBuffer)
#endif
public:
    QDataBuffer(int res)
    {
        capacity = res;
        if (res) {
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_GCC("-Walloc-size-larger-than=")
            buffer = (Type*) malloc(capacity * sizeof(Type));
            QT_WARNING_POP
            Q_CHECK_PTR(buffer);
        } else {
            buffer = nullptr;
        }
        siz = 0;
    }

    ~QDataBuffer()
    {
        if (buffer)
            free(buffer);
    }

    inline void reset() { siz = 0; }

    inline bool isEmpty() const { return siz==0; }

    inline int size() const { return siz; }
    inline Type *data() const { return buffer; }

    inline Type &at(int i) { Q_ASSERT(i >= 0 && i < siz); return buffer[i]; }
    inline const Type &at(int i) const { Q_ASSERT(i >= 0 && i < siz); return buffer[i]; }
    inline Type &last() { Q_ASSERT(!isEmpty()); return buffer[siz-1]; }
    inline const Type &last() const { Q_ASSERT(!isEmpty()); return buffer[siz-1]; }
    inline Type &first() { Q_ASSERT(!isEmpty()); return buffer[0]; }
    inline const Type &first() const { Q_ASSERT(!isEmpty()); return buffer[0]; }

    inline void add(const Type &t) {
        reserve(siz + 1);
        buffer[siz] = t;
        ++siz;
    }

    inline void pop_back() {
        Q_ASSERT(siz > 0);
        --siz;
    }

    inline void resize(int size) {
        reserve(size);
        siz = size;
    }

    inline void reserve(int size) {
        if (size > capacity) {
            if (capacity == 0)
                capacity = 1;
            while (capacity < size)
                capacity *= 2;
            buffer = (Type*) realloc(buffer, capacity * sizeof(Type));
            Q_CHECK_PTR(buffer);
        }
    }

    inline void shrink(int size) {
        capacity = size;
        if (size) {
            buffer = (Type*) realloc(buffer, capacity * sizeof(Type));
            Q_CHECK_PTR(buffer);
        } else {
            free(buffer);
            buffer = nullptr;
        }
    }

    inline void swap(QDataBuffer<Type> &other) {
        qSwap(capacity, other.capacity);
        qSwap(siz, other.siz);
        qSwap(buffer, other.buffer);
    }

    inline QDataBuffer &operator<<(const Type &t) { add(t); return *this; }

private:
    int capacity;
    int siz;
    Type *buffer;
};

QT_END_NAMESPACE


#endif // DATABUFFER_H
