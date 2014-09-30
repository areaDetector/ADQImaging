// * QImage.cpp
// *
// * This is a driver for QImage detectors.
// *
// * Author: Arthur Glowacki
// *         APS-ANL
// *
// * Created:  June 13, 2014

#include "QImaging.h"

#define MAX_ENUM_STATES 16

static void exposureTaskC(void *drvPvt)
{
    QImage *pPvt = (QImage *)drvPvt;
    //pPvt->exposureTask();
	pPvt->consumerTask();
}
static void frameTaskC(void *drvPvt)
{
    QImage *pPvt = (QImage *)drvPvt;
    pPvt->frameTask();
}

void QCAMAPI QImageCallback(void* usrPtr, unsigned long frameId, QCam_Err errorcode, unsigned long flags)
{
	QImage *pPtr = (QImage *)usrPtr;

	asynPrint(pPtr->pasynUserSelf, ASYN_TRACE_FLOW, "QImage Callback\n");

	//
	//pPtr->lock();
	/*
	if (flags & qcCallbackExposeDone) 
	{
		task.count = ++(pPtr->expCnt);
		pPtr->setIntegerParam(pPtr->qExpCnt, pPtr->expCnt);
		pPtr->callParamCallbacks();
		pPtr->lock();
		//pPtr->exposureQueue.push(task);
		pPtr->unlock();
	}
	*/
	if (flags & qcCallbackDone) 
	{
		asynPrint(pPtr->pasynUserSelf, ASYN_TRACE_FLOW, "QImage Callback: frameid %d\n", frameId);
		pPtr->pFrames[frameId].errorcode = errorcode;
		pPtr->pFrames[frameId].flags = flags;
		pPtr->pFrames[frameId].count = ++(pPtr->frmCnt);
		pPtr->setIntegerParam(pPtr->qFrmCnt, pPtr->frmCnt);
		pPtr->pushCollectedFrame(frameId);
		pPtr->callParamCallbacks();
	}
	//pPtr->unlock();
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------


//  * Constructor for QImage; most parameters are simply passed to ADDriver::ADDriver.
//  * After calling the base class constructor this method creates a thread to compute the simulated detector data,
//  * and sets reasonable default values for parameters defined in this class, asynNDArrayDriver and ADDriver.
//  * \param[in] portName The name of the asyn port driver to be created.
//  * \param[in] dataType The initial data type (NDDataType_t) of the images that this driver will create.
//  * \param[in] numbuffs The number of frames to hold in the circular buffer.
//  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
//  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
//  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
//  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
//  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
//  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.

QImage::QImage(const char *portName, const char *model, NDDataType_t ndDataType, int numbuffs, int debug, int maxBuffers, size_t maxMemory, int priority, int stackSize)

	: ADDriver(portName, 1, (int)NUM_QIMAGE_PARAMS, maxBuffers, maxMemory, asynEnumMask, asynEnumMask, 0, 1, priority, stackSize)
{
	int           status = asynSuccess;
	asynStatus    success = asynSuccess;
	const char   *functionName = "QImage";

	isSettingsInit = false;
	_adAcquire = false;
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: max buffers %d, max mem %d\n", functionName, maxBuffers, maxMemory);
	qHandle = 0;
	exiting_ = 0;

	createParam(qMaxBitDepthRBVString, asynParamInt32, &qMaxBitDepthRBV);
	createParam(qSerialNumberRBVString, asynParamOctet, &qSerialNumberRBV);
	createParam(qUniqueIdRBVString, asynParamInt32, &qUniqueIdRBV);
	createParam(qCcdTypeRBVString, asynParamInt32, &qCcdTypeRBV);
	createParam(qCooledRBVString, asynParamOctet, &qCooledRBV);
	createParam(qRegulatedCoolingRBVString, asynParamOctet, &qRegulatedCoolingRBV);
	createParam(qFanControlRBVString, asynParamOctet, &qFanControlRBV);
	createParam(qHighSensitivityModeRBVString, asynParamOctet, &qHighSensitivityModeRBV);
	createParam(qBlackoutModeRBVString, asynParamOctet, &qBlackoutModeRBV);
	createParam(qAsymmetricalBinningRBVString, asynParamOctet, &qAsymmetricalBinningRBV);
	createParam(qCoolerActiveString, asynParamInt32, &qCoolerActive);
	createParam(qReadoutSpeedString, asynParamInt32, &qReadoutSpeed);
	createParam(qOffsetString, asynParamInt32, &qOffset);
	createParam(qImageFormatString, asynParamInt32, &qImageFormat);
	createParam(qAcquireTimeRBVString, asynParamFloat64, &qAcquireTimeRBV);
	createParam(qMinXRBVString, asynParamInt32, &qMinXRBV);
	createParam(qMinYRBVString, asynParamInt32, &qMinYRBV);
	createParam(qSizeXRBVString, asynParamInt32, &qSizeXRBV);
	createParam(qSizeYRBVString, asynParamInt32, &qSizeYRBV);
	createParam(qTriggerModeRBVString, asynParamInt32, &qTriggerModeRBV);
	createParam(qGainRBVString, asynParamFloat64, &qGainRBV);
	createParam(qTemperatureRBVString, asynParamFloat64, &qTemperatureRBV);
	createParam(qReadoutSpeedRBVString, asynParamInt32, &qReadoutSpeedRBV);
	createParam(qOffsetRBVString, asynParamInt32, &qOffsetRBV);
	createParam(qImageFormatRBVString, asynParamInt32, &qImageFormatRBV);
	createParam(qCoolerActiveRBVString, asynParamInt32, &qCoolerActiveRBV);
	createParam(qRegulatedCoolingLockRBVString, asynParamInt32, &qRegulatedCoolingLockRBV);
	createParam(qExposureStatusMessageRBVString, asynParamOctet, &qExposureStatusMessageRBV);
	createParam(qFrameStatusMessageRBVString, asynParamOctet, &qFrameStatusMessageRBV);
	createParam(qTrgCntString, asynParamInt32, &qTrgCnt);
	createParam(qExpCntString, asynParamInt32, &qExpCnt);
	createParam(qFrmCntString, asynParamInt32, &qFrmCnt);
	createParam(qShowDiagsString, asynParamInt32, &qShowDiags);
	createParam(qResetCamString, asynParamInt32, &qResetCam);
	createParam(qExposureMaxString, asynParamFloat64, &qExposureMax);
	createParam(qExposureMinString, asynParamFloat64, &qExposureMin);
	createParam(qGainMaxString, asynParamFloat64, &qGainMax);
	createParam(qGainMinString, asynParamFloat64, &qGainMin);
	createParam(qBinningString, asynParamInt32, &qBinning);
	createParam(qAutoExposureString, asynParamInt32, &qAutoExposure);
	createParam(qWhiteBalanceString, asynParamInt32, &qWhiteBalance);
	createParam(qInitializeString, asynParamInt32, &qInitialize);
	
	// Set some default values for parameters
	status |= setStringParam(ADManufacturer, "QImaging");
	status |= setStringParam(ADModel, model);
	status |= setIntegerParam(NDDataType, ndDataType);
	status |= setIntegerParam(ADNumImages, 100);
	status |= setIntegerParam(ADStatus, ADStatusIdle);
	status |= setStringParam(qExposureStatusMessageRBV, "Waiting");
	status |= setStringParam(qFrameStatusMessageRBV, "Waiting");
	status |= setIntegerParam(qTrgCnt, 0);
	status |= setIntegerParam(qExpCnt, 0);
	status |= setIntegerParam(qFrmCnt, 0);
	status |= setIntegerParam(qShowDiags, debug);
	if (status) 
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: unable to set default parameters\n", functionName);
		return;
	}
	status |= callParamCallbacks();

	camPushSleepAmt = 0.001; 

	num_buffs = numbuffs;
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Allocating [[%d]] Buffers\n\n", functionName, numbuffs);
	expSntl = 0;

	// Create the epicsEvents for signaling to the QImage task when acquisition starts and stops
	this->stopEventId = epicsEventCreate(epicsEventEmpty);
	if (!this->stopEventId) {
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s epicsEventCreate failure for stop event\n", driverName, functionName);
		return;
	}

	if (connectQImage() == asynSuccess)
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Camera Connected\n\n", functionName);
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: Failed to connect to camera\n\n", functionName);
		status |= setIntegerParam(ADMinX, 0);
		status |= setIntegerParam(ADMinY, 0);
		status |= setIntegerParam(NDArraySizeX, 1);
		status |= setIntegerParam(ADSizeX, 1);
		status |= setIntegerParam(NDArraySizeY, 1);
		status |= setIntegerParam(ADSizeY, 1);
		//status |= setIntegerParam(qImageFormat, imageFormat);
		status |= setIntegerParam(NDArraySize, 8);
		//status |= setIntegerParam(NDDataType, dataType);
		status |= callParamCallbacks();
		return;
	}
	if (!status)
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	else
		return;
	if (!status)
		status |= resultCode(functionName, "QCam_SetParam(qprmTriggerType)", QCam_SetParam((QCam_Settings*)&qSettings, qprmTriggerType, qcTriggerSoftware));
	else
		return;
	if (!status)
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
	else return;

	triggerType = qcTriggerSoftware;

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Allocating %d Buffers\n\n", functionName, num_buffs);
	initializeFrames();


	// Create the exposure task thread 
	status = (epicsThreadCreate("exposureTask", epicsThreadPriorityMedium, epicsThreadGetStackSize(epicsThreadStackMedium),
		(EPICSTHREADFUNC)exposureTaskC, this) == NULL);
	if (status)
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s epicsThreadCreate failure for exposure task\n", driverName, functionName);
	// Create the frame task thread 
	status = (epicsThreadCreate("frameTask", epicsThreadPriorityMedium, epicsThreadGetStackSize(epicsThreadStackMedium),
		(EPICSTHREADFUNC)frameTaskC, this) == NULL);
	if (status)
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s epicsThreadCreate failure for frame task\n", driverName, functionName);

	// Register the shutdown function for epicsAtExit
	epicsAtExit(QImageShutdown, (void*)this);

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: QImage Constructor finished\n\n", functionName);

}

// Configuration command, called directly or from iocsh
extern "C" int QImageConfig(const char *portName, const char *model, int dataType, int numbuffs, int debug, int maxBuffers, size_t maxMemory, int priority, int stackSize)
{
	new QImage(portName, model, (NDDataType_t)dataType, numbuffs, debug, maxBuffers, maxMemory, priority, stackSize);
	return(asynSuccess);
}

// Code for iocsh registration
static const iocshArg QImageConfigArg0 = { "Port name", iocshArgString };
static const iocshArg QImageConfigArg1 = { "Model name", iocshArgString };
static const iocshArg QImageConfigArg2 = { "Data type", iocshArgInt };
static const iocshArg QImageConfigArg3 = { "num of buffs", iocshArgInt };
static const iocshArg QImageConfigArg4 = { "debug level", iocshArgInt };
static const iocshArg QImageConfigArg5 = { "maxBuffers", iocshArgInt };
static const iocshArg QImageConfigArg6 = { "maxMemory", iocshArgInt };
static const iocshArg QImageConfigArg7 = { "priority", iocshArgInt };
static const iocshArg QImageConfigArg8 = { "stackSize", iocshArgInt };
static const iocshArg * const QImageConfigArgs[] = { &QImageConfigArg0,
&QImageConfigArg1,
&QImageConfigArg2,
&QImageConfigArg3,
&QImageConfigArg4,
&QImageConfigArg5,
&QImageConfigArg6,
&QImageConfigArg7,
&QImageConfigArg8 };

static const iocshFuncDef configQImage = { "QImageConfig", 9, QImageConfigArgs };

static void configQImageCallFunc(const iocshArgBuf *args)
{
	QImageConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival, args[5].ival, args[6].ival, args[7].ival, args[8].ival);
}

static void QImageRegister(void)
{
	iocshRegister(&configQImage, configQImageCallFunc);
}

extern "C" {
	epicsExportRegistrar(QImageRegister);
}


asynStatus QImage::connectQImage()
{
	int               status        = asynSuccess;
    QCam_CamListItem  list[10];
	unsigned long     listLen;
	const char        *functionName = "connectQImage";

	listLen = sizeof(list)/sizeof(QCam_CamListItem);

	// Start QImage
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Starting Camera Initialization\n\n", functionName);

	// Initialize QImage Driver
	status |= resultCode(functionName, "QCam_LoadDriver", QCam_LoadDriver());
	
	// Get a list of cameras
	if (!status) status |= resultCode(functionName, "QCam_ListCameras", QCam_ListCameras(list, &listLen));

	if (listLen < 1)
		return((asynStatus)-1);

	// Gets camera handle for first camera in list
	if (!status) status |= resultCode(functionName, "QCam_OpenCamera", QCam_OpenCamera(list[0].cameraId, &qHandle));
	
	if (!status) status |= getCameraInfo();
	
	if (!status) status |= callParamCallbacks();
	
	if (!status && isSettingsInit == false)
	{
		resultCode(functionName, "QCam_CreateCameraSettingsStruct", QCam_CreateCameraSettingsStruct(&qSettings));
		resultCode(functionName, "QCam_InitializeCameraSettings", QCam_InitializeCameraSettings(qHandle, &qSettings));
		isSettingsInit = true;
	}

	if (!status) status |= queryQImageSettings();

	////if (!status) status |= initializeQImage();

	return((asynStatus)status);
}

asynStatus QImage::getCameraInfo()
{
	const char        *functionName = "getCameraInfo";
	unsigned long     value;
	unsigned long     coolerEnabled;
	char              val[MAX_ARRAY_LEN];
	char              *avail[] = { "unavailable", "available" };
	int status = asynSuccess;

	if (!status) status |= resultCode(functionName, "GetSerialString", QCam_GetSerialString(qHandle, val, MAX_ARRAY_LEN));
	if (!status) status |= setStringParam(qSerialNumberRBV, val);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfBitDepth)", QCam_GetInfo(qHandle, qinfBitDepth, &value));
	if (!status) status |= setIntegerParam(qMaxBitDepthRBV, value);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfCcdHeight)", QCam_GetInfo(qHandle, qinfCcdHeight, &maxWidth));
	if (!status) status |= setIntegerParam(ADMaxSizeY, maxWidth);
	if (!status) status |= setIntegerParam(ADSizeY, maxWidth);
	if (!status) status |= setIntegerParam(NDArraySizeY, maxWidth);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfCcdWidth)", QCam_GetInfo(qHandle, qinfCcdWidth, &maxHeight));
	if (!status) status |= setIntegerParam(ADMaxSizeX, maxHeight);
	if (!status) status |= setIntegerParam(ADSizeX, maxHeight);
	if (!status) status |= setIntegerParam(NDArraySizeX, maxHeight);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfUniqueId)", QCam_GetInfo(qHandle, qinfUniqueId, &value));
	if (!status) status |= setIntegerParam(qUniqueIdRBV, value);
	if (!status) status |= resultCode(functionName, "QCam_GetCameraModelString", QCam_GetCameraModelString(qHandle, val, MAX_ARRAY_LEN));
	if (!status) status |= setStringParam(ADModel, val);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfCcdType)", QCam_GetInfo(qHandle, qinfCcdType, &value));
	switch (value)
	{
	case 0:
	case 1:
	case 2:
		if (!status)
			status |= setIntegerParam(qCcdTypeRBV, value);
		break;
	default:
		if (!status)
			status |= setIntegerParam(qCcdTypeRBV, 3);
		break;
	}
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfCooled)", QCam_GetInfo(qHandle, qinfCooled, &value));
	if (!status) coolerEnabled = value;
	if (!status) status |= setStringParam(qCooledRBV, avail[value]);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfRegulatedCooling)", QCam_GetInfo(qHandle, qinfRegulatedCooling, &value));
	if (!status) coolerReg = value;
	if (!status) status |= setStringParam(qRegulatedCoolingRBV, avail[value]);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfFanControl)", QCam_GetInfo(qHandle, qinfFanControl, &value));
	if (!status) status |= setStringParam(qFanControlRBV, avail[value]);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfHighSensitivityMode)", QCam_GetInfo(qHandle, qinfHighSensitivityMode, &value));
	if (!status) status |= setStringParam(qHighSensitivityModeRBV, avail[value]);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfBlackoutMode)", QCam_GetInfo(qHandle, qinfBlackoutMode, &value));
	if (!status) status |= setStringParam(qBlackoutModeRBV, avail[value]);
	if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfAsymmetricalBinning)", QCam_GetInfo(qHandle, qinfAsymmetricalBinning, &value));
	if (!status) status |= setStringParam(qAsymmetricalBinningRBV, avail[value]);

	return((asynStatus)status);
}

void QImage::consumerTask()
{
	int          status = asynSuccess;
	int          arrayCallbacks, imageCounter;
	const char   *functionName = "consumerTask";
	int imageMode;

	while (!exiting_)
	{
		captureEvent.wait();
		
		getIntegerParam(ADImageMode, &imageMode);

		capFrameMutex.lock();
		int cSize = (int)collectedFrames.size();
		capFrameMutex.unlock();

		//check collectedFrames
		for (int i = 0; i < cSize; i++)// (collectedFrames.size() > 0 && exiting_ == false && _adAcquire)
		{
			if (exiting_ != false && _adAcquire == false)
			{
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  QCam_Abort\n", functionName);
				status |= resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  QCam_Abort finished\n", functionName);

				break;
			}

			capFrameMutex.lock();
			//lock collectedFrames
			int frameId = collectedFrames.front();
			collectedFrames.pop();
			capFrameMutex.unlock();

			asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s:%s: processing frameId %d\n", driverName, functionName, frameId);

			/*sim code

			// Get the current parameters
			getIntegerParam(NDArrayCounter, &imageCounter);
			getIntegerParam(ADNumImages, &numImages);
			getIntegerParam(ADNumImagesCounter, &numImagesCounter);
			getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
			imageCounter++;
			numImagesCounter++;
			setIntegerParam(NDArrayCounter, imageCounter);
			setIntegerParam(ADNumImagesCounter, numImagesCounter);

			*/

			//proecess frame id
			status |= getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
			status |= getIntegerParam(NDArrayCounter, &imageCounter);
			status |= setIntegerParam(NDArrayCounter, ++imageCounter);
			status |= callParamCallbacks();
			status |= resultCode(functionName, "FrameCallback", pFrames[frameId].errorcode);

			if ((arrayCallbacks))// && (expCnt >= (int)pFrames[frameId].count))
			{
				// Put the frame number and time stamp into the buffer
				// Set the the start time
				epicsTimeGetCurrent(&startTime);
				pFrames[frameId].ndArray->uniqueId = imageCounter;
				pFrames[frameId].ndArray->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
				//this->pImage[task.usrData]->uniqueId = imageCounter;
				//this->pImage[task.usrData]->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
				// Get any attributes that have been defined for this driver
				this->getAttributes(pFrames[frameId].ndArray->pAttributeList);
				// Call the NDArray callback
				asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, //ASYN_TRACE_FLOW
					"%s:%s: calling NDArray callback\n", driverName, functionName);
				status |= doCallbacksGenericPointer(pFrames[frameId].ndArray, NDArrayData, 0);
			}
			_capturedFrames++;
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  captured %d of %d\n", functionName, _capturedFrames, _numImages);

			freeFrameMutex.lock();
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  pushing free frame %d\n", functionName, frameId);
			freeFrames.push(frameId);
			freeFrameMutex.unlock();

			if ( ( (imageMode == ADImageSingle) && (_capturedFrames >= 1) ) 
				|| ( (imageMode == ADImageMultiple) && (_capturedFrames >= _numImages) ) )
			{
				q_acquire(0);
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  QCam_Abort\n", functionName);
				status |= resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  QCam_Abort finished\n", functionName);

			}
			/*
			else
			{
				status |= resultCode(functionName, "QCam_Trigger", QCam_Trigger(qHandle));
			}
			*/
			/*
			// See if acquisition is done
			if ((imageMode == ADImageSingle) ||
			((imageMode == ADImageMultiple) &&
			(numImagesCounter >= numImages))) {

			// First do callback on ADStatus.
			setStringParam(ADStatusMessage, "Waiting for acquisition");
			setIntegerParam(ADStatus, ADStatusIdle);
			callParamCallbacks();

			acquire = 0;
			setIntegerParam(ADAcquire, acquire);
			asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
			"%s:%s: acquisition completed\n", driverName, functionName);
			}
			*/
			/* not needed
			if ((frameId >= 0) && (frameId<(unsigned long)num_buffs))
			{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "QImage frameTask call queueFrame\n");
			if (queueFrame(task.usrData) == asynError)
			{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Problem Queueing New Frame\n", functionName);
			}
			}
			*/

		}
	}
}

void QImage::frameTask()
{
	int          status = asynSuccess;
	//int          arrayCallbacks, imageCounter;
	const char   *functionName = "frameTask";
	int imageMode;

	bool procCollect;
	bool procAcquire;
	/*
	while (isInitialized == false)
	{
		epicsThreadSleep(0.01);
	}
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: initialization finished. Frame task starting\n", driverName, functionName);
	*/
	while (!exiting_)
	{
		procCollect = false;
		procAcquire = false;
		

		getIntegerParam(ADImageMode, &imageMode);
		
		if (_adAcquire)
		{
			freeFrameMutex.lock();
			if (freeFrames.size() > 0)
			{
				if ( ( (imageMode == ADImageSingle) && (_pushedFrames < 1) )
					|| ( (imageMode == ADImageMultiple) && (_pushedFrames < _numImages) )
					|| imageMode == ADImageContinuous)
				{
					//freeFrameMutex.lock();
					int frameId = freeFrames.front();
					asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  pop free frame %d\n", functionName, frameId);
					freeFrames.pop();
					freeFrameMutex.unlock();
					asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Calling QCam_QueueFrame for frameId %d\n", functionName, frameId);
					//status |= resultCode(functionName, "QCam_QueueFrame", QCam_QueueFrame(qHandle, pFrames[frameId].qFrame, QImageCallback, qcCallbackDone | qcCallbackExposeDone, this, frameId));
					
					status |= resultCode(functionName, "QCam_QueueFrame", QCam_QueueFrame(qHandle, pFrames[frameId].qFrame, QImageCallback, qcCallbackDone, this, frameId));
					if (triggerType == qcTriggerSoftware)
						status |= resultCode(functionName, "QCam_Trigger", QCam_Trigger(qHandle));
					
					// After frame #1, we queue settings #2.
					//pSettings = (frame == 2) ? &m_settings.expFlip1 : &m_settings.expFlip2;
					//status |= resultCode(functionName, "QCam_QueueSettings", QCam_QueueSettings(qHandle, (QCam_Settings*)&qSettings, QImageCallback, qcCallbackDone, this, frameId));
					_pushedFrames++;
					/*
					if (imageMode == ADImageSingle || ((imageMode == ADImageMultiple) && _pushedFrames >= _numImages))
					{
					_adAcquire = false;
					}
					*/
				}
				else
				{
					freeFrameMutex.unlock();
					if (triggerType == qcTriggerSoftware)
					{
						status |= resultCode(functionName, "QCam_Trigger", QCam_Trigger(qHandle));
					}
					epicsThreadSleep(camPushSleepAmt);
				}
				procAcquire = true;
			}
			else
			{
				freeFrameMutex.unlock();
				if (triggerType == qcTriggerSoftware)
				{
					status |= resultCode(functionName, "QCam_Trigger", QCam_Trigger(qHandle));
				}
				epicsThreadSleep(camPushSleepAmt);
			}

		}
		if (false == _adAcquire && false == procCollect && false == procAcquire)
		{
			epicsThreadSleep(camPushSleepAmt);
		}
	}
}

asynStatus QImage::initializeFrames()
{
	int				status = asynSuccess;
	size_t			dims[2] = { 2048, 2048 };
	NDDataType_t	dataType = NDUInt8;
	//int colorMode;
	int minX, sizeX;
	int minY, sizeY;
	unsigned long roiX, roiY, roiWidth, roiHeight, imageFormat, binVal;
	//int binning;
	const char    *functionName = "initializeFrames";

	q_acquire(0);
	//status |= resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));


	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &roiX);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &roiY);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &roiWidth);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &roiHeight);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmImageFormat, &imageFormat);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmBinning, &binVal);
	
	minX = (int)roiX;
	minY = (int)roiY;
	sizeX = (int)(roiWidth);
	sizeY = (int)(roiHeight);
	
	dims[0] = sizeX;
	dims[1] = sizeY;
	
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: minX = %d, minY = %d, sizeX = %d, sizeY = %d, roiW %u, roiH %u, bin %u \n\n", functionName, minX, minY, sizeX, sizeY, roiWidth, roiHeight, binVal);
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: dim[0] = %u, dim[1] = %u \n\n", functionName, dims[0], dims[1]);

	switch (imageFormat)
	{
	case qfmtMono8:
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode mono 8\n", functionName);
		dataType = NDUInt8;
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt8));
		break;
	case qfmtBayer8:
		dataType = NDUInt8;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode bayer 8\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt8));
		break;
	case qfmtRgbPlane8:
		dataType = NDUInt8;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode rgb 8\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt8));
		break;
	case qfmtMono16:
		dataType = NDUInt16;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode mono 16\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt16));
		break;
	case qfmtBayer16:
		dataType = NDUInt16;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode bayer 16\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt16));
		break;
	case qfmtRgbPlane16:
		dataType = NDUInt16;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode rgb 16\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt16));
		break;
	case qfmtBgr24:
		dataType = NDUInt32;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode bgr 24\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt32));
		break;
	case qfmtRgb24:
		dataType = NDUInt32;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode rgb 24\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt32));
		break;
	case qfmtXrgb32:
		dataType = NDUInt32;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode xrgb 32\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt32));
	case qfmtBgrx32:
		dataType = NDUInt32;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode bgrx 32\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsUInt32));
		break;
	case qfmtRgb48:
		dataType = NDFloat64;
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  getting camera color mode rgb 48\n", functionName);
		rawDataSize = (unsigned long)(dims[0] * dims[1] * sizeof(epicsFloat64));
		break;
	default:
		rawDataSize = -1;
		break;
	}

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: num_buffs = %d \n\n", functionName, num_buffs);
	if (pFrames.size() == num_buffs)
	{
		for (int i = 0; i < num_buffs; i++)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Deallocate NDArray[%d] \n\n", functionName, i);
			pFrames[i].ndArray->release();
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Attepting to allocate NDArray[%d]: %u x %u \n\n", functionName, i, dims[0], dims[1]);
			pFrames[i].ndArray = pNDArrayPool->alloc(2, (size_t*)dims, dataType, 0, NULL);
			pFrames[i].qFrame->bufferSize = rawDataSize;
			pFrames[i].qFrame->pBuffer = pFrames[i].ndArray->pData;
		}
	}
	else
	{
		freeFrameMutex.lock();
		for (int i = 0; i < num_buffs; i++)
		{
			QNDFrame frame;
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Attepting to allocate NDArray[%d]: %u x %u \n\n", functionName, i, dims[0], dims[1]);
			frame.ndArray = pNDArrayPool->alloc(2, (size_t*)dims, dataType, 0, NULL);
			if (frame.ndArray)
			{
				frame.qFrame = new QCam_Frame();
				frame.qFrame->bufferSize = rawDataSize;
				frame.qFrame->pBuffer = frame.ndArray->pData;
				pFrames.push_back(frame);
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  pushing free frame %d\n", functionName, i);
				freeFrames.push(i);
			}
			else
			{
				asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR allocating frame buffer from NDArrayPool\n\n", functionName);
				return asynError;
			}
		}
		freeFrameMutex.unlock();
	}
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: frame buffer initialized\n", functionName);


	status |= setIntegerParam(ADMinX, minX);
	status |= setIntegerParam(ADMinY, minY);
	status |= setIntegerParam(NDArraySizeX, sizeX);
	status |= setIntegerParam(ADSizeX, sizeX);
	status |= setIntegerParam(NDArraySizeY, sizeY);
	status |= setIntegerParam(ADSizeY, sizeY);
	status |= setIntegerParam(qImageFormat, imageFormat);
	status |= setIntegerParam(NDArraySize, rawDataSize); 
	status |= setIntegerParam(NDDataType, dataType);
	status |= callParamCallbacks();

	return((asynStatus)status);

}

asynStatus QImage::initializeQImage()
{
	int           status = asynSuccess;
	int           detStatus;
	int           ivalue;
	double        dvalue;
	signed long   svalue;
	unsigned long uvalue;
	const char    *functionName = "initializeQImage";

	status |= getIntegerParam(ADStatus, &detStatus);
	if (detStatus != ADStatusIdle)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: ERROR: Detector not Idle\n\n", functionName);
		return(asynError);
	}
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Initializing Detector\n\n", functionName);
		status |= setIntegerParam(ADStatus, ADStatusWaiting);
		status |= callParamCallbacks();
	}
	if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	if (!status) status |= getDoubleParam(ADAcquireTime, &dvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmExposure)", QCam_SetParam((QCam_Settings*)&qSettings, qprmExposure, (unsigned long)(dvalue*1e6)));
	if (!status) status |= getDoubleParam(ADGain, &dvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmNormalizedGain)", QCam_SetParam((QCam_Settings*)&qSettings, qprmNormalizedGain, (unsigned long)(dvalue*1e6)));
	if (!status) status |= getIntegerParam(qOffset, (int *)&svalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParamS32(qprmS32AbsoluteOffset)", QCam_SetParamS32((QCam_Settings*)&qSettings, qprmS32AbsoluteOffset, svalue));
	if (!status) status |= getIntegerParam(qImageFormat, (int *)&uvalue);
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n-----\n%s: Querying Image type %d\n-----\n", functionName, uvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmImageFormat)", QCam_SetParam((QCam_Settings*)&qSettings, qprmImageFormat, uvalue));
	if (!status) status |= getIntegerParam(ADTriggerMode, &ivalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmTriggerType)", QCam_SetParam((QCam_Settings*)&qSettings, qprmTriggerType, ivalue));
	if (!status) status |= getIntegerParam(qReadoutSpeed, &ivalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmReadoutSpeed)", QCam_SetParam((QCam_Settings*)&qSettings, qprmReadoutSpeed, ivalue));
	if (!status) status |= getIntegerParam(ADMinX, (int *)&uvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmRoiX)", QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiX, uvalue));
	if (!status) status |= getIntegerParam(ADMinY, (int *)&uvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmRoiY)", QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiY, uvalue));
	if (!status) status |= getIntegerParam(ADSizeX, (int *)&uvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmRoiWidth)", QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiWidth, uvalue));
	if (!status) status |= getIntegerParam(ADSizeY, (int *)&uvalue);
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmRoiHeight)", QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiHeight, uvalue));
	if (!status) status |= resultCode(functionName, "QCam_GetParam(qprmCoolerActive)", QCam_GetParam((QCam_Settings*)&qSettings, qprmCoolerActive, &uvalue));
	if (uvalue)
	{
		if (coolerReg)
		{
			if (!status) status |= getDoubleParam(ADTemperature, &dvalue);
			if (!status) status |= resultCode(functionName, "QCam_SetParamS32(qprmS32RegulatedCoolingTemp)", QCam_SetParamS32((QCam_Settings*)&qSettings, qprmS32RegulatedCoolingTemp, (signed long)dvalue));
		}
	}

	if (!status) status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));

	if (queryQImageSettings() != asynSuccess)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: ERROR: Problem Querying Detector\n\n", functionName);
		status |= asynError;
	}
	else
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Detector Queried\n\n", functionName);
	/*
	if (initializeBuffers() != asynSuccess)
	{
	asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: ERROR: Problem Initializing Buffers\n\n", functionName);
	status |= asynError;
	}
	else
	asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: Buffers Initialized\n\n", functionName);
	*/
	expSntl = 1;
	trgCnt = expCnt = frmCnt = 0;
	status |= setIntegerParam(qTrgCnt, trgCnt);
	status |= setIntegerParam(qExpCnt, expCnt);
	status |= setIntegerParam(qFrmCnt, frmCnt);
	status |= setIntegerParam(ADStatus, ADStatusIdle);
	status |= callParamCallbacks();

	if (status)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: ERROR: Problem Initializing Detector\n\n", functionName);
	}
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Detector Initialized\n\n", functionName);
	}

	return((asynStatus)status);
}

// Function called at IOC exit
void QImage::shutdown()
{
	const char *functionName = "shutdown";
	exiting_ = 1;
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Shutting down the IOC\n", functionName);
	if (disconnectQImage() != asynSuccess) {
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Unable to Disconnect Detector\n", functionName);
	}else
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Detector Disconnected\n", functionName);

	for (int i = 0; i < collectedFrames.size(); i++)
		collectedFrames.pop();

	captureEvent.signal();
}

asynStatus QImage::disconnectQImage()
{
	int        status        = asynSuccess;
	const char *functionName = "disconnectQImage";

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Disconnecting the detector\n", functionName);
	
	isSettingsInit = false;
	//release settings
	status |= resultCode(functionName, "QCam_ReleaseCameraSettingsStruct", QCam_ReleaseCameraSettingsStruct(&qSettings));

	// Terminate QImage
	//for (int i = 0; i < num_buffs; i++)
	//{
	/*
	for (std::vector<qFrame*>::iterator itr = pImage.begin(); itr != pImage.end(); itr++)
	{
	//	if (pImage[i])
		if ( *itr )
		{
			//pImage[i]->release();
			(*itr)->release();
		}
	}
	
	pImage.clear();
	delete [] qFrame.frame;
	*/
	// Release handle when finished
	if (qHandle) 
	{
		if (!status) status |= resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));
		if (!status) status |= resultCode(functionName, "QCam_SetStreaming", QCam_SetStreaming(qHandle, false));
		if (!status) status |= resultCode(functionName, "QCam_CloseCamera", QCam_CloseCamera(qHandle));
		QCam_ReleaseDriver();
	} 
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: No camera handle found\n", functionName);
		status = asynError;
	}
	

	qHandle = 0;

	return((asynStatus)status);
}

void QImage::pushCollectedFrame(int id)
{
	capFrameMutex.lock();
	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s: pushing frameId %d\n", driverName, id);
	collectedFrames.push(id);
	capFrameMutex.unlock();
	captureEvent.signal();
}

asynStatus QImage::queryQImageSettings()
{
	int           status = asynSuccess;
	unsigned long uvalue;
	signed long   svalue;
	//unsigned long uTable[32];
	//int tableSize = 32;
	const char    *functionName = "queryQImageSettings";

	char *strings[MAX_ENUM_STATES];
	int values[MAX_ENUM_STATES];
	int severities[MAX_ENUM_STATES];
	int count = 0;

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	//QCam_GetParam64
	//QCam_IsRangeTable
	//QCam_IsSparseTable
	//QCam_GetParamMin
	//QCam_GetParamMax
	//QCam_GetParamSparseTable

	//exposure time is 64 bit

	/**range parameter types
	offset
	region of interest
	intensifier gain
	trigger delay
	normalized gain
	normalized intensifier gain
	absolute offset
	**/

	/**sparse parameter types
	cooling 
	readout speed
	shutter state
	sync b mode
	binning
	trigger type
	image format
	color wheel
	camera mode
	**/

	if (QCam_IsSparseTable((QCam_Settings*)&qSettings, qprmBinning) == qerrSuccess)
	{
		QCam_GetParamSparseTable((QCam_Settings*)&qSettings, qprmBinning, &binningTable[0], &binningTableSize);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\nBinning options\n");
		for (int i = 0; i < binningTableSize; i++)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%ld x %ld\n", binningTable[i], binningTable[i]);
		}
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n");
	}

	if (QCam_IsSparseTable((QCam_Settings*)&qSettings, qprmImageFormat) == qerrSuccess)
	{
		QCam_GetParamSparseTable((QCam_Settings*)&qSettings, qprmImageFormat, &imageFormatTable[0], &imageFormatTableSize);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\nImage Format options\n");
		for (int i = 0; i < imageFormatTableSize; i++)
		{
			switch (imageFormatTable[i])
			{
			case qfmtRaw8:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Raw 8\n");
				break;
			case qfmtRaw16:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Raw 16\n");
				break;
			case qfmtMono8:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Mono 8\n");
				break;
			case qfmtMono16:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Mono 16\n");
				break;
			case qfmtBayer8:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Bayer 8\n");
				break;
			case qfmtBayer16:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Bayer 16\n");
				break;
			case qfmtRgbPlane8:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "RGB Plane 8\n");
				break;
			case qfmtRgbPlane16:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "RGB Plane 16\n");
				break;
			case qfmtBgr24:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "BGR 24\n");
				break;
			case qfmtXrgb32:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "XRGB 32\n");
				break;
			case qfmtRgb48:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "RGB 48\n");
				break;
			case qfmtBgrx32:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "BGRX 32\n");
				break;
			case qfmtRgb24:
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "RGB 24\n");
				break;
			}
			
		}
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n");
	}

	/*
	if (QCam_IsSparseTable((QCam_Settings*)&qSettings, qprmReadoutSpeed) == qerrSuccess)
	{
		QCam_GetParamSparseTable((QCam_Settings*)&qSettings, qprmReadoutSpeed, &uTable[0], &tableSize);
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n ReadoutSpeed options\n");
		for (int i = 0; i < tableSize; i++)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%ld\n", uTable[i]);
		}
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n");
	}
	*/

	if (!status) status |= resultCode(functionName, "QCam_ReadDefaultSettings", QCam_ReadDefaultSettings(qHandle, (QCam_Settings*)&qSettings));
	if (!status) status |= resultCode(functionName, "QCam_GetParamMax(qprmExposure)", QCam_GetParamMax((QCam_Settings*)&qSettings, qprmExposure, &uvalue));
	if (!status) expMax = uvalue / 1.0e6;
	if (!status) status |= setDoubleParam(qExposureMax, expMax);
	if (!status) status |= resultCode(functionName, "QCam_GetParamMin(qprmExposure)", QCam_GetParamMin((QCam_Settings*)&qSettings, qprmExposure, &uvalue));
	if (!status) expMin = uvalue / 1.0e6;
	if (!status) status |= setDoubleParam(qExposureMin, expMin);
	if (!status) status |= resultCode(functionName, "QCam_GetParamMax(qprmNormalizedGain)", QCam_GetParamMax((QCam_Settings*)&qSettings, qprmNormalizedGain, &uvalue));
	if (!status) gainMax = uvalue / 1.0e6;
	if (!status) status |= setDoubleParam(qGainMax, gainMax);
	if (!status) status |= resultCode(functionName, "QCam_GetParamMin(qprmNormalizedGain)", QCam_GetParamMin((QCam_Settings*)&qSettings, qprmNormalizedGain, &uvalue));
	if (!status) gainMin = uvalue / 1.0e6;
	if (!status) status |= setDoubleParam(qGainMin, gainMin);
	if (!status) status |= resultCode(functionName, "QCam_GetParamS32Max(qprmS32AbsoluteOffset)", QCam_GetParamS32Max((QCam_Settings*)&qSettings, qprmS32AbsoluteOffset, &offsetMax));
	if (!status) status |= resultCode(functionName, "QCam_GetParamS32Min(qprmS32AbsoluteOffset)", QCam_GetParamS32Min((QCam_Settings*)&qSettings, qprmS32AbsoluteOffset, &offsetMin));
	if (coolerReg) {
		if (!status) status |= resultCode(functionName, "QCam_GetParamS32Max(qprmS32RegulatedCoolingTemp)", QCam_GetParamS32Max((QCam_Settings*)&qSettings, qprmS32RegulatedCoolingTemp, &tempMax));
		if (!status) status |= resultCode(functionName, "QCam_GetParamS32Min(qprmS32RegulatedCoolingTemp)", QCam_GetParamS32Min((QCam_Settings*)&qSettings, qprmS32RegulatedCoolingTemp, &tempMin));
	}
	else
	{
		tempMax = 0;
		tempMin = 0;
	}

	// Needed for async acq
	if (!status) status |= resultCode(functionName, "QCam_SetStreaming", QCam_SetStreaming(qHandle, true));

	if (!status) {
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n");
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: expMax    = %.6f\n", functionName, expMax);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: expMin    = %.6f\n", functionName, expMin);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: gainMax   = %.6f\n", functionName, gainMax);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: gainMin   = %.6f\n", functionName, gainMin);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: offsetMax = %ld\n", functionName, offsetMax);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: offsetMin = %ld\n", functionName, offsetMin);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: tempMax   = %ld\n", functionName, tempMax);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: tempMin   = %ld\n", functionName, tempMin);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n");
	}

	////////////


	status |= resultCode(functionName, "QCam_GetParam(qprmExposure)", QCam_GetParam((QCam_Settings*)&qSettings, qprmExposure, &uvalue));
	status |= setDoubleParam(qAcquireTimeRBV, uvalue / 1.0e6);
	status |= setDoubleParam(ADAcquireTime, uvalue / 1.0e6);
	status |= resultCode(functionName, "QCam_GetParam(qprmRoiX)", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &uvalue));
	status |= setIntegerParam(qMinXRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmRoiY)", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &uvalue));
	status |= setIntegerParam(qMinYRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmRoiWidth)", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &uvalue));
	status |= setIntegerParam(qSizeXRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmRoiHeight)", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &uvalue));
	status |= setIntegerParam(qSizeYRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmTriggerType)", QCam_GetParam((QCam_Settings*)&qSettings, qprmTriggerType, &uvalue));
	status |= setIntegerParam(qTriggerModeRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmNormalizedGain)", QCam_GetParam((QCam_Settings*)&qSettings, qprmNormalizedGain, &uvalue));
	status |= setDoubleParam(qGainRBV, uvalue / 1.0e6);
	status |= setDoubleParam(ADGain, uvalue / 1.0e6);
	status |= resultCode(functionName, "QCam_GetParam(qprmReadoutSpeed)", QCam_GetParam((QCam_Settings*)&qSettings, qprmReadoutSpeed, &uvalue));
	status |= setIntegerParam(qReadoutSpeedRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParamS32(qprmS32AbsoluteOffset)", QCam_GetParamS32((QCam_Settings*)&qSettings, qprmS32AbsoluteOffset, &svalue));
	status |= setIntegerParam(qOffsetRBV, svalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmImageFormat)", QCam_GetParam((QCam_Settings*)&qSettings, qprmImageFormat, &uvalue));
	status |= setIntegerParam(qImageFormatRBV, uvalue);
	//asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Camera Image Type is %d\n", functionName, uvalue);
	switch (uvalue)
	{
		case qfmtMono8:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is Mono 8 bit\n", functionName);
			break;
		case qfmtMono16:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is Mono 16 bit\n", functionName);
			break;
		case qfmtBayer8:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is Bayer 8 bit\n", functionName);
			break;
		case qfmtBayer16:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is Bayer 16 bit\n", functionName);
			break;
		case qfmtRgbPlane8:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is RGB Plane 8 bit\n", functionName);
			break;
		case qfmtRgbPlane16:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is RGB Plane 16 bit\n", functionName);
			break;
		default:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Camera Image Type is unknown\n", functionName);
			break;
	}
	//status |= setIntegerParam(qImageFormatRBV, uvalue);
	status |= resultCode(functionName, "QCam_GetParam(qprmCoolerActive)", QCam_GetParam((QCam_Settings*)&qSettings, qprmCoolerActive, &uvalue));
	status |= setIntegerParam(qCoolerActiveRBV, uvalue);
	if (uvalue) {
		if (coolerReg) {
			status |= resultCode(functionName, "QCam_GetParamS32(qprmS32RegulatedCoolingTemp)", QCam_GetParamS32((QCam_Settings*)&qSettings, qprmS32RegulatedCoolingTemp, &svalue));
			status |= setDoubleParam(qTemperatureRBV, (double)svalue);
			status |= resultCode(functionName, "QCam_GetInfo(qinfRegulatedCoolingLock)", QCam_GetInfo(qHandle, qinfRegulatedCoolingLock, &uvalue));
			status |= setIntegerParam(qRegulatedCoolingLockRBV, uvalue);
		}
		else {
			status |= setDoubleParam(qTemperatureRBV, -1000);
			status |= setIntegerParam(qRegulatedCoolingLockRBV, 0);
		}
	}
	else {
		status |= setDoubleParam(qTemperatureRBV, -1000);
		status |= setIntegerParam(qRegulatedCoolingLockRBV, 0);
	}

	if (!status) status |= callParamCallbacks();

	if (!status) status |= doCallbacksEnum(strings, values, severities, count, qBinning, 0);

	return((asynStatus)status);
}

//  * Report status of the driver.
//  * Prints details about the driver if details>0.
//  * It then calls the ADDriver::report() method.
//  * \param[in] fp File pointed passed by caller where the output is written to.
//  * \param[in] details If >0 then driver details are printed.

asynStatus QImage::q_acquire(epicsInt32 value)
{
	epicsInt32 status = asynSuccess;
	int	tMode, eventStatus;

	const char     *functionName = "q_acquire";

	status |= getIntegerParam(ADStatus, &adStatus);
	status |= getIntegerParam(ADTriggerMode, &tMode);
	if (value)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  acquire called\n", functionName);

		_pushedFrames = 0;
		_capturedFrames = 0;
		status |= getIntegerParam(ADNumImages, &_numImages);

		if (adStatus == ADStatusIdle || _adAcquire == false)
		{
			resetFrameQueues();

			_adAcquire = true;
			status |= setStringParam(qExposureStatusMessageRBV, "Waiting");
			status |= setStringParam(qFrameStatusMessageRBV, "Waiting");
			trgCnt = expCnt = frmCnt = 0;
			status |= setIntegerParam(ADStatus, ADStatusAcquire);
			status |= setIntegerParam(qTrgCnt, trgCnt);
			status |= setIntegerParam(qExpCnt, expCnt);
			status |= setIntegerParam(qFrmCnt, frmCnt);
			status |= setIntegerParam(ADNumImagesCounter, 0);
			eventStatus = epicsEventTryWait(this->stopEventId);
			if (eventStatus == epicsEventWaitOK)
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: stopEvent cleared\n", functionName);
			if (tMode == qcTriggerSoftware)
			{
				asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  cam trigger called\n", functionName);
				status |= resultCode(functionName, "QCam_Trigger", QCam_Trigger(qHandle));
				trgCnt++;
				status |= setIntegerParam(qTrgCnt, trgCnt);
			}
		}
		else
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  Camera already acquiring\n", functionName);
	}
	else
	{
		_adAcquire = false;
		
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  acquire stop called\n", functionName);
		// This was a command to stop acquisition 
		// Send the stop event 
		if (tMode == qcTriggerSoftware)
			epicsEventSignal(this->stopEventId);

		if (triggerType != qcTriggerSoftware)
		{
			status |= resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));
		}
		
		//clear queues
		status |= setIntegerParam(ADStatus, ADStatusIdle);
		status |= setIntegerParam(ADAcquire, 0);
	}

	status |= callParamCallbacks();

	return((asynStatus)status);

}

asynStatus QImage::q_setTriggerMode(epicsInt32 value)
{
	epicsInt32 status = asynSuccess;
	const char     *functionName = "q_setTriggerMode";

	//status |= getIntegerParam(ADStatus, &adStatus);
	//status |= getIntegerParam(ADTriggerMode, &tMode);

	if (value >= qcTrigger_last || value < 0)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:  invalid triggertype value %d, max = %d\n", functionName, value, qcTrigger_last);
		status = asynError;
	}
	else
	{
		switch (value)
		{
		case qcTriggerFreerun:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerFreerun\n", functionName);
			break;
		case qcTriggerEdgeHi:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerEdgeHi\n", functionName);
			break;
		case qcTriggerEdgeLow:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerEdgeLow\n", functionName);
			break;
		case qcTriggerPulseHi:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerPulseHi\n", functionName);
			break;
		case qcTriggerPulseLow:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerPulseLow\n", functionName);
			break;
		case qcTriggerSoftware:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerSoftware\n", functionName);
			break;
		case qcTriggerStrobeHi:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerStrobeHi\n", functionName);
			break;
		case qcTriggerStrobeLow:
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  setting triggertype to qcTriggerStrobeLow\n", functionName);
			break;
		}
		
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		QCam_SetParam((QCam_Settings*)&qSettings, qprmTriggerType, value);
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		triggerType = value;

		status |= callParamCallbacks();
	}
	return((asynStatus)status);

}

void QImage::resetFrameQueues()
{
	const char     *functionName = "resetFrameQueues";

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  QCam_Abort\n", functionName);
	resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  QCam_Abort finished\n", functionName);

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  clearing captured frames\n", functionName);
	capFrameMutex.lock();
	for (int j = 0; j < collectedFrames.size(); j++)
	{
		collectedFrames.pop();
	}
	capFrameMutex.unlock();
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  cleared collected frames\n", functionName);
	freeFrameMutex.lock();
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  clearing free frames\n", functionName);
	int cnt = (int)freeFrames.size();
	for (int j = 0; j < cnt; j++)
	{
		//asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:  poping free frames %d\n", functionName, freeFrames.front());
		freeFrames.pop();
	}
	for (int i = 0; i < num_buffs; i++)
	{
		//asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:  pushing free frame %d\n", functionName, i);
		freeFrames.push(i);
	}
	freeFrameMutex.unlock();

	_pushedFrames = 0;
	_capturedFrames = 0;
}

asynStatus QImage::q_setBinning(epicsInt32 function, epicsInt32 value)
{

	epicsInt32 status = asynSuccess;
	const char     *functionName = "q_setBinning";
	bool foundBin = false;

	if (value > binningTableSize)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: binning value %d does not exist for this detector\n", functionName, value);
	}
	else
	{
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		QCam_SetParam((QCam_Settings*)&qSettings, qprmBinning, binningTable[value]);
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		//status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		//QCam_GetParam((QCam_Settings*)&qSettings, qprmBinning, &binVal);

		if (!status) status |= initializeFrames();
	}
	
	return((asynStatus)status);
}

asynStatus QImage::q_setImageSize(epicsInt32 function, epicsInt32 value)
{

	epicsInt32 status = asynSuccess;
	const char     *functionName = "q_setImageSize";
	unsigned long camVal, roiX, roiY, roiWidth, roiHeight;

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &roiX);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &roiY);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &roiWidth);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &roiHeight);

	if (function == ADSizeX)
	{
		if ((unsigned long)value > maxWidth)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size x to %d, max image size x = %d\n", functionName, value, maxWidth);
			return asynError;
		}
		if ((unsigned long)value > (maxWidth - roiX))
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size: maxwidth[%ul] - roix[%ul] < value [%d] \n", functionName, maxWidth, roiX, value);
			return asynError;
		}
		QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiWidth, value);
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &camVal);

	}
	else if (function == ADSizeY)
	{
		
		if ((unsigned long)value > maxHeight)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size y to %d, max image size y = %d\n", functionName, value, maxHeight);
			return asynError;
		}
		if ((unsigned long)value > (maxHeight - roiY))
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size: maxwidth[%ul] - roix[%ul] < value [%d] \n", functionName, maxHeight, roiY, value);
			return asynError;
		}
		QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiHeight, value);
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &camVal);
	}
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: unknown function for image size change\n", functionName);
		return asynError;
	}

	if (camVal != value)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size from %d to %d\n", functionName, camVal, value);
		status = asynError;
	}
	else
	{
		status |= initializeFrames();
	}

	return((asynStatus)status);
}

asynStatus QImage::q_setMinXY(epicsInt32 function, epicsInt32 value)
{

	epicsInt32 status = asynSuccess;
	const char     *functionName = "q_setMinXY";
	unsigned long camVal, roiX, roiY, roiWidth, roiHeight;

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &roiX);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &roiY);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &roiWidth);
	QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &roiHeight);

	if (value < 0)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Error! Cannot have min less than 0\n", functionName, camVal, value);
		return asynError;
	}

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	if (function == ADMinX)
	{
		if ((unsigned long)value > maxWidth)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size x to %d, max image size x = %d\n", functionName, value, maxWidth);
			return asynError;
		}

		QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiX, value);
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &camVal);

	}
	else if (function == ADMinY)
	{
		if ((unsigned long)value > maxHeight)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting image size y to %d, max image size y = %d\n", functionName, value, maxHeight);
			return asynError;
		}

		QCam_SetParam((QCam_Settings*)&qSettings, qprmRoiY, value);
		status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &camVal);
	}
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: unknown function for min ROI change\n", functionName);
		return asynError;
	}

	if (camVal != value)
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: error setting min ROI from %d to %d\n", functionName, camVal, value);
		status = asynError;
	}
	else
	{
		status |= initializeFrames();
	}

	return((asynStatus)status);
}

asynStatus QImage::q_setDataTypeAndColorMode(epicsInt32 function, epicsInt32 value)
{
	epicsInt32 status = asynSuccess;
	const char     *functionName = "q_setDataTypeAndColorMode";
	bool foundFormat = false; 

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	for (int i = 0; i < imageFormatTableSize; i++)
	{
		if (value == imageFormatTable[i])
		{
			//found supported image format
			status |= resultCode(functionName, "QCam_SetParam(qprmImageFormat)", QCam_SetParam((QCam_Settings*)&qSettings, qprmImageFormat, imageFormatTable[i]));
			foundFormat = true;
			break;
		}
	}
	 if (foundFormat)
	 {
		 resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
		 status |= initializeFrames();
	 }
	 else
	 {
		 asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:  Unsupported image format!\n", functionName);
		 status = asynError;
	 }
	
	return((asynStatus)status);
}

asynStatus QImage::q_setTemperature(epicsFloat64 value)
{

	epicsInt32 status = asynSuccess;
	const char     *functionName = "q_setTemperature";
	signed long currentTemp;
	double dvalue;
	unsigned long uvalue;

	if (coolerReg)
	{
		status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Setting temp to %d\n\n", functionName, value);

		if (!status) status |= getDoubleParam(ADTemperature, &dvalue);
		if (!status) status |= resultCode(functionName, "QCam_SetParamS32(qprmS32RegulatedCoolingTemp)", QCam_SetParamS32((QCam_Settings*)&qSettings, qprmS32RegulatedCoolingTemp, (signed long)dvalue));
		if (!status) status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));


		if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		if (!status) status |= resultCode(functionName, "QCam_GetParamS32(qprmS32RegulatedCoolingTemp)", QCam_GetParamS32((QCam_Settings*)&qSettings, qprmS32RegulatedCoolingTemp, &currentTemp));
		if (!status) status |= setDoubleParam(qTemperatureRBV, (double)currentTemp);

		if (!status) status |= resultCode(functionName, "QCam_GetInfo(qinfRegulatedCoolingLock)", QCam_GetInfo(qHandle, qinfRegulatedCoolingLock, &uvalue));
		if (!status) status |= setIntegerParam(qRegulatedCoolingLockRBV, uvalue);
	}
	else
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: No cooler supported\n\n", functionName);
		return asynError;
	}
	
	return((asynStatus)status);

}

asynStatus QImage::q_resetCamera(epicsInt32 value)
{
	const char     *functionName = "q_resetCamera";

	if (disconnectQImage() == asynSuccess) asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Camera Disconnected\n\n", functionName);
	expSntl = 0;
	// Allocate buffers
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Allocating Buffers\n\n", functionName);
	
	q_acquire(0);

	if ( connectQImage() == asynSuccess )
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Camera Connected\n\n", functionName);
	else
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: Camera NOT Connected\n\n", functionName);

	return initializeQImage();
}

asynStatus QImage::q_setCoolerActive(epicsInt32 value)
{
	const char     *functionName = "q_setCoolerActive";
	epicsInt32 status = asynSuccess;
	unsigned long uvalue;
	
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "\n\n%s: Setting Cooler Active to %d\n\n", functionName, value);

	if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmCoolerActive)", QCam_SetParam((QCam_Settings*)&qSettings, qprmCoolerActive, value));
	if (!status) status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));

	//check settings
	if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	if (!status) status |= resultCode(functionName, "QCam_GetParam(qprmCoolerActive)", QCam_GetParam((QCam_Settings*)&qSettings, qprmCoolerActive, &uvalue));
	if (!status) status |= setIntegerParam(qCoolerActiveRBV, uvalue);

	return((asynStatus)status);
}

asynStatus QImage::q_setReadoutSpeed(epicsInt32 value)
{
	const char     *functionName = "q_setCoolerActive";
	epicsInt32 status = asynSuccess;
	unsigned long uvalue;
	int ivalue;

	if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	
	if ((value != 0) && (value != 1))
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Invalid Readout Speed\n", functionName);
		status |= setIntegerParam(qReadoutSpeed, 1);
		return asynError;
	}

	if (!status) status |= getIntegerParam(qReadoutSpeed, &ivalue);
	uvalue = ivalue;
	if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmReadoutSpeed)", QCam_SetParam((QCam_Settings*)&qSettings, qprmReadoutSpeed, uvalue));
	if (!status) status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));

	//check settings
	if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	if (!status) status |= resultCode(functionName, "QCam_GetParam(qprmCoolerActive)", QCam_GetParam((QCam_Settings*)&qSettings, qprmCoolerActive, &uvalue));
	if (!status) status |= setIntegerParam(qReadoutSpeed, uvalue);

	return((asynStatus)status);
}

asynStatus QImage::q_autoExposure(epicsInt32 value)
{
	const char     *functionName = "q_autoExposure";
	epicsInt32 status = asynSuccess;
	unsigned long roiX, roiY, roiWidth, roiHeight, oldExp, newExp;
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Calling Auto Expose\n", functionName);

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	if (!status) status |= resultCode(functionName, "QCam_GetParam(qprmExposure)", QCam_GetParam((QCam_Settings*)&qSettings, qprmExposure, &oldExp));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiX", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &roiX));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiY", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &roiY));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiWidth", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &roiWidth));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiHeight", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &roiHeight));
	if (!status) status |= resultCode(functionName, "QCam_AutoExpose", QCam_AutoExpose((QCam_Settings*)&qSettings, roiX, roiY, roiWidth, roiHeight));
	if (!status) status |= resultCode(functionName, "QCam_GetParam(qprmExposure)", QCam_GetParam((QCam_Settings*)&qSettings, qprmExposure, &newExp));
	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Old exposure %f, New Exposure %f\n", functionName, oldExp / 1.0e6, newExp / 1.0e6);

	return((asynStatus)status);
}

asynStatus QImage::q_whiteBalance(epicsInt32 value)
{
	const char     *functionName = "q_whiteBalance";
	epicsInt32 status = asynSuccess;
	unsigned long roiX, roiY, roiWidth, roiHeight;

	asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: Calling White Balance\n", functionName);

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiX", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiX, &roiX));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiY", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiY, &roiY));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiWidth", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiWidth, &roiWidth));
	if (!status) status |= resultCode(functionName, "QCam_GetParam roiHeight", QCam_GetParam((QCam_Settings*)&qSettings, qprmRoiHeight, &roiHeight));
	if (!status) status |= resultCode(functionName, "QCam_WhiteBalance", QCam_WhiteBalance((QCam_Settings*)&qSettings, roiX, roiY, roiWidth, roiHeight));
	
	return((asynStatus)status);
}

asynStatus QImage::readEnum(asynUser *pasynUser, 
							char *strings[], 
							int values[], 
							int severities[],
							size_t nElements,
							size_t *nIn)
{
	int i;
	int enumCount;
	char enumString[64];
	int status = asynSuccess;
	static const char *functionName = "readEnum";
	unsigned long Table[32];
	int TableSize = 32;

	*nIn = 0;

	status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));

	if (pasynUser->reason == qBinning)
	{

		QCam_GetParamSparseTable((QCam_Settings*)&qSettings, qprmBinning, &Table[0], &TableSize);
		for (int i = 0; i < TableSize; i++)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Binning %ld x %ld\n", Table[i], Table[i]);
			if (strings[*nIn]) free(strings[*nIn]);
			sprintf(enumString, "%ldx%ld\0", Table[i], Table[i]);
			strings[*nIn] = epicsStrDup(enumString);
			values[*nIn] = i;
			severities[*nIn] = 0;
			(*nIn)++;
		}
	}
	else if (pasynUser->reason == qImageFormat)
	{
		QCam_GetParamSparseTable((QCam_Settings*)&qSettings, qprmImageFormat, &Table[0], &TableSize);
		for (int i = 0; i < TableSize; i++)
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "image format %ld\n", Table[i]);
			if (strings[*nIn]) free(strings[*nIn]);
			switch (Table[i])
			{
			case qfmtRaw8:
				strncpy(enumString, "Raw 8\0", 6);
				break;
			case qfmtRaw16:
				strncpy(enumString, "Raw 16\0", 7);
				break;
			case qfmtMono8:
				strncpy(enumString, "Mono 8\0", 7);
				break;
			case qfmtMono16:
				strncpy(enumString, "Mono 16\0", 8);
				break;
			case qfmtBayer8:
				strncpy(enumString, "Bayer 8\0", 8);
				break;
			case qfmtBayer16:
				strncpy(enumString, "Bayer 16\0", 9);
				break;
			case qfmtRgbPlane8:
				strncpy(enumString, "RGB Plane 8\0", 12);
				break;
			case qfmtRgbPlane16:
				strncpy(enumString, "RGB Plane 16\0", 13);
				break;
			case qfmtBgr24:
				strncpy(enumString, "BGR 24\0", 7);
				break;
			case qfmtXrgb32:
				strncpy(enumString, "XRGB 32\0", 8);
				break;
			case qfmtRgb48:
				strncpy(enumString, "RGB 48\0", 7);
				break;
			case qfmtBgrx32:
				strncpy(enumString, "BGRX 32\0", 8);
				break;
			case qfmtRgb24:
				strncpy(enumString, "RGB 24\0", 7);
				break;
			}
			strings[*nIn] = epicsStrDup(enumString);
			values[*nIn] = i;
			severities[*nIn] = 0;
			(*nIn)++;
		}
	}
	else
	{
		return ADDriver::readEnum(pasynUser, strings, values, severities, nElements, nIn);
	}
	

	// Get the featureInfo for this parameter, set if it exists, is implemented, and is ATenum
	/*
	featureInfo *info = &featureInfo_[index];
	if (!info->exists) return asynError;
	if (!info->isImplemented) {
		if (strings[0]) free(strings[0]);
		if (strings[1]) free(strings[1]);
		if (info->type == ATbool) {
			strings[0] = epicsStrDup("N.A. 0");
			strings[1] = epicsStrDup("N.A. 1");
			*nIn = 2;
		}
		else {
			strings[0] = epicsStrDup("N.A.");
			*nIn = 1;
		}
		return asynSuccess;
	}
	if (info->type != ATenum) return asynError;
	
	status = AT_GetEnumCount(handle_, info->featureNameWC, &enumCount);
	for (i = 0; ((i<enumCount) && (i<(int)nElements)); i++) 
	{
		status |= AT_IsEnumIndexImplemented(handle_, info->featureNameWC, i, &isImplemented);
		if (!isImplemented) continue;
		if (strings[*nIn]) free(strings[*nIn]);
		status |= AT_GetEnumStringByIndex(handle_, info->featureNameWC, i,
			enumStringWC, MAX_FEATURE_NAME_LEN - 1);
		WCSToMBS(enumString, enumStringWC, sizeof(enumString) - 1);
		strings[*nIn] = epicsStrDup(enumString);
		values[*nIn] = i;
		severities[*nIn] = 0;
		(*nIn)++;
	}
	*/
	if (status) 
	{
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
			"%s:%s: error calling enum functions, status=%d\n",
			driverName, functionName, status);
	}
	return asynSuccess;
}


void QImage::report(FILE *fp, int details)
{
    fprintf(fp, "QImage detector %s\n", this->portName);
    if (details > 0) 
	{
        fprintf(fp, "\nHello File\n");
    }
    // Invoke the base class method
    ADDriver::report(fp, details);
}

asynStatus QImage::resultCode(const char *funcName, const char *cmdName, QCam_Err errorcode)
{
	int        status        = asynSuccess;
	int        debug;
	char       errMsg[MAX_FILENAME_LEN];
	const char *functionName = "resultCode";

	status |= getIntegerParam(qShowDiags, &debug);
	switch(errorcode) 
	{
		case qerrSuccess:  sprintf(errMsg, "%s:  %s", cmdName, "Success"); break;
		case qerrNotSupported:  sprintf(errMsg, "%s:  %s", cmdName, "Not Supported"); break;
		case qerrInvalidValue:  sprintf(errMsg, "%s:  %s", cmdName, "Invalid Value"); break;
		case qerrBadSettings:  sprintf(errMsg, "%s:  %s", cmdName, "Bad Settings"); break;
		case qerrNoUserDriver:  sprintf(errMsg, "%s:  %s", cmdName, "No User Driver"); break;
		case qerrNoFirewireDriver:  sprintf(errMsg, "%s:  %s", cmdName, "No Firewire Driver"); break;
		case qerrDriverConnection:  sprintf(errMsg, "%s:  %s", cmdName, "Driver Connection"); break;
		case qerrDriverAlreadyLoaded:  sprintf(errMsg, "%s:  %s", cmdName, "Driver Already Loaded"); break;
		case qerrDriverNotLoaded:  sprintf(errMsg, "%s:  %s", cmdName, "Driver Not Loaded"); break;
		case qerrInvalidHandle:  sprintf(errMsg, "%s:  %s", cmdName, "Invalid Handle"); break;
		case qerrUnknownCamera: sprintf(errMsg, "%s:  %s", cmdName, "Unknown Camera"); break;
		case qerrInvalidCameraId: sprintf(errMsg, "%s:  %s", cmdName, "Invalid Camera Id"); break;
		case qerrHardwareFault: sprintf(errMsg, "%s:  %s", cmdName, "Hardware Fault"); break;
		case qerrFirewireFault: sprintf(errMsg, "%s:  %s", cmdName, "Firewire Fault"); break;
		case qerrCameraFault: sprintf(errMsg, "%s:  %s", cmdName, "Camera Fault"); break;
		case qerrDriverFault: sprintf(errMsg, "%s:  %s", cmdName, "Driver Fault"); break;
		case qerrInvalidFrameIndex: sprintf(errMsg, "%s:  %s", cmdName, "Invalid Frame Index"); break;
		case qerrBufferTooSmall: sprintf(errMsg, "%s:  %s", cmdName, "Buffer Too Small"); break;
		case qerrOutOfMemory: sprintf(errMsg, "%s:  %s", cmdName, "Out Of Memory"); break;
		case qerrOutOfSharedMemory: sprintf(errMsg, "%s:  %s", cmdName, "Out Of Shared Memory"); break;
		case qerrBusy: sprintf(errMsg, "%s:  %s", cmdName, "Busy"); break;
		case qerrQueueFull: sprintf(errMsg, "%s:  %s", cmdName, "Queue Full"); break;
		case qerrCancelled: sprintf(errMsg, "%s:  %s", cmdName, "Cancelled"); break;
		case qerrNotStreaming: sprintf(errMsg, "%s:  %s", cmdName, "Not Streaming"); break;
		case qerrLostSync: sprintf(errMsg, "%s:  %s", cmdName, "Lost Sync"); break;
		case qerrBlackFill: sprintf(errMsg, "%s:  %s", cmdName, "Black Fill"); break;
		case qerrFirewireOverflow: sprintf(errMsg, "%s:  %s", cmdName, "Firewire Overflow"); break;
		case qerrUnplugged: sprintf(errMsg, "%s:  %s", cmdName, "Unplugged"); break;
		case qerrAccessDenied: sprintf(errMsg, "%s:  %s", cmdName, "Access Denied"); break;
		case qerrStreamFault: sprintf(errMsg, "%s:  %s", cmdName, "Stream Fault"); break;
		case qerrQCamUpdateNeeded: sprintf(errMsg, "%s:  %s", cmdName, "QCam Update Needed"); break;
		case qerrRoiTooSmall: sprintf(errMsg, "%s:  %s", cmdName, "Roi Too Small"); break;
		case qerr_last: sprintf(errMsg, "%s:  %s", cmdName, "qerr_last"); break;
		default: sprintf(errMsg, "%s:  %s", cmdName,    "Unknown Error Code"); break;
	}
	if ((errorcode != 0)||(debug >= 2))
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%-20s:%-50s:%-25s\n", funcName, cmdName, errMsg);
	if (!strcmp(cmdName, "ExposureCallback"))
		status |= setStringParam(qExposureStatusMessageRBV, errMsg);
	else if (!strcmp(cmdName, "FrameCallback"))
		status |= setStringParam(qFrameStatusMessageRBV,    errMsg);
	else if ((errorcode != 0)||(debug >= 2))
		status |= setStringParam(ADStatusMessage,           errMsg);
	if ((errorcode == 0)||(errorcode == 1))
		status |= asynSuccess;
	else
		status |= asynError;
	status |= callParamCallbacks();

	return((asynStatus)status);
}

//  * Called when asyn clients call pasynInt32->write().
//  * This function performs actions for some parameters, including ADAcquire, ADColorMode, etc.
//  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
//  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
//  * \param[in] value Value to write.
asynStatus QImage::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int            function = pasynUser->reason;
	int            status = asynSuccess;
	int	           debug;
	const char     *paramName;
	const char     *functionName = "writeInt32";

	status = getIntegerParam(qShowDiags, &debug);
	if (debug >= 2)
	{
		status |= getParamName(function, &paramName);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  %s\n", functionName, paramName);
	}


	if (!qHandle)
		return asynError;

	// For a real detector this is where the parameter is sent to the hardware
	if (function == ADAcquire)
	{
		status |= q_acquire(value);
	}
	else if (function == ADTriggerMode)
	{
		status |= q_setTriggerMode(value);
	}
	else if (function == qResetCam)
	{
		status |= q_resetCamera(value);
	}
	else if (function == qInitialize)
	{
		status |= initializeQImage();
	}
	else if (function == qReadoutSpeed)
	{
		status |= q_setReadoutSpeed(value);
	}
	else if (function == qOffset)
	{
		if (value < offsetMin)
			status |= setIntegerParam(qOffset, offsetMin);
		if (value > offsetMax)
			status |= setIntegerParam(qOffset, offsetMax);
	}
	else if (function == qImageFormat)
	{
		status |= q_setDataTypeAndColorMode(function, value);
	}
	else if (function == qBinning)
	{
		status |= q_setBinning(function, value);
	}
	else if (function == ADSizeX || function == ADSizeY)
	{
		status |= q_setImageSize(function, value);
	}
	else if (function == ADMinX || function == ADMinY)
	{
		status |= q_setMinXY(function, value);
	}
	else if (function == qAutoExposure)
	{
		status |= q_autoExposure(value);
	}
	else if (function == qWhiteBalance)
	{
		//status |= q_whiteBalance(value);
	}
	else if (function == qCoolerActive)
	{
		status |= q_setCoolerActive(value);
	}
	else
	{
		// If this parameter belongs to a base class call its method
		if (function < FIRST_QIMAGE_PARAM)
			status |= ADDriver::writeInt32(pasynUser, value);
	}

	if (status == asynSuccess)
	{
		setIntegerParam(function, value);
	}

	// Do callbacks so higher layers see any changes
	status |= callParamCallbacks();

	if (status)
		asynPrint(pasynUser, ASYN_TRACE_ERROR, ////REAL ASYN_TRACE_ERROR
		"%s:writeInt32 error, status=%d function=%d, value=%d\n",
		driverName, status, function, value);
	else
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
		"%s:writeInt32: function=%d, value=%d\n",
		driverName, function, value);



	return((asynStatus)status);

}

//  * Called when asyn clients call pasynFloat64->write().
//  * This function performs actions for some parameters, including ADAcquireTime, ADGain, etc.
//  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
//  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
//  * \param[in] value Value to write.
asynStatus QImage::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	int				function = pasynUser->reason;
	int             status = asynSuccess;
	int             debug;
	const char		*paramName;
	const char		*functionName = "writeFloat64";

	status |= getIntegerParam(qShowDiags, &debug);
	if (debug >= 2)
	{
		getParamName(function, &paramName);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:  %s\n", functionName, paramName);
	}


	if (!qHandle)
		return asynError;
	
	// For a real detector this is where the parameter is sent to the hardware
	if (function == ADAcquireTime)
	{
		if (value < expMin)
		{
			value = expMin;
		}
		if (value > expMax)
		{
			value = expMax;
		}

		unsigned long lVal = (unsigned long)(value*1e6);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: setting exposure to %f: min %f, max %f\n", functionName, value, expMin, expMax);
		status = setDoubleParam(ADAcquireTime, value);
		status = setDoubleParam(qAcquireTimeRBV, value);
		if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmExposure)", QCam_SetParam((QCam_Settings*)&qSettings, qprmExposure, lVal));
		if (!status) status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
	}
	else if (function == ADGain)
	{

		if (value < gainMin)
		{
			value = gainMin;
		}
		if (value > gainMax)
		{
			value = gainMax;
		}

		unsigned long lVal = (unsigned long)(value*1e6);
		asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s: setting gain to %f: min %f, max %f\n", functionName, value, gainMin, gainMax);
		status |= setDoubleParam(ADGain, value);
		status |= setDoubleParam(qGainRBV, value);
		if (!status) status |= resultCode(functionName, "QCam_ReadSettingsFromCam", QCam_ReadSettingsFromCam(qHandle, (QCam_Settings*)&qSettings));
		if (!status) status |= resultCode(functionName, "QCam_SetParam(qprmNormalizedGain)", QCam_SetParam((QCam_Settings*)&qSettings, qprmNormalizedGain, lVal));
		if (!status) status |= resultCode(functionName, "QCam_SendSettingsToCam", QCam_SendSettingsToCam(qHandle, (QCam_Settings*)&qSettings));
	}
	else if (function == ADTemperature)
	{
		if (value < tempMin)
		{
			status |= setIntegerParam(ADTemperature, tempMin);
			q_setTemperature(tempMin);
		}
		else if (value > tempMax)
		{
			status |= setIntegerParam(ADTemperature, tempMax);
			q_setTemperature(tempMax);
		}
		else
		{
			q_setTemperature(value);
		}
	}
	else
	{
		// If this parameter belongs to a base class call its method 
		if (function < FIRST_QIMAGE_PARAM)
			status |= ADDriver::writeFloat64(pasynUser, value);
	}

	if (status == asynSuccess)
	{
		status |= setDoubleParam(function, value);
	}

	// Do callbacks so higher layers see any changes
	status |= callParamCallbacks();

	if (status)
		asynPrint(pasynUser, ASYN_TRACE_ERROR, ////REAL ASYN_TRACE_ERROR
		"%s:writeFloat64 error, status=%d function=%d, value=%f\n",
		driverName, status, function, value);
	else
		asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
		"%s:writeFloat64: function=%d, value=%f\n",
		driverName, function, value);

	return((asynStatus)status);

}



//------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
asynStatus QImage::queueFrame(unsigned long display)
{
int           status        = asynSuccess;
size_t        dims[2] = { 2048, 2048 };
NDDataType_t  dataType;
QCam_Frame    *pFrame;
const char    *functionName = "queueFrame";

if ((display>=0)&&(display<(unsigned long)num_buffs))
{
//status |= getIntegerParam(ADSizeX, (int *)&dims[0]);
//status |= getIntegerParam(ADSizeY, (int *)&dims[1]);
status |= getIntegerParam(NDDataType, (int *)&dataType);
if (expSntl)
{
if (pImage.size() > display)
if(pImage[display])
pImage[display]->release();
}
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Allocating NDArray [%d] %d x %d datatype %d\n", functionName, display, dims[0], dims[1], dataType);
NDArray* ndarr = pNDArrayPool->alloc(2, dims, dataType, 0, NULL);
//this->pImage[display] = this->pNDArrayPool->alloc(2, dims, dataType, 0, NULL);
//if (!this->pImage[display])
if (ndarr == NULL)
{
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Error: Problem allocating NDArray %d\n", functionName, display);
return asynError;
}
pImage.push_back(ndarr);
//qFrame.frame[display].pBuffer = this->pImage[display]->pData;
qFrame.frame[display].pBuffer = ndarr->pData;
pFrame = &qFrame.frame[display];
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Calling QCam_QueueFrame\n", functionName);
status |= resultCode(functionName, "QCam_QueueFrame", QCam_QueueFrame(qHandle, pFrame, QImageCallback, qcCallbackDone | qcCallbackExposeDone, this, display));
}
else
{
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Frame Out of Range\n", functionName);
status = asynError;
}

return((asynStatus)status);
}

asynStatus QImage::initializeBuffers()
{
int           status        = asynSuccess;
size_t        dims[2];
NDDataType_t  dataType;
const char    *functionName = "initializeBuffers";

status |= resultCode(functionName, "QCam_Abort", QCam_Abort(qHandle));
status |= getIntegerParam(ADSizeX, (int *)&dims[0]);
status |= getIntegerParam(ADSizeY, (int *)&dims[1]);
status |= getIntegerParam(NDDataType, (int *)&dataType);
switch((int)dataType) {
case 0:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsInt8));
break;
case 1:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsUInt8));
break;
case 2:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsInt16));
break;
case 3:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsUInt16));
break;
case 4:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsInt32));
break;
case 5:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsUInt32));
break;
case 6:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsFloat32));
break;
case 7:
rawDataSize = (unsigned long)(dims[0]*dims[1]*sizeof(epicsFloat64));
break;
default:
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "\n\n%s: ERROR: Unknown Data Type\n\n", functionName);
status |= asynError;
}
for(int i=0; i<num_buffs; i++)
{
qFrame.frame[i].bufferSize = rawDataSize;
status |= queueFrame(i);
}
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: frame buffer initialized\n", functionName);

return((asynStatus)status);
}


/*
void QImage::exposureTask()
{
int         status        = asynSuccess;
int			iMode, tMode, numImages, imagesCounted, stopStatus, debug;
double      acquirePeriod, expTime, delay;
taskData    task;
const char	*functionName = "exposureTask";

while (!exiting_) {
this->lock();
if (!this->exposureQueue.empty()) {
task = this->exposureQueue.front();
this->exposureQueue.pop();
this->unlock();
status = getIntegerParam(qShowDiags, &debug);
if (debug >= 1)
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Acquisition OK\n", functionName);
status |= getIntegerParam(ADTriggerMode, &tMode);
if (tMode == qcTriggerSoftware) {
status |= setIntegerParam(ADStatus, ADStatusAcquire);
status |= callParamCallbacks();
}
status |= resultCode(functionName, "ExposureCallback", task.errorcode);
status |= getIntegerParam(ADImageMode, &iMode);
status |= getIntegerParam(ADNumImages, &numImages);
status |= getIntegerParam(ADNumImagesCounter, &imagesCounted);
status |= getDoubleParam(ADAcquirePeriod, &acquirePeriod);
status |= getDoubleParam(ADAcquireTime, &expTime);
status |= setIntegerParam(ADNumImagesCounter, ++imagesCounted);
status |= callParamCallbacks();
if (tMode == qcTriggerSoftware) {
if (((imagesCounted >= numImages) && (iMode == ADImageMultiple)) ||
((imagesCounted >= 1)         && (iMode == ADImageSingle))) {
if (debug >= 1)
asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s: Acquisition is Done\n", functionName);
status |= setIntegerParam(ADStatus, ADStatusIdle);
status |= setIntegerParam(ADAcquire, 0);
status |= callParamCallbacks();
} else {
delay = acquirePeriod - expTime;
if (delay > 0.0)
stopStatus = epicsEventWaitWithTimeout(this->stopEventId, delay);
else
stopStatus = epicsEventWaitWithTimeout(this->stopEventId, RETIGA_POLL_TIME);
if (stopStatus != epicsEventWaitOK) {
status |= resultCode(functionName, "QCam_Trigger", QCam_Trigger(qHandle));
status |= setIntegerParam(qTrgCnt, ++trgCnt);
status |= callParamCallbacks();
}
}
}
}
else this->unlock();
}
}
*/
