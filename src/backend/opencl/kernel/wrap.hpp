/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <Array.hpp>
#include <Param.hpp>
#include <cache.hpp>
#include <common/dispatch.hpp>
#include <debug_opencl.hpp>
#include <kernel_headers/wrap.hpp>
#include <kernel_headers/wrap_dilated.hpp>
#include <math.hpp>
#include <program.hpp>
#include <traits.hpp>
#include <type_util.hpp>
#include <map>
#include <mutex>
#include <string>
#include "config.hpp"

using cl::Buffer;
using cl::EnqueueArgs;
using cl::Kernel;
using cl::KernelFunctor;
using cl::NDRange;
using cl::Program;
using std::string;

namespace opencl {
namespace kernel {

template<typename T>
void wrap(Param out, const Param in, const dim_t wx, const dim_t wy,
          const dim_t sx, const dim_t sy, const dim_t px, const dim_t py,
          const bool is_column) {
    std::string ref_name = std::string("wrap_") +
                           std::string(dtype_traits<T>::getName()) +
                           std::string("_") + std::to_string(is_column);

    int device       = getActiveDeviceId();
    kc_entry_t entry = kernelCache(device, ref_name);

    if (entry.prog == 0 && entry.ker == 0) {
        ToNumStr<T> toNumStr;
        std::ostringstream options;
        options << " -D is_column=" << is_column
                << " -D ZERO=" << toNumStr(scalar<T>(0))
                << " -D T=" << dtype_traits<T>::getName();
        options << getTypeBuildDefinition<T>();

        Program prog;
        buildProgram(prog, wrap_cl, wrap_cl_len, options.str());

        entry.prog = new Program(prog);
        entry.ker  = new Kernel(*entry.prog, "wrap_kernel");

        addKernelToCache(device, ref_name, entry);
    }

    dim_t nx = (out.info.dims[0] + 2 * px - wx) / sx + 1;
    dim_t ny = (out.info.dims[1] + 2 * py - wy) / sy + 1;

    NDRange local(THREADS_X, THREADS_Y);

    dim_t groups_x = divup(out.info.dims[0], local[0]);
    dim_t groups_y = divup(out.info.dims[1], local[1]);

    NDRange global(local[0] * groups_x * out.info.dims[2],
                   local[1] * groups_y * out.info.dims[3]);

    auto wrapOp =
        KernelFunctor<Buffer, const KParam, const Buffer, const KParam,
                      const int, const int, const int, const int, const int,
                      const int, const int, const int, const int, const int>(
            *entry.ker);

    wrapOp(EnqueueArgs(getQueue(), global, local), *out.data, out.info,
           *in.data, in.info, wx, wy, sx, sy, px, py, nx, ny, groups_x,
           groups_y);

    CL_DEBUG_FINISH(getQueue());
}

template<typename T>
void wrap_dilated(Param out, const Param in, const dim_t wx, const dim_t wy,
                  const dim_t sx, const dim_t sy, const dim_t px,
                  const dim_t py, const dim_t dx, const dim_t dy,
                  const bool is_column) {
    std::string ref_name = std::string("wrap_dilated_") +
                           std::string(dtype_traits<T>::getName()) +
                           std::string("_") + std::to_string(is_column);

    int device       = getActiveDeviceId();
    kc_entry_t entry = kernelCache(device, ref_name);

    if (entry.prog == 0 && entry.ker == 0) {
        ToNumStr<T> toNumStr;
        std::ostringstream options;
        options << " -D is_column=" << is_column
                << " -D ZERO=" << toNumStr(scalar<T>(0))
                << " -D T=" << dtype_traits<T>::getName();
        options << getTypeBuildDefinition<T>();

        Program prog;
        buildProgram(prog, wrap_dilated_cl, wrap_dilated_cl_len, options.str());

        entry.prog = new Program(prog);
        entry.ker  = new Kernel(*entry.prog, "wrap_dilated_kernel");

        addKernelToCache(device, ref_name, entry);
    }

    dim_t nx = 1 + (out.info.dims[0] + 2 * px - (((wx - 1) * dx) + 1)) / sx;
    dim_t ny = 1 + (out.info.dims[1] + 2 * py - (((wy - 1) * dy) + 1)) / sy;

    NDRange local(THREADS_X, THREADS_Y);

    dim_t groups_x = divup(out.info.dims[0], local[0]);
    dim_t groups_y = divup(out.info.dims[1], local[1]);

    NDRange global(local[0] * groups_x * out.info.dims[2],
                   local[1] * groups_y * out.info.dims[3]);

    auto wrapOp =
        KernelFunctor<Buffer, const KParam, const Buffer, const KParam,
                      const int, const int, const int, const int, const int,
                      const int, const int, const int, const int, const int,
                      const int, const int>(*entry.ker);

    wrapOp(EnqueueArgs(getQueue(), global, local), *out.data, out.info,
           *in.data, in.info, wx, wy, sx, sy, px, py, dx, dy, nx, ny, groups_x,
           groups_y);

    CL_DEBUG_FINISH(getQueue());
}

}  // namespace kernel
}  // namespace opencl
