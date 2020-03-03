#include <stdlib.h>
#include <stdio.h>

#include "sc_interface.h"
//#include <cutils/log.h>
#include "sdklog.h"
#include "sunxiMemInterface.h"
#include "DmaIon.h"


int sdk_log_print(int prio, const char *tag,const char *fmt, ...){}

int allocOpen(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	int ret = FAIL;
	if(NULL == param_in)
	{
		ALOGE("allocOpen failed,param_in is null\n");
		return -1;
	}
	
	dma_mem_des_t* p =  param_in;

	switch(memType)
	{
	case MEM_TYPE_DMA:
        ret = IonAllocOpen();
		break;	
		
	case MEM_TYPE_CDX_NEW:
		p->ops = MemAdapterGetOpsS();
		if(NULL != p->ops)
		{
			ret = CdcMemOpen((struct ScMemOpsS *)p->ops);
		}
		else
		{
			ALOGE("allocOpen failed,p->ops null\n");
		}
		break;
		
	default:
		ALOGE("allocOpen: can't find memType=%d\n",memType);
		break;
	}

	return ret;
}


int allocClose(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	int ret = FAIL;
	if(NULL == param_in)
	{
		ALOGE("allocClose failed,param_in is null\n");
		return -1;
	}
	dma_mem_des_t* p =  param_in;
	switch(memType)
	{
	case MEM_TYPE_DMA:
        ret = IonAllocClose();
		break;	
		
	case MEM_TYPE_CDX_NEW:
		if(NULL != p->ops)
		{
			CdcMemClose((struct ScMemOpsS *)p->ops);
			p->ops = NULL;
			ret = SUCCESS;
		}
		else
		{
			ALOGE("allocClose failed,p->ops null\n");
		}
		break;
		
	default:
		ALOGE("allocClose: can't find memType=%d\n",memType);
		break;
	}
	return ret;
}

int allocAlloc(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	int ret = FAIL;
	if(NULL == param_in){
		ALOGE("allocAlloc failed,input param is null\n");
		return ret;
	}
	
	dma_mem_des_t* p = param_in;
	p->memType = memType;
	switch(memType){
	case MEM_TYPE_DMA:
        p->vir = IonAlloc(p->size);
        p->phy = IonVir2phy(p->vir);
        p->ion_buffer.vir = p->vir;
        p->ion_buffer.phy = p->phy; 
        p->ion_buffer.fd_data.aw_fd = IonVir2fd(p->vir);
		ret = 0;
		break;	
		
	case MEM_TYPE_CDX_NEW:
		p->vir = (int)CdcMemPalloc((struct ScMemOpsS *)p->ops, p->size, NULL, NULL);
		p->phy = (int)CdcMemGetPhysicAddressCpu((struct ScMemOpsS *)p->ops, (void*)p->vir);
		ret = 0;
		break;

	default:
		ALOGE("allocAlloc: can't find memType=%d\n",memType);
		break;
	}

	return ret;
}

void allocFree(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	if(NULL == param_in){
		ALOGE("allocFree failed,input param is null\n");
		return;
	}
	dma_mem_des_t* p = param_in;
	switch(memType){
	case MEM_TYPE_DMA:
        IonFree(param_in->vir);
        memset(&p->ion_buffer,0,sizeof(p->ion_buffer));
		break;	
		
	case MEM_TYPE_CDX_NEW:
		CdcMemPfree((struct ScMemOpsS *)p->ops, (void*)p->vir, NULL, NULL);
		break;

	default:
		ALOGE("allocFree: can't find memType=%d\n",memType);
		break;
	}
}

int allocVir2phy(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	int ret = FAIL;
	if(NULL == param_in){
		ALOGE("allocVir2phy failed,input param is null\n");
		return -1;
	}
	
	dma_mem_des_t* p = param_in;
	switch(memType){
	case MEM_TYPE_DMA:
        p->phy = IonVir2phy(p->vir);
		break;	
		
	case MEM_TYPE_CDX_NEW:
		p->phy = (int)CdcMemGetPhysicAddressCpu((struct ScMemOpsS *)p->ops, (void*)p->vir);
		break;

	default:
		ALOGE("allocVir2phy: can't find memType=%d\n",memType);
		break;
	}
	
	return ret;
}

//warning !!!below function no test
int allocPhy2vir(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	int ret = FAIL;
	if(NULL == param_in){
		ALOGE("allocPhy2vir failed,input param is null\n");
		return -1;
	}
	
	dma_mem_des_t* p = param_in;
	switch(memType){
	case MEM_TYPE_DMA:
        p->vir = IonPhy2vir(p->phy);
		break;	
		
	case MEM_TYPE_CDX_NEW:
		p->vir = (int)CdcMemGetVirtualAddressCpu((struct ScMemOpsS *)p->ops, (void*)p->phy);
		break;

	default:
		ALOGE("allocPhy2vir: can't find memType=%d\n",memType);
		break;
	}

	return ret;
}

void flushCache(unsigned int memType, dma_mem_des_t * param_in, void * param_out)
{
	if(NULL == param_in){
		ALOGE("flushCache failed,input param is null\n");
		return;
	}
	int ret= 0;
	dma_mem_des_t* p = param_in;
	switch(memType){
	case MEM_TYPE_DMA:
        IonDmaSync(p->ion_buffer.fd_data.aw_fd);
		break;	
		
	case MEM_TYPE_CDX_NEW:
		CdcMemFlushCache((struct ScMemOpsS *)p->ops, (void*)p->vir, p->size);
		break;

	default:
		ALOGE("flushCache: can't find memType=%d\n",memType);
		break;
	}
}
