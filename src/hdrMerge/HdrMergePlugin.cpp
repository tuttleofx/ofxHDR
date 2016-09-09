#include "HdrMergePlugin.hpp"
#include "../common/RobertsonMerge.hpp"
#include "../common/Presets.hpp"
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <regex>


namespace cameraColorCalibration {
namespace hdrMerge {

HdrMergePlugin::HdrMergePlugin(OfxImageEffectHandle handle, std::size_t nbClips) : 
    cameraColorCalibration::hdrBase::HdrBasePlugin(handle, nbClips)
{}

void HdrMergePlugin::getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames)
{
  frames.setFramesNeeded(*getInputClip(), getInputClip()->getFrameRange());
}

void HdrMergePlugin::render(const OFX::RenderArguments &args)
{
  std::cout << "render : [info] time: " << args.time << std::endl;
  std::cout << "render : [info] fieldToRender: " << args.fieldToRender << std::endl;
  std::cout << "render : [info] renderQualityDraft: " << args.renderQualityDraft << std::endl;
  std::cout << "render : [info] renderScale: " << args.renderScale.x << ", " << args.renderScale.y << std::endl;
  std::cout << "render : [info] interactiveRenderStatus: " << args.interactiveRenderStatus << std::endl;
  std::cout << "render : [info] args.renderWindow: (" << args.renderWindow.x1 << ", " << args.renderWindow.y1 << "), (" << args.renderWindow.x2 << ", "  << args.renderWindow.y2 << ")" << std::endl;
  
  if(abort())
  {
    return;
  }

  try
  {
    std::cout << "render : load source ..." << std::endl;
    if(!loadSources())
    {
      std::cerr << "render : [error] impossible to load sources" << std::endl;
      return;
    }

    if(abort())
    {
      return;
    }
    
    std::cout << "render : [output] fetch"  << std::endl;
    OFX::Image **outputPtr;
    if(!loadOutput(outputPtr, args.time))
    {
      std::cout << "render : [output clip] is NULL" << std::endl;
      return;
    }
    
    cameraColorCalibration::common::Image<float> output(*outputPtr);

    //Debug Render
    if(renderDebug(output))
    {
      return;
    }
    
    std::cout << "render : initialize output HDR ..." << std::endl;
    cameraColorCalibration::common::Image<float> hdrImage(getSource().front().getWidth(), getSource().front().getHeight(), 3);

    {
      std::cout << "render : [merge]" << std::endl;
     
      cameraColorCalibration::common::RobertsonMerge merge;
      cameraColorCalibration::common::rgbCurve weight(K_QUANTIZATION);
      cameraColorCalibration::common::rgbCurve response(K_QUANTIZATION);

      getWeightFunction(weight);
      getResponseFunction(response);
      
      std::cout << "render : [merge] targetExposure: " << getTargetExposure() << std::endl;

      merge.process(getSource(), getExposure(), weight, response, hdrImage, getTargetExposure());
      std::cout << "render : [merge] -- OK" << std::endl;
    }



    std::cout << "render : HDR" << std::endl;
    output.copyFrom(hdrImage);
  }
  catch(std::exception &e)
  {
    this->sendMessage(OFX::Message::eMessageError, "hdrmerge.render", e.what());
  }
}
/*
void HdrMergePlugin::changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName)
{
  if(args.reason != OFX::InstanceChangeReason::eChangeTime)
  {
    //A clip changed
//    _uptodate = false;
  
    updateSourceParameters();
  }
}


void HdrMergePlugin::updateSourceParameters()
{
  if(_srcClip->isConnected())
  {
    OfxRangeD range =  _srcClip->getFrameRange();
    int nbImages = range.max - range.min + 1;
    for(int image = 0; image < nbImages; ++image)
    {
      setImageOptionsSecret(image, false);
      //Sync Label to frame range
      _sourceImages[image]->setLabel("Image " + std::to_string(int(range.min + image)));
    }
  }
  else
  {
    for(std::size_t image = 0; image < K_MAX_SOURCE_IMAGES; ++image)
    {
      _iso[image]->setValue(100);
      _aperture[image]->setValue(1.4f);
      _shutter[image]->setValue(0.5f);
      _ev[image]->setValue(0);
    }
    
    for(std::size_t image = 0; image < K_MAX_SOURCE_IMAGES; ++image)
    {
      setImageOptionsSecret(image, true);
    }
  }
}
 */

} // namespace hdrMerge 
} // namespace cameraColorCalibration