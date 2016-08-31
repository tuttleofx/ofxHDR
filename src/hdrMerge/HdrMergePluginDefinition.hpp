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
#define kParamResponseLoad "responseLoad"
#define kParamResponseExport "responseExport"
#define kParamResponseRefreshKeys "responseRefreshKeys"
#define kParamResponseClearKeys "responseClearKeys"
#define kParamResponseFromKeys "responseFromKeys"
#define kParamResponseRed "responseRed"
#define kParamResponseGreen "responseGreen"
#define kParamResponseBlue "responseBlue"


//Advanced Group Parameters
#define kParamGroupWeight "groupWeight"

#define kParamWeightPreset "weightPreset"
#define kParamWeightFilePath "weightFilePath"
#define kParamWeightLoad "weightLoad"
#define kParamWeightExport "weightExport"
#define kParamWeightRefreshKeys "weightRefreshKeys"
#define kParamWeightClearKeys "weightClearKeys"
#define kParamWeightFromKeys "weightFromKeys"
#define kParamWeightRed "weightRed"
#define kParamWeightGreen "weightGreen"
#define kParamWeightBlue "weightBlue"


//Invalidation Parameters
#define kParamForceInvalidation "forceInvalidation"