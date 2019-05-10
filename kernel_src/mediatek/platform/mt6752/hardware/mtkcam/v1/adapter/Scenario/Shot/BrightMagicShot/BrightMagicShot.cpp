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
#include <mtkcam/camshot/IBurstShot.h>
#include <mtkcam/camshot/ISmartShot.h>
#include <mtkcam/camshot/_params.h>
//
#include <mtkcam/exif/IBaseCamExif.h>
//
#include <Shot/IShot.h>
//
#include "ImpShot.h"
#include "BrightMagicShot.h"
//
#include <mtkcam/hwutils/CamManager.h>
using namespace NSCam::Utils;
//
#include <mtkcam/utils/Format.h>
#include <mtkcam/utils/ImageBufferHeap.h>
#include <mtkcam/iopipe/SImager/ISImager.h>

#include <mtkcam/featureio/aaa_hal_if.h>
//
#include "arcsoft_night_shot.h"
#include "ammem.h"
//
using namespace android;
using namespace NSShot;
using namespace NSCamShot;
using namespace NS3A;


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
#define NIGHT_SHOT_FEATURE   (1)
#endif

#define SAFE_MEM_FREE(MEM)		if (MNull != MEM) {free(MEM); MEM = MNull;}
#define	SAFE_MEM_DEL(MEM)			if (MNull != MEM) {delete MEM; MEM = MNull;}
#define	SAFE_MEMARR_DEL(MEM)		if (MNull != MEM) {delete[] MEM; MEM = MNull;}

//#define	NIGHT_SHOT_DEBUG
#define	NIGHT_SHOT_MEM_SIZE		(10*1024*1024)
MHandle	g_hNSMemMgr = MNull;
MVoid*	g_pNSMem = MNull;
MHandle	g_hEnhancer = MNull;
ANS_INPUTINFO g_InputInfo = {0};
ASVLOFFSCREEN g_DstImg = {0};
ANS_PARAM g_Param = {0};


/******************************************************************************
 *
 ******************************************************************************/
extern "C"
sp<IShot>
createInstance_BrightMagicShot(
    char const*const    pszShotName, 
    uint32_t const      u4ShotMode, 
    int32_t const       i4OpenId
)
{
    sp<IShot>       pShot = NULL;
    sp<BrightMagicShot>  pImpShot = NULL;
    //
    //  (1.1) new Implementator.
    pImpShot = new BrightMagicShot(pszShotName, u4ShotMode, i4OpenId);
    if  ( pImpShot == 0 ) {
        CAM_LOGE("[%s] new BrightMagicShot", __FUNCTION__);
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
BrightMagicShot::
onCreate()
{
#warning "[TODO] BrightMagicShot::onCreate()"

	MBOOL   ret = MFALSE;
	int res = MOK;

	g_pNSMem 	= malloc(NIGHT_SHOT_MEM_SIZE);
	g_hNSMemMgr	= MMemMgrCreate(g_pNSMem, NIGHT_SHOT_MEM_SIZE);
	if(MNull == g_pNSMem || MNull == g_hNSMemMgr)
	{
		MY_LOGE("NightShot memory init is null");
		return MFALSE;
	}

 	res = ANS_Init(g_hNSMemMgr, &g_hEnhancer);
	if(MOK != res)
	{
		MY_LOGE("NightShot ANS_Init error res = %d", res);
		if(g_hNSMemMgr)
		{
			MMemMgrDestroy(g_hNSMemMgr);
			g_hNSMemMgr = MNull;
		}

		SAFE_MEM_FREE(g_pNSMem);
		
		return MFALSE;
	}

    mpIMemDrv =  IMemDrv::createInstance();
    if (mpIMemDrv == NULL)
    {
        MY_LOGE("g_pIMemDrv is NULL \n");
        return MFALSE;
    }
	mDebugInfo = IDbgInfoContainer::createInstance();
	
    ret = MTRUE;
    return  ret;
}


/******************************************************************************
 *  This function is invoked when this object is ready to destryoed in the
 *  destructor. All resources must be released before this returns.
 ******************************************************************************/
void
BrightMagicShot::
onDestroy()
{
#warning "[TODO] BrightMagicShot::onDestroy()"

	if(MNull != g_hEnhancer)
	{
		ANS_Uninit(&g_hEnhancer);
		g_hEnhancer = MNull;
	}
	
  	if(g_hNSMemMgr)
	{
		MMemMgrDestroy(g_hNSMemMgr);
		g_hNSMemMgr = MNull;
	}

	SAFE_MEM_FREE(g_pNSMem);

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
    if  (mpIMemDrv)
    {
        mpIMemDrv->destroyInstance();
        mpIMemDrv = NULL;
    }

	if(mDebugInfo != NULL) {
        mDebugInfo->destroyInstance();
    }
}


/*******************************************************************************
*
*******************************************************************************/
BrightMagicShot::
BrightMagicShot(
	char const*const pszShotName,
    uint32_t const u4ShotMode,
    int32_t const i4OpenId
)
    : ImpShot(pszShotName, u4ShotMode, i4OpenId)
{
    
}


/******************************************************************************
 *
 ******************************************************************************/
BrightMagicShot::
~BrightMagicShot()
{
    
}


/******************************************************************************
 *
 ******************************************************************************/
bool
BrightMagicShot::
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
BrightMagicShot::
onCmd_reset()
{
#warning "[TODO] BrightMagicShot::onCmd_reset()"
    bool ret = true;
    return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
bool
BrightMagicShot::
onCmd_capture()
{
	bool ret = false;
	
	ret = doCapture();
	if	( ! ret )
	{
		goto lbExit;
	}
	ret = true;
lbExit:
	releaseBufs();
	return ret;
}


/******************************************************************************
 *
 ******************************************************************************/
void
BrightMagicShot::
onCmd_cancel()
{
    AutoCPTLog cptlog(Event_Shot_cancel);
#warning "[TODO] BrightMagicShot::onCmd_cancel()"
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL 
BrightMagicShot::
fgCamShotNotifyCb(MVOID* user, NSCamShot::CamShotNotifyInfo const msg)
{
    AutoCPTLog cptlog(Event_Shot_handleNotifyCb);
    BrightMagicShot *pBrightMagicShot = reinterpret_cast <BrightMagicShot *>(user); 
    if (NULL != pBrightMagicShot) 
    {
        if ( NSCamShot::ECamShot_NOTIFY_MSG_EOF == msg.msgType) 
        {
            pBrightMagicShot->mpShotCallback->onCB_Shutter(true, 0);                                                     
        }
    }

    return MTRUE; 
}


/******************************************************************************
 *
 ******************************************************************************/
MBOOL
BrightMagicShot::
fgCamShotDataCb(MVOID* user, NSCamShot::CamShotDataInfo const msg)
{
    BrightMagicShot *pBrightMagicShot = reinterpret_cast<BrightMagicShot *>(user); 
    if (NULL != pBrightMagicShot) 
    {
        if (NSCamShot::ECamShot_DATA_MSG_POSTVIEW == msg.msgType) 
        {
            //pBrightMagicShot->handlePostViewData((MUINT8*)msg.pBuffer->getBufVA(0), msg.pBuffer->getBufSizeInBytes(0));  
        }
        else if (NSCamShot::ECamShot_DATA_MSG_JPEG == msg.msgType)
        {
            pBrightMagicShot->handleJpegData(
                    (IImageBuffer*)msg.pBuffer,
                    (IImageBuffer*)msg.ext1,
                    (IDbgInfoContainer*)msg.ext2
                    );
        }
		else if (NSCamShot::ECamShot_DATA_MSG_YUV == msg.msgType)
        {
            pBrightMagicShot->handleYuvDataCallback((MUINT8*)msg.pBuffer->getBufVA(0), msg.pBuffer->getBufSizeInBytes(0));
			{   //dbginfo
                IDbgInfoContainer* pDbgInfo = reinterpret_cast<IDbgInfoContainer*>(msg.ext2);
                BrightMagicShot *self = reinterpret_cast<BrightMagicShot *>(user);
                if (self == NULL)
                {
                	CAM_LOGE("[fgCamShotDataCb] user is NULL");
                    return MFALSE;
                }
                pDbgInfo->copyTo(self->mDebugInfo);
            }
        }
    }

    return MTRUE; 
}


/******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
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
BrightMagicShot::
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
BrightMagicShot::
handleYuvDataCallback(MUINT8* const puBuf, MUINT32 const u4Size)
{
    MY_LOGD("(puBuf, size) = (%p, %d)", puBuf, u4Size);

    return MFALSE;
}


/******************************************************************************
*
*******************************************************************************/
bool
BrightMagicShot::
doCapture()
{
    MBOOL ret = MFALSE;    

    ret = requestBufs();
	ret = createYUVFrame();
	ret = NightshotProcess();	
	
	EImageFormat mPostviewFormat = static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);
		
	ret = ImgProcess(mpDesBuf, mu4W_yuv, mu4H_yuv, eImgFmt_NV21, mpPostviewImgBuf, mPostviewWidth, mPostviewHeight, mPostviewFormat);
	ret = createJpegImgWithThumbnail(mpDesBuf, mpPostviewImgBuf);

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
BrightMagicShot::
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
BrightMagicShot::
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
BrightMagicShot::
requestBufs()
{
    MBOOL   ret = MFALSE;
	
    mu4W_yuv = mShotParam.mi4PictureWidth;
    mu4H_yuv = mShotParam.mi4PictureHeight;
	EImageFormat mPostviewFormat = static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);  	
	
   	// YUV source buffer
	for (int i = 0; i < BMC_IMAGE_NUM; i++)
    {
   		mpSource[i] = allocMem(eImgFmt_NV21, mu4W_yuv, mu4H_yuv);
		if(!mpSource[i])
    	{
        	MY_LOGE("[requestBufs] mpSource[%d] alloc fail", i);
        	goto lbExit;
    	}	
    }

	// YUV dst buffer
    mpDesBuf = allocMem(eImgFmt_NV21, mu4W_yuv, mu4H_yuv);
	if(!mpDesBuf)
	{
		MY_LOGE("[requestBufs] mpDesBuf alloc fail");
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
BrightMagicShot::
releaseBufs()
{
	for (int i = 0; i < BMC_IMAGE_NUM; i++) {
		deallocMem(mpSource[i]);
	}
	deallocMem(mpDesBuf);
	deallocMem(mpPostviewImgBuf);

    return  MTRUE;
}


/*******************************************************************************
*
*******************************************************************************/
long getNStime()
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
BrightMagicShot::
createYUVFrame()
{
	MBOOL ret = MTRUE; 
	MBOOL isMfbShot = MFALSE;

	NSCamShot::IBurstShot *pBurstShot = NSCamShot::IBurstShot::createInstance(eShotMode_BrightMagicShot, "BrightMagicShot");

	//
	MUINT32 nrtype = queryCapNRType(getCaptureIso(), isMfbShot);
	// 
	pBurstShot->init();
	// 
	pBurstShot->enableNotifyMsg( NSCamShot::ECamShot_NOTIFY_MSG_EOF ); 
	//
	EImageFormat ePostViewFmt = 
		static_cast<EImageFormat>(mShotParam.miPostviewDisplayFormat);

	for(int i = 0; i < BMC_IMAGE_NUM; i++) {
		pBurstShot->registerImageBuffer(NSCamShot::ECamShot_BUF_TYPE_YUV, mpSource[i]);
	}

	pBurstShot->enableDataMsg(NSCamShot::ECamShot_DATA_MSG_YUV);

	// shot param 
	NSCamShot::ShotParam rShotParam(
			eImgFmt_NV21,					 //yuv format 
			mShotParam.mi4PictureWidth, 	 //picutre width 
			mShotParam.mi4PictureHeight,	 //picture height
			0,		 						 //picture transform 
			ePostViewFmt,					 //postview format 
			mShotParam.mi4PostviewWidth,	 //postview width 
			mShotParam.mi4PostviewHeight,	 //postview height 
			0,								 //postview transform
			mShotParam.mu4ZoomRatio 		 //zoom   
			);										 
																	 
	// sensor param 
	NSCamShot::SensorParam rSensorParam(
			getOpenId(),							//sensor idx
			SENSOR_SCENARIO_ID_NORMAL_CAPTURE,		//Scenaio 
			10, 									//bit depth 
			MFALSE, 								//bypass delay 
			MFALSE									//bypass scenario 
			);

	NSCamShot::SensorParam rSensorBiningParam(
			getOpenId(),							//sensor idx
			SENSOR_SCENARIO_ID_CUSTOM1,				//Scenaio 
			10, 									//bit depth 
			MFALSE, 								//bypass delay 
			MFALSE									//bypass scenario 
			);

	//
	pBurstShot->setCallbacks(fgCamShotNotifyCb, fgCamShotDataCb, this);
	//
	ret = pBurstShot->setShotParam(rShotParam);
	//
	ret = ret && pBurstShot->setIspProfile(nrtype==ECamShot_NRTYPE_SWNR ? EIspProfile_Capture_SWNR : EIspProfile_Capture);
	// 
	MY_LOGD("[take 6 pictures start] \n");
	long timeStart = getNStime();
	if (0 == strcmp(mShotParam.mu4Bingingmode.string(), "on"))	//qiaoxiujun,bining mode
	{
		ret = pBurstShot->start(rSensorBiningParam, BMC_IMAGE_NUM);
	}
	else
	{
		ret = pBurstShot->start(rSensorParam, BMC_IMAGE_NUM);
	}
	MY_LOGD("[take 6 pictures end] cost time = %d", getNStime() - timeStart);
	//
	ret = pBurstShot->uninit(); 
	//
	pBurstShot->destroyInstance(); 

	return	ret;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
NightshotProcess()
{
#ifdef NIGHT_SHOT_DEBUG
	char path[256];
	FILE* file = MNull;
	for(int i = 0; i < MAX_INPUT_IMAGES; i ++)
	{
		sprintf(path, "/mnt/sdcard/DCIM/nightshot_%dx%d_%d_%s.nv21", mu4W_yuv, mu4H_yuv, i, get_time_stamp());
		file = fopen(path, "wb");
		fwrite((void*)(mpSource[i]->getBufVA(0)), mpSource[i]->getBufSizeInBytes(0), 1, file);
		fclose(file);
	}
#endif

	if(MNull == g_hEnhancer)
	{
		MY_LOGE("MNull == g_hEnhancer");
		return false;
	}

	g_DstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
	g_DstImg.i32Width  = mu4W_yuv;
	g_DstImg.i32Height = mu4H_yuv;
	g_DstImg.pi32Pitch[0] = g_DstImg.i32Width;
	g_DstImg.pi32Pitch[1] = g_DstImg.i32Width;
	g_DstImg.ppu8Plane[0] = (MUInt8*)(mpDesBuf->getBufVA(0));
	g_DstImg.ppu8Plane[1] = g_DstImg.ppu8Plane[0] + g_DstImg.i32Width * g_DstImg.i32Height;

	ASVLOFFSCREEN srcImg[MAX_INPUT_IMAGES];

	for(int i = 0; i < MAX_INPUT_IMAGES; i ++)
	{
		srcImg[i].u32PixelArrayFormat = ASVL_PAF_NV21;
		srcImg[i].i32Width = mu4W_yuv;
		srcImg[i].i32Height = mu4H_yuv;
		srcImg[i].pi32Pitch[0] = srcImg[i].i32Width;
		srcImg[i].pi32Pitch[1] = srcImg[i].i32Width;
		srcImg[i].ppu8Plane[0] = (MUInt8*)(mpSource[i]->getBufVA(0));
		srcImg[i].ppu8Plane[1] = srcImg[i].ppu8Plane[0] + srcImg[i].i32Width * srcImg[i].i32Height ;

		g_InputInfo.lImgNum = MAX_INPUT_IMAGES;
		g_InputInfo.pImages[i] = &(srcImg[i]);
	}

	ANS_GetDefaultParam(&g_Param);
	MY_LOGD("ANS_GetDefaultParam g_Param.lRefNum = %d", g_Param.lRefNum);

	MY_LOGD("ANS_Enhancement In g_hEnhancer=%p", g_hEnhancer);
	long timeStart = getNStime();
	int res = ANS_Enhancement(g_hEnhancer, &g_InputInfo, &g_DstImg, &g_Param, MNull, MNull);
	MY_LOGD("ANS_Enhancement--->cost time = %d", getNStime() - timeStart);
	MY_LOGD("ANS_Enhancement Out res=%d, g_Param.lRefNum=%d", res, g_Param.lRefNum);
	
	if(MOK != res)
	{
		MY_LOGE("ANS_Enhancement process error res = %d", res);
		return false;
	}

#ifdef NIGHT_SHOT_DEBUG
	sprintf(path, "/mnt/sdcard/DCIM/nightshot_%dx%d_res_%s.nv21", mu4W_yuv, mu4H_yuv, get_time_stamp());
	file = fopen(path, "wb");
	fwrite((void*)(mpDesBuf->getBufVA(0)), mpDesBuf->getBufSizeInBytes(0), 1, file);
	fclose(file);
#endif

	return true;
}


/*******************************************************************************
*
*******************************************************************************/
MBOOL
BrightMagicShot::
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
BrightMagicShot::
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
    	MY_LOGE("BrightMagic Shot::createJpegImg can't get ISImager instance.");
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
BrightMagicShot::
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

