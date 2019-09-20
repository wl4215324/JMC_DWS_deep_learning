/*
 * ion_alloc.c
 *
 * john.fu@allwinnertech.com
 *
 * ion memory allocate
 *
 */

#include <sys/ioctl.h>
#include <errno.h>

#include "DmaIon.h"
#include "ion_alloc_list.h"

#define ION_ALLOC_ALIGN		SZ_4k
#define DEV_NAME		    "/dev/ion"
#define ION_IOC_SUNXI_POOL_INFO	10

static ion_alloc_context *g_pAllocContext = NULL;


/*return total meminfo with MB*/
int GetIonTotalMem()
{
	int ret = 0;
	struct aw_ion_custom_info cdata;
	struct sunxi_pool_info binfo ;
    memset(&binfo, 0, sizeof(binfo));

	int ion_fd = open(DEV_NAME, O_WRONLY);

	if (ion_fd < 0) {
		printf("open ion dev failed, cannot get ion mem.");
		goto err;
	}

	cdata.aw_cmd = ION_IOC_SUNXI_POOL_INFO;
	cdata.aw_arg = (unsigned long)&binfo;
	ret = ioctl(ion_fd, AW_MEM_ION_IOC_CUSTOM, &cdata);
	if (ret < 0){
		printf("Failed to ioctl ion device, errno:%s\n", strerror(errno));
		goto err;
	}

	printf("ion dev get free pool [%d MB], total [%d MB]\n", binfo.free_mb, binfo.total / 1024);
	ret = binfo.total;
err:
	if(ion_fd > 0)
		close(ion_fd);
	return ret;
}

/*funciton begin*/
int IonAllocOpen()
{
	printf("\n===begin IonAllocOpen \n");

	if (g_pAllocContext != NULL)
	{
		printf("ion allocator has already been created \n");
		goto SUCCEED_OUT;
	}

	g_pAllocContext = (ion_alloc_context*)malloc(sizeof(ion_alloc_context));
	if (g_pAllocContext == NULL)
	{
		printf("create ion allocator failed, out of memory \n");
		goto ERROR_OUT;
	}
	else
	{
		printf("pid: %d, g_pAllocContext = %p \n", getpid(), g_pAllocContext);
	}

	memset((void*)g_pAllocContext, 0, sizeof(ion_alloc_context));

	g_pAllocContext->fd = open(DEV_NAME, O_RDWR, 0);

	if (g_pAllocContext->fd <= 0)
	{
		printf("open %s failed \n", DEV_NAME);
		goto ERROR_OUT;
	}

	AW_MEM_INIT_LIST_HEAD(&g_pAllocContext->list);

SUCCEED_OUT:
	g_pAllocContext->ref_cnt++;
	return 0;

ERROR_OUT:
	if (g_pAllocContext != NULL && g_pAllocContext->fd > 0)
	{
		close(g_pAllocContext->fd);
		g_pAllocContext->fd = 0;
	}

	if (g_pAllocContext != NULL)
	{
		free(g_pAllocContext);
		g_pAllocContext = NULL;
	}
	return -1;
}

int IonAllocClose()
{
	struct aw_mem_list_head *pos, *q;
	ion_buffer_node *tmp;

	printf("\n===IonAllocClose \n");

	if (--g_pAllocContext->ref_cnt <= 0)
	{
		printf("pid: %d, release g_pAllocContext = %p \n", getpid(), g_pAllocContext);

		aw_mem_list_for_each_safe(pos, q, &g_pAllocContext->list)
		{
			tmp = aw_mem_list_entry(pos, ion_buffer_node, i_list);
			printf("IonAllocClose del item phy = 0x%lx vir = 0x%lx, size = %d \n", tmp->phy, tmp->vir, tmp->size);
			aw_mem_list_del(pos);
			free(tmp);
		}

		close(g_pAllocContext->fd);
		g_pAllocContext->fd = 0;

		free(g_pAllocContext);
		g_pAllocContext = NULL;
	}
	else
	{
		printf("ref cnt: %d > 0, do not free \n", g_pAllocContext->ref_cnt);
	}
	return 0;
}

/* return virtual address: 0 failed */
unsigned long IonAlloc(int size)
{
	aw_ion_dma_allocation_info_t alloc_data;
	ion_fd_data_t fd_data;
	struct ion_handle_data handle_data;
	struct aw_ion_custom_info custom_data;
	sunxi_phys_data   phys_data;

	int rest_size = 0;
	unsigned long addr_phy = 0;
	unsigned long addr_vir = 0;
	ion_buffer_node * alloc_buffer = NULL;
	int ret = 0;
	printf("\n===IonAlloc \n");
	if (g_pAllocContext == NULL)
	{
		printf("IonAlloc do not opened, should call ion_alloc_open() before ion_alloc(size) \n");
		goto ALLOC_OUT;
	}

	if(size <= 0)
	{
		printf("can not alloc size 0 \n");
		goto ALLOC_OUT;
	}

	alloc_data.aw_len = (size_t)size;
	alloc_data.aw_align = ION_ALLOC_ALIGN ;

#ifdef CONFIG_IOMMU
	alloc_data.aw_heap_id_mask = AW_ION_SYSTEM_HEAP_MASK;//for IOMMU
#else

#if USE_ION_DMA
	alloc_data.aw_heap_id_mask = AW_ION_DMA_HEAP_MASK|AW_ION_CARVEOUT_HEAP_MASK;
#else
	alloc_data.aw_heap_id_mask = AW_ION_CARVEOUT_HEAP_MASK;
#endif

#endif
	alloc_data.flags = AW_ION_CACHED_FLAG | AW_ION_CACHED_NEEDS_SYNC_FLAG;

	ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_ALLOC, &alloc_data);
	if (ret)
	{
		printf("ION_IOC_ALLOC error \n");
		goto ALLOC_OUT;
	}

	/* get dmabuf fd */
	fd_data.handle = alloc_data.handle;
	ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_MAP, &fd_data);
	if(ret)
	{
		printf("ION_IOC_MAP err, ret %d, dmabuf fd 0x%08x\n", ret, (unsigned int)fd_data.aw_fd);
		goto ALLOC_OUT;
	}

	/* mmap to user */
	addr_vir = (unsigned long)mmap(NULL, alloc_data.aw_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data.aw_fd, 0);
	if((unsigned long)MAP_FAILED == addr_vir)
	{
		printf("mmap err, ret %d\n", (unsigned int)addr_vir);
		addr_vir = 0;
		goto ALLOC_OUT;
	}

#ifndef CONFIG_IOMMU
	/* get phy address */
	memset(&phys_data, 0, sizeof(phys_data));
	phys_data.handle = alloc_data.handle;
	phys_data.size = size;
	custom_data.aw_cmd = ION_IOC_SUNXI_PHYS_ADDR;
	custom_data.aw_arg = (unsigned long)&phys_data;

	ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_CUSTOM, &custom_data);
	if(ret) {
		printf("ION_IOC_CUSTOM err, ret %d\n", ret);
		addr_phy = 0;
		addr_vir = 0;
		goto ALLOC_OUT;
	}

	addr_phy = phys_data.phys_addr;
#endif
	alloc_buffer = (ion_buffer_node *)malloc(sizeof(ion_buffer_node));
	if (alloc_buffer == NULL)
	{
		printf("malloc buffer node failed");

		/* unmmap */
		ret = munmap((void*)addr_vir, alloc_data.aw_len);
		if(ret)
			printf("munmap err, ret %d\n", ret);

		/* close dmabuf fd */
		close(fd_data.aw_fd);

		/* free buffer */
		handle_data.handle = alloc_data.handle;
		ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_FREE, &handle_data);

		if(ret)
			printf("ION_IOC_FREE err, ret %d\n", ret);

		addr_phy = 0;
		addr_vir = 0; /* value of MAP_FAILED is -1, should return 0 */

		goto ALLOC_OUT;
	}
	alloc_buffer->phy 	= addr_phy;
	alloc_buffer->vir 	= addr_vir;
	alloc_buffer->size	= size;
	alloc_buffer->fd_data.handle = fd_data.handle;
	alloc_buffer->fd_data.aw_fd = fd_data.aw_fd;

	//printf("ion vir addr 0x%08x, size %d, dmabuf fd %d\n", addr_vir, size, fd_data.aw_fd);

	aw_mem_list_add_tail(&alloc_buffer->i_list, &g_pAllocContext->list);

ALLOC_OUT:
	return addr_vir;
}

int IonFree(void *pbuf)
{
	int flag = 0;
	unsigned long addr_vir = (unsigned long)pbuf;
	ion_buffer_node *tmp;
	int ret;
	struct ion_handle_data handle_data;
	int nFreeSize = 0;
	printf("\n===IonFree \n");
	if (0 == pbuf)
	{
		printf("can not free NULL buffer \n");
		return 0;
	}

	if (g_pAllocContext == NULL)
	{
		printf("IonAlloc do not opened, should call ion_alloc_open() before ion_alloc(size) \n");
		return 0;
	}

	aw_mem_list_for_each_entry(tmp, &g_pAllocContext->list, i_list)
	{
		if (tmp->vir == addr_vir)
		{
			//printf("IonFree item phy= 0x%lx vir= 0x%lx, size= %d \n", tmp->phy, tmp->vir, tmp->size);
			/*unmap user space*/
			if (munmap(pbuf, tmp->size) < 0)
			{
				printf("munmap 0x%p, size: %d failed \n", (void*)addr_vir, tmp->size);
			}
			nFreeSize = tmp->size;

			/*close dma buffer fd*/
			close(tmp->fd_data.aw_fd);
			/* free buffer */
			handle_data.handle = tmp->fd_data.handle;

			ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_FREE, &handle_data);
			if (ret)
				printf("TON_IOC_FREE failed \n");

			aw_mem_list_del(&tmp->i_list);
			free(tmp);
			flag = 1;
			break;
		}
	}

	if (0 == flag)
		printf("IonFree failed, do not find virtual address: 0x%lx \n", addr_vir);
	return nFreeSize;
}

int IonVir2fd(void *pbuf)
{
	int flag = 0, fd = -1;
	unsigned long addr_vir = (unsigned long)pbuf;
	ion_buffer_node * tmp;

	if (0 == pbuf)
	{
		printf("can not vir2phy NULL buffer \n");
		return 0;
	}

	aw_mem_list_for_each_entry(tmp, &g_pAllocContext->list, i_list)
	{
		if (addr_vir >= tmp->vir && addr_vir < tmp->vir + tmp->size)
		{
			fd = tmp->fd_data.aw_fd;
			printf("ion mem vir = 0x%08x, fd = %d\n", addr_vir, fd);
			flag = 1;
			break;
		}
	}

	if (0 == flag)
		printf("IonVir2fd failed, do not find virtual address: 0x%lx \n", addr_vir);

	return fd;
}

unsigned long IonVir2phy(void *pbuf)
{
	int flag = 0;
	unsigned long addr_vir = (unsigned long)pbuf;
	unsigned long addr_phy = 0;
	ion_buffer_node * tmp;

	if (0 == pbuf)
	{
		printf("can not vir2phy NULL buffer \n");
		return 0;
	}

	aw_mem_list_for_each_entry(tmp, &g_pAllocContext->list, i_list)
	{
		if (addr_vir >= tmp->vir && addr_vir < tmp->vir + tmp->size)
		{
			addr_phy = tmp->phy + addr_vir - tmp->vir;
			printf("IonVir2phy phy= 0x%08x vir= 0x%08x \n", addr_phy, addr_vir);
			flag = 1;
			break;
		}
	}

	if (0 == flag)
		printf("IonVir2phy failed, do not find virtual address: 0x%lx \n", addr_vir);

	return addr_phy;
}

unsigned long IonPhy2vir(void *pbuf)
{
	int flag = 0;
	unsigned long addr_vir = 0;
	unsigned long addr_phy = (unsigned long)pbuf;
	ion_buffer_node * tmp;

	if (0 == pbuf)
	{
		printf("can not phy2vir NULL buffer \n");
		return 0;
	}

	aw_mem_list_for_each_entry(tmp, &g_pAllocContext->list, i_list)
	{
		if (addr_phy >= tmp->phy && addr_phy < tmp->phy + tmp->size)
		{
			addr_vir = tmp->vir + addr_phy - tmp->phy;
			flag = 1;
			break;
		}
	}

	if (0 == flag)
		printf("IonPhy2vir failed, do not find physical address: 0x%lx \n", addr_phy);

	return addr_vir;
}

void IonFlushCache(void *startAddr, int size)
{
	sunxi_cache_range range;
	int ret;

	/* clean and invalid user cache */
	range.start = (unsigned long)startAddr;
	range.end = (unsigned long)startAddr + size;

	ret = ioctl(g_pAllocContext->fd, ION_IOC_SUNXI_FLUSH_RANGE, &range);
	if (ret)
		printf("ION_IOC_SUNXI_FLUSH_RANGE failed \n");

	return;
}

void IonFlushCacheAll()
{
	ioctl(g_pAllocContext->fd, ION_IOC_SUNXI_FLUSH_ALL, 0);
}

unsigned long IonAllocDrm(int size)
{
	aw_ion_dma_allocation_info_t alloc_data;
	ion_fd_data_t fd_data;
	struct ion_handle_data handle_data;
	struct aw_ion_custom_info custom_data;
	sunxi_phys_data   phys_data;

	int rest_size = 0;
	unsigned long addr_phy = 0;
	unsigned long addr_vir = 0;
	ion_buffer_node * alloc_buffer = NULL;
	int ret = 0;

	if (g_pAllocContext == NULL)
	{
		printf("IonAlloc do not opened, should call ion_alloc_open() before ion_alloc(size) \n");
		goto ALLOC_OUT;
	}

	if(size <= 0)
	{
		printf("can not alloc size 0 \n");
		goto ALLOC_OUT;
	}

	/*alloc buffer*/
	alloc_data.aw_len = size;
	alloc_data.aw_align = ION_ALLOC_ALIGN ;
	alloc_data.aw_heap_id_mask = AW_ION_SECURE_HEAP_MASK;
	alloc_data.flags = AW_ION_CACHED_FLAG | AW_ION_CACHED_NEEDS_SYNC_FLAG;
	ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_ALLOC, &alloc_data);
	if (ret)
	{
		printf("ION_IOC_ALLOC error %s \n", strerror(errno));
		goto ALLOC_OUT;
	}

	/* get dmabuf fd */
	fd_data.handle = alloc_data.handle;
	ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_MAP, &fd_data);
	if(ret)
	{
		printf("ION_IOC_MAP err, ret %d, dmabuf fd 0x%08x\n", ret, (unsigned int)fd_data.aw_fd);
		goto ALLOC_OUT;
	}


	/* mmap to user */
	addr_vir = (unsigned long)mmap(NULL, alloc_data.aw_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd_data.aw_fd, 0);
	if((unsigned long)MAP_FAILED == addr_vir)
	{
		addr_vir = 0;
		goto ALLOC_OUT;
	}

	/* get phy address */
	memset(&phys_data, 0, sizeof(phys_data));
	phys_data.handle = alloc_data.handle;
	phys_data.size = size;
	custom_data.aw_cmd = ION_IOC_SUNXI_PHYS_ADDR;
	custom_data.aw_arg = (unsigned long)&phys_data;

	ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_CUSTOM, &custom_data);
	if(ret)
	{
		printf("ION_IOC_CUSTOM err, ret %d\n", ret);
		addr_phy = 0;
		addr_vir = 0;
		goto ALLOC_OUT;
	}

	addr_phy = phys_data.phys_addr;
	alloc_buffer = (ion_buffer_node *)malloc(sizeof(ion_buffer_node));
	if (alloc_buffer == NULL)
	{
		printf("malloc buffer node failed");

		/* unmmap */
		ret = munmap((void*)addr_vir, alloc_data.aw_len);
		if(ret)
			printf("munmap err, ret %d\n", ret);

		/* close dmabuf fd */
		close(fd_data.aw_fd);

		/* free buffer */
		handle_data.handle = alloc_data.handle;
		ret = ioctl(g_pAllocContext->fd, AW_MEM_ION_IOC_FREE, &handle_data);

		if(ret)
			printf("ION_IOC_FREE err, ret %d\n", ret);

		addr_phy = 0;
		addr_vir = 0; /*value of MAP_FAILED is -1, should return 0*/

		goto ALLOC_OUT;
	}


	alloc_buffer->size	    = size;
	alloc_buffer->phy 	    = addr_phy;
	alloc_buffer->vir 	    = addr_vir;
	alloc_buffer->fd_data.handle = fd_data.handle;
	alloc_buffer->fd_data.aw_fd = fd_data.aw_fd;

	aw_mem_list_add_tail(&alloc_buffer->i_list, &g_pAllocContext->list);

ALLOC_OUT:
	return addr_vir;
}

int IonDmaSync(int dmafd)
{
	struct aw_ion_fd_data data2;
	data2.aw_fd = dmafd;
	ioctl(g_pAllocContext->fd, AW_MEMION_IOC_SYNC, &data2);
	return 0;
}

