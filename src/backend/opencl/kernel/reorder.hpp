/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <Param.hpp>
#include <cache.hpp>
#include <common/dispatch.hpp>
#include <debug_opencl.hpp>
#include <kernel_headers/reorder.hpp>
#include <program.hpp>
#include <traits.hpp>
#include <string>

using cl::Buffer;
using cl::EnqueueArgs;
using cl::Kernel;
using cl::KernelFunctor;
using cl::NDRange;
using cl::Program;
using std::string;

namespace opencl {
namespace kernel {
// Kernel Launch Config Values
static const int TX    = 32;
static const int TY    = 8;
static const int TILEX = 512;
static const int TILEY = 32;

template<typename T>
void reorder(Param out, const Param in, const dim_t* rdims) {
    std::string refName = std::string("reorder_kernel_") +
                          std::string(dtype_traits<T>::getName());

    int device       = getActiveDeviceId();
    kc_entry_t entry = kernelCache(device, refName);

    if (entry.prog == 0 && entry.ker == 0) {
        std::ostringstream options;
        options << " -D T=" << dtype_traits<T>::getName();
        options << getTypeBuildDefinition<T>();

        const char* ker_strs[] = {reorder_cl};
        const int ker_lens[]   = {reorder_cl_len};
        Program prog;
        buildProgram(prog, 1, ker_strs, ker_lens, options.str());
        entry.prog = new Program(prog);
        entry.ker  = new Kernel(*entry.prog, "reorder_kernel");

        addKernelToCache(device, refName, entry);
    }

    auto reorderOp =
        KernelFunctor<Buffer, const Buffer, const KParam, const KParam,
                      const int, const int, const int, const int, const int,
                      const int>(*entry.ker);

    NDRange local(TX, TY, 1);

    int blocksPerMatX = divup(out.info.dims[0], TILEX);
    int blocksPerMatY = divup(out.info.dims[1], TILEY);
    NDRange global(local[0] * blocksPerMatX * out.info.dims[2],
                   local[1] * blocksPerMatY * out.info.dims[3], 1);

    reorderOp(EnqueueArgs(getQueue(), global, local), *out.data, *in.data,
              out.info, in.info, rdims[0], rdims[1], rdims[2], rdims[3],
              blocksPerMatX, blocksPerMatY);

    CL_DEBUG_FINISH(getQueue());
}
}  // namespace kernel
}  // namespace opencl
