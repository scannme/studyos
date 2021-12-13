#include "cosmostypes.h"
#include "cosmosmctrl.h"

void init_memmgr()
{
    //初始化内存页结构msadsc_t
    init_msadsc();
    //初始化内存区结构memarea_t
    init_memarea();
    //处理内存占用
    init_search_krloccupymm(&kmachbsp);
    //合并内存页到内存区中
    init_merlove_mem();
    init_memmgrob();
    return;
}

void disp_memmgrob()
{

	test_divsion_pages();
	return;
}

void init_memmgrob()
{
	machbstart_t *mbsp = &kmachbsp;
	memmgrob_t *mobp = &memmgrob;
	memmgrob_t_init(mobp);
	if (NULL == mbsp->mb_e820expadr || 0 == mbsp->mb_e820exnr)
	{
		system_error("mbsp->mb_e820expadr==NULL\n");
	}
	if (NULL == mbsp->mb_memmappadr || 0 == mbsp->mb_memmapnr)
	{
		system_error("mbsp->mb_memmappadr==NULL\n");
	}
	if (NULL == mbsp->mb_memznpadr || 0 == mbsp->mb_memznnr)
	{
		system_error("mbsp->mb_memznpadr==NULL\n");
	}
	mobp->mo_pmagestat = (phymmarge_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820expadr);
	mobp->mo_pmagenr = mbsp->mb_e820exnr;
	mobp->mo_msadscstat = (msadsc_t *)phyadr_to_viradr((adr_t)mbsp->mb_memmappadr);
	mobp->mo_msanr = mbsp->mb_memmapnr;
	mobp->mo_mareastat = (memarea_t *)phyadr_to_viradr((adr_t)mbsp->mb_memznpadr);
	mobp->mo_mareanr = mbsp->mb_memznnr;
	mobp->mo_memsz = mbsp->mb_memmapnr << PSHRSIZE;
	mobp->mo_maxpages = mbsp->mb_memmapnr;
	uint_t aidx = 0;
	for (uint_t i = 0; i < mobp->mo_msanr; i++)
	{
		if (1 == mobp->mo_msadscstat[i].md_indxflgs.mf_uindx &&
			MF_MOCTY_KRNL == mobp->mo_msadscstat[i].md_indxflgs.mf_mocty &&
			PAF_ALLOC == mobp->mo_msadscstat[i].md_phyadrs.paf_alloc)
		{
			aidx++;
		}
	}
	mobp->mo_alocpages = aidx;
	mobp->mo_freepages = mobp->mo_maxpages - mobp->mo_alocpages;
	return;
}

void memmgrob_t_init(memmgrob_t *initp)
{
	list_init(&initp->mo_list);
	knl_spinlock_init(&initp->mo_lock);
	initp->mo_stus = 0;
	initp->mo_flgs = 0;
	initp->mo_memsz = 0;
	initp->mo_maxpages = 0;
	initp->mo_freepages = 0;
	initp->mo_alocpages = 0;
	initp->mo_resvpages = 0;
	initp->mo_horizline = 0;
	initp->mo_pmagestat = NULL;
	initp->mo_pmagenr = 0;
	initp->mo_msadscstat = NULL;
	initp->mo_msanr = 0;
	initp->mo_mareastat = NULL;
	initp->mo_mareanr = 0;
	initp->mo_privp = NULL;
	initp->mo_extp = NULL;
	return;
}
u64_t init_msadsc_core(machbstart_t *mbsp, msadsc_t *msavstart, u64_t msanr)
{
    //获取phymmarge_t结构数组开始地址
    phymmarge_t *pmagep = (phymmarge_t *)phyadr_to_viradr((adr_t)mbsp->mb_e820expadr);
    u64_t mdindx = 0;
    //扫描Phymmarge_t结构数组
    for (u64_t i =0; i < mbsp->mb_e820exnr; i++) {
        //判断phymmarge_+t结构的类型是不是可用的内存
        if (PMR_T_OSAPUSERRAM == pmagep[i].pmr_dtype) {
            //遍历phymmarge_t结构的地址区间
            for (u64_t start = pmagep[i].pmr_saddr; start < pmagep[i].pmr_end; start += 4096) {
                //每次加上4kb-1比较是否等于phymarge_t结构的结束地址
                if ((start + 4096 -1) <= pmagep[i].pmr_end) {
                    //与当前地址为参数写入第mdinx个msadsc结构
                    write_one_msadsc(&msavstart[mdindx], start);
                    mdindx++;
                }
            }
        }

    }

    return mdindx;
}
void write_one_msadsc(msadsc_t *msap, u64_t phyadr) 
{
    //对msadsc_t结构做基本的初始化,比如链表,锁，标志位
    msadsc_t_init(msap);
    //把64位的变量地址
    phyadrflgs_t *tmp =(phyadrflgs_t *)(&phyadr);
    //把页的物理地址写入到msadsc_t结构中
    msap->md_phyadrs.paf_padrs = tmp->paf_padrs;
    return;
}

