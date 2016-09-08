#pragma once
#include "Image.hpp"
#include "rgbCurve.hpp"


namespace cameraColorCalibration {
namespace common {

class RobertsonCalibrate {
public:
    
  /**
   * @brief
   * @param[in] maxIter
   * @param[in] threshold
   */
  RobertsonCalibrate(std::size_t maxIter = 500, double threshold = 0.01f) :
    _maxIteration(maxIter),
    _threshold(threshold)
  {}

  /**
   * @brief
   * @param[in] groups
   * @param[out] response
   * @param[in] times
   */
  void process(const std::vector< std::vector< Image<float> > > &ldrImageGroups, 
               const std::vector< std::vector<float> > &times,
               const rgbCurve &weight,
               rgbCurve &response);


  int getMaxIteration() const 
  { 
    return _maxIteration; 
  }

  float getThreshold() const 
  {
    return _threshold; 
  }
  
  void setMaxIteration(int value) 
  { 
    _maxIteration = value; 
  }
  
  void setThreshold(float value) 
  { 
    _threshold = value; 
  }

  const Image<float>& getRadiance(std::size_t group) const 
  { 
    assert(group < _radiance.size());
    
    return _radiance[group]; 
  }

private:
  std::vector< Image<float> > _radiance;
  double _threshold;
  std::size_t _maxIteration;
};

} // namespace common
} // namespace cameraColorCalibration