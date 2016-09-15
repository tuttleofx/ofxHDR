#include "HdrCalibPlugin.hpp"
#include "../common/RobertsonMerge.hpp"
#include "../common/RobertsonCalibrate.hpp"
#include "../hdrMerge/HdrMergePlugin.hpp"
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <regex>


namespace cameraColorCalibration {
namespace hdrCalibration {
  
HdrCalibPlugin::HdrCalibPlugin(OfxImageEffectHandle handle, std::size_t nbClips) :
  cameraColorCalibration::hdrBase::HdrBasePlugin(handle, nbClips)
{
  updateOutputIndexRange();
  _hdrCalculateResponse->setEnabled(hasInputGroup());
}

void HdrCalibPlugin::getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames)
{
  //(!) TODO get only needed frames when user don't want calculate response (a merge)
  for(std::size_t group = 0; group < getNbConnectedInput(); ++group)
  {
    frames.setFramesNeeded(*getInputClip(getConnectedGroupIndex(group)), getInputClip(getConnectedGroupIndex(group))->getFrameRange());
  }
}

void HdrCalibPlugin::render(const OFX::RenderArguments &args)
{
  std::cout << "render : [info] time: " << args.time << std::endl;
  std::cout << "render : [info] fieldToRender: " << args.fieldToRender << std::endl;
  std::cout << "render : [info] renderQualityDraft: " << args.renderQualityDraft << std::endl;
  std::cout << "render : [info] renderScale: " << args.renderScale.x << ", " << args.renderScale.y << std::endl;
  std::cout << "render : [info] interactiveRenderStatus: " << args.interactiveRenderStatus << std::endl;
  std::cout << "render : [info] args.renderWindow: (" << args.renderWindow.x1 << ", " << args.renderWindow.y1 << "), (" << args.renderWindow.x2 << ", "  << args.renderWindow.y2 << ")" << std::endl;
  std::cout << "render : [info] output clip index : " << _hdrOutputIndex->getValue() - 1 << std::endl;
  
  if(abort())
  {
    return;
  }
  
  //Get output index
  int outputClipIndex = _hdrOutputIndex->getValue() - 1;
  
  if(!isGroupConnected(outputClipIndex))
  {
    std::cerr << "render : [error] invalid output index" << std::endl;
    return;
  }
  
  std::size_t groupIndex = getConnectedGroupIndex(outputClipIndex);
  std::cout << "Group Index : " << groupIndex << std::endl;
  
  std::cout << "render : [load] sources"  << std::endl;
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
  OFX::Image *outputPtr;
  if(!loadOutput(outputPtr, args.time))
  {
    std::cout << "render : [output clip] is NULL" << std::endl;
    return;
  }
  cameraColorCalibration::common::Image<float> output(outputPtr);
  
  //Debug Render
  if(renderDebug(output, groupIndex))
  {
    return;
  }
  
  //Process Data
  cameraColorCalibration::common::rgbCurve weight(K_QUANTIZATION);
  cameraColorCalibration::common::rgbCurve response(K_QUANTIZATION);
  
  getWeightFunction(weight);
  response.setLinear();

  //User want calculate response
  if(_wantCalculateResponse)
  {
    _wantCalculateResponse = false;

    std::cout << "render : [calibration]" << std::endl;
    cameraColorCalibration::common::RobertsonCalibrate calibration;
    
    calibration.process(getAllSources(), getAllExposure(), weight, response);
    std::cout << "render : [calibration] -- OK" << std::endl;

    std::cout << "render : [Display Response]" << std::endl;
    setResponseFunctionKeyFrames(response);
    std::cout << "render : [Display Response] -- OK" << std::endl;
    
    output.copyFrom(calibration.getRadiance(groupIndex));
    return;
  }

  if(hasResponseKeyFrames())
  {
    getResponseFunctionFromKeyFrames(response);
  }

  std::cout << "render : [merge]" << std::endl;
  cameraColorCalibration::common::RobertsonMerge merge;
  cameraColorCalibration::common::Image<float> hdrImage(getSource(groupIndex).front().getWidth(), getSource(groupIndex).front().getHeight(), 3);

  std::cout << "render : [merge] targetExposure: " << getTargetExposure() << std::endl;
  merge.process(getSource(groupIndex), 
                getExposure(groupIndex), 
                weight,
                response,
                hdrImage, 
                getTargetExposure());
  output.copyFrom(hdrImage);
  
}


void HdrCalibPlugin::changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName)
{
  if(args.reason != OFX::InstanceChangeReason::eChangeTime)
  {
    updateOutputIndexRange();
    _hdrCalculateResponse->setEnabled(hasInputGroup());
  }
  
  cameraColorCalibration::hdrBase::HdrBasePlugin::changedClip(args, clipName);
}

void HdrCalibPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
  //Change output index
  if((paramName == kParamCalibrationOutputIndex) && (args.reason == OFX::eChangeUserEdit))
  {
    if(!isGroupConnected(_hdrOutputIndex->getValue() - 1))
    {
      int min;
      int max;
      _hdrOutputIndex->getRange(min, max);
      _hdrOutputIndex->setValue(min);
    }
    return;
  }
  
  //Calculate response
  if(paramName == kParamCalibrationCalculateResponse)
  {
    _wantCalculateResponse = true;
    invalidRender();
    return;
  }
  
  cameraColorCalibration::hdrBase::HdrBasePlugin::changedParam(args, paramName);
}

void HdrCalibPlugin::updateOutputIndexRange()
{
  std::size_t min = 0;
  std::size_t max = 0;
  
  if(!hasInputGroup())
  {
    //No connected clip
    _hdrOutputIndex->setEnabled(false);
    _hdrOutputIndex->setValue(0);
  }
  else
  {
    _hdrOutputIndex->setEnabled(true);
    min = getFirstConnectedGroupIndex() + 1;
    max = getLastConnectedGroupIndex() + 1;
    
    std::size_t value = _hdrOutputIndex->getValue();
    if((value < min) || (value > max) || (!isGroupConnected(value - 1)))
    {
      _hdrOutputIndex->setValue(min);
    }
  }
  _hdrOutputIndex->setRange(min, max);
  _hdrOutputIndex->setDisplayRange(min, max);
}

} // namespace hdrCalibration
} // namespace cameraColorCalibration