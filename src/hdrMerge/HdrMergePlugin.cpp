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
    OFX::ImageEffect(handle)
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

void HdrMergePlugin::getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames)
{
  frames.setFramesNeeded(*_srcClip, _srcClip->getFrameRange());
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
    
    if(!loadGroup())
      return;

    if(abort())
      return;
    
    //Debug Output
    if(_debugActive->getValue())
    {
      std::cout << "render : [debug] fetch"  << std::endl;
      OFX::Image *outputPtr = _dstClip->fetchImage(args.time);
      if(outputPtr == NULL)
      {
        std::cout << "render : [debug output clip] is NULL" << std::endl;
        return;
      }

      cameraColorCalibration::common::Image<float> output(outputPtr);
      
      std::size_t outputIndex = _debugOutput->getValue() - 1;
      
      if(_ldrImages.size() <= outputIndex)
      {
        output.setRed();
      }
      else
      {
        std::cout << "render : Debug" << std::endl;
        output.copyFrom(_ldrImages[outputIndex]);
      }
      return;
    }
    
    std::cout << "render : initialize output HDR ..." << std::endl;
    _hdrImage.createInternalBuffer(_ldrImages.front().getWidth(), _ldrImages.front().getHeight(), 3);

    {
      std::cout << "render : [merge]" << std::endl;
      float targetExposure = _targetShutter->getValue();
      std::cout << "render : [merge] targetExposure: " << targetExposure << std::endl;

      cameraColorCalibration::common::RobertsonMerge merge;
      cameraColorCalibration::common::rgbCurve weight(K_QUANTIZATION);
      cameraColorCalibration::common::rgbCurve response(K_QUANTIZATION);

      getWeightFunction(weight);
      getResponseFunction(response);

      merge.process(_ldrImages, _luminances, weight, response, _hdrImage, targetExposure);
      std::cout << "render : [merge] -- OK" << std::endl;
    }

      /*cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                                      _targetIso->getValue(),
                                                                                      _targetAperture->getValue());*/
  //    float averageLuminance = std::accumulate(_luminances.begin(), _luminances.end(), 0) / (double)(_luminances.size());
  //    
  //    _hdrImage.multiply(targetExposure / averageLuminance);
    

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
  catch(std::exception &e)
  {
    this->sendMessage(OFX::Message::eMessageError, "hdrmerge.render", e.what());
  }
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
//    _uptodate = false;
  
    updateSourceParameters();
  }
}

void HdrMergePlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
  //check if the parameter is a MetaData
  if(changedMetaData(paramName))
  {
    //a meta data has changed, ev have been update by the function
    return;
  }

  if(paramName == kParamResponsePreset)
  {
    updateResponsePreset();
    return;
  }
  
  if(paramName == kParamWeightPreset)
  {
    updateWeightPreset();
    return;
  }
  
  if((paramName == kParamResponseExport) || (paramName == kParamWeightExport))
  {
    try 
    {
      cameraColorCalibration::common::rgbCurve curve(K_QUANTIZATION);
      if(paramName == kParamResponseExport)
      {
        getResponseFunction(curve);
        curve.write(_responseFilePath->getValue(), "Response Function");
      }
      else
      {
        getWeightFunction(curve);
        curve.write(_weightFilePath->getValue(), "Weight Function");
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

bool HdrMergePlugin::loadGroup()
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

  OfxRangeD frameRange = _srcClip->getFrameRange();
  
  float targetExposure = _targetShutter->getValue();
  /*cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                                    _targetIso->getValue(),
                                                                                    _targetAperture->getValue());*/
  std::cout << "[load]   target EV: "<< targetExposure << "." << std::endl;

  _ldrImages = std::vector< cameraColorCalibration::common::Image<float> >(frameRange.max - frameRange.min + 1);
  for(double frame = frameRange.min; frame < frameRange.max + 1.0; ++frame)
  {
    std::cout << "[load]  Image :  " << frame << std::endl;
    int inputIndex = frame - frameRange.min;
    assert(inputIndex <= K_MAX_SOURCE_IMAGES);
    float exposure = _shutter[inputIndex]->getValue();
    /*cameraColorCalibration::common::RobertsonMerge::getExposure(_shutter[inputIndex]->getValue(),
                                                                                  _iso[inputIndex]->getValue(),
                                                                                  _aperture[inputIndex]->getValue());*/

    std::cout << "[load]   EV:" << exposure << "." << std::endl;
//    float avgLuminance = std::pow(2.f, exposure - targetExposure);
    float avgLuminance = exposure; // - targetExposure;
    std::cout << "[load]   used Relative EV: " << avgLuminance << "." << std::endl;
    _luminances.push_back( avgLuminance );

    OFX::Image *imagePtr = _srcClip->fetchImage(frame);
    if(imagePtr == NULL)
    {
      std::cerr << "[load] error : can't load image " << std::endl;
      return false;
    }
    _ldrImages[inputIndex].setOfxImage(imagePtr);
  }
  return true;
}

void HdrMergePlugin::updateResponsePreset()
{
  cameraColorCalibration::common::EPresetResponse preset;
  preset = static_cast<cameraColorCalibration::common::EPresetResponse>(_responsePreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eResponsePresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eResponsePresetFromFile);
  
  _responseExport->setIsSecret(!custom);
  _responseFilePath->setIsSecret(!custom && !fromFile);
  _responseRed->setEnabled(custom);
  _responseGreen->setEnabled(custom);
  _responseBlue->setEnabled(custom);
}

void HdrMergePlugin::updateWeightPreset()
{
  cameraColorCalibration::common::EPresetWeight preset;
  preset = static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue());
  
  bool gaussianCustom = (preset == cameraColorCalibration::common::eWeightPresetGaussianCustom); 
  bool custom = (preset == cameraColorCalibration::common::eWeightPresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eWeightPresetFromFile);
  
  _weightExport->setIsSecret(!custom);
  _weightGaussianSize->setIsSecret(!gaussianCustom);
  _weightFilePath->setIsSecret(!custom && !fromFile);
  _weightRed->setEnabled(custom);
  _weightGreen->setEnabled(custom);
  _weightBlue->setEnabled(custom);
}

void HdrMergePlugin::getResponseFunction(cameraColorCalibration::common::rgbCurve &response)
{
  cameraColorCalibration::common::EPresetResponse preset;
  preset = static_cast<cameraColorCalibration::common::EPresetResponse>(_responsePreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eResponsePresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eResponsePresetFromFile);
  
  if(custom)
  {
    getResponseFunctionFromKeyFrames(response);
    return;
  }
  
  if(fromFile)
  {
    getFunctionFromFile(_responseFilePath->getValue(), response);
  }
  
  cameraColorCalibration::common::initResponseFromPreset(response, preset);
}

void HdrMergePlugin::getWeightFunction(cameraColorCalibration::common::rgbCurve &weight)
{
  cameraColorCalibration::common::EPresetWeight preset;
  preset = static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eWeightPresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eWeightPresetFromFile);
  
  if(custom)
  {
    getWeightFunctionFromKeyFrames(weight);
    return;
  }
  
  if(fromFile)
  {
    getFunctionFromFile(_weightFilePath->getValue(), weight);
  }
  
  cameraColorCalibration::common::initWeightFromPreset(weight, _weightGaussianSize->getValue(), preset);
}

void HdrMergePlugin::getFunctionFromFile(const std::string &path, cameraColorCalibration::common::rgbCurve &curve)
{
  try 
  {
    curve.read(path);
  }
  catch(std::exception &e)
  {
    this->sendMessage(OFX::Message::eMessageError, "hdrmerge.curve.load", e.what());
  }
}
  
void HdrMergePlugin::getResponseFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &response)
{
  for(std::size_t index = 0; index < response.getSize(); ++index)
  {
    response.getCurveRed()[index] = _responseRed->getValueAtTime(index);
    response.getCurveGreen()[index] = _responseGreen->getValueAtTime(index);
    response.getCurveBlue()[index] = _responseBlue->getValueAtTime(index);
  }
}

void HdrMergePlugin::getWeightFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &weight)
{
  for(std::size_t index = 0; index < weight.getSize(); ++index)
  {
    weight.getCurveRed()[index] = _weightRed->getValueAtTime(index);
    weight.getCurveGreen()[index] = _weightGreen->getValueAtTime(index);
    weight.getCurveBlue()[index] = _weightBlue->getValueAtTime(index);
  }
}

void HdrMergePlugin::refreshResponseFunctionKeyFrames()
{
  cameraColorCalibration::common::rgbCurve response(K_QUANTIZATION);
  getResponseFunction(response);
  
  this->beginEditBlock("[HdrMerge] set keyframes from response function");
  
  clearResponseFunctionKeyFrames();
  
  progressStart("Create response function keyframes", "hdrmerge.response.keyframes");
  const double updateCoefficient = 1.0 / (double)response.getSize();
  for(std::size_t index = 0; index < response.getSize(); ++index)
  {
    _responseRed->setValueAtTime(index, response.getCurveRed()[index]);
    _responseGreen->setValueAtTime(index, response.getCurveGreen()[index]);
    _responseBlue->setValueAtTime(index, response.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
  
  this->endEditBlock();
}

void HdrMergePlugin::refreshWeightFunctionKeyFrames()
{
  cameraColorCalibration::common::rgbCurve weight(K_QUANTIZATION);
  getWeightFunction(weight);
  
  this->beginEditBlock("[HdrMerge] set keyframes from weight function");
  
  clearWeightFunctionKeyFrames();
  
  progressStart("Create weight function keyframes", "hdrmerge.weight.keyframes");
  const double updateCoefficient = 1.0 / (double)weight.getSize();
  for(std::size_t index = 0; index < weight.getSize(); ++index)
  {
    _weightRed->setValueAtTime(index, weight.getCurveRed()[index]);
    _weightGreen->setValueAtTime(index, weight.getCurveGreen()[index]);
    _weightBlue->setValueAtTime(index, weight.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
  
  this->endEditBlock();
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
  updateSourceParameters();
  updateResponsePreset();
  updateWeightPreset();
}

} // namespace hdrMerge 
} // namespace cameraColorCalibration