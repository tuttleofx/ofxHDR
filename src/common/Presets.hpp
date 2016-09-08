#pragma once
#include "rgbCurve.hpp"

namespace cameraColorCalibration {
namespace common {
    

//kParamResponsePreset options
enum EPresetResponse
{
  eResponsePresetFromFile = 0,
  eResponsePresetLinear,
  eResponsePresetGamma,
  eResponsePresetLog10,
  eResponsePresetCustom
};

static const std::vector< std::pair<std::string, std::string> > kPresetStringResponse = { 
  {"From File", ""},
  {"Linear", ""},
  {"Gamma", ""},
  {"Log10", ""},
  {"Custom" , ""}
};
  
//kParamWeightPreset options
enum EPresetWeight
{
  eWeightPresetFromFile = 0,
  eWeightPresetLinear,
  eWeightPresetGaussian,
  eWeightPresetGaussianCustom,
  eWeightPresetTriangular,
  eWeightPresetPlateau,
  eWeightPresetFlat,
  eWeightPresetCustom
};

static const std::vector< std::pair<std::string, std::string> > kPresetStringWeight = { 
  {"From File", ""},
  {"Linear", ""},
  {"Gaussian", ""},
  {"Gaussian Custom", ""},
  {"Triangular", ""},
  {"Plateau", ""},
  {"Flat" , ""},
  {"Custom" , ""}
};

/**
 * @brief Initialize response function with a preset
 * @param response
 * @param preset
 */
void initResponseFromPreset(rgbCurve& response, EPresetResponse preset);

/**
 * @brief Initialize weight function with a preset
 * @param weight
 * @param preset
 */
void initWeightFromPreset(rgbCurve& weight, double gaussianSize, EPresetWeight preset); 

  
} // namespace common 
} // namespace cameraColorCalibration