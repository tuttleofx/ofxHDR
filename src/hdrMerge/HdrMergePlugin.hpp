#pragma once
#include "ofxsImageEffect.h"
#include "HdrMergePluginFactory.hpp"
#include "HdrMergePluginDefinition.hpp"
#include "../hdrBase/HdrBasePlugin.hpp"


namespace cameraColorCalibration {
namespace hdrMerge {

/**
 * @brief HdrCalibPlugin Class
 */
class HdrMergePlugin : public cameraColorCalibration::hdrBase::HdrBasePlugin 
{
public:
  
  /**
   * @brief Plugin Constructor
   * @param handle
   */
  HdrMergePlugin(OfxImageEffectHandle handle, std::size_t nbClips);

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
};

} // namespace hdrMerge 
} // namespace cameraColorCalibration