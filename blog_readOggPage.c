// readOggFile.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 8字节数组转成 unsigned long long
unsigned long long ToULL(unsigned char num[8], int len)
{
	unsigned long long ret = 0;
	if(len==8)
	{	
		int i=0;
		for(i=0; i<len; i++)
		{
			ret |= ((unsigned long long)num[i] << (i*8));
		}
	}
	return ret;
}

// 4字节数组转成 unsigned int
unsigned int ToUInt(unsigned char num[4], int len)
{
	unsigned int ret = 0;
	if(len==4)
	{
		int i=0;
		for(i=0; i<len; i++)
		{
			ret |= ((unsigned int)num[i] << (i*8));
		}
	}
	return ret;
}

int readOggPage(char *oggFile)
{
	typedef struct PAGE_HEADER{  
		char           Oggs[4];        
		unsigned char  ver;
		unsigned char  header_type_flag;
		unsigned char  granule_position[8];
		unsigned char  stream_serial_num[4];
		unsigned char  page_sequence_number[4];
		unsigned char  CRC_checksum[4];
		unsigned char  seg_num;
		unsigned char  segment_table[];
	}PAGE_HEADER;
	
	FILE *fp=fopen(oggFile,"rb");
	
	while(!feof(fp))
	{
		// 1、读取 page_header
		PAGE_HEADER page_header;
		if(1 != fread(&page_header,sizeof(page_header),1,fp))
			break;
		printf("page_num:%03u; ",ToUInt(page_header.page_sequence_number, 4));
		printf("Oggs:%c %c %c %c; ",page_header.Oggs[0],page_header.Oggs[1],page_header.Oggs[2],page_header.Oggs[3]);
		printf("type=%d, granule_position:%08llu; ", page_header.header_type_flag,ToULL(page_header.granule_position, 8));
		//printf("seg_num:%d \n",page_header.seg_num);
		
		// 2、读取 Segment_table
		unsigned char *pSegment_table = (unsigned char *)malloc(page_header.seg_num);
		fread(pSegment_table,sizeof(unsigned char),page_header.seg_num,fp);
		
		// 3、计算段数据总大小
		unsigned int TotalSegSize = 0;
		int i=0;
		for(i=0; i<page_header.seg_num; i++)
		{
			TotalSegSize += pSegment_table[i];
		}
		printf("TotalSegSize:%d \n",TotalSegSize);
		
		// 4、读取段数据
		unsigned char *pSegment_data = (unsigned char *)malloc(TotalSegSize);
		fread(pSegment_data,sizeof(unsigned char),TotalSegSize,fp);
		
		if(page_header.header_type_flag == 4)
			printf("Last 4 Byte: %x %x %x %x\n",pSegment_data[TotalSegSize-4],pSegment_data[TotalSegSize-3], pSegment_data[TotalSegSize-2],pSegment_data[TotalSegSize-1]);
		
		free(pSegment_data);
		free(pSegment_table);
	}
	fclose(fp);
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("Usage: %s <ogg file>\n", argv[0]);
		return -1;
	}

	readOggPage(argv[1]);
	return 0;
}