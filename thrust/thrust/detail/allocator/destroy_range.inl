/*
 *  Copyright 2008-2021 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include <thrust/detail/config.h>

#if defined(_CCCL_IMPLICIT_SYSTEM_HEADER_GCC)
#  pragma GCC system_header
#elif defined(_CCCL_IMPLICIT_SYSTEM_HEADER_CLANG)
#  pragma clang system_header
#elif defined(_CCCL_IMPLICIT_SYSTEM_HEADER_MSVC)
#  pragma system_header
#endif // no system header

#include <thrust/detail/allocator/allocator_traits.h>
#include <thrust/detail/allocator/destroy_range.h>
#include <thrust/detail/memory_wrapper.h>
#include <thrust/detail/type_traits/pointer_traits.h>
#include <thrust/for_each.h>

THRUST_NAMESPACE_BEGIN
namespace detail
{
namespace allocator_traits_detail
{

// destroy_range has three cases:
// if Allocator has an effectful member function destroy:
//   1. destroy via the allocator
// else
//   2. if T has a non-trivial destructor, destroy the range without using the allocator
//   3. if T has a trivial destructor, do a no-op

template <typename Allocator, typename T>
struct has_effectful_member_destroy : has_member_destroy<Allocator, T>
{};

// std::allocator::destroy's only effect is to invoke its argument's destructor
template <typename U, typename T>
struct has_effectful_member_destroy<std::allocator<U>, T> : thrust::detail::false_type
{};

// case 1: Allocator has an effectful 1-argument member function "destroy"
template <typename Allocator, typename Pointer>
struct enable_if_destroy_range_case1
    : ::cuda::std::enable_if<has_effectful_member_destroy<Allocator, typename pointer_element<Pointer>::type>::value>
{};

// case 2: Allocator has no member function "destroy", but T has a non-trivial destructor
template <typename Allocator, typename Pointer>
struct enable_if_destroy_range_case2
    : ::cuda::std::enable_if<!has_effectful_member_destroy<Allocator, typename pointer_element<Pointer>::type>::value
                             && !::cuda::std::is_trivially_destructible<typename pointer_element<Pointer>::type>::value>
{};

// case 3: Allocator has no member function "destroy", and T has a trivial destructor
template <typename Allocator, typename Pointer>
struct enable_if_destroy_range_case3
    : ::cuda::std::enable_if<!has_effectful_member_destroy<Allocator, typename pointer_element<Pointer>::type>::value
                             && ::cuda::std::is_trivially_destructible<typename pointer_element<Pointer>::type>::value>
{};

template <typename Allocator>
struct destroy_via_allocator
{
  Allocator& a;

  _CCCL_HOST_DEVICE destroy_via_allocator(Allocator& a) noexcept
      : a(a)
  {}

  template <typename T>
  inline _CCCL_HOST_DEVICE void operator()(T& x) noexcept
  {
    allocator_traits<Allocator>::destroy(a, &x);
  }
};

// destroy_range case 1: destroy via allocator
template <typename Allocator, typename Pointer, typename Size>
_CCCL_HOST_DEVICE typename enable_if_destroy_range_case1<Allocator, Pointer>::type
destroy_range(Allocator& a, Pointer p, Size n) noexcept
{
  thrust::for_each_n(allocator_system<Allocator>::get(a), p, n, destroy_via_allocator<Allocator>(a));
}

// we must prepare for His coming
struct gozer
{
  _CCCL_EXEC_CHECK_DISABLE
  template <typename T>
  inline _CCCL_HOST_DEVICE void operator()(T& x) noexcept
  {
    x.~T();
  }
};

// destroy_range case 2: destroy without the allocator
template <typename Allocator, typename Pointer, typename Size>
_CCCL_HOST_DEVICE typename enable_if_destroy_range_case2<Allocator, Pointer>::type
destroy_range(Allocator& a, Pointer p, Size n) noexcept
{
  thrust::for_each_n(allocator_system<Allocator>::get(a), p, n, gozer());
}

// destroy_range case 3: no-op
template <typename Allocator, typename Pointer, typename Size>
_CCCL_HOST_DEVICE typename enable_if_destroy_range_case3<Allocator, Pointer>::type
destroy_range(Allocator&, Pointer, Size) noexcept
{
  // no op
}

} // namespace allocator_traits_detail

template <typename Allocator, typename Pointer, typename Size>
_CCCL_HOST_DEVICE void destroy_range(Allocator& a, Pointer p, Size n) noexcept
{
  return allocator_traits_detail::destroy_range(a, p, n);
}

} // namespace detail
THRUST_NAMESPACE_END
