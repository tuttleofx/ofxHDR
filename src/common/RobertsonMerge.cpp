#include "RobertsonMerge.hpp"
#include <cassert>
#include <cmath>
#include <iostream>


namespace cameraColorCalibration {
namespace common {
  
RobertsonMerge::RobertsonMerge(std::size_t channelQuantization) :
  _weight(rgbCurve(channelQuantization))
{
  _weight.setGaussian();
}

void RobertsonMerge::process(const std::vector< Image<float> > &images, 
  Image<float> &radiance, 
  const std::vector<float> &times, 
  const rgbCurve &response)
{
  //checks
  assert(!response.isEmpty());
  assert(!radiance.isEmpty());
  assert(!images.empty());
  assert(images.size() == times.size());
  Image<float>::checkSameDimensions(images);

  //get images width, height and number of channels
  std::size_t width = images.front().getWidth();
  std::size_t height = images.front().getHeight();
  std::size_t channels = radiance.getNbChannels();

  //create the new radiance 
  radiance.setZero();

  Image<float> wsum(width, height, channels);
  wsum.setZero();

  for(std::size_t i = 0; i < images.size(); ++i) 
  {
    float sqrTime = times[i] * times[i];

    for(std::size_t y = 0; y < height; ++y)
    {
      for(std::size_t x = 0; x < width; ++x)
      {
        //for each pixels
        const float *ptrImage = images[i].getPixel(x, y);
        float *ptrRadiance = radiance.getPixel(x, y);
        float *ptrWsum = wsum.getPixel(x, y);

        for(std::size_t channel = 0; channel < channels; ++channel)
        {
          float w = _weight(*ptrImage, channel);
          float r = response(*ptrImage, channel);

          *ptrRadiance += w * r * times[i];
          *ptrWsum += w * sqrTime;

          ++ptrImage;
          ++ptrRadiance;
          ++ptrWsum;
        }
      }
    }
  }
  
  radiance.divide(wsum);
}

} // namespace common
} // namespace cameraColorCalibration