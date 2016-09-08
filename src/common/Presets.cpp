#include "Presets.hpp"

namespace cameraColorCalibration {
namespace common {


void initResponseFromPreset(rgbCurve& response, EPresetResponse preset)
{
  switch(preset)
  {
    case EPresetResponse::eResponsePresetFromFile : break;
    case EPresetResponse::eResponsePresetLinear : response.setLinear(); break;
    case EPresetResponse::eResponsePresetGamma : response.setGamma(); break;
    case EPresetResponse::eResponsePresetLog10 : response.setLog10(); break;
    case EPresetResponse::eResponsePresetCustom : break;
    default : response.setLinear();
  }
}

void initWeightFromPreset(rgbCurve& weight, double gaussianSize, EPresetWeight preset)
{
  switch(preset)
  {
    case EPresetWeight::eWeightPresetFromFile : break;
    case EPresetWeight::eWeightPresetLinear : weight.setLinear(); break;
    case EPresetWeight::eWeightPresetGaussian : weight.setGaussian(); break;
    case EPresetWeight::eWeightPresetGaussianCustom : weight.setGaussian(gaussianSize); break;
    case EPresetWeight::eWeightPresetTriangular : weight.setTriangular(); break;
    case EPresetWeight::eWeightPresetPlateau : weight.setPlateau(); break;
    case EPresetWeight::eWeightPresetFlat : weight.setOne(); break;
    case EPresetWeight::eWeightPresetCustom : break;
    default : weight.setGaussian();
  }
}


} // namespace common 
} // namespace cameraColorCalibration