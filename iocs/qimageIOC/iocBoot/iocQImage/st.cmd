< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/qimagingApp.dbd")
qimagingApp_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("PREFIX", "invid:")
epicsEnvSet("PORT",   "RETIGA")
epicsEnvSet("MODEL",  "4000DC")
epicsEnvSet("CAMERA", "0")
epicsEnvSet("NUMBUFFS","10")
epicsEnvSet("QSIZE",  "20")
epicsEnvSet("XSIZE",  "2024")
epicsEnvSet("YSIZE",  "2048")
epicsEnvSet("NCHANS", "2048")
epicsEnvSet("DEBUG",  "1")

epicsEnvSet("NDInt8"   , "0")
epicsEnvSet("NDUInt8"  , "1")
epicsEnvSet("NDInt16"  , "2")
epicsEnvSet("NDUInt16" , "3")
epicsEnvSet("NDInt32"  , "4")
epicsEnvSet("NDUInt32" , "5")
epicsEnvSet("NDFloat32", "6")
epicsEnvSet("NDFloat64", "7")


QImageConfig("$(PORT)", "$(MODEL)", $(NDInt16), $(NUMBUFFS), $(DEBUG), -1, -1, 0, 0)
dbLoadRecords("$(ADCORE)/db/ADBase.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADQIMAGING)/db/qimaging.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin
NDStdArraysConfigure("Image1", 5, 0, "$(PORT)", 0, 0)
dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")

# Make NELEMENTS in the following be a little bigger than 2048*2048
# Use the following command for 16-bit images.  This can be used for 16-bit detector as long as accumulate mode would not result in 16-bit overflow
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int16,FTVL=SHORT,NELEMENTS=5600000")


# This creates a waveform large enough for 1024x1024x3 (e.g. RGB color) arrays.
# This waveform only allows transporting 8-bit images
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int8,FTVL=UCHAR,NELEMENTS=12582912")
# This waveform only allows transporting 16-bit images
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int16,FTVL=USHORT,NELEMENTS=12582912")
# This waveform allows transporting 32-bit images
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int32,FTVL=LONG,NELEMENTS=12582912")


# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
set_requestfile_path("$(ADQIMAGING)/App/Db")

#asynSetTraceIOMask("$(PORT)",0,4)
asynSetTraceIOMask("$(PORT)",0,9)


# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX)")

iocInit()

