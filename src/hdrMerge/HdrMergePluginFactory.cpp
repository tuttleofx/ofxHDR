#include "HdrMergePluginFactory.hpp"
#include "HdrMergePluginDefinition.hpp"
#include "HdrMergePlugin.hpp"
#include "../common/Presets.hpp"


namespace cameraColorCalibration {
namespace hdrMerge {

void HdrMergePluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  //Plugin Labels
  desc.setLabels(
    "HdrMerge",
    "HdrMerge",
    "HdrMerge");
  
  //Plugin grouping
  desc.setPluginGrouping("tuttle/image/process/color");

  //Plugin description
  desc.setPluginDescription("HdrMerge is used to use a response function of a couple (camera / lens) in order to do a proper color calibration of an HDR. ");

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

void HdrMergePluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  //Clips
  cameraColorCalibration::hdrBase::describeClip(desc, context, K_NB_CLIPS);
  
  //Sources Group
  cameraColorCalibration::hdrBase::describeInputsGroup(desc, context, K_NB_CLIPS)->setAsTab();
  
  //Target Group
  cameraColorCalibration::hdrBase::describeTargetGroup(desc, context);
  
  //Weight group
  cameraColorCalibration::hdrBase::describeWeightGroup(desc, context);
  
  //Response group
  cameraColorCalibration::hdrBase::describeResponseGroup(desc, context, true);
  
  //Debug group
  cameraColorCalibration::hdrBase::describeDebugGroup(desc, context);
  
  //Invalidation
  cameraColorCalibration::hdrBase::describeInvalidation(desc, context);
}

OFX::ImageEffect* HdrMergePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
  return new HdrMergePlugin(handle, K_NB_CLIPS);
}

} // namespace hdrMerge 
} // namespace cameraColorCalibration