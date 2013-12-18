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

#define NR_THREADS   1

typedef struct mem_region {
    int phy_src;
    int phy_reg;
    int phy_dst;

    int len_src;
    int len_reg;
    int len_dst;

    uint8_t *vir_src;
    uint8_t *vir_reg;
    uint8_t *vir_dst;
} mem_region_t;

#define SRC_WIDTH   640
#define SRC_HEIGHT  480

#define DST_WIDTH   640
#define DST_HEIGHT  480

#define SCREEN_WIDTH    1280
#define SCREEN_HEIGHT   800

void iep_async_notify(int result) {
    ALOGD("%s result %d\n", __func__, result);
}

void* iep_process_thread(void *param) 
{
    int i, cnt = 0, ret = 0;

    mem_region_t *mr = (mem_region_t*)param;

    int phy_src, phy_reg, phy_dst;
    int len_src, len_reg, len_dst;
    uint8_t *vir_reg, *vir_src, *vir_dst;
    iep_img src;
    iep_img dst;

    len_reg = mr->len_reg;
    len_src = mr->len_src;
    len_dst = mr->len_dst;

    phy_reg = mr->phy_reg;
    phy_src = mr->phy_src;
    phy_dst = mr->phy_dst;

    vir_reg = mr->vir_reg;
    vir_src = mr->vir_src;
    vir_dst = mr->vir_dst;

    iep_interface *api = iep_interface::create_new();

    FILE *infile = fopen("/data/yuv420p_640x480.seq", "rb");
    FILE *outfile = fopen("/data/rgba565.seq", "wb");

    fread(vir_src, 1, SRC_WIDTH * SRC_HEIGHT, infile);
    fread(vir_src + SRC_WIDTH * SRC_HEIGHT, 1, SRC_WIDTH * SRC_HEIGHT / 2, infile);
    
    src.act_w = SRC_WIDTH;
    src.act_h = SRC_HEIGHT;
    src.x_off = 0;
    src.y_off = 0;
    src.vir_w = SRC_WIDTH;
    src.vir_h = SRC_HEIGHT;
    src.format = IEP_FORMAT_YCbCr_420_P;
    src.mem_addr = (uint32_t*)phy_src;
    src.uv_addr  = (uint32_t*)(phy_src + SRC_WIDTH * SRC_HEIGHT);
    src.v_addr = (uint32_t*)(phy_src + SRC_WIDTH * SRC_HEIGHT + SRC_WIDTH * SRC_HEIGHT / 4);
    
    dst.act_w = SCREEN_WIDTH;
    dst.act_h = SCREEN_HEIGHT;
    dst.x_off = 0;
    dst.y_off = 0;
    dst.vir_w = SCREEN_WIDTH;
    dst.vir_h = SCREEN_HEIGHT;
    dst.format = IEP_FORMAT_ARGB_8888;//IEP_FORMAT_YCbCr_420_SP;
    dst.mem_addr = (uint32_t*)phy_dst;
    dst.uv_addr = (uint32_t*)(phy_dst + SCREEN_WIDTH * SCREEN_HEIGHT);
    dst.v_addr = 0;

    ALOGD("%s %d\n", __func__, __LINE__);

    api->init(&src, &dst);

    ALOGD("%s %d\n", __func__, __LINE__);

    while (cnt++ < 1) {
#if 1
        iep_param_direct_path_interface_t dpi;

        dpi.enable  = 1;
        dpi.off_x   = 0;
        dpi.off_y   = 0;
        dpi.width   = SCREEN_WIDTH;
        dpi.height  = SCREEN_HEIGHT;
        dpi.layer   = 0;
        
        if (0 > api->config_direct_lcdc_path(&dpi)) {
            ALOGE("Failure to Configure DIRECT LCDC PATH\n");
        }
#endif
        
        ALOGD("%s %d\n", __func__, __LINE__);
        
        api->run_async(NULL/*iep_async_notify*/);

        ret = api->poll();
        if (ret != 0) {
            ALOGE("iep poll failure, return %d\n", ret);
        }
    }

    // just for test, to make sure all the tasks had been notified,
    // error occur when reclaim iep_interface objects before notify.
    sleep(3);

    fclose(infile);
    fclose(outfile);

    iep_interface::reclaim(api);

    return NULL;
}

int main(int argc, char **argv)
{
    int i;
    pthread_t td[3];

    int pmem_phy_head;
    uint8_t *pmem_vir_head;

    VPUMemLinear_t vpumem; 

    VPUMallocLinear(&vpumem, 12<<20);

    pmem_phy_head = vpumem.phy_addr;//region.offset;
    pmem_vir_head = (uint8_t*)vpumem.vir_addr;

    mem_region_t mr[3];

    for (i=0; i<NR_THREADS; i++) {
        
        mr[i].len_reg = 0x100;
        mr[i].len_src = 5<<20;
        mr[i].len_dst = 5<<20;

        mr[i].phy_reg = vpumem.phy_addr;
        pmem_phy_head += mr[i].len_reg;
        mr[i].phy_src = pmem_phy_head;
        pmem_phy_head += mr[i].len_src;
        mr[i].phy_dst = pmem_phy_head;
        pmem_phy_head += mr[i].len_dst;

        mr[i].vir_reg = (uint8_t*)vpumem.vir_addr;
        pmem_vir_head += mr[i].len_reg;
        mr[i].vir_src = pmem_vir_head;
        pmem_vir_head += mr[i].len_src;
        mr[i].vir_dst = pmem_vir_head;
        pmem_vir_head += mr[i].len_dst;

        pthread_create(&td[i], NULL, iep_process_thread, &mr[i]);
    }

    for (i=0; i<NR_THREADS; i++) {
        pthread_join(td[i], NULL);
    }

    VPUFreeLinear(&vpumem);

    return 0;
}

