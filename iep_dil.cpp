#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>

#include <sys/mman.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "cutils/log.h"

#include "iep_api.h"
#include "iep.h"
#include "../libon2/vpu_mem.h"

#define SRC_WIDTH   1920
#define SRC_HEIGHT  1080

#define DST_WIDTH   1920
#define DST_HEIGHT  1080

#define PIC_LEN (SRC_WIDTH*SRC_HEIGHT*3/2)

typedef struct {
    int phy_addr;
    uint8_t *vir_addr;
} mem_block;

int main(int argc, char **argv)
{
    int i;
    pthread_t td[3];

    int pmem_phy_head;
    uint8_t *pmem_vir_head;

    VPUMemLinear_t vpumem; 

    VPUMallocLinear(&vpumem, 12<<20);

    pmem_phy_head = vpumem.phy_addr;
    pmem_vir_head = (uint8_t*)vpumem.vir_addr;

    int phy_src0, phy_dst, phy_src1;
    uint8_t *vir_src0, *vir_dst, *vir_src1;

    mem_block src0, src1, dst0;
    mem_block *cur, *pre;
   
    iep_img src;
    iep_img dst;

    src0.phy_addr = vpumem.phy_addr;
    src1.phy_addr = src0.phy_addr + (3<<20);
    dst0.phy_addr = src1.phy_addr + (3<<20);

    src0.vir_addr = (uint8_t*)vpumem.vir_addr;
    src1.vir_addr = src0.vir_addr + (3<<20);
    dst0.vir_addr = src1.vir_addr + (3<<20);

    iep_interface *api = iep_interface::create_new();

    FILE *infile = fopen("/data/xxx.yuv", "rb");
    FILE *outfile = fopen("/data/yyy.yuv", "wb");

    pre = &src0;
    cur = &src1;

    fread(pre->vir_addr, 1, PIC_LEN, infile);

    do {

        if (PIC_LEN  > fread(cur->vir_addr, 1, PIC_LEN, infile)) {
            break;
        }

        src.act_w = SRC_WIDTH;
        src.act_h = SRC_HEIGHT;
        src.x_off = 0;
        src.y_off = 0;
        src.vir_w = SRC_WIDTH;
        src.vir_h = SRC_HEIGHT;
        src.format = IEP_FORMAT_YCbCr_420_SP;
        src.mem_addr = (uint32_t*)pre->phy_addr;
        src.uv_addr  = (uint32_t*)(pre->phy_addr + SRC_WIDTH * SRC_HEIGHT);
        src.v_addr = (uint32_t*)(pre->phy_addr + SRC_WIDTH * SRC_HEIGHT + SRC_WIDTH * SRC_HEIGHT / 4);
        
        dst.act_w = DST_WIDTH;
        dst.act_h = DST_HEIGHT;
        dst.x_off = 0;
        dst.y_off = 0;
        dst.vir_w = DST_WIDTH;
        dst.vir_h = DST_HEIGHT;
        dst.format = IEP_FORMAT_YCbCr_420_SP;//IEP_FORMAT_YCbCr_420_P;
        dst.mem_addr = (uint32_t*)dst0.phy_addr;
        dst.uv_addr = (uint32_t*)(dst0.phy_addr + DST_WIDTH * DST_HEIGHT);
        dst.v_addr = 0;

        ALOGD("%s %d\n", __func__, __LINE__);

        iep_param_yuv_deinterlace_t dil;

        dil.high_freq_en = 0;
        dil.dil_mode = IEP_DEINTERLACE_MODE_I4O1;
    #if 1
        dil.field_order = FIELD_ORDER_TOP_FIRST;
    #else 
        dil.field_order = FIELD_ORDER_BOTTOM_FIRST;
    #endif
        dil.dil_high_freq_fct = 0;
        dil.dil_ei_mode = 0;
        dil.dil_ei_smooth = 0;
        dil.dil_ei_sel = 0;
        dil.dil_ei_radius = 2;

        iep_img src1;

        src1.act_w    = DST_WIDTH;
        src1.act_h    = DST_HEIGHT;
        src1.x_off    = 0;
        src1.y_off    = 0;
        src1.vir_w    = DST_WIDTH;
        src1.vir_h    = DST_HEIGHT;
        src1.format   = IEP_FORMAT_YCbCr_420_SP;
        src1.mem_addr = (uint32_t*)cur->phy_addr;
        src1.uv_addr  = (uint32_t*)(cur->phy_addr + SRC_WIDTH * SRC_HEIGHT);

        ALOGE("%s %d, %d\n", __func__, __LINE__, dil.dil_mode);

        api->init(&src, &dst);

        if (0 > api->config_yuv_deinterlace(&dil, &src1, &dst)) {
            ALOGE("Failure to Configure YUV DEINTERLACE\n");
        }

        if (0 == api->run_sync()) {
            ALOGD("%d success\n", getpid());
        } else {
            ALOGE("%d failure\n", getpid());
        }

        fwrite(dst0.vir_addr, 1, PIC_LEN, outfile);

        dil.field_order = FIELD_ORDER_BOTTOM_FIRST;

        if (0 > api->config_yuv_deinterlace(&dil, &src1, &dst)) {
            ALOGE("Failure to Configure YUV DEINTERLACE\n");
        }

        if (0 == api->run_sync()) {
            ALOGD("%d success\n", getpid());
        } else {
            ALOGE("%d failure\n", getpid());
        }

        fwrite(dst0.vir_addr, 1, PIC_LEN, outfile);

        {
            mem_block *tmp = pre;
            pre = cur;
            cur = tmp;
        }
    } while (1);

    fclose(infile);
    fclose(outfile);

    iep_interface::reclaim(api);

    VPUFreeLinear(&vpumem);

    return 0;
}

