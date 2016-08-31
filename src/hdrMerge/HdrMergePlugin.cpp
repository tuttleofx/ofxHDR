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

HdrMergePlugin::HdrMergePlugin(OfxImageEffectHandle handle) : 
    OFX::ImageEffect(handle),
    _response(1 << 12),
    _merge(1 << 12)
{   
  for(std::size_t image = 0; image < K_MAX_SOURCE_IMAGES; ++image)
  {
    _sourceImages[image] = fetchGroupParam(kParamGroupImage(image));
    _iso[image] = fetchIntParam(kParamImageIso(image));
    _aperture[image] = fetchDoubleParam(kParamImageAperture(image));
    _shutter[image] = fetchDoubleParam(kParamImageShutter(image));
    _ev[image] = fetchDoubleParam(kParamImageEv(image));
  }
  //reset all plugins options
  reset();
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

  //Merge source parameters isn't uptodate
  if(!_uptodate)
  {
    std::cout << "render : load source ..." << std::endl;
    if(!loadGroups() || abort())
    {
      return;
    }
    
    std::cout << "render : initialize output HDR ..." << std::endl;
    _hdrImage.createInternalBuffer(_ldrImages.front().getWidth(), _ldrImages.front().getHeight(), 3);
    
    std::cout << "render : [merge]" << std::endl;
    _merge.process(_ldrImages, _hdrImage, _luminances, _response);
    std::cout << "render : [merge] -- OK" << std::endl;
    _uptodate = true;
  }
  
  //Write Output
  std::cout << "render : [output clip] fetch"  << std::endl;
  OFX::Image *outputPtr = _dstClip->fetchImage(args.time);
  if(outputPtr == NULL)
  {
    std::cout << "render : [output clip] is NULL" << std::endl;
    return;
  }
  
  cameraColorCalibration::common::Image<float> output(outputPtr);
  std::cout << "render : HDR" << std::endl;
  output.copyFrom(_hdrImage);
}

bool HdrMergePlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  return false;
}

void HdrMergePlugin::changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName)
{
  if(args.reason != OFX::InstanceChangeReason::eChangeTime)
  {
    //A clip changed
    _uptodate = false;
  
    updateSourceParameters();
  }
}

void HdrMergePlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
  //check if the parameter is a MetaData
  if(changedMetaData(paramName))
  {
    //a meta data has changed, ev have been update by the function
    _uptodate = false;
    return;
  }

  if(paramName == kParamResponsePreset)
  {
    updateResponsePreset();
    _uptodate = false;
    return;
  }
  
  if(paramName == kParamWeightPreset)
  {
    updateWeightPreset();
    _uptodate = false;
    return;
  }
  
  if((paramName == kParamResponseLoad) || (paramName == kParamWeightLoad))
  {
    try 
    {
      if(paramName == kParamResponseLoad)
      {
        cameraColorCalibration::common::rgbCurve fileCurve(_responseFilePath->getValue());
        _response = fileCurve;
      }
      else
      {
        cameraColorCalibration::common::rgbCurve fileCurve(_weightFilePath->getValue());
        _merge.setWeightFunction(fileCurve);
      }
      this->sendMessage(OFX::Message::eMessageMessage, "hdrmerge.curve.load", "Function loaded from file.");
      _uptodate = false;
    }
    catch(std::exception &e)
    {
      this->sendMessage(OFX::Message::eMessageError, "hdrmerge.curve.load", e.what());
    }
  }
  
  if((paramName == kParamResponseExport) || (paramName == kParamWeightExport))
  {
    try 
    {
      if(paramName == kParamResponseExport)
      {
        _response.write(_responseFilePath->getValue(), "Response Function");
      }
      else
      {
        _merge.getWeightFunction().write(_weightFilePath->getValue(), "Weight Function");
      }
      this->sendMessage(OFX::Message::eMessageMessage, "hdrmerge.curve.export", "Function saved in file.");
    }
    catch(std::exception &e)
    {
      this->sendMessage(OFX::Message::eMessageError, "hdrmerge.curve.export", e.what());
    }
  }
  
  if(paramName == kParamResponseRefreshKeys)
  {
    refreshResponseFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamWeightRefreshKeys)
  {
    refreshWeightFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamResponseClearKeys)
  {
    clearResponseFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamWeightClearKeys)
  {
    clearWeightFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamResponseFromKeys)
  {
    updateResponseFunctionFromKeyFrames();
    _uptodate = false;
    return;
  }
  
  if(paramName == kParamWeightFromKeys)
  {
    updateWeightFunctionFromKeyFrames();
    _uptodate = false;
    return;
  }
}

bool HdrMergePlugin::changedMetaData(const std::string &paramName)
{
  std::regex reg("imageParam(.*)_([0-9]*)"); //MetaData pattern
  std::smatch sm;
  std::regex_match(paramName, sm, reg); //4 matches (pattern / parameter / group / image)
    
  if(sm.size() != 3)
  {
    //the changed param is not a source metaData
    if(paramName.substr(0,16) == "targetImageParam")
    {
       //the changed param is a target metaData
      float value = cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                               _targetIso->getValue(),
                                                                              _targetAperture->getValue());
      _targetEv->setValue(value);
      return true;
    }
    //the changed param is not a metaData
    return false;
  }
  
  //const std::string &parameter = sm[1];  //Iso, Aperture or Shutter
  std::size_t image = std::stoi(sm[2]); //HDR image number
  
  float value = cameraColorCalibration::common::RobertsonMerge::getExposure(_shutter[image]->getValue(),
                                                                            _iso[image]->getValue(),
                                                                            _aperture[image]->getValue());
  _ev[image]->setValue(value);
  
  //the changed param is a metaData
  return true; 
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

bool HdrMergePlugin::loadGroups()
{ 
  //clear process data
  _ldrImages.clear();
  _luminances.clear();
  
  //Check if the node is connected
  if(!_srcClip->isConnected())
  {
    progressEnd();
    return false;
  }

  std::size_t start = (std::size_t)_srcClip->getFrameRange().min;
  std::size_t last = (std::size_t)_srcClip->getFrameRange().max;
  
  float targetExposure = cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                                    _targetIso->getValue(),
                                                                                    _targetAperture->getValue());

  for(std::size_t image = start; image <= last; ++image)
  {
    std::cout << "[load]  Image :  " << image << std::endl;

    float exposure = cameraColorCalibration::common::RobertsonMerge::getExposure(_shutter[image - start]->getValue(),
                                                                                  _iso[image - start]->getValue(),
                                                                                  _aperture[image - start]->getValue());

    std::cout << "[load]   compute EV ("<< exposure << ") ..." << std::endl;
    float avgLuminance = std::pow(2.f, exposure - targetExposure);
    std::cout << "[load]   add Relative EV ("<< avgLuminance << ") ..." << std::endl;
    _luminances.push_back( avgLuminance );

    std::cout << "[load]   add Image ... " << std::endl;
    OFX::Image *imagePtr = _srcClip->fetchImage(image);
    if(imagePtr == NULL)
    {
      std::cerr << "[load] error : can't load image " << std::endl;
      return false;
    }
    _ldrImages.emplace_back(cameraColorCalibration::common::Image<float>(imagePtr));
  }
  return true;
}

void HdrMergePlugin::refreshResponseFunctionKeyFrames()
{
  clearResponseFunctionKeyFrames();
  progressStart("Create response function keyframes", "hdrmerge.response.keyframes");
  const double updateCoefficient = 1 / (double)_response.getSize();
  for(std::size_t index = 0; index < _response.getSize(); ++index)
  {
    _responseRed->setValueAtTime(index, _response.getCurveRed()[index]);
    _responseGreen->setValueAtTime(index, _response.getCurveGreen()[index]);
    _responseBlue->setValueAtTime(index, _response.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
}

void HdrMergePlugin::refreshWeightFunctionKeyFrames()
{
  clearWeightFunctionKeyFrames();
  progressStart("Create weight function keyframes", "hdrmerge.weight.keyframes");
  const cameraColorCalibration::common::rgbCurve &weight = _merge.getWeightFunction();
  const double updateCoefficient = 1 / (double)weight.getSize();
  for(std::size_t index = 0; index < weight.getSize(); ++index)
  {
    _weightRed->setValueAtTime(index, weight.getCurveRed()[index]);
    _weightGreen->setValueAtTime(index, weight.getCurveGreen()[index]);
    _weightBlue->setValueAtTime(index, weight.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
}

void HdrMergePlugin::updateResponsePreset()
{
  cameraColorCalibration::common::EPresetResponse preset;
  preset = static_cast<cameraColorCalibration::common::EPresetResponse>(_responsePreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eResponsePresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eResponsePresetFromFile);
  
  _responseLoad->setIsSecret(!fromFile);
  _responseExport->setIsSecret(!custom);
  _responseFilePath->setIsSecret(!custom && !fromFile);
  _responseFromKeys->setIsSecret(!custom);
  _responseRed->setEnabled(custom);
  _responseGreen->setEnabled(custom);
  _responseBlue->setEnabled(custom);
  
  cameraColorCalibration::common::initResponseFromPreset(_response, preset);
}

void HdrMergePlugin::updateWeightPreset()
{
  cameraColorCalibration::common::EPresetWeight preset;
  preset = static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eWeightPresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eWeightPresetFromFile);
  
  _weightLoad->setIsSecret(!fromFile);
  _weightExport->setIsSecret(!custom);
  _weightFilePath->setIsSecret(!custom && !fromFile);
  _weightFromKeys->setIsSecret(!custom);
  _weightRed->setEnabled(custom);
  _weightGreen->setEnabled(custom);
  _weightBlue->setEnabled(custom);
  
  if(!fromFile && !custom)
  {
    cameraColorCalibration::common::rgbCurve weight(_merge.getWeightFunction().getSize());
    cameraColorCalibration::common::initWeightFromPreset(weight, preset);
    _merge.setWeightFunction(weight);
  }
}

void HdrMergePlugin::updateResponseFunctionFromKeyFrames(bool sendMessage)
{
  for(std::size_t index = 0; index < _response.getSize(); ++index)
  {
    _response.getCurveRed()[index] = _responseRed->getValueAtTime(index);
    _response.getCurveGreen()[index] = _responseGreen->getValueAtTime(index);
    _response.getCurveBlue()[index] = _responseBlue->getValueAtTime(index);
  }
  if(sendMessage)
  {
    this->sendMessage(OFX::Message::eMessageMessage, "hdrmerge.response.update", "Response Function updated from keyFrames.");
  }
}

void HdrMergePlugin::updateWeightFunctionFromKeyFrames(bool sendMessage)
{
  cameraColorCalibration::common::rgbCurve weightFunction(_merge.getWeightFunction().getSize());
  for(std::size_t index = 0; index < weightFunction.getSize(); ++index)
  {
    weightFunction.getCurveRed()[index] = _weightRed->getValueAtTime(index);
    weightFunction.getCurveGreen()[index] = _weightGreen->getValueAtTime(index);
    weightFunction.getCurveBlue()[index] = _weightBlue->getValueAtTime(index);
  }
  _merge.setWeightFunction(weightFunction);
  if(sendMessage)
  {
    this->sendMessage(OFX::Message::eMessageMessage, "hdrmerge.weight.update", "Weight Function updated from keyFrames.");
  }
}

void HdrMergePlugin::clearResponseFunctionKeyFrames()
{
  _responseRed->deleteAllKeys();
  _responseGreen->deleteAllKeys();
  _responseBlue->deleteAllKeys();
  _responseRed->setValue(0);
  _responseGreen->setValue(0);
  _responseBlue->setValue(0);
}

void HdrMergePlugin::clearWeightFunctionKeyFrames()
{
  _weightRed->deleteAllKeys();
  _weightGreen->deleteAllKeys();
  _weightBlue->deleteAllKeys();
  _weightRed->setValue(0);
  _weightGreen->setValue(0);
  _weightBlue->setValue(0);
}

void HdrMergePlugin::setImageOptionsSecret(std::size_t imageIndex, bool isSecret)
{
  _sourceImages[imageIndex]->setIsSecret(isSecret);
  _iso[imageIndex]->setIsSecret(isSecret);
  _aperture[imageIndex]->setIsSecret(isSecret);
  _shutter[imageIndex]->setIsSecret(isSecret);
  _ev[imageIndex]->setIsSecret(isSecret);
}

void HdrMergePlugin::reset() 
{
  _uptodate = false;
  updateSourceParameters();
  updateResponsePreset();
  updateWeightPreset();
  
  if(static_cast<cameraColorCalibration::common::EPresetResponse>(_responsePreset->getValue()) 
          == cameraColorCalibration::common::eResponsePresetCustom)
  {
    //Response Function is custom
    updateResponseFunctionFromKeyFrames(false); //don't print message
  }
  
  if(static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue()) 
          == cameraColorCalibration::common::eWeightPresetCustom)
  {
    //Weight Function is custom
    updateWeightFunctionFromKeyFrames(false); //don't print message
  }
}

} // namespace hdrMerge 
} // namespace cameraColorCalibration