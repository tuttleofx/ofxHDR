#include "RobertsonCalibrate.hpp"
#include "RobertsonMerge.hpp"
#include <iostream>
#include <fstream>
#include <cassert>


namespace cameraColorCalibration {
namespace common {

void RobertsonCalibrate::process(const std::vector< std::vector< Image<float> > > &ldrImageGroups, 
                                 const std::vector< std::vector<float> > &times,
                                 const rgbCurve &weight,
                                 rgbCurve &response)
{
  //checks
  for (int g = 0; g < ldrImageGroups.size(); ++g)
  {
    assert(ldrImageGroups[g].size() == times[g].size());
    Image<float>::checkSameDimensions(ldrImageGroups[g]);
  }

  //set channels count always RGB
  static const std::size_t channels = 3;

  //get channels quantization
  std::size_t channelQuantization = ldrImageGroups[0][0].getChannelQuantization();

  //create radiance vector of image
  _radiance = std::vector< Image<float> >(ldrImageGroups.size());
  for(auto& radianceImg: _radiance)
  {
    radianceImg.createInternalBuffer(ldrImageGroups[0][0].getWidth(), ldrImageGroups[0][0].getHeight(), channels);
  }

  //initialize response
  response = rgbCurve(channelQuantization);
  response.setLinear();
  response.normalize();

  //initialize cardinality
  rgbCurve card(channelQuantization);
  card.setZero();

  //compute cardinal curve
  for(unsigned int g = 0; g < ldrImageGroups.size(); ++g)
  {
    const std::vector< Image<float> > &ldrImagesGroup = ldrImageGroups[g];

    for(unsigned int i = 0; i < ldrImagesGroup.size(); ++i) 
    {
      const Image<float> &image = ldrImagesGroup[i];

      //for each pixel
      for(std::size_t y = 0; y < image.getHeight(); ++y)
      {
        for(std::size_t x = 0; x < image.getWidth(); ++x)
        {
          const float *ptr = image.getPixel(x, y);
          
          for(std::size_t channel = 0; channel < channels; ++channel) 
          {
            //number of pixel with the same value 
            card(*ptr, channel) += 1; 
            
            ++ptr;
          }
        }
      }
    }
  }
  card.write("/datas/deli/log/card.csv");
  card.interpolateMissingValues();
  card.write("/datas/deli/log/card_inter.csv");
  
  //inverse cardinal curve value (for optimized division in the loop)
  card.inverseAllValues();
  card.write("/datas/deli/log/card_inv.csv");
  //create merge operator
  RobertsonMerge merge;

  for(std::size_t iter = 0; iter < _maxIteration; ++iter) 
  {
    std::cout << "--> iteration : "<< iter << std::endl;

    std::cout << "1) compute radiance "<< std::endl;
    //initialize radiance
    for(std::size_t g = 0; g < ldrImageGroups.size(); ++g)
    {
      merge.process(ldrImageGroups[g], times[g], weight, response, _radiance[g], 1.0f);
    }

    std::cout << "2) initialization new response "<< std::endl;
    //initialize new response
    rgbCurve newResponse = rgbCurve(channelQuantization);
    newResponse.setZero();

    std::cout << "3) compute new response "<< std::endl;
    //compute new response
    for(unsigned int g = 0; g < ldrImageGroups.size(); ++g)
    {
      const std::vector< Image<float> > &ldrImagesGroup = ldrImageGroups[g];
      const Image<float> &radiance = _radiance[g];

      for(unsigned int i = 0; i < ldrImagesGroup.size(); ++i) 
      {
        for(std::size_t y = 0; y < ldrImagesGroup[i].getHeight(); ++y)
        {
          for(std::size_t x = 0; x < ldrImagesGroup[i].getWidth(); ++x)
          {
            //for each pixels
            const float *ptr = ldrImagesGroup[i].getPixel(x, y);
            float *ptrRadiance = radiance.getPixel(x, y);

            for(std::size_t channel = 0; channel < channels; ++channel)
            {
                newResponse(*ptr, channel) += times[g][i] * (*ptrRadiance);

                ++ptr;
                ++ptrRadiance;
            }
          }
        }
      }
    }
    newResponse.write("/datas/deli/log/iter_"+std::to_string(iter)+"_unorm__result_dresponse.csv");
    newResponse.interpolateMissingValues();
    newResponse.write("/datas/deli/log/iter_"+std::to_string(iter)+"_unorm__result_inter_dresponse.csv");
    //dividing the response by the cardinal curve
    newResponse *= card;
    
    std::cout << "4) normalize response"<< std::endl;
    //normalization
    newResponse.write("/datas/deli/log/iter_"+std::to_string(iter)+"_unorm_response.csv");
    newResponse.normalize();
    newResponse.write("/datas/deli/log/iter_"+std::to_string(iter)+"_response.csv");
    
    std::cout << "5) compute difference"<< std::endl;    
    //calculate difference between the old response and the new one
    rgbCurve responseDiff = newResponse - response;
    responseDiff.setAllAbsolute();

    double diff = rgbCurve::sumAll(responseDiff) / channels;

    //update the response
    response = newResponse;
    
    std::cout << "6) check end condition"<< std::endl; 
    //check end condition
    if(diff < _threshold) 
    {
      std::cout << "[BREAK] difference < threshold " << std::endl;
      break;
    }
    std::cout << "-> difference is " << diff << std::endl;
  }
}

} // namespace common
} // namespace cameraColorCalibration