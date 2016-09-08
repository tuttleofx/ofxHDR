#include "RobertsonMerge.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>


namespace cameraColorCalibration {
namespace common {
  
void RobertsonMerge::process(const std::vector< Image<float> > &images, 
                              const std::vector<float> &times,
                              const rgbCurve &weight,
                              const rgbCurve &response,
                              Image<float> &radiance, 
                              float targetTime)
{
  //checks
  assert(!response.isEmpty());
  assert(!radiance.isEmpty());
  assert(!images.empty());
  assert(images.size() == times.size());
  Image<float>::checkSameDimensions(images);
  
  //reset radiance image
  radiance.setZero();

  //get images width, height
  const std::size_t width = images.front().getWidth();
  const std::size_t height = images.front().getHeight();
  
  //min and max trusted values
  const float minTrustedValue = 0.0f - std::numeric_limits<float>::epsilon();
  const float maxTrustedValue = 1.0f + std::numeric_limits<float>::epsilon();
  
  for(std::size_t y = 0; y < height; ++y)
  {
    for(std::size_t x = 0; x < width; ++x)
    {
      //for each pixels
      float *ptrRadiance = radiance.getPixel(x, y);
      
      for(std::size_t channel = 0; channel < radiance.getNbChannels(); ++channel)
      {
        double wsum = 0.0f;
        double wdiv = 0.0f;
//        float minTimeSaturation = std::numeric_limits<float>::max();
//        float maxTimeSaturation = std::numeric_limits<float>::min();

        for(std::size_t i = 0; i < images.size(); ++i) 
        {
          //for each images
          const double value = *images[i].getPixel(x, y, channel);
          const double time = times[i];
          const double vt = value / time;
          const double w = weight(value, channel) + 0.001;
          const float r = response(value, channel);

//          wsum += w * time * r;
//          wdiv += w * time * time;
          wsum += w * r / time;
          wdiv += w;
//          wsum += w * vt;
//          wdiv += w;

//          //saturation detection
//          if(value > maxTrustedValue) 
//          {
//            minTimeSaturation = std::min(minTimeSaturation, time);
//          }
//          
//          if(value < minTrustedValue) 
//          {
//            maxTimeSaturation = std::max(maxTimeSaturation, time);
//          }
        }

//        //saturation correction
//        if((wdiv == 0.0f) && 
//               (maxTimeSaturation > std::numeric_limits<float>::min())) 
//        {
//          wsum = minTrustedValue;
//          wdiv = maxTimeSaturation;
//        }
//        
//        if((wdiv == 0.0f) && 
//               (minTimeSaturation < std::numeric_limits<float>::max())) 
//        {
//          wsum = maxTrustedValue;
//          wdiv = minTimeSaturation;
//        }

        if(wdiv > 0.0001f) 
        {
          *ptrRadiance = (wsum / wdiv) * targetTime;
        } 
        else
        {
          *ptrRadiance = 0.0f;
        }
        
        ++ptrRadiance; //next channel
      } 
    }
  }
}


} // namespace common
} // namespace cameraColorCalibration
