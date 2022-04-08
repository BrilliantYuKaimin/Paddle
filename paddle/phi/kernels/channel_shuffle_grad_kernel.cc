// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/phi/kernels/channel_shuffle_grad_kernel.h"
#include <string>
#include <vector>
#include "paddle/phi/backends/all_context.h"
#include "paddle/phi/core/dense_tensor.h"
#include "paddle/phi/core/kernel_registry.h"
#include "paddle/phi/kernels/funcs/math_function.h"

namespace phi {

template <typename T, typename Context>
void ChannelShuffleGradKernel(const Context& ctx,
                              const DenseTensor& out_grad,
                              int groups,
                              const std::string& data_format,
                              DenseTensor* x_grad) {
  auto* dout = &out_grad;
  auto* dx = x_grad;
  ctx.template Alloc<T>(dx);
  bool channel_last = (data_format == "NHWC");
  auto do_dims = dout->dims();
  auto dx_dims = dx->dims();

  DenseTensor t(*dout);
  if (!channel_last) {
    t.Resize({do_dims[0], do_dims[1] / groups, groups, do_dims[2], do_dims[3]});
  } else {
    t.Resize({do_dims[0], do_dims[1], do_dims[2], do_dims[3] / groups, groups});
  }
  auto axis = !channel_last ? std::vector<int>{0, 2, 1, 3, 4}
                            : std::vector<int>{0, 1, 2, 4, 3};

  DenseTensor o(*dx);
  if (!channel_last) {
    o.Resize({dx_dims[0], groups, dx_dims[1] / groups, dx_dims[2], dx_dims[3]});
  } else {
    o.Resize({dx_dims[0], dx_dims[1], dx_dims[2], groups, dx_dims[3] / groups});
  }
  phi::funcs::Transpose<Context, T, 5> trans;
  trans(ctx, t, &o, axis);
  dx->Resize(dx_dims);
}

}  // namespace phi

PD_REGISTER_KERNEL(channel_shuffle_grad,
                   CPU,
                   ALL_LAYOUT,
                   phi::ChannelShuffleGradKernel,
                   float,
                   double) {}

#if defined(PADDLE_WITH_CUDA) || defined(PADDLE_WITH_HIP)
PD_REGISTER_KERNEL(channel_shuffle_grad,
                   GPU,
                   ALL_LAYOUT,
                   phi::ChannelShuffleGradKernel,
                   float,
                   double) {}
#endif
