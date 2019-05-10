/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "MtkCam/Shot"
//
#include <mtkcam/Log.h>
#include <mtkcam/common.h>
//
#include <mtkcam/exif/IDbgInfoContainer.h>
//
#include <mtkcam/hwutils/CameraProfile.h>
using namespace CPTool;
//
#include <mtkcam/hal/IHalSensor.h>
//
#include <mtkcam/camshot/ICamShot.h>
#include <mtkcam/camshot/ISingleShot.h>
#include <mtkcam/camshot/ISmartShot.h>
//
#include <mtkcam/exif/IBaseCamExif.h>
//
#include <Shot/IShot.h>
//
#include "ImpShot.h"
#include "NormalShot.h"
//
#include <mtkcam/hwutils/CamManager.h>
using namespace NSCam::Utils;
//
#include <mtkcam/utils/Format.h>
#include <mtkcam/utils/ImageBufferHeap.h>
#include <mtkcam/iopipe/SImager/ISImager.h>

#include <aaa_types.h>
#include <aaa_error_code.h>
#include <aaa_hal.h>
#include <ae_param.h>
#include <ae_mgr.h>

#include "MyUtils.h"
#include "sensor_drv.h"
//
#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "arcsoft_smart_denoise.h"
//
using namespace android;
using namespace NSShot;

/******************************************************************************
 *
 ******************************************************************************/
#define MY_LOGV(fmt, arg...)        CAM_LOGV("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGD(fmt, arg...)        CAM_LOGD("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        CAM_LOGI("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        CAM_LOGW("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        CAM_LOGE("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGA(fmt, arg...)        CAM_LOGA("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
#define MY_LOGF(fmt, arg...)        CAM_LOGF("(%d)(%s)[%s] "fmt, ::gettid(), getShotName(), __FUNCTION__, ##arg)
//
#define MY_LOGV_IF(cond, ...)       do { if ( (cond) ) { MY_LOGV(__VA_ARGS__); } }while(0)
#define MY_LOGD_IF(cond, ...)       do { if ( (cond) ) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGI_IF(cond, ...)       do { if ( (cond) ) { MY_LOGI(__VA_ARGS__); } }while(0)
#define MY_LOGW_IF(cond, ...)       do { if ( (cond) ) { MY_LOGW(__VA_ARGS__); } }while(0)
#define MY_LOGE_IF(cond, ...)       do { if ( (cond) ) { MY_LOGE(__VA_ARGS__); } }while(0)
#define MY_LOGA_IF(cond, ...)       do { if ( (cond) ) { MY_LOGA(__VA_ARGS__); } }while(0)
#define MY_LOGF_IF(cond, ...)       do { if ( (cond) ) { MY_LOGF(__VA_ARGS__); } }while(0)

#ifdef ARCSOFT_CAMERA_FEATURE
#define SMART_DENOISE_FEATURE   (1)
#endif

#define SAFE_MEM_FREE(MEM)		if (MNull != MEM) {free(MEM); MEM = MNull;}
//#define SMART_DENOISE_DEBUG
#define DENOISE_MEM_SIZE		(8 * 1024 * 1024)


/******************************************************************************
 *
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_NormalShot(
    char const*const    pszShotName, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId
)
{
    sp<IShot>       pShot = NULL;
    sp<NormalShot>  pImpShot = NULL;
    //
    //  (1.1) new Implementator.
    pImpShot = new NormalShot(pszShotName, u4ShotMode, i4OpenId);
    if  ( pImpShot == 0 ) {
        CAM_LOGE("[%s] new NormalShot", __FUNCTION__);
        goto lbExit;
    }
    //
    //  (1.2) initialize Implementator if needed.
    if  ( ! pImpShot->onCreate() ) {
        CAM_LOGE("[%s] onCreate()", __FUNCTION__);
        goto lbExit;
    }
    //
    //  (2)   new Interface.
    pShot = new IShot(pImpShot);
    if  ( pShot == 0 ) {
        CAM_LOGE("[%s] new IShot", __FUNCTION__);
        goto lbExit;
    }
    //
lbExit:
    //
    //  Free all resources if this function fails.
    if  ( pShot == 0 && pImpShot != 0 ) {
        pImpShot->onDestroy();
        pImpShot = NULL;
    }
    //
    return  pShot;
}


/******************************************************************************
 *  This function is invoked when this object is firstly created.
 *  All resources can be allocated here.
 ******************************************************************************/
bool
NormalShot::
onCreate()
{
#warning "[TODO] NormalShot::onCreate()"
#if SMART_DENOISE_FEATURE
    bool ret = MFALSE;
	mpIMemDrv =  IMemDrv::createInstance();
    if (mpIMemDrv == NULL)
    {
        MY_LOGE("mpIMemDrv is NULL \n");
        return MFALSE;
    }

	g_pMem = malloc(DENOISE_MEM_SIZE);
	g_hMemMgr = MMemMgrCreate(g_pMem, DENOISE_MEM_SIZE);
	if (MNull == g_pMem || MNull == g_hMemMgr)
	{
		MY_LOGE("Denoise init memory error");
		return MFALSE;
	}

	int res = ASD_CreateHandle(g_hMemMgr, &g_hDenoiseHandler);
	if(MOK != res)
	{
		MY_LOGE("Denoise ASD_CreateHandle error res = %d", res);
		if(g_hMemMgr)
		{
			MMemMgrDestroy(g_hMemMgr);
			g_hMemMgr = MNull;
		}

		SAFE_MEM_FREE(g_pMem);
		return MFALSE;
	}

	g_pNcfFile = new MUInt8[ASD_NCF_MEM_LEN];

	//raw sensor
	AE_MODE_CFG_T rCaptureInfo;
	MUINT32 ISOSpeed;
	MUINT32 RealISOValue;
	
	NS3A::IAeMgr::getInstance().getCaptureParams(mSensorDev, rCaptureInfo);
	ISOSpeed = NS3A::IAeMgr::getInstance().getAEISOSpeedMode(mSensorDev);
	RealISOValue = rCaptureInfo.u4RealISO;

	if (ISOSpeed !=0) {
        ISOValue = ISOSpeed;
    } else {
        ISOValue = RealISOValue;
    }

	SensorDrv *const pSensorDrv = SensorDrv::get();
	NSFeature::SensorInfoBase* pSensorInfo;
	pSensorInfo = pSensorDrv->getMainSensorInfo();
	mSensorDrvName = pSensorInfo->getDrvName();

	MY_LOGD("mSensorDrvName=%s",mSensorDrvName);

    mDebugInfo = IDbgInfoContainer::createInstance();
			
	ret = MTRUE;
#else
    bool ret = true;
#endif	
    return ret;
}


/******************************************************************************
 *  This function is invoked when this object is ready to destryoed in the
 *  destructor. All resources must be released before this returns.
 ******************************************************************************/
void
NormalShot::
onDestroy()
{
#warning "[TODO] NormalShot::onDestroy()"

#if SMART_DENOISE_FEATURE
	if (g_hDenoiseHandler != MNull)
	{
		ASD_ReleaseHandle(g_hDenoiseHandler);
		g_hDenoiseHandler = MNull;
	}

	if(g_hMemMgr != MNull)
	{
		MMemMgrDestroy(g_hMemMgr);
		g_hMemMgr = MNull;
	}
	
	if (g_pMem != MNull)
	{
		free(g_pMem);
		g_pMem = MNull;
	}	

	if (g_pNcfFile != MNull)
	{
		delete[] g_pNcfFile;
		g_pNcfFile = MNull;
	}

    mu4W_yuv = 0;
    mu4H_yuv = 0;

	if (mvImgBufMap.size() != 0)
    {
        MY_LOGE("ImageBuffer leakage here!");
        for (Vector<ImageBufferMap>::iterator it = mvImgBufMap.begin();
                it != mvImgBufMap.end();
                it++)
        {
            MY_LOGE("Freeing memID(0x%x),virtAddr(0x%x)!", it->memBuf.memID, it->memBuf.virtAddr);
            mpIMemDrv->unmapPhyAddr(&it->memBuf);
            mpIMemDrv->freeVirtBuf(&it->memBuf);
        }
    }
    if (mpIMemDrv)
    {
        mpIMemDrv->destroyInstance();
        mpIMemDrv = NULL;
    }

	if (mDebugInfo != NULL) 
	{
        mDebugInfo->destroyInstance();
    }
#endif
}


/******************************************************************************
 *
 ******************************************************************************/
NormalShot::
NormalShot(
    char const*const pszShotName, 
    uint32_t const u4ShotMode, 
    int32_t const i4OpenId
)
    : ImpShot(pszShotName, u4ShotMode, i4OpenId)
{
    if (i4OpenId == 0) {
		mSensorDev = SENSOR_DEV_MAIN;
    }
    else if (i4OpenId == 1) {
		mSensorDev == SENSOR_DEV_SUB;
    }
    else {
		mSensorDev == SENSOR_DEV_NONE;
    }
}


/******************************************************************************
 *
 ******************************************************************************/
NormalShot::
~NormalShot()
{
}


/******************************************************************************
 *
 ******************************************************************************/
bool
NormalShot::
sendCommand(
    uint32_t const  cmd, 
    uint32_t const  arg1, 
    uint32_t const  arg2,
    uint32_t const  arg3
)
{
    AutoCPTLog cptlog(Event_Shot_sendCmd, cmd, arg1);
    bool ret = true;
    //
    switch  (cmd)
    {
    //  This command is to reset this class. After captures and then reset, 
    //  performing a new capture should work well, no matter whether previous 
    //  captures failed or not.
    //
    //  Arguments:
    //          N/A
    case eCmd_reset:
        ret = onCmd_reset();
        break;

    //  This command is to perform capture.
    //
    //  Arguments:
    //          N/A
    case eCmd_capture:
        ret = onCmd_capture();
        break;

    //  This command is to perform cancel capture.
    //
    //  Arguments:
    //          N/A
    case eCmd_cancel:
        onCmd_cancel();
        break;
    //
    default:
        ret = ImpShot::sendCommand(cmd, arg1, arg2, arg3);
    }
    //
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
NormalShot::
onCmd_reset()
{
#warning "[TODO] NormalShot::onCmd_reset()"
    bool ret = true;
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
NormalShot::
onCmd_capture()
{ 
#if SMART_DENOISE_FEATURE
    MBOOL   ret = MFALSE;

    ret = doCapture();
    if  ( ! ret )
    {
        goto lbExit;
    }
    ret = MTRUE;
lbExit:
    releaseBufs();
#else
    AutoCPTLog cptlog(Event_Shot_capture);
    MBOOL ret = MTRUE; 
    MBOOL isMfbShot = MFALSE;
    NSCamShot::ICamShot *pSingleShot = NSCamShot::ISmartShot::createInstance(static_cast<EShotMode>(mu4ShotMode)
                                                                            , "NormalShot"
                                                                            , getOpenId()
                                                                            , mShotParam.mu4MultiFrameBlending
                                                                            , &isMfbShot
                                                                            );
    //
    MUINT32 nrtype = queryCapNRType( getCaptureIso(), isMfbShot);
    // 
    pSingleShot->init(); 
    // 
    pSingleShot->enableNotifyMsg( NSCamShot::ECamShot_NOTIFY_MSG_EOF ); 
    //
    EImageFormat ePostViewFmt = 
        static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);

    pSingleShot->enableDataMsg(
            NSCamShot::ECamShot_DATA_MSG_JPEG
#if 0
            | ((ePostViewFmt != eImgFmt_UNKNOWN) ? 
             NSCamShot::ECamShot_DATA_MSG_POSTVIEW : NSCamShot::ECamShot_DATA_MSG_NONE)
#endif
            ); 

    // shot param 
    NSCamShot::ShotParam rShotParam(
            eImgFmt_YUY2,                    //yuv format 
            mShotParam.mi4PictureWidth,      //picutre width 
            mShotParam.mi4PictureHeight,     //picture height
            mShotParam.mu4Transform,         //picture transform 
            ePostViewFmt,                    //postview format 
            mShotParam.mi4PostviewWidth,     //postview width 
            mShotParam.mi4PostviewHeight,    //postview height 
            0,                               //postview transform
            mShotParam.mu4ZoomRatio          //zoom   
            );                                  
 
    // jpeg param 
    NSCamShot::JpegParam rJpegParam(
            NSCamShot::ThumbnailParam(
    		mJpegParam.mi4JpegThumbWidth, 
     		mJpegParam.mi4JpegThumbHeight, 
          	mJpegParam.mu4JpegThumbQuality, 
         	MTRUE),
            mJpegParam.mu4JpegQuality,         //Quality 
            MFALSE                             //isSOI 
            ); 
 
    CamManager* pCamMgr = CamManager::getInstance();
    MUINT32 scenario = ( pCamMgr->isMultiDevice() && (pCamMgr->getFrameRate(getOpenId()) == 0) )
                    ? SENSOR_SCENARIO_ID_NORMAL_PREVIEW : SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
                                                                     
    // sensor param 
    NSCamShot::SensorParam rSensorParam(
            getOpenId(),                            //sensor idx
            scenario,                               //Scenaio 
            10,                                     //bit depth 
            MFALSE,                                 //bypass delay 
            MFALSE                                  //bypass scenario 
            );  
    //
    pSingleShot->setCallbacks(fgCamShotNotifyCb, fgCamShotDataCb, this); 
    //     
    ret = pSingleShot->setShotParam(rShotParam); 
    //
    ret = pSingleShot->setJpegParam(rJpegParam); 
    //
    ret = pSingleShot->sendCommand( NSCamShot::ECamShot_CMD_SET_PREVIEW_BUFHDL, (MINT32)mpPrvBufHandler,0,0); 
    //
    ret = pSingleShot->sendCommand( NSCamShot::ECamShot_CMD_SET_NRTYPE, nrtype, 0, 0 );
    // 
    ret = pSingleShot->startOne(rSensorParam); 
    //
    ret = pSingleShot->uninit(); 
    //
    pSingleShot->destroyInstance(); 
#endif

    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
void
NormalShot::
onCmd_cancel()
{
    AutoCPTLog cptlog(Event_Shot_cancel);
#warning "[TODO] NormalShot::onCmd_cancel()"
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL 
NormalShot::
fgCamShotNotifyCb(MVOID* user, NSCamShot::CamShotNotifyInfo const msg)
{
    AutoCPTLog cptlog(Event_Shot_handleNotifyCb);
    NormalShot *pNormalShot = reinterpret_cast <NormalShot *>(user); 
    if (NULL != pNormalShot) 
    {
        if ( NSCamShot::ECamShot_NOTIFY_MSG_EOF == msg.msgType) 
        {
            pNormalShot->mpShotCallback->onCB_Shutter(true, 0); 
        }
    }

    return MTRUE; 
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
NormalShot::
fgCamShotDataCb(MVOID* user, NSCamShot::CamShotDataInfo const msg)
{
    NormalShot *pNormalShot = reinterpret_cast<NormalShot *>(user); 
    if (NULL != pNormalShot) 
    {
        if (NSCamShot::ECamShot_DATA_MSG_POSTVIEW == msg.msgType) 
        {
            //pNormalShot->handlePostViewData((MUINT8*)msg.pBuffer->getBufVA(0), msg.pBuffer->getBufSizeInBytes(0));  
        }
        else if (NSCamShot::ECamShot_DATA_MSG_JPEG == msg.msgType)
        {
            pNormalShot->handleJpegData(
                    (IImageBuffer*)msg.pBuffer,
                    (IImageBuffer*)msg.ext1,
                    (IDbgInfoContainer*)msg.ext2
                    );
        }
	#if SMART_DENOISE_FEATURE
		else if (NSCamShot::ECamShot_DATA_MSG_YUV == msg.msgType)
        {
            pNormalShot->handleYuvDataCallback((MUINT8*)msg.pBuffer->getBufVA(0), msg.pBuffer->getBufSizeInBytes(0));
            {   //dbginfo
                IDbgInfoContainer* pDbgInfo = reinterpret_cast<IDbgInfoContainer*>(msg.ext2);
                NormalShot *self = reinterpret_cast<NormalShot *>(user);
                if (self == NULL)
                {
                    CAM_LOGE("[fgCamShotDataCb] user is NULL");
                    return MFALSE;
                }
                pDbgInfo->copyTo(self->mDebugInfo);
            }
        }
	#endif
    }

    return MTRUE; 
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
handlePostViewData(MUINT8* const puBuf, MUINT32 const u4Size)
{
#if 0
    AutoCPTLog cptlog(Event_Shot_handlePVData);
    MY_LOGD("+ (puBuf, size) = (%p, %d)", puBuf, u4Size); 
    mpShotCallback->onCB_PostviewDisplay(0, 
                                         u4Size, 
                                         reinterpret_cast<uint8_t const*>(puBuf)
                                        ); 

    MY_LOGD("-"); 
#endif
    return  MTRUE;
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
handleJpegData(IImageBuffer* pJpeg, IImageBuffer* pThumb, IDbgInfoContainer* pDbg)
{
    AutoCPTLog cptlog(Event_Shot_handleJpegData);
    MUINT8* puJpegBuf = (MUINT8*)pJpeg->getBufVA(0);
    MUINT32 u4JpegSize = pJpeg->getBitstreamSize();
    MUINT8* puThumbBuf = NULL;
    MUINT32 u4ThumbSize = 0;
    if( pThumb != NULL )
    {
        puThumbBuf = (MUINT8*)pThumb->getBufVA(0);
        u4ThumbSize = pThumb->getBitstreamSize();
    }

    MY_LOGD("+ (puJpgBuf, jpgSize, puThumbBuf, thumbSize, dbg) = (%p, %d, %p, %d, %p)",
            puJpegBuf, u4JpegSize, puThumbBuf, u4ThumbSize, pDbg); 

    MUINT8 *puExifHeaderBuf = new MUINT8[ DBG_EXIF_SIZE ]; 
    MUINT32 u4ExifHeaderSize = 0; 

    CPTLogStr(Event_Shot_handleJpegData, CPTFlagSeparator, "makeExifHeader");
    makeExifHeader(eAppMode_PhotoMode, puThumbBuf, u4ThumbSize, puExifHeaderBuf, u4ExifHeaderSize, pDbg); 
    MY_LOGD("(thumbbuf, size, exifHeaderBuf, size) = (%p, %d, %p, %d)", 
                      puThumbBuf, u4ThumbSize, puExifHeaderBuf, u4ExifHeaderSize); 

    // dummy raw callback 
    mpShotCallback->onCB_RawImage(0, 0, NULL);   

    // Jpeg callback 
    CPTLogStr(Event_Shot_handleJpegData, CPTFlagSeparator, "onCB_CompressedImage");
    mpShotCallback->onCB_CompressedImage(0,
                                         u4JpegSize, 
                                         reinterpret_cast<uint8_t const*>(puJpegBuf),
                                         u4ExifHeaderSize,                       //header size 
                                         puExifHeaderBuf,                    //header buf
                                         0,                       //callback index 
                                         true                     //final image 
                                         ); 
    MY_LOGD("-"); 

    delete [] puExifHeaderBuf; 

    return MTRUE; 
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
handleYuvDataCallback(MUINT8* const puBuf, MUINT32 const u4Size)
{
    MY_LOGD("(puBuf, size) = (%p, %d)", puBuf, u4Size);

    return MFALSE;
}


/******************************************************************************
*
*******************************************************************************/
bool
NormalShot::
doCapture()
{
    MBOOL ret = MFALSE;

    ret = requestBufs();
	ret = createYUVFrame(mpSource);
	ret = DenoiseProcess(mpSource);	
	
	EImageFormat mPostviewFormat = static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);
		
	ret = ImgProcess(mpSource, mu4W_yuv, mu4H_yuv, eImgFmt_YUY2, mpPostviewImgBuf, mPostviewWidth, mPostviewHeight, mPostviewFormat);
	ret = createJpegImgWithThumbnail(mpSource, mpPostviewImgBuf);

   	if (!ret)
	{
        MY_LOGI("Capture fail \n");
    }
  
    return ret;
}


/******************************************************************************
*
*******************************************************************************/
IImageBuffer*
NormalShot::
allocMem(MUINT32 fmt, MUINT32 w, MUINT32 h)
{
    IImageBuffer* pBuf;

    if( fmt != eImgFmt_JPEG )
    {
        /* To avoid non-continuous multi-plane memory, allocate ION memory and map it to ImageBuffer */
        MUINT32 plane = NSCam::Utils::Format::queryPlaneCount(fmt);
        ImageBufferMap bufMap;

        bufMap.memBuf.size = 0;
        for (int i = 0; i < plane; i++)
        {
            bufMap.memBuf.size += (NSCam::Utils::Format::queryPlaneWidthInPixels(fmt,i, w) * NSCam::Utils::Format::queryPlaneBitsPerPixel(fmt,i) / 8) * NSCam::Utils::Format::queryPlaneHeightInPixels(fmt, i, h);
        }

        if (mpIMemDrv->allocVirtBuf(&bufMap.memBuf)) {
            MY_LOGE("g_pIMemDrv->allocVirtBuf() error \n");
            return NULL;
        }

        if (mpIMemDrv->mapPhyAddr(&bufMap.memBuf)) {
            MY_LOGE("mpIMemDrv->mapPhyAddr() error \n");
            return NULL;
        }

        MINT32 bufBoundaryInBytes[3] = {0, 0, 0};
        MUINT32 bufStridesInBytes[3] = {0};

        for (MUINT32 i = 0; i < plane; i++)
        { 
            bufStridesInBytes[i] = NSCam::Utils::Format::queryPlaneWidthInPixels(fmt,i, w) * NSCam::Utils::Format::queryPlaneBitsPerPixel(fmt,i) / 8;
        }
        IImageBufferAllocator::ImgParam imgParam(
                fmt, 
                MSize(w,h), 
                bufStridesInBytes, 
                bufBoundaryInBytes, 
                plane
                );

        PortBufInfo_v1 portBufInfo = PortBufInfo_v1(
                                        bufMap.memBuf.memID,
                                        bufMap.memBuf.virtAddr,
                                        bufMap.memBuf.useNoncache, 
                                        bufMap.memBuf.bufSecu, 
                                        bufMap.memBuf.bufCohe);

        sp<ImageBufferHeap> pHeap = ImageBufferHeap::create(
                                                        LOG_TAG,
                                                        imgParam,
                                                        portBufInfo);
        if(pHeap == 0)
        {
            MY_LOGE("pHeap is NULL");
            return NULL;
        }
        //
        pBuf = pHeap->createImageBuffer();
        pBuf->incStrong(pBuf);

        bufMap.pImgBuf = pBuf;
        mvImgBufMap.push_back(bufMap);
    }
    else
    {
        MINT32 bufBoundaryInBytes = 0;
        IImageBufferAllocator::ImgParam imgParam(
                MSize(w,h), 
                w * h * 6 / 5,  //FIXME
                bufBoundaryInBytes
                );

        pBuf = mpIImageBufAllocator->alloc_ion(LOG_TAG, imgParam);
    }
    if (!pBuf || !pBuf->lockBuf( LOG_TAG, eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN | eBUFFER_USAGE_SW_WRITE_OFTEN ) )
    {
        MY_LOGE("Null allocated or lock Buffer failed\n");
        return  NULL;
    }

    pBuf->syncCache(eCACHECTRL_INVALID);

    return pBuf;
}


/******************************************************************************
*
*******************************************************************************/
void
NormalShot::
deallocMem(IImageBuffer *pBuf)
{
    pBuf->unlockBuf(LOG_TAG);
    if (pBuf->getImgFormat() == eImgFmt_JPEG)
    {
        mpIImageBufAllocator->free(pBuf);
    }
    else
    {
        pBuf->decStrong(pBuf);
        for (Vector<ImageBufferMap>::iterator it = mvImgBufMap.begin();
                it != mvImgBufMap.end();
                it++)
        {
            if (it->pImgBuf == pBuf)
            {
                mpIMemDrv->unmapPhyAddr(&it->memBuf);
                if (mpIMemDrv->freeVirtBuf(&it->memBuf))
                {
                    MY_LOGE("m_pIMemDrv->freeVirtBuf() error");
                }
                else
                {
                    mvImgBufMap.erase(it);
                }
                break;
            }
        }
    }
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
requestBufs()
{
    MBOOL   ret = MFALSE;
	
    mu4W_yuv = mShotParam.mi4PictureWidth;
    mu4H_yuv = mShotParam.mi4PictureHeight;
	EImageFormat mPostviewFormat = static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);  	
	
   	// YUV source buffer
    mpSource = allocMem(eImgFmt_YUY2, mu4W_yuv, mu4H_yuv);
	if(!mpSource)
    {
        MY_LOGE("[requestBufs] mpSource alloc fail");
        goto lbExit;
    }
	
    //postview buffer
	mPostviewWidth = mShotParam.mi4PostviewWidth;
	mPostviewHeight = mShotParam.mi4PostviewHeight;

    mpPostviewImgBuf = allocMem(mPostviewFormat, mPostviewWidth, mPostviewHeight);
	if(!mpPostviewImgBuf)
    {
        MY_LOGE("[requestBufs] mpPostviewImgBuf alloc fail");
        goto lbExit;
    }

    ret = MTRUE;
	
lbExit:
	if	( ! ret )
	{
		releaseBufs();
	}
	return	ret;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
releaseBufs()
{
	deallocMem(mpSource);
	deallocMem(mpPostviewImgBuf);

    return  MTRUE;
}


/*******************************************************************************
*
*******************************************************************************/
long getDenoiseCosttime()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec*1000+time.tv_usec/1000);
}


/*******************************************************************************
*
*******************************************************************************/
static char* get_time_stamp()
{
	char timestr[40];
	
	time_t time_st;
	struct tm *tm_pt;
	struct timeval tv;
		
	time(&time_st);
	tm_pt = gmtime(&time_st);
	gettimeofday(&tv, NULL);
		  
	sprintf(timestr, "%d%02d%02d_%02d%02d%02d.%03d", tm_pt->tm_year+1900, tm_pt->tm_mon+1, tm_pt->tm_mday, tm_pt->tm_hour, tm_pt->tm_min, tm_pt->tm_sec, tv.tv_usec/1000);
		
	return timestr;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
createYUVFrame(IImageBuffer* Srcbufinfo)
{
	MBOOL ret = MTRUE; 
	MBOOL isMfbShot = MFALSE;
	NSCamShot::ICamShot *pSingleShot = NSCamShot::ISmartShot::createInstance(static_cast<EShotMode>(mu4ShotMode)
																			, "NormalShot"
																			, getOpenId()
																			, mShotParam.mu4MultiFrameBlending
																			, &isMfbShot
																			);
	//
	MUINT32 nrtype = queryCapNRType( getCaptureIso(), isMfbShot);
	// 
	pSingleShot->init(); 
	// 
	pSingleShot->enableNotifyMsg( NSCamShot::ECamShot_NOTIFY_MSG_EOF ); 
	//
	EImageFormat ePostViewFmt = 
		static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);

	pSingleShot->registerImageBuffer(NSCamShot::ECamShot_BUF_TYPE_YUV, Srcbufinfo);

	pSingleShot->enableDataMsg(NSCamShot::ECamShot_DATA_MSG_YUV);

	// shot param 
	NSCamShot::ShotParam rShotParam(
			eImgFmt_YUY2,					 //yuv format 
			mShotParam.mi4PictureWidth, 	 //picutre width 
			mShotParam.mi4PictureHeight,	 //picture height
			0,		 						 //picture transform 
			ePostViewFmt,					 //postview format 
			mShotParam.mi4PostviewWidth,	 //postview width 
			mShotParam.mi4PostviewHeight,	 //postview height 
			0,								 //postview transform
			mShotParam.mu4ZoomRatio 		 //zoom   
			);										 

 
	CamManager* pCamMgr = CamManager::getInstance();
	MUINT32 scenario = ( pCamMgr->isMultiDevice() && (pCamMgr->getFrameRate(getOpenId()) == 0) )
					? SENSOR_SCENARIO_ID_NORMAL_PREVIEW : SENSOR_SCENARIO_ID_NORMAL_CAPTURE;
																	 
	// sensor param 
	NSCamShot::SensorParam rSensorParam(
			getOpenId(),							//sensor idx
			scenario,								//Scenaio 
			10, 									//bit depth 
			MFALSE, 								//bypass delay 
			MFALSE									//bypass scenario 
			);	
	//
	pSingleShot->setCallbacks(fgCamShotNotifyCb, fgCamShotDataCb, this); 
	//	   
	ret = pSingleShot->setShotParam(rShotParam); 
	//
	ret = pSingleShot->sendCommand( NSCamShot::ECamShot_CMD_SET_PREVIEW_BUFHDL, (MINT32)mpPrvBufHandler, 0, 0); 
	//
	ret = pSingleShot->sendCommand( NSCamShot::ECamShot_CMD_SET_NRTYPE, nrtype, 0, 0 );
	// 
	ret = pSingleShot->startOne(rSensorParam); 
	//
	ret = pSingleShot->uninit(); 
	//
	pSingleShot->destroyInstance(); 

	return	ret;
}


/*******************************************************************************
*
*******************************************************************************/
MVoid ParserConfFile(char *pNcfpath, char const* mSensorDrvName, int iISO, int iImgWidth, int iImgHeight)
{
	char path[64];
	strcpy(path, "/system/vendor/denoise");
	
	if (iISO >= 800 && iISO < 1200)	// [800-1200)
	{
		sprintf(pNcfpath, "%s/%s/ISO%d_%dX%d.ncf", path, mSensorDrvName, 800, iImgWidth, iImgHeight); 
	}
	else if (iISO >= 1200 && iISO <= 1600)	// [1200-1600]
	{
		sprintf(pNcfpath, "%s/%s/ISO%d_%dX%d.ncf", path, mSensorDrvName, 1600, iImgWidth, iImgHeight); 
	}
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
DenoiseProcess(IImageBuffer* Srcbufinfo)
{
	//Todo:only support rear camera
	if (mSensorDev != SENSOR_DEV_MAIN) {
		MY_LOGE("only rear camera support denoise");
		return false;	
	}

#if defined(ACER_JADEL)
	//Todo:only support resolution 4160x3120 and 4096x2304
	if((mu4W_yuv != 4160 || mu4H_yuv != 3120) && (mu4W_yuv != 4096 || mu4H_yuv != 2304))
	{
		MY_LOGE("image resolution not support, mu4W_yuv=%d, mu4H_yuv=%d", mu4W_yuv, mu4H_yuv);
		return false;
	}
#elif defined(ACER_Z410)
    //Todo:only support resolution 2560x1920 and 2560x1440
	if((mu4W_yuv != 2560 || mu4H_yuv != 1920) && (mu4W_yuv != 2560 || mu4H_yuv != 1440))
	{
		MY_LOGE("image resolution not support, mu4W_yuv=%d, mu4H_yuv=%d", mu4W_yuv, mu4H_yuv);
		return false;
	}
#else 
    return false;
#endif

    if (ISOValue < 800 || ISOValue > 1600) {
		MY_LOGE("ISOValue < 800 not need denoise");
		return false;
	}
	
#ifdef SMART_DENOISE_DEBUG	//dump input yuv image  
	char fileName[256];
	sprintf(fileName, "/mnt/sdcard/DCIM/normalshot_%dx%d_src_%s.yuyv",mu4W_yuv, mu4H_yuv, get_time_stamp());
	FILE *fp = fopen(fileName, "wb");
	if (NULL == fp)
	{
   		MY_LOGE("fail to open file to save img: %s", fileName);
   		return false;
 	}
    
	fwrite((MUINT8*)Srcbufinfo->getBufVA(0), 1, Srcbufinfo->getBufSizeInBytes(0), fp);
	fclose(fp);
#endif //SMART_DENOISE_DEBUG

	int ret = MOK;

	ASD_OFFSCREEN srcImg = {0}; 
	srcImg.lPixelArrayFormat = ASD_PAF_YUV422_YUYV;
	srcImg.lWidth = mu4W_yuv;
	srcImg.lHeight = mu4H_yuv;
	srcImg.pixelArray.chunky.lLineBytes = 2*srcImg.lWidth;
	srcImg.pixelArray.chunky.pPixel = (MVoid *)Srcbufinfo->getBufVA(0);
	
	MY_LOGD("srcImg:{format=%08x, width=%d, height=%d, lLineBytes=%d, pPixel=%p}", 
		srcImg.lPixelArrayFormat , srcImg.lWidth, srcImg.lHeight, srcImg.pixelArray.chunky.lLineBytes, 
		srcImg.pixelArray.chunky.pPixel);

	if(MNull == g_hDenoiseHandler)
	{
		MY_LOGE("MNull == g_hDenoiseHandler");
		return false;
	}
	char szConfig[256];
	ParserConfFile(szConfig, mSensorDrvName, ISOValue, srcImg.lWidth, srcImg.lHeight);
	MY_LOGE("ParserConfFile szConfig: %s", szConfig);
	
	if(MNull == g_pNcfFile)
	{
		MY_LOGE("g_pNcfFile MNull == g_pNcfFile");
		return false;
	}
	
	FILE *fpNoise = fopen(szConfig, "rb");
	if(MNull == fpNoise)
	{
		MY_LOGE("file open error MNull == fpNoise");
		return false;
	}
	fread(g_pNcfFile, ASD_NCF_MEM_LEN, 1, fpNoise);
	fclose(fpNoise);

	MY_LOGD("ASD_NoiseConfigFromMemory in g_hDenoiseHandler = %p, g_pNcfFile=%p", g_hDenoiseHandler, g_pNcfFile);
	ret = ASD_NoiseConfigFromMemory(g_hDenoiseHandler, g_pNcfFile);
	if(ret != MOK)
	{
		MY_LOGE("ASD_NoiseConfigFromMemory error ret = %d", ret);
		return false;
	}

	ASD_SetDenoiserType(g_hDenoiseHandler, ASD_BEST_QUALITY);

	long timeStart = getDenoiseCosttime();
#if defined(ACER_JADEL)	
	ASD_SetTunerLevel(g_hDenoiseHandler, 90, ASD_LUMIN_CHANNEL, ASD_TUNER_NOISE_LEVEL);
    ASD_SetTunerLevel(g_hDenoiseHandler, 90, ASD_COLOR_CHANNEL, ASD_TUNER_NOISE_LEVEL);
#endif
	ret = ASD_Denoise(g_hDenoiseHandler, &srcImg, &srcImg, MNull, MNull);
	MY_LOGD("ASD_Denoise--->cost time = %d", getDenoiseCosttime() - timeStart);
	if(ret != MOK)
	{
		MY_LOGE("ASD_Denoise error ret = %d", ret);
		return false;
	}

#ifdef SMART_DENOISE_DEBUG	//dump output yuv image
	sprintf(fileName, "/mnt/sdcard/DCIM/normalshot_%dx%d_res_%s.yuyv",mu4W_yuv, mu4H_yuv, get_time_stamp());
	fp = fopen(fileName, "wb");
	if (NULL == fp)
	{
   		MY_LOGE("fail to open file to save img: %s", fileName);
   		return false;
 	}
    
	fwrite((MUINT8*)Srcbufinfo->getBufVA(0), 1, Srcbufinfo->getBufSizeInBytes(0), fp);
	fclose(fp);
#endif //SMART_DENOISE_DEBUG

    return true;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
ImgProcess(IImageBuffer* Srcbufinfo, MUINT32 srcWidth, MUINT32 srcHeight, EImageFormat srctype, IImageBuffer* Desbufinfo, MUINT32 desWidth, MUINT32 desHeight, EImageFormat destype, MUINT32 transform) const
{
    MY_LOGD("[Resize] srcAdr 0x%x srcWidth %d srcHeight %d desAdr 0x%x desWidth %d desHeight %d ",(MUINT32)Srcbufinfo,Srcbufinfo->getImgSize().w,Srcbufinfo->getImgSize().h,(MUINT32)Desbufinfo,Desbufinfo->getImgSize().w,Desbufinfo->getImgSize().h);
    
    Srcbufinfo->syncCache(eCACHECTRL_FLUSH);
    
    NSCam::NSIoPipe::NSSImager::ISImager *mpISImager = NSCam::NSIoPipe::NSSImager::ISImager::createInstance(Srcbufinfo);
    if (mpISImager == NULL)
    {
        MY_LOGE("Null ISImager Obj \n");
        return MFALSE;
    }

    //
    mpISImager->setTargetImgBuffer(Desbufinfo);
    //
    mpISImager->setTransform(transform);
    //
    mpISImager->setEncodeParam(1, 90);
    //
    mpISImager->execute();
	//
    MY_LOGD("[Resize] Out");
    return  MTRUE;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
createJpegImg(IImageBuffer const * rSrcImgBufInfo
      , NSCamShot::JpegParam const & rJpgParm
      , MUINT32 const u4Transform
      , IImageBuffer const * rJpgImgBufInfo
      , MUINT32 & u4JpegSize)
{
    MBOOL ret = MTRUE;

    MY_LOGD("[createJpegImg] - E.");
    MY_LOGD("[createJpegImg] - rSrcImgBufInfo.eImgFmt=%d", rSrcImgBufInfo->getImgFormat());
    MY_LOGD("[createJpegImg] - u4Transform=%d", u4Transform);

    // (1). Create Instance
    NSCam::NSIoPipe::NSSImager::ISImager *pISImager = NSCam::NSIoPipe::NSSImager::ISImager::createInstance(rSrcImgBufInfo);
    if(!pISImager) {
    	MY_LOGE("Normal Shot::createJpegImg can't get ISImager instance.");
    	return MFALSE;
    }

    // init setting
    pISImager->setTargetImgBuffer(rJpgImgBufInfo);
    //
    pISImager->setTransform(u4Transform);
    //
    pISImager->setEncodeParam(rJpgParm.fgIsSOI, rJpgParm.u4Quality);
    //
    pISImager->execute();
    //
    u4JpegSize = rJpgImgBufInfo->getBitstreamSize();

    pISImager->destroyInstance();

    MY_LOGD("[createJpegImg] - X. ret: %d.", ret);
    return ret;
}



/*******************************************************************************
*
*******************************************************************************/
MBOOL
NormalShot::
createJpegImgWithThumbnail(IImageBuffer const *rYuvImgBufInfo, IImageBuffer const *rPostViewBufInfo)
{
    MBOOL ret = MTRUE;
    MUINT32 stride[3];
    MY_LOGD("[createJpegImgWithThumbnail] in");
    MBOOL bVertical = ( mShotParam.mu4Transform == NSCam::eTransform_ROT_90 || 
            mShotParam.mu4Transform == NSCam::eTransform_ROT_270 );
    IImageBuffer* jpegBuf = allocMem(eImgFmt_JPEG, 
                                     bVertical? mu4H_yuv: mu4W_yuv, 
                                     bVertical? mu4W_yuv: mu4H_yuv);
    if(jpegBuf == NULL)
    {
        MY_LOGE("[createJpegImgWithThumbnail] jpegBuf alloc fail");
        ret = MFALSE;
        return ret;
    }

    //rThumbImgBufInfo
    IImageBuffer* thumbBuf = NULL;
	if (0 != mJpegParam.mi4JpegThumbWidth && 0 != mJpegParam.mi4JpegThumbHeight)
	{	
		thumbBuf = allocMem(eImgFmt_JPEG, 
                                      bVertical? mJpegParam.mi4JpegThumbHeight: mJpegParam.mi4JpegThumbWidth,
                                      bVertical? mJpegParam.mi4JpegThumbWidth: mJpegParam.mi4JpegThumbHeight);
		if(thumbBuf == NULL)
	    {
	        MY_LOGE("[createJpegImgWithThumbnail] thumbBuf alloc fail");
	    	ret = MFALSE;
	    	return ret;
	    }
	}

    MUINT32 u4JpegSize = 0;
    MUINT32 u4ThumbSize = 0;

    NSCamShot::JpegParam yuvJpegParam(mJpegParam.mu4JpegQuality, MFALSE);
    ret = ret && createJpegImg(rYuvImgBufInfo, yuvJpegParam, mShotParam.mu4Transform, jpegBuf, u4JpegSize);

    // (3.1) create thumbnail
    // If postview is enable, use postview buffer,
    // else use yuv buffer to do thumbnail
    if (0 != mJpegParam.mi4JpegThumbWidth && 0 != mJpegParam.mi4JpegThumbHeight)
    {
        NSCamShot::JpegParam rParam(mJpegParam.mu4JpegThumbQuality, MTRUE);
        ret = ret && createJpegImg(rPostViewBufInfo, rParam, mShotParam.mu4Transform, thumbBuf, u4ThumbSize);
    }

    jpegBuf->syncCache(eCACHECTRL_INVALID);
	if(thumbBuf != NULL)
	{
    	thumbBuf->syncCache(eCACHECTRL_INVALID);
	}

	handleJpegData(jpegBuf, thumbBuf, mDebugInfo);

    deallocMem(jpegBuf);
	if(thumbBuf != NULL)
	{
    	deallocMem(thumbBuf);
	}
    MY_LOGD("[createJpegImgWithThumbnail] out");
    return ret;
}

