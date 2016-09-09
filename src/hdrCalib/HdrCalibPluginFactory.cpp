#include "HdrCalibPluginFactory.hpp"
#include "HdrCalibPluginDefinition.hpp"
#include "HdrCalibPlugin.hpp"
#include "../hdrBase/HdrBasePluginDescriber.hpp"
#include "../common/Presets.hpp"
#include "../hdrBase/HdrBasePluginDefinition.hpp"


namespace cameraColorCalibration {
namespace hdrCalibration {

void HdrCalibPluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  //Plugin Labels
  desc.setLabels(
    "HdrCalib",
    "HdrCalib",
    "HdrCalib");
  
  //Plugin grouping
  desc.setPluginGrouping("tuttle/image/process/color");

  //Plugin description
  desc.setPluginDescription("HdrCalib is used to compute the response function of a couple (camera / lens)  from multiple groups of multi-bracketing images. This response function is a requirement in order to do a proper color calibration of an HDR. ");

  //Supported contexts
  desc.addSupportedContext(OFX::eContextFilter);
  desc.addSupportedContext(OFX::eContextGeneral);
  desc.addSupportedContext(OFX::eContextPaint);

  //Supported pixel depths
  desc.addSupportedBitDepth(OFX::eBitDepthUByte);
  desc.addSupportedBitDepth(OFX::eBitDepthUShort);
  desc.addSupportedBitDepth(OFX::eBitDepthFloat);

  //Flags
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(false);
  desc.setTemporalClipAccess(true);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);
}

void HdrCalibPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  cameraColorCalibration::hdrBase::describeClip(desc, context, K_NB_CLIPS);
  
  //Calibration Group
  {
    OFX::GroupParamDescriptor *groupCalibration = desc.defineGroupParam(kParamGroupCalibration);
    groupCalibration->setLabel("Calibration");
    groupCalibration->setAsTab();
    
    {
      OFX::IntParamDescriptor *param = desc.defineIntParam(kParamCalibrationOutputIndex);
      param->setLabel("HDR Output Index");
      param->setHint("The index of the input group you want as output of this node.");
      param->setRange(0, K_MAX_GROUPS);
      // DisplayRange will be updated regarding the number of connected input clips
      param->setDisplayRange(0, 0);
      param->setAnimates(false);
      param->setEnabled(false);
      param->setParent(*groupCalibration);
      param->setLayoutHint(OFX::eLayoutHintNoNewLine);
    }
    
    {
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamCalibrationCalculateResponse);
      param->setLabel("Calculate Response");
      param->setHint("Calculate Response with all sources informations");
      param->setEnabled(false);
      param->setParent(*groupCalibration);
      param->setLayoutHint(OFX::eLayoutHintDivider);
    }
    
    cameraColorCalibration::hdrBase::describeInputsGroup(desc, context, K_NB_CLIPS)->setParent(*groupCalibration);
  }
  
  //Target Group
  cameraColorCalibration::hdrBase::describeTargetGroup(desc, context);

  //Algorithm options group
  {
    OFX::GroupParamDescriptor *groupAdvanced = desc.defineGroupParam(kParamGroupAlgorithm);
    groupAdvanced->setLabel("Algorithm");
    groupAdvanced->setAsTab();
    
    {
      OFX::IntParamDescriptor *param = desc.defineIntParam(kParamAlgorithmIterations);
      param->setLabel("Iterations");
      param->setHint("Maximum number of algorithm iteration");
      param->setRange(1, 5000);
      param->setDisplayRange(1, 1000);
      param->setDefault(500);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupAdvanced);
    }
    
    {
      OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAlgorithmThreshold);
      param->setLabel("Threshold");
      param->setHint("Response precision");
      param->setRange(0, 10);
      param->setDisplayRange(0, 1);
      param->setDefault(0.01);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupAdvanced);
    }
  }
  
  //Weight group
  cameraColorCalibration::hdrBase::describeWeightGroup(desc, context);
  
  //Response group
  cameraColorCalibration::hdrBase::describeResponseGroup(desc, context, false);
  
  //Debug group
  cameraColorCalibration::hdrBase::describeDebugGroup(desc, context);
  
  //Invalidation
  cameraColorCalibration::hdrBase::describeInvalidation(desc, context);
  
}

OFX::ImageEffect* HdrCalibPluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
  return new HdrCalibPlugin(handle, K_NB_CLIPS);
}

} // namespace hdrCalibration
} // namespace cameraColorCalibration

