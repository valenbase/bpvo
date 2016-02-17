/*
   This file is part of bpvo.

   bpvo is free software: you can redistribute it and/or modify
   it under the terms of the Lesser GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   bpvo is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   Lesser GNU General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with bpvo.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Contributor: halismai@cs.cmu.edu
 */

#include "bpvo/channels.h"
#include "bpvo/census.h"
#include "bpvo/imgproc.h"
#include <cmath>

#include <opencv2/imgproc/imgproc.hpp>

#define DO_BITPLANES_WITH_TBB 1
#if DO_BITPLANES_WITH_TBB
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#endif


namespace bpvo {

RawIntensity::RawIntensity(float,float) {}

RawIntensity::RawIntensity(const cv::Mat& I)
{
  this->compute(I);
}

void RawIntensity::compute(const cv::Mat& I)
{
  //assert( I.type() == cv::DataType<uint8_t>::type );
  I.convertTo(_I, CV_32FC1);
}

cv::Mat_<float> RawIntensity::computeSaliencyMap() const
{
  cv::Mat_<float> ret(_I.size());
  gradientAbsoluteMagnitude(_I, ret);
  return ret;
}

void RawIntensity::computeSaliencyMap(cv::Mat_<float>& buffer) const
{
  gradientAbsoluteMagnitude(_I, buffer);
}

template <typename DstType> static inline
void extractChannel(const cv::Mat& C, cv::Mat& dst, int b, float sigma)
{
  assert( b >= 0 && b < 8  );

  dst.create(C.size(), cv::DataType<DstType>::type);

  auto src_ptr = C.ptr<const uint8_t>();
  auto dst_ptr = dst.ptr<DstType>();
  auto n = C.rows * C.cols;

#if defined(WITH_OPENMP)
#pragma omp simd
#endif
  for(int i = 0; i < n; ++i) {
    dst_ptr[i] = static_cast<DstType>( (src_ptr[i] & (1<<b)) >> b );
  }

  if(sigma > 0.0f)
    cv::GaussianBlur(dst, dst, cv::Size(5,5), sigma, sigma);
}

void BitPlanes::compute(const cv::Mat& I)
{
  assert( I.type() == cv::DataType<uint8_t>::type );

  auto C = census(I, _sigma_ct);

#if DO_BITPLANES_WITH_TBB
  tbb::parallel_for(tbb::blocked_range<int>(0, NumChannels),
                    [=](const tbb::blocked_range<int>& r)
                    {
                      for(int c = r.begin(); c != r.end(); ++c)
                      {
                        extractChannel<float>(C, _channels[c], c, _sigma_bp);
                      }
                    });
#else
#if defined(WITH_OPENMP)
#pragma omp parallel for
#endif
  for(size_t i = 0; i < NumChannels; ++i) {
    extractChannel<float>(C, _channels[i], i, _sigma_bp);
  }
#endif
}

cv::Mat_<float> BitPlanes::computeSaliencyMap() const
{
  assert( !_channels.front().empty() );

  cv::Mat_<float> ret(_channels[0].rows, _channels[0].cols);
  gradientAbsoluteMagnitude(_channels[0], ret);
  for(int i = 1; i < NumChannels; ++i)
    gradientAbsoluteMagnitudeAcc(_channels[i], ret.ptr<float>());

  return ret;

#if 0
  auto rows = _channels[0].rows, cols = _channels[0].cols;

  // TODO there is a lot of duplicate code here fix
  cv::Mat_<float> ret(rows, cols);
  {
    //
    // first channel, we initialize the map
    //
    auto dst_ptr = ret.ptr<float>();
    memset(dst_ptr, 0.0f, sizeof(float)*cols);
    dst_ptr += cols;
    for(int y = 1; y < rows - 1; ++y) {
      auto s0 = _channels[0].ptr<float>(y - 1),
           s1 = _channels[0].ptr<float>(y + 1),
           s = _channels[0].ptr<float>(y);

      dst_ptr[0] = 0.0f;
#pragma omp simd
      for(int x = 1; x < cols - 1; ++x) {
        dst_ptr[x] = std::fabs(static_cast<float>(s[x+1]) - static_cast<float>(s[x-1])) +
            std::fabs(static_cast<float>(s1[x]) - static_cast<float>(s0[x]));
      }

      dst_ptr[cols - 1] = 0.0f;
      dst_ptr += cols;
    }
    memset(dst_ptr, 0, sizeof(float)*cols);
  }

  {
    // for the rest of the channels, we +=
    for(int c = 1; c < NumChannels; ++c) {
      auto dst_ptr = ret.ptr<float>();
      dst_ptr += cols;
      for(int y = 1; y < rows - 1; ++y) {
        auto s0 = _channels[c].ptr<float>(y - 1),
             s1 = _channels[c].ptr<float>(y + 1),
             s = _channels[c].ptr<float>(y);

        dst_ptr[0] = 0.0f;
#pragma omp simd
        for(int x = 1; x < cols - 1; ++x) {
          dst_ptr[x] += std::fabs(s[x+1] - s[x-1]) + std::fabs(s1[x] - s0[x]);
        }

        dst_ptr[cols - 1] = 0.0f;
        dst_ptr += cols;
      }
    }
  }

  return ret;
#endif
}

}; // bpvo

