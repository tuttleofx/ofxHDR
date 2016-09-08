#pragma once

/**
 * Plugin Parameters definition
 */

//Source Group Parameters
#define kParamGroupSource "groupSource"


//Image Group Parameters
#define kParamGroupImage(I) "groupImage_" + std::to_string(I)

#define kParamImageIso(I) "imageParamIso_" + std::to_string(I) //(!) nameID used for regex in ChangedParam
#define kParamImageAperture(I) "imageParamAperture_" + std::to_string(I) //(!) nameID used for regex in ChangedParam
#define kParamImageShutter(I) "imageParamShutter_" + std::to_string(I) //(!) nameID used for regex in ChangedParam
#define kParamImageEv(I) "imageEv_" + std::to_string(I) //(!) nameID used for regex in ChangedParam


//Target Group Parameters
#define kParamGroupTarget "groupTarget"

#define kParamTargetImageIso "targetImageParamIso"
#define kParamTargetImageAperture "targetImageParamAperture" 
#define kParamTargetImageShutter "targetImageParamShutter"
#define kParamTargetImageEv "targetImageEv" 


//Response Group Parameters
#define kParamGroupResponse "groupResponse"

#define kParamResponsePreset "responsePreset"
#define kParamResponseFilePath "responseFilePath"
#define kParamResponseExport "responseExport"
#define kParamResponseRefreshKeys "responseRefreshKeys"
#define kParamResponseClearKeys "responseClearKeys"
#define kParamResponseRed "responseRed"
#define kParamResponseGreen "responseGreen"
#define kParamResponseBlue "responseBlue"


//Advanced Group Parameters
#define kParamGroupWeight "groupWeight"

#define kParamWeightPreset "weightPreset"
#define kParamWeightGaussianCustom "weightGaussianCustom"
#define kParamWeightFilePath "weightFilePath"
#define kParamWeightExport "weightExport"
#define kParamWeightRefreshKeys "weightRefreshKeys"
#define kParamWeightClearKeys "weightClearKeys"
#define kParamWeightRed "weightRed"
#define kParamWeightGreen "weightGreen"
#define kParamWeightBlue "weightBlue"


//Debug Group
#define kParamGroupDebug "groupDebug"

#define kParamDebugActive "debugActive"
#define kParamDebugOutput "debugOutput"


//Invalidation Parameters
#define kParamForceInvalidation "forceInvalidation"