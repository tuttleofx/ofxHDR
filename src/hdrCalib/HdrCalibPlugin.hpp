#pragma once
#include "ofxsImageEffect.h"
#include "HdrCalibPluginFactory.hpp"
#include "HdrCalibPluginDefinition.hpp"
#include "../hdrBase/HdrBasePlugin.hpp"


namespace cameraColorCalibration {
namespace hdrCalibration {

/**
 * @brief HdrCalibPlugin Class
 */
class HdrCalibPlugin : public cameraColorCalibration::hdrBase::HdrBasePlugin 
{
private:
  //(!) Don't delete these, OFX::ImageEffect is managing them

  //Calibration Parameters
  OFX::IntParam *_hdrOutputIndex = fetchIntParam(kParamCalibrationOutputIndex); 
  OFX::PushButtonParam *_hdrCalculateResponse = fetchPushButtonParam(kParamCalibrationCalculateResponse);
  
  //Algorithm Parameters
  OFX::IntParam *_algorithmMaxIteration = fetchIntParam(kParamAlgorithmIterations);
  OFX::DoubleParam *_algorithmThreshold = fetchDoubleParam(kParamAlgorithmThreshold);
  
  //User want to calculate the response
  bool _wantCalculateResponse = false;
  
public:
  
  /**
   * @brief Plugin Constructor
   * @param handle
   */
  HdrCalibPlugin(OfxImageEffectHandle handle, std::size_t nbClips);
  
  /** 
   * @brief the get frames needed action
   * If the effect wants change the frames needed on an input clip from the default values (which is the same as the frame to be renderred)
   * it should do so by calling the OFX::FramesNeededSetter::setFramesNeeded function on the \em frames argument.
  */
  virtual void getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames);
  
  /**
   * @brief Override render method
   * @param[in] args
   */
  virtual void render(const OFX::RenderArguments &args);
  
  /**
   * @brief Override changedClip method
   * @param[in] args
   * @param[in] clipName
   */
  virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName);
  
  /**
   * @brief Override changedParam method
   * @param[in] args
   * @param[in] paramName
   */
  virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName);
  
  /**
   * @brief Update Output Index Range 
   */
  void updateOutputIndexRange();
};

} // namespace hdrCalibration
} // namespace cameraColorCalibration