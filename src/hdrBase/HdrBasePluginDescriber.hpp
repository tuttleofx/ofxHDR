#pragma once
#include "ofxsImageEffect.h"


namespace cameraColorCalibration {
namespace hdrBase {

void describeClip(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context, std::size_t nbClips);
OFX::GroupParamDescriptor* describeInputsGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context, std::size_t nbClips);
OFX::GroupParamDescriptor* describeTargetGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context);
OFX::GroupParamDescriptor* describeResponseGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context, bool allowEditing = true);
OFX::GroupParamDescriptor* describeWeightGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context);
OFX::GroupParamDescriptor* describeDebugGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context);
void describeInvalidation(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context);

} // namespace hdrBase 
} // namespace cameraColorCalibration
