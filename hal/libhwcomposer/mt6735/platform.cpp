#define DEBUG_LOG_TAG "PLAT"

#include "gralloc_mtk_defs.h"

#include <hardware/hwcomposer.h>
#include <hardware/gralloc.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "utils/debug.h"
#include "utils/tools.h"

#ifdef USE_HWC2
#include "hwc2.h"
#endif

#include "hwdev.h"
#include "platform.h"

#include "m4u_lib.h"
#include "DpBlitStream.h"

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include "ui/gralloc_extra.h"

#include "bandwidth_control.h"

#include <cutils/properties.h>

#define HW_MT6795M "0335"
extern unsigned int mapDpOrientation(const uint32_t transform);

// ---------------------------------------------------------------------------

BWC_MONITOR g_bwc_monitor;

// ---------------------------------------------------------------------------

ANDROID_SINGLETON_STATIC_INSTANCE(Platform);

Platform::Platform()
{
    m_config.platform = PLATFORM_MT6795;

    m_config.compose_level = COMPOSE_ENABLE_ALL;

    m_config.mirror_state = MIRROR_ENABLED;

    m_config.overlay_cap = (OVL_CAP_DIM | OVL_CAP_DIM_HW | OVL_CAP_P_FENCE);

    const char *files[] =
    {
        "/proc/chip/hw_code",
    };
    int i = 0;
    char str[8] = {0};
    FILE* fp = fopen(files[i], "rb");
    int rd_size;

    assert(!fp);

    if (4 != (rd_size = fread(str, 1, 4, fp)))
    {
        HWC_LOGE("ERROR: read error");
    }

    if (strcmp (str,HW_MT6795M) == 0)
    {
        char propVal[PROPERTY_VALUE_MAX];
        property_get("ro.build.characteristics", propVal, "default");
        if (strcmp(propVal, "tablet") == 0)
        {
            m_config.mirror_state = MIRROR_DISABLED;
        }
    }
    fclose(fp);

    m_config.ovl_overlap_limit = 0;

    m_config.bq_count = 4;

    m_config.use_async_bliter = false;

    m_config.use_async_bliter_ultra = false;

    m_config.is_support_ext_path_for_virtual = false;

    m_config.enable_rgbx_scaling = false;

    m_config.only_wfd_by_hwc = false;

    m_config.support_color_transform = true;
}

void Platform::initOverlay()
{
    //MMUDevice::getInstance().enable(M4U_PORT_DISP_OVL0);
    //MMUDevice::getInstance().config(M4U_PORT_DISP_OVL0, true);
}

bool Platform::isUILayerValid(
    int /*dpy*/, struct hwc_layer_1* layer, PrivateHandle* priv_handle)
{
    if (priv_handle->format != HAL_PIXEL_FORMAT_IMG1_BGRX_8888 &&
        (priv_handle->format < HAL_PIXEL_FORMAT_RGBA_8888 ||
         priv_handle->format > HAL_PIXEL_FORMAT_BGRA_8888))
        return false;

    // hw does not support HWC_BLENDING_COVERAGE
    if (layer->blending == HWC_BLENDING_COVERAGE)
        return false;

    // [NOTE]
    // 1. overlay engine does not support RGBX format
    //    the only exception is that the format is RGBX and the constant alpha is 0xFF
    //    in such a situation, the display driver would disable alpha blending automatically,
    //    treating this format as RGBA with ignoring the undefined alpha channel
    // 2. overlay does not support using constant alpah
    //    and premult blending at same time
    if ((layer->planeAlpha != 0xFF) &&
        (priv_handle->format == HAL_PIXEL_FORMAT_RGBX_8888 ||
         priv_handle->format == HAL_PIXEL_FORMAT_IMG1_BGRX_8888 ||
         layer->blending == HWC_BLENDING_PREMULT))
        return false;

    // opaqaue layer should ignore alpha channel
    if (layer->blending == HWC_BLENDING_NONE)
    {
        switch (priv_handle->format)
        {
            case HAL_PIXEL_FORMAT_RGBA_8888:
                priv_handle->format = HAL_PIXEL_FORMAT_RGBX_8888;
                break;

            case HAL_PIXEL_FORMAT_BGRA_8888:
                return false;

            default:
                break;
        }
    }

    int w = getSrcWidth(layer);
    int h = getSrcHeight(layer);

    // ovl cannot accept <=0
    if (w <= 0 || h <= 0)
        return false;

    // [NOTE]
    // Since OVL does not support float crop, adjust coordinate to interger
    // as what SurfaceFlinger did with hwc before version 1.2
    int src_left = getSrcLeft(layer);
    int src_top = getSrcTop(layer);

    // cannot handle source negative offset
    if (src_left < 0 || src_top < 0)
        return false;

    // cannot handle rotation
    if (layer->transform != 0)
        return false;

    // cannot handle scaling
    if (WIDTH(layer->displayFrame) != w || HEIGHT(layer->displayFrame) != h)
        return false;

    return true;
}

bool Platform::isMMLayerValid(
    int dpy, struct hwc_layer_1* layer, PrivateHandle* priv_handle, bool& /*is_high*/)
{
    // only use MM layer without any blending consumption
    if (layer->blending != HWC_BLENDING_NONE)
        return false;

    int srcWidth = getSrcWidth(layer);
    int srcHeight = getSrcHeight(layer);

    // check src rect is not empty
    if (srcWidth <= 1 || srcHeight <= 1)
        return false;

    int dstWidth = WIDTH(layer->displayFrame);
    int dstHeight = HEIGHT(layer->displayFrame);

    // constraint to prevent bliter making error.
    // bliter would crop w or h to 0
    if (dstWidth <= 1 ||  dstHeight <= 1)
        return false;

    bool is_blit_valid = DpBlitStream::queryHWSupport(
                            srcWidth, srcHeight,
                            dstWidth, dstHeight, mapDpOrientation(layer->transform));

    if (!is_blit_valid)
        return false;

    int secure = (priv_handle->usage & (GRALLOC_USAGE_PROTECTED | GRALLOC_USAGE_SECURE));

    bool is_disp_valid = (dpy == HWC_DISPLAY_PRIMARY) || secure ||
                         (!(dstWidth & 0x01) && !(dstHeight & 0x01));

    return is_disp_valid;
}

size_t Platform::getLimitedExternalDisplaySize()
{
    // 4k resolution
    return 1920 * 1080;
}

int Platform::getBandwidthLimit()
{
    // TODO: use call back update value instead of quering each time
    return (g_bwc_monitor.query_hwc_max_pixel() << 2);
}

int Platform::computePitch(
    const int& pitch_bytes, const int& /*width_bytes*/, int* status)
{
    if (status != NULL) *status = COND_NOT_ALIGN;

    return pitch_bytes;
}

#ifdef USE_HWC2

bool Platform::isUILayerValid(const sp<HWCLayer>& layer, int32_t* line)
{
    return PlatformCommon::isUILayerValid(layer, line);
}


bool Platform::isMMLayerValid(const sp<HWCLayer>& layer, int32_t* line)
{
    return PlatformCommon::isMMLayerValid(layer, line);
}

#endif // USE_HWC2
