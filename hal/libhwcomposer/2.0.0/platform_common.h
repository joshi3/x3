#ifndef HWC_PLATFORM_COMMON_H_
#define HWC_PLATFORM_COMMON_H_

#include <utils/Singleton.h>
#include "hwc2.h"

using namespace android;

struct hwc_layer_1;
struct PrivateHandle;

// ---------------------------------------------------------------------------

enum OVL_GEN {
    OVL_GEN_6589 = 0x0001,
    OVL_GEN_6582 = 0x0002,
    OVL_GEN_6592 = 0x0004,
    OVL_GEN_6752 = 0x0008,
    OVL_GEN_6795 = 0x0010,
    OVL_GEN_6595 = 0x0020,
    OVL_GEN_6755 = 0x0040,
    OVL_GEN_6797 = 0x0080,
    OVL_GEN_6799 = 0x0100,
    OVL_GEN_6759 = 0x0200,
    OVL_GEN_6758 = 0x0400,
    OVL_GEN_6739 = 0x0800,
    OVL_GEN_6771 = 0x1000,
    OVL_GEN_6775 = 0x2000,
};

enum MDP_GEN {
    MDP_GEN_6589 = 0x0001,
    MDP_GEN_6582 = 0x0002,
    MDP_GEN_6595 = 0x0004,
    MDP_GEN_6797 = 0x0008,
    MDP_GEN_6757 = 0x0010,
    MDP_GEN_6739 = 0x0020,
};

enum PLATFORM_INFO {
    PLATFORM_NOT_DEFINE = 0,

    PLATFORM_MT8127 = OVL_GEN_6582 | (MDP_GEN_6582 << 16),
    PLATFORM_MT8135 = OVL_GEN_6589 | (MDP_GEN_6589 << 16),

    PLATFORM_MT6571 = OVL_GEN_6592 | (MDP_GEN_6582 << 16),
    PLATFORM_MT6572 = OVL_GEN_6582 | (MDP_GEN_6582 << 16),

    PLATFORM_MT6582 = OVL_GEN_6582 | (MDP_GEN_6582 << 16),
    PLATFORM_MT6589 = OVL_GEN_6589 | (MDP_GEN_6589 << 16),

    PLATFORM_MT6592 = OVL_GEN_6592 | (MDP_GEN_6582 << 16),
    PLATFORM_MT6595 = OVL_GEN_6595 | (MDP_GEN_6595 << 16),

    PLATFORM_MT6795 = OVL_GEN_6795 | (MDP_GEN_6595 << 16),
    PLATFORM_MT6739 = OVL_GEN_6739 | (MDP_GEN_6739 << 16),

    PLATFORM_MT6752 = OVL_GEN_6752 | (MDP_GEN_6595 << 16),
    PLATFORM_MT6755 = OVL_GEN_6755 | (MDP_GEN_6595 << 16),
    PLATFORM_MT6757 = OVL_GEN_6755 | (MDP_GEN_6757 << 16),

    PLATFORM_MT6763 = OVL_GEN_6759 | (MDP_GEN_6595 << 16),

    PLATFORM_MT6771 = OVL_GEN_6771 | (MDP_GEN_6797 << 16),
    PLATFORM_MT6775 = OVL_GEN_6775 | (MDP_GEN_6797 << 16),

    PLATFORM_MT6795 = OVL_GEN_6595 | (MDP_GEN_6595 << 16),
    PLATFORM_MT6797 = OVL_GEN_6797 | (MDP_GEN_6797 << 16),
    PLATFORM_MT6799 = OVL_GEN_6799 | (MDP_GEN_6797 << 16),
    PLATFORM_MT6758 = OVL_GEN_6758 | (MDP_GEN_6797 << 16),
};

enum HWC_MIRROR_STATE {
    MIRROR_UNDEFINED = 0, // reserved to avoid using this state by accident

    MIRROR_ENABLED   = (1 << 0),
    MIRROR_PAUSED    = (1 << 1),
    MIRROR_DISABLED  = (1 << 2),
};

enum HWC_OVL_CAPABILITY {
    OVL_CAP_UNDEFINE = 0,
    OVL_CAP_DIM      = (1 << 0),
    OVL_CAP_DIM_HW   = (1 << 1),
    OVL_CAP_P_FENCE  = (1 << 2),
};

enum HWC_MIR_FORMAT {
    MIR_FORMAT_UNDEFINE = 0,
    MIR_FORMAT_RGB888   = 1,
    MIR_FORMAT_YUYV     = 2,
    MIR_FORMAT_YV12     = 3,
};

// An abstract class of Platform. Each function of Platform must have a condidate in
// PlatformCommon to avoid compilation error except pure virtual functions.
class PlatformCommon
{
public:

    // COND_NOT_ALIGN:
    //   pitch of buffers do not align any value
    // COND_ALIGNED:
    //   pitch of buffers is aligned
    // COND_ALIGNED_SHIFT:
    //   pitch of buffers is aligned, and start address of each line is shifted
    enum ALIGN_CONDITION {
        COND_NOT_ALIGN     = 0,
        COND_ALIGNED       = 1,
        COND_ALIGNED_SHIFT = 2
    };

    PlatformCommon() { };
    virtual ~PlatformCommon() { };

    // initOverlay() is used to init overlay related setting
    void initOverlay();

    // isUILayerValid() is ued to verify
    // if ui layer could be handled by hwcomposer
    virtual bool isUILayerValid(int dpy, struct hwc_layer_1* layer,
            PrivateHandle* priv_handle);
    virtual bool isUILayerValid(const sp<HWCLayer>& layer, int32_t* line);

    // isMMLayerValid() is used to verify
    // if mm layer could be handled by hwcomposer
    virtual bool isMMLayerValid(int dpy, struct hwc_layer_1* layer,
            PrivateHandle* priv_handle, bool& is_high);
    virtual bool isMMLayerValid(const sp<HWCLayer>& layer, int32_t* line);

    // getUltraVideoSize() is used to return the limitation of video resolution
    // when this device connect with the maximum resolution of external display
    size_t getLimitedVideoSize();

    // getUltraDisplaySize() is used to return the limitation of external display
    // when this device play maximum resolution of video
    size_t getLimitedExternalDisplaySize();

    struct PlatformConfig
    {
        PlatformConfig();

        // platform define related hw family, includes ovl and mdp engine
        int platform;

        // compose_level defines default compose level
        int compose_level;

        // mirror_state defines mirror enhancement state
        int mirror_state;

        // overlay engine's capability
        int overlay_cap;

        // bq_count defines suggested amounts for bufferqueue
        int bq_count;

        // mir_scale_ratio defines the maxinum scale ratio of mirror source
        float mir_scale_ratio;

        // format_mir_mhl defines which color format
        // should be used as mirror result for MHL
        int format_mir_mhl;

        // ovl_overlap_limit is max number of overlap layers in one OVL composition
        // To avoid OVL from unferflow situation
        int ovl_overlap_limit;

        // hdcp checking id handled by display driver
        bool bypass_wlv1_checking;

        // can UI process prexform buffer
        int prexformUI;

        // can rdma support roi update
        int rdma_roi_update;

        // force full invalidate for partial update debug through setprop
        bool force_full_invalidate;

        // use_async_bliter defines the bliter mode
        bool use_async_bliter;

        // use async bliter ultra
        bool use_async_bliter_ultra;

        // force hwc to wait fence for display
        bool wait_fence_for_display;

        // Smart layer switch
        bool enable_smart_layer;

        // enable rgba rotate
        bool enable_rgba_rotate;

        // enable rgbx scaling
        bool enable_rgbx_scaling;

        bool av_grouping;

        // dump input buffers of hwc2
        char dump_buf_type;
        int32_t dump_buf;
        char dump_buf_cont_type;
        int32_t dump_buf_cont;
        bool dump_buf_log_enable;

        // debug flag for filleBalck function, this flag will fill white content
        // into outputbuffer if it no need to fill black
        bool fill_black_debug;

        bool always_setup_priv_hnd;

        // enable/disable uipq debug
        bool uipq_debug;

        bool wdt_ioctl;

        // If ture, only WiFi-display is composed by HWC, and
        // other virtual displays such as CTS and screenrecord are composed by GPU
        bool only_wfd_by_hwc;

        // If there is only one ovl hw, virtual displays can composed by GPU, and
        // the output format is converted to YV12 by BlitDevice
        bool blitdev_for_virtual;

        // to indicate how HWC process the virtual display when only one ovl hw.
        bool is_support_ext_path_for_virtual;

        // If true, the flow will change from validate -> present to
        // 1. If skip, present
        // 2. If no skip, present -> validate -> present
        bool is_skip_validate;

        bool support_color_transform;

        double mdp_scale_percentage;

        bool extend_mdp_capacity;

        int32_t rpo_ui_max_src_width;
    };
    PlatformConfig m_config;
};

#endif // HWC_PLATFORM_H_
