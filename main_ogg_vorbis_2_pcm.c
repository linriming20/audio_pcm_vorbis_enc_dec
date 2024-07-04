#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "vorbis/codec.h"


#define BUF_SIZE    (4096)
ogg_int16_t convbuffer[BUF_SIZE];

int main(int argc, char **argv)
{
    ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */
    vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
    vorbis_comment   vc; /* struct that stores all the bitstream user comments */
    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block     vb; /* local working space for packet->PCM decode */
    char *buffer = NULL;
    int  bytes = 0;
    int  ret = 0;
    int convsize = 0;
    int total_read_bytes = 0;

    /* 检查参数 */
    if(argc != 3)
    {
        printf("Usage: \n"
                "\t %s <in-ogg-vorbis-file> <out-pcm-file>\n"
                "examples: \n"
                "\t %s ./audio/out1.ogg out1.pcm\n"
                "\t %s ./audio/out2.ogg out2.pcm\n"
                , argv[0], argv[0], argv[0]);
        return -1;
    }
    char *in_ogg_vorbis_file_name = argv[1];
    char *out_pcm_file_name = argv[2];
    FILE *fp_in_ov = NULL;
    FILE *fp_out_pcm = NULL;

    /* 文件操作 */
    fp_in_ov = fopen(in_ogg_vorbis_file_name, "rb");
    if (!fp_in_ov) {
        printf("Error opening input file: %s\n", in_ogg_vorbis_file_name);
        ret = -1;
        goto exit;
    }
    fp_out_pcm = fopen(out_pcm_file_name, "wb");
    if (!fp_out_pcm) {
        printf("Error opening output file\n");
        ret = -1;
        goto exit;
    }

    /* 初始化，接下来就可以读取ogg page了 */
    ogg_sync_init(&oy);

    while(1) // 如果只有一路逻辑流，可以不需要这个while循环
    {
        /* 目的通过第一页拿到逻辑比特流的"唯一序列号"bitstream_serial_number, 4096个字节一般足够了 */
        /* 填充数据 */
        buffer = ogg_sync_buffer(&oy, BUF_SIZE);
        bytes = fread(buffer, 1, BUF_SIZE, fp_in_ov);
        ogg_sync_wrote(&oy, bytes);
        total_read_bytes += bytes;

        vorbis_info_init(&vi);
        vorbis_comment_init(&vc);

        /* 获取vorbis的3个头部 */
        int i = 0;
        while(i < 3)
        {
            int result = 1;
            do{
                if(bytes != BUF_SIZE || (result = ogg_sync_pageout(&oy, &og)) != 1)
                {
                    fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
                    exit(1);
                }

                if(result == 1) // 1-成功 0-数据不够 <0-出错
                {
                    if(i == 0)
                    {
                        /* 获取逻辑比特流的"唯一序列号"bitstream_serial_number */
                        ogg_stream_init(&os, ogg_page_serialno(&og));
                    }

                    // ogg_stream_pagein 不管返回值，有问题的话 ogg_stream_packetout 会报错
                    ogg_stream_pagein(&os, &og);
                    result = ogg_stream_packetout(&os, &op);
                    if(result == 1)
                    {
                        result = vorbis_synthesis_headerin(&vi, &vc, &op);
                        if(result < 0)
                        {
                            fprintf(stderr,"Corrupt header.  Exiting.\n");
                            exit(1);
                        }
                    }
                    else if(result < 0)
                    {
                        fprintf(stderr,"Corrupt header.  Exiting.\n");
                        exit(1);
                    }
                    else if(result == 0)
                    {
                        break; /* 数据不够，填充 */
                    }
                }
                else if(result == 0)
                {
                    break; /* 数据不够，填充 */
                }

                /* 继续下一个header */
                i++;
                break;
            }while(0);

            /* 数据不够出1个page，继续填充数据 */
            if(result == 0)
            {
                buffer = ogg_sync_buffer(&oy, BUF_SIZE);
                bytes = fread(buffer, 1, BUF_SIZE, fp_in_ov);
                ogg_sync_wrote(&oy, bytes);
                total_read_bytes += bytes;
            }
        }

        /* 解析完3个header了，可以选择吧信息打印一下 */
        {
            char **ptr=vc.user_comments;
            while(*ptr){
                fprintf(stdout,"%s\n",*ptr);
                ++ptr;
            }
            fprintf(stdout,"\nBitstream is %d channel, %ldHz\n",vi.channels,vi.rate);
            fprintf(stdout,"Encoded by: %s\n\n",vc.vendor);
        }
        convsize = BUF_SIZE/vi.channels;

        /* 开始解码出数据 */
        if(vorbis_synthesis_init(&vd, &vi) != 0 || vorbis_block_init(&vd, &vb))
        {
            fprintf(stderr, "vorbis_synthesis_init or vorbis_block_init failed.\n");
            exit(1);
        }

        int eos = 0;
        while(!eos){
            int result = 1;
            do{
                result = ogg_sync_pageout(&oy, &og);
                if(result < 0)
                {
                    fprintf(stderr,"Corrupt or missing data in bitstream; continuing...\n");
                    exit(1);
                }
                if(result == 1) // 1-成功 0-数据不够 <0-出错
                {
                    // ogg_stream_pagein 不管返回值，有问题的话 ogg_stream_packetout 会报错
                    ogg_stream_pagein(&os, &og);

                    while(1)
                    {
                        /* 循环处理 */
                        result = ogg_stream_packetout(&os, &op);
                        if(result == 1)
                        {
                            /* 数据处理 */
                            if(vorbis_synthesis(&vb,&op) == 0)
                                vorbis_synthesis_blockin(&vd, &vb);

                            /* 解出数据 */
                            float **pcm;
                            int samples;
                            while((samples = vorbis_synthesis_pcmout(&vd, &pcm)) > 0)
                            {
                                int bout = (samples < convsize ? samples : convsize);
                                printf("decode samples: %d\n", bout);
                                for(i = 0; i < vi.channels; i++)
                                {
                                    ogg_int16_t *ptr = convbuffer + i;
                                    float *mono = pcm[i];
                                    for(int j = 0; j < bout; j++)
                                    {
                                        int val = floor(mono[j] * 32767.f + .5f);
                                        val = val > 32767 ? 32767 : val;
                                        val = val < -32767 ? -32767 : val;
                                        *ptr = val;
                                        /* 一个通道一个通道处理 */
                                        ptr += vi.channels;
                                    }
                                }
                                /* 写入解码后的数据 */
                                fwrite(convbuffer, sizeof(short) * vi.channels, bout, fp_out_pcm);

                                /* 告诉libvorbis我们消耗了多少个采样点 */
                                vorbis_synthesis_read(&vd, bout);
                            }
                        }
                        else if(result < 0)
                        {
                            fprintf(stderr,"Corrupt header.  Exiting.\n");
                            exit(1);
                        }
                        else if(result == 0)
                        {
                            break; /* 数据结束 */
                        }

                    }
                    if(ogg_page_eos(&og))
                        eos = 1;
                }
                else if(result == 0)
                {
                    break; /* 需要继续填充数据 */
                }
            }while(0);

            /* 数据不够出1个page，继续填充数据 */
            if(result == 0)
            {
                buffer = ogg_sync_buffer(&oy, BUF_SIZE);
                bytes = fread(buffer, 1, BUF_SIZE, fp_in_ov);
                ogg_sync_wrote(&oy, bytes);
                total_read_bytes += bytes;
                printf("read = %d, total_read_bytes: %d\n", bytes, total_read_bytes);
            }
        }

        /* 结束了就清理状态 */
        vorbis_block_clear(&vb);
        vorbis_dsp_clear(&vd);

        /* 逻辑流结束了 */
        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);  /* 必须最后调用 */
    }
    ogg_sync_clear(&oy);

exit:
    if(fp_in_ov) fclose(fp_in_ov);
    if(fp_out_pcm) fclose(fp_out_pcm);

    return ret;
}
