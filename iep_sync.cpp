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

void* iep_process_thread(void *param) 
{
    int i;

    int iep_fd = open("/dev/iep", O_RDWR, 0);
    IEP_MSG msg;
    mem_region_t *mr = (mem_region_t*)param;

    int phy_src, phy_reg, phy_dst;
    int len_src, len_reg, len_dst;
    uint8_t *vir_reg, *vir_src, *vir_dst;

    len_reg = mr->len_reg;
    len_src = mr->len_src;
    len_dst = mr->len_dst;

    phy_reg = mr->phy_reg;
    phy_src = mr->phy_src;
    phy_dst = mr->phy_dst;

    vir_reg = mr->vir_reg;
    vir_src = mr->vir_src;
    vir_dst = mr->vir_dst;

    memset(&msg, 0, sizeof(IEP_MSG));

    msg.src.act_w = 640;
    msg.src.act_h = 480;
    msg.src.x_off = 0;
    msg.src.y_off = 0;
    msg.src.vir_w = 640;
    msg.src.vir_h = 480;
    msg.src.format = IEP_FORMAT_YCbCr_420_SP;
    msg.src.mem_addr = (uint32_t*)phy_src;
    msg.src.uv_addr  = (uint32_t*)(phy_src + 640 * 480);
    msg.src.v_addr = 0;
    msg.dein_mode = 6;
    
    msg.dst.act_w = 640;
    msg.dst.act_h = 480;
    msg.dst.x_off = 0;
    msg.dst.y_off = 0;
    msg.dst.vir_w = 640;
    msg.dst.vir_h = 480;
    msg.dst.format = IEP_FORMAT_YCbCr_420_SP;
    msg.dst.mem_addr = (uint32_t*)phy_dst;
    msg.dst.uv_addr = (uint32_t*)(phy_dst + 640 * 480);
    msg.dst.v_addr = 0;

    while (i++ < 2) {
        usleep(4*1000);

        printf("%s %d\n", __func__, __LINE__);

        memset(vir_reg, 0, len_reg);
        //memset(vir_dst, 0, 128);
        memset(vir_dst, 0, 640*480*3/2);

        if (0 > ioctl(iep_fd, IEP_SET_PARAMETER, &msg)) {
            printf("ioctl IEP_SET_PARAMETER failure\n");
            continue;
        }

        printf("%s %d\n", __func__, __LINE__);

        if (0 > ioctl(iep_fd, IEP_GET_RESULT_SYNC, 0)) {
            printf("%d, failure\n", getpid());
        } else {
            char str[128];
            memcpy(str, vir_dst, 128);
            printf("%d, success, %s\n", getpid(), str);
        }

        /*for (i=0; i<640*480*3/2; i++) {
            if (vir_dst[i] != 0) {
                printf("%d != 0\n", i);
            }
        }*/

        /*int *reg = (int*)vir_reg;

        for (i=0; i<len_reg/4; i++) {
            printf("%08x ", reg[i]);

            if ((i+1) % 4 == 0) {
                printf("\n");
            }
        }

        printf("\n");*/
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int i;
    pthread_t td[3];

    int pmem_phy_head;
    uint8_t *pmem_vir_head;

    VPUMemLinear_t vpumem; 

    VPUMallocLinear(&vpumem, 3<<20);

    pmem_phy_head = vpumem.phy_addr;//region.offset;
    pmem_vir_head = (uint8_t*)vpumem.vir_addr;

    mem_region_t mr[3];

    for (i=0; i<NR_THREADS; i++) {
        
        mr[i].len_reg = 0x100;
        mr[i].len_src = 1<<20;
        mr[i].len_dst = 1<<20;

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

