#pragma once

#include <Core/Span.hpp>

#include <utility>
#include <Core/FunctionRef.hpp>

namespace nbl {

template<typename T>
void quicksort(Span<T> values, nbl::FunctionRef<i32(const T&, const T&)> compareFunc = [](const T& t1, const T& t2) { return t1 < t2; }) {

    if(values.length() <= 1) return;

    T& pivot = values[values.length() - 1];
    usize i = -1;

    for(usize j = 0; j <= values.length() - 2; j++) {
        if(compareFunc(values[j], pivot)) {
            i++;
            std::swap(values[i], values[j]);
        }
    }

    std::swap(values[i + 1], values[values.length() - 1]);
    usize pivotIndex = i + 1;

    quicksort(values.slice(0, pivotIndex - 1), compareFunc);
    quicksort(values.slice(pivotIndex + 1), compareFunc);

}

}
