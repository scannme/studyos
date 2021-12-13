
#include "bastype_t.h"
#include "cosmostypes.h"
#include "cosmosmctrl.h"

void msadsc_t_init(msadsc_t *initp)
{
	list_init(&initp->md_list);
	knl_spinlock_init(&initp->md_lock);
	initp->md_indxflgs.mf_olkty = MF_OLKTY_INIT;
	initp->md_indxflgs.mf_lstty = MF_LSTTY_LIST;
	initp->md_indxflgs.mf_mocty = MF_MOCTY_FREE;
	initp->md_indxflgs.mf_marty = MF_MARTY_INIT;
	initp->md_indxflgs.mf_uindx = MF_UINDX_INIT;
	initp->md_phyadrs.paf_alloc = PAF_NO_ALLOC;
	initp->md_phyadrs.paf_shared = PAF_NO_SHARED;
	initp->md_phyadrs.paf_swap = PAF_NO_SWAP;
	initp->md_phyadrs.paf_cache = PAF_NO_CACHE;
	initp->md_phyadrs.paf_kmap = PAF_NO_KMAP;
	initp->md_phyadrs.paf_lock = PAF_NO_LOCK;
	initp->md_phyadrs.paf_dirty = PAF_NO_DIRTY;
	initp->md_phyadrs.paf_busy = PAF_NO_BUSY;
	initp->md_phyadrs.paf_rv2 = PAF_RV2_VAL;
	initp->md_phyadrs.paf_padrs = PAF_INIT_PADRS;
	initp->md_odlink = NULL;
	return;
}



bool_t ret_msadsc_vadrandsz(machbstart_t *mbsp, msadsc_t **retmasvp, u64_t *retmasnr)
{
    if (NULL == mbsp || NULL == retmasvp || NULL == retmasnr) {
        return FALSE;
    }

    if (mbsp->mb_e820exnr < 1 || NULL == mbsp->mb_e820expadr || (mbsp->mb_e820exnr *sizeof(phymmarge_t)) != mbsp->mb_e820exsz) {
        *retmasvp = NULL;
        *retmasnr = 0;
        return FALSE;
    }
    phymmarge_t *pmagep = (phymmarge_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820expadr);
    u64_t usrmemsz = 0, msadnr = 0;
    for (u64_t i = 0; i < mbsp->mb_e820exnr; i++) {
        if (PMR_T_OSAPUSERRAM == pmagep[i].pmr_type) {
            usrmemsz += pmagep[i].pmr_lsize;
            msadnr += (pmagep[i].pmr_lsize >> 12); 
        }
    }
    if (0 == usrmemsz || (usrmemsz >> 12) < 1 || msadnr < 1) {
        *retmasvp = NULL;
        *retmasnr = 0;
        return FALSE;
    }
    if (0 != initchkadr_is_ok(mbsp, mbsp->mb_nextwtpadr, (msadnr *sizeof(msadsc_t)))) {
        system_error("ret msadsc varandsz initchkaddr is ok err\n");
    }

    *retmasvp = (msadsc_t*)phyadr_to_viradr((adr_t)mbsp->mb_nextwtpadr);
    *retmasnr = msadnr;

    return TRUE;
}

void write_one_msadsc(msadsc_t *msap, u64_t phyadr)
{
    //对msadsc_t结构进行初始化
    msadsc_t_init(msap);
    //这是把一个64位的变量地址转换成phyadrflgs_t类型方便取的其中的地址位段
    phyadrflgs_t *tmp = (phyadrflgs_t *)(&phyadr);
    //把页的物理地址写入到msadsc_t结构中
    msap->md_phyadrs.paf_padrs = tmp->paf_padrs;
    return;
}
u64_t init_msadsc_core(machbstart_t *mbsp, msadsc_t *msavstart, u64_t msanr)
{
    //获取phymmarge_t 结构数组开始地址
    phymmarge_t *pmagep = (phymmarge_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820expadr);
    u64_t mdindex = 0;
    //扫描phymmarge_t结构数组
    for (u64_t i = 0; i < mbsp->mb_e820exnr; i++) {
        //判断phymmarge_t结构的类型是不是可用内存
        if (PMR_T_OSAPUSERRAM == pmagep[i].pmr_type) {
            for (u64_t start = pmagep[i].pmr_saddr; start < pmagep[i].pmr_end; start +=4096) {
                if ((start + 4096 -1) <= pmagep[i].pmr_end) {
                    write_one_msadsc(&msavstart[mdindex], start);
                    mdindex++;
                }
            }
        }
    }

    return mdindex;
}
void init_msadsc()
{
    u64_t coremdnr = 0, msadscnr = 0;
    msadsc_t *msadscvp = NULL;
    machbstart_t *mbsp =&kmachbsp;

    //计算msadsc_t结构数组的开始地址和数组元素个数
    if (ret_msadsc_vadrandsz(mbsp, &msadscvp, &msadscnr) == FALSE) {
        system_error("init msadscnr ret msadsc vadrandsz err\n");
    }

    //开始真正的初始化msadsc_t结构数组
    coremdnr = init_msadsc_core(mbsp, msadscvp, msadscnr);

    if (coremdnr != msadscnr) {
        system_error("init msadscnr init msadc core err\n");
    }

    //将msadsc_t 结构数组的开始的物理地址写入kmachbsp结构中
    mbsp->mb_memmappadr = viradr_to_phyadr((adr_t)msadscvp);
    mbsp->mb_memmapnr = coremdnr;
    mbsp->mb_memmapsz = coremdnr * sizeof(msadsc_t);
    mbsp->mb_nextwtpadr = PAGE_ALIGN(mbsp->mb_memmappadr + mbsp->mb_memmapsz);
    return;
}

//搜索一段内存地址空间所对应的msadsc_t结构
u64_t search_segment_occupymsadsc(msadsc_t *msastart, u64_t msanr, u64_t ocpystat, u64_t ocpyend)
{
    u64_t mphyadr = 0, fsmsnr = 0;
    msadsc_t *fstatmp = NULL;

    for (u64_t mnr = 0; mnr < msanr; mnr++) {
        if ((msastart[mnr].md_phyadrs.paf_padrs << PSHRSIZE) == ocpystat)
        {
            //找出第一个地址对应的msadsc_t结构,就跳转到step1
            fstatmp = &msastart[mnr];
            goto step1;
        }
    }
step1:
	fsmsnr = 0;
	if (NULL == fstatmp)
	{
		return 0;
	}
	for (u64_t tmpadr = ocpystat; tmpadr < ocpyend; tmpadr += PAGESIZE, fsmsnr++)
	{
		mphyadr = fstatmp[fsmsnr].md_phyadrs.paf_padrs << PSHRSIZE;
		if (mphyadr != tmpadr)
		{
			return 0;
		}
		if (MF_MOCTY_FREE != fstatmp[fsmsnr].md_indxflgs.mf_mocty ||
			0 != fstatmp[fsmsnr].md_indxflgs.mf_uindx ||
			PAF_NO_ALLOC != fstatmp[fsmsnr].md_phyadrs.paf_alloc)
		{
			return 0;
		}
		fstatmp[fsmsnr].md_indxflgs.mf_mocty = MF_MOCTY_KRNL;
		fstatmp[fsmsnr].md_indxflgs.mf_uindx++;
		fstatmp[fsmsnr].md_phyadrs.paf_alloc = PAF_ALLOC;
	}
	u64_t ocpysz = ocpyend - ocpystat;
	if ((ocpysz & 0xfff) != 0)
	{
		if (((ocpysz >> PSHRSIZE) + 1) != fsmsnr)
		{
			return 0;
		}
		return fsmsnr;
	}
	if ((ocpysz >> PSHRSIZE) != fsmsnr)
	{
		return 0;
	}
	return fsmsnr;
}

  


bool_t search_krloccupymsadsc_core(machbstart_t *mbsp)
{
    u64_t retschmnr = 0;
    msadsc_t *msadstat = (msadsc_t *)phyadr_to_viradr((adr_t)mbsp->mb_memmappadr);
    u64_t msanr = mbsp->mb_memmapnr;
    //搜索BIOS中断表所占用的内存页所对应msadsc_t结构
    retschmnr = search_segment_occupymsadsc(msadstat, msanr, 0, 0x1000);
    if (0 == retschmnr) {
        return FALSE;
    }

    //搜索内核栈占用的内存页所对应的msadsc_t结构
    retschmnr = search_segment_occupymsadsc(msadstat, msanr, mbsp->mb_krlinitstack &(~(0xfffUL)), mbsp->mb_krlinitstack);
    if (0 == retschmnr) {
        return FALSE;
    }
    //搜索内核占用的内存所对应的msadsc_t结构
    retschmnr = search_segment_occupymsadsc(msadstat, msanr, mbsp->mb_krlimgpadr, mbsp->mb_nextwtpadr);
    if (0 == retschmnr) {
        return FALSE;
    }
    //搜索内核映像文件占用的内存页对应的msadsc_t结构
    retschmnr = search_segment_occupymsadsc(msadstat, msanr, mbsp->mb_imgpadr, mbsp->mb_imgpadr + mbsp->mb_imgsz);
    if (0 == retschmnr) {
        return FALSE;
    }

    return TRUE;


}
//初始化搜索内核占用的内存页面
void init_search_krloccupymm(machbstart_t *mbsp)
{
    //实际初始化搜索内核占用的内存页面
    if (search_krloccupymsadsc_core(mbsp) == FALSE) {
        system_error("search_krloccupymsadsc_core failed\n");
    }
    return;
}