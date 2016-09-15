import nuke
import numbers

#Parse a simple division string
def divisionToFloat( str ):
    "This function convert a string to a float and can parse a simple division"
    
    if isinstance(str, numbers.Real):
        return float( str )
    
    strs = str.split('/')
    
    if len(strs) > 1:
        return ( float( strs[0] ) / float( strs[1] ) )
    
    return float( strs[0] )

#retrieve Data From a Tag
def retrieveDataFromTag( node, frame, tagList ) :
    "This function return the value of one metaData from a list of same exif tag"
    
    tags = tagList.split('\n')
    data = None
    for tag in tags:
        data = node.metadata(tag, time=frame)
        if data != None:
            break
            
    if data == None:
        return 0
        
    return divisionToFloat(data)
    
    
#Retrieve MetaData
def retrieveMetaDataFromList( node, exifIsoList, exifApList, exifShList ):
    "This function fill metaData Fields from sources"
    for group in range(node.inputs()):
        nGroup = node.input(group)
        strGroup = str(group)
        idxFrame = 0
        for frame in nGroup.frameRange():
            strFrame = str(idxFrame)
            
            iso = retrieveDataFromTag( nGroup, frame, exifIsoList )
            aperture = retrieveDataFromTag( nGroup, frame, exifApList )
            shutter = retrieveDataFromTag( nGroup, frame, exifShList )
            node['imageParamIso_' + strGroup + '_' + strFrame].setValue( iso )
            node['imageParamAperture_' + strGroup + '_' + strFrame].setValue( aperture )
            node['imageParamShutter_' + strGroup + '_' + strFrame].setValue( shutter )
            
            idxFrame += 1
    return

#Check Camera Type
def checkCameraType( node ):
    "This function fill Camera Field and detect if they are different camera in metadata"
    tags = []
    for group in range(node.inputs()):
        nGroup = node.input(group)

        idxFrame = 0
        for frame in nGroup.frameRange():
            data = nGroup.metadata('exif/0/Model', time=frame)

            if tags.count(data) == 0:
                tags.append(data)

            idxFrame += 1
        
    text = ""
    for tag in tags:
        text += str(tag) + "<br/>"
        
        
    if len(tags) > 1 :
		node['md_camera'].setValue( '<span style="color: red;">' + text + '</span>' )
		return
		
    node['md_camera'].setValue( '<span style="color: green;">' + text  + '</span>')
   
    if len(tags) == 1 and node['responseFilePath'].getValue() == '' :
		node['responseFilePath'].setValue('~/' + tag.replace (" ", "_") + '.csv')

    return
    

def metaDataAction():
    n = nuke.thisNode()
    retrieveMetaDataFromList( n, n['md_iso'].getValue(), n['md_ap'].getValue(), n['md_sh'].getValue() )
    checkCameraType(n)


def initOFXhdrPlugin():
    n = nuke.thisNode()
    mdTab = nuke.Tab_Knob('MetaData', 'MetaData')
    mdBtn = nuke.PyScript_Knob('metadata','Retrieve MetaData')
    mdDivCamera = nuke.Text_Knob('md_div_camera','Camera','')
    mdCamera = nuke.Text_Knob('md_camera','',' ')
    mdDivExif = nuke.Text_Knob('md_div_exif','Exif Tags','')
    mdIso = nuke.Multiline_Eval_String_Knob('md_iso','ISO','raw/image/iso_speed\nexif/2/ISOSpeedRatings')
    mdAp = nuke.Multiline_Eval_String_Knob('md_ap','Aperture','raw/image/aperture\ninput/fnumber\nexif/2/FNumber\nexif/2/ApertureValue')
    mdSh = nuke.Multiline_Eval_String_Knob('md_sh','Shutter','raw/image/shutter_speed\ninput/exposure_time\nexif/2/ExposureTime')
    
    n.addKnob(mdTab)
    n.addKnob(mdBtn)
    n.addKnob(mdDivCamera)
    n.addKnob(mdCamera)
    n.addKnob(mdDivExif)
    n.addKnob(mdIso)
    n.addKnob(mdAp)
    n.addKnob(mdSh)
    
    mdBtn.setValue('metaDataAction()')
    

nuke.addOnCreate(initOFXhdrPlugin, (), nodeClass='OFXtuttleofx.hdrcalib_v1')
nuke.addOnCreate(initOFXhdrPlugin, (), nodeClass='OFXtuttleofx.hdrmerge_v1')
