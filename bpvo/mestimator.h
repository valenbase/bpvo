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

#ifndef BPVO_MESTIMATOR_H
#define BPVO_MESTIMATOR_H

#include <bpvo/types.h>
#include <vector>

namespace bpvo {

class MEstimator
{
 public:
  /**
  */
  static void ComputeWeights(LossFunctionType, const std::vector<float>& residuals,
                             float sigma, std::vector<float>& weights);

  /**
   * computes the weights for valid points only, invalid points are assigned
   * zero weight
   */
  static void ComputeWeights(LossFunctionType, const std::vector<float>& residuals,
                             const std::vector<uint8_t>& valid, float sigma,
                             std::vector<float>& weights);
}; // MEstimator

/**
 * Estimates the scale of the data using robust standard deviation. We also keep
 * the change in the estimated scale across iterations so that time is not
 * wasted if the scale is stable
 */
class AutoScaleEstimator
{
 public:
  AutoScaleEstimator(float tol = 1e-6);

  void reset();
  float getScale() const;

  /**
  */
  float estimateScale(const std::vector<float>& residuals,
                      const std::vector<uint8_t>& valid);

 private:
  float _scale = 1.0, _delta_scale = 1e10, _tol = 1e-4;
  std::vector<float> _buffer;
}; // AutoScaleEstimator

}; // bpvo

#endif // BPVO_MESTIMATOR_H
