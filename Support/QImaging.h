// * QImage.h
// *
// * Header file for QImage.cpp
// *
// * Author: Jeff Gebhardt
// *         BCDA-AES-APS-ANL
// *
// * Created: June 13, 2011
// * Updated: Sep 30, 2014 Arhtur Glowacki


#ifndef QIMAGE_H
#define QIMAGE_H

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <comdef.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <cantProceed.h>
#include <iocsh.h>
#include <epicsExit.h>
#include <queue>

#include "QCamApi.h"
#include "ADDriver.h"

#include <epicsExport.h>

static const char *driverName = "QImage";
#define RETIGA_POLL_TIME .010
#define MAX_FILENAME_LEN  256
#define MAX_ARRAY_LEN      40

struct taskData {
	unsigned long usrData;
	QCam_Err      errorcode;
	unsigned long flags;
	unsigned long count;
};

// Driver for QImage detectors
class QImage : public ADDriver {
public:
    QImage(const char *portName, const char *model, NDDataType_t dataType, int numbuffs, int debug, int maxBuffers, size_t maxMemory, int priority, int stackSize);

	friend void QCAMAPI QImageCallback(void* usrPtr, unsigned long usrData, QCam_Err errorcode, unsigned long flags);

    // These are the methods that we override from ADDriver
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
	virtual void report(FILE *fp, int details);
    // These are new methods
	void exposureTask();
	void frameTask();
	void shutdown(); // This is called by epicsAtExit

 protected:
  	int		qMaxBitDepthRBV;
	#define FIRST_QIMAGE_PARAM qMaxBitDepthRBV
	int		qSerialNumberRBV;
	int		qUniqueIdRBV;
	int		qCcdTypeRBV;
	int		qCooledRBV;
	int		qRegulatedCoolingRBV;
	int		qFanControlRBV;
	int		qHighSensitivityModeRBV;
	int		qBlackoutModeRBV;
	int		qAsymmetricalBinningRBV;
	int		qCoolerActive;
	int		qReadoutSpeed;
	int		qOffset;
	int		qImageFormat;
	int		qAcquireTimeRBV;
	int		qMinXRBV;
	int		qMinYRBV;
	int		qSizeXRBV;
	int		qSizeYRBV;
	int		qTriggerModeRBV;
	int		qGainRBV;
	int		qTemperatureRBV;
	int		qReadoutSpeedRBV;
	int		qOffsetRBV;
	int		qImageFormatRBV;
	int		qCoolerActiveRBV;
	int     qRegulatedCoolingLockRBV;
	int		qExposureStatusMessageRBV;
	int		qFrameStatusMessageRBV;
	int     qTrgCnt;
	int     qExpCnt;
	int		qFrmCnt;
	int		qShowDiags;
	int     qResetCam;
	int		qInitialize;
	#define LAST_QIMAGE_PARAM qInitialize

#define NUM_QIMAGE_PARAMS (&LAST_QIMAGE_PARAM - &FIRST_QIMAGE_PARAM + 1)

 private:
    // These are the methods that are new to this class
	asynStatus resultCode(const char *funcName, const char *cmdName, QCam_Err errorcode);
	asynStatus connectQImage();
	asynStatus disconnectQImage();
	asynStatus queueFrame(unsigned long frame);
	asynStatus initializeBuffers();
	asynStatus initializeQImage();
	asynStatus queryQImage();

    // Our data
	epicsTimeStamp  startTime;
	epicsEventId    stopEventId;
	NDArray         **pImage;
	QCam_Handle		qHandle;
	QCam_Settings	qSettings;
	int             num_buffs;
	int             trgCnt;
	int             expCnt;
	int		        frmCnt;
	int				expSntl;
	bool            stopEvent;
	double			expMax;
	double			expMin;
	double			gainMax;
	double			gainMin;
	signed long		offsetMax;
	signed long		offsetMin;
	signed long		tempMax;
	signed long		tempMin;
	unsigned long   coolerReg;
	unsigned long   rawDataSize;
	struct {
		QCam_Frame     *frame;
		unsigned long  display;
	} qFrame;
	std::queue<taskData> exposureQueue,
		                 frameQueue;
};

#define qMaxBitDepthRBVString				"MAX_BIT_DEPTH_RBV"
#define qSerialNumberRBVString				"SERIAL_NUMBER_RBV"
#define qUniqueIdRBVString					"UNIQUE_ID_RBV"
#define qCcdTypeRBVString					"CCD_TYPE_RBV"
#define qCooledRBVString					"COOLED_RBV"
#define qRegulatedCoolingRBVString			"REGULATED_COOLING_RBV"
#define qFanControlRBVString				"FAN_CONTROL_RBV"
#define qHighSensitivityModeRBVString		"HIGH_SENSITIVITY_MODE_RBV"
#define qBlackoutModeRBVString				"BLACK_OUT_MODE_RBV"
#define qAsymmetricalBinningRBVString		"ASYMMETRICAL_BINNING_RBV"
#define qCoolerActiveString					"COOLER_ACTIVE"
#define qReadoutSpeedString					"READOUT_SPEED"
#define qOffsetString						"OFFSET"
#define qImageFormatString					"IMAGE_FORMAT"
#define qAcquireTimeRBVString				"ACQUIRE_TIME_RBV"
#define qMinXRBVString						"MIN_X_RBV"
#define qMinYRBVString						"MIN_Y_RBV"
#define qSizeXRBVString						"SIZE_X_RBV"
#define qSizeYRBVString						"SIZE_Y_RBV"
#define qTriggerModeRBVString				"TRIGGER_MODE_RBV"
#define qGainRBVString						"GAIN_RBV"
#define qTemperatureRBVString				"TEMPERATURE_RBV"
#define qReadoutSpeedRBVString				"READOUT_SPEED_RBV"
#define qOffsetRBVString					"OFFSET_RBV"
#define qImageFormatRBVString				"IMAGE_FORMAT_RBV"
#define qCoolerActiveRBVString				"COOLER_ACTIVE_RBV"
#define qRegulatedCoolingLockRBVString		"REGULATED_COOLING_LOCK_RBV"
#define qExposureStatusMessageRBVString		"EXPOSURE_STATUS_MESSAGE"
#define qFrameStatusMessageRBVString		"FRAME_STATUS_MESSAGE"
#define qTrgCntString					    "TRIGGER_COUNT"
#define qExpCntString					    "EXPOSURE_COUNT_RBV"
#define qFrmCntString					    "FRAME_COUNT_RBV"
#define qShowDiagsString					"SHOW_DIAGS"
#define qResetCamString						"RESET_DETECTOR"
#define qInitializeString					"INITIALIZE_DETECTOR"

static void QImageShutdown(void* arg) {
		QImage *p = (QImage*)arg;
		p->shutdown();
}

#endif