#include <stdio.h>
#include <stdlib.h>

#include "vorbis/vorbisenc.h"


int main(int argc, char **argv)
{
    ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
    ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */
    vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block     vb; /* local working space for packet->PCM decode */
    int eos=0,ret;

    /* 检查参数 */
    if(argc != 7)
    {
        printf("Usage: \n"
                "\t %s <in-pcm-file> <sample-rate> <channels> <samples once> <quality(-0.1~1.0)> <out-ogg-file>\n"
                "examples: \n"
                "\t %s ./audio/test_8000_16_1.pcm  8000 1 160 0.8 out1.ogg\n"
                "\t %s ./audio/test_44100_16_2.pcm 44100 2 1024 0.5 out2.ogg\n"
                , argv[0], argv[0], argv[0]);
        return -1;
    }
    char *in_pcm_file_name = argv[1];
    unsigned int sample_rate = atoi(argv[2]);
    unsigned int channels = atoi(argv[3]);
    unsigned int samples_cnt = atoi(argv[4]);
    float quality = strtof(argv[5], NULL);
    char *out_ogg_file_name = argv[6];
    FILE *fp_in_pcm = NULL;
    FILE *fp_out_ogg = NULL;
    char *readbuffer = NULL;

    /* 文件操作 */
    fp_in_pcm = fopen(in_pcm_file_name, "rb");
    if (!fp_in_pcm) {
        printf("Error opening input file: %s\n", in_pcm_file_name);
        return -1;
    }
    fp_out_ogg = fopen(out_ogg_file_name, "wb");
    if (!fp_out_ogg) {
        printf("Error opening output file\n");
        fclose(fp_in_pcm);
        return -1;
    }
    readbuffer = malloc(samples_cnt * channels * sizeof(short));
    if (!readbuffer) {
        printf("alloc memory failed.\n");
        fclose(fp_in_pcm);
        fclose(fp_out_ogg);
        return -1;
    }

    vorbis_info_init(&vi);
    // 初始化，支持平均码率和可变码率
    //ret = vorbis_encode_init(&vi, 2, 44100, -1, 128000, -1);
    ret = vorbis_encode_init_vbr(&vi, channels, sample_rate, quality);
    if(ret)
    {
        printf("vorbis_encode_init_vbr failed with %d\n", ret);
        return -1;
    }

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);

    /* 流初始化，需要用到“逻辑比特流唯一序列号” */
    ogg_stream_init(&os, 0x12345678);

    // 添加头部：Vorbis流以3个头部开始，编解码参数(ogg比特流特性)+注释+比特流码本
    {
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        /* 添加注释 */
        vorbis_comment   vc;
        vorbis_comment_init(&vc);
        vorbis_comment_add_tag(&vc, "ENCODER", argv[0]);

        vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
        ogg_stream_packetin(&os, &header); /* automatically placed in its own page */
        ogg_stream_packetin(&os, &header_comm);
        ogg_stream_packetin(&os, &header_code);

        /* This ensures the actual audio data will start on a new page, as per spec */
        while(!eos){
            int result = ogg_stream_flush(&os, &og);
            if(result == 0)
                break;
            fwrite(og.header, 1, og.header_len, fp_out_ogg);
            fwrite(og.body, 1, og.body_len, fp_out_ogg);
        }

        vorbis_comment_clear(&vc);
    }

    while(!eos)
    {
        long i = 0;
        long bytes = fread(readbuffer, 1, samples_cnt*channels*sizeof(short), fp_in_pcm);
        if(bytes == 0)
        {
            /* 文件结束 */
        }
        else
        {
            /* 分配数据空间 */
            float **buffer = vorbis_analysis_buffer(&vd, samples_cnt);
            for(i = 0; i < bytes/channels/sizeof(short); i++)
            {
                buffer[0][i] = ((readbuffer[i*channels*sizeof(short)+1]<<8)|(0x00ff&(int)readbuffer[i*channels*sizeof(short)])) / 32768.f;
                if(channels == 2)
                {
                    buffer[1][i] = ((readbuffer[i*channels*sizeof(short)+3]<<8)|(0x00ff&(int)readbuffer[i*channels*sizeof(short)+2])) / 32768.f;
                }
                else if(channels == 3) // 更多声道数据填充
                {
                }
            }
        }

        /* 告知提交了多少数据 */
        vorbis_analysis_wrote(&vd, i);

        while(vorbis_analysis_blockout(&vd, &vb) == 1) /* 数据预分析，分块，一个一个块进行编码 */
        {
            /* 数据分析 */
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);

            while(vorbis_bitrate_flushpacket(&vd, &op))
            {
                /* 将数据包"焊接"到比特流中 */
                ogg_stream_packetin(&os, &op);

                /* 得到ogg page页面数据 */
                while(!eos)
                {
                    int result = ogg_stream_pageout(&os, &og);
                    if(result == 0)
                        break;
                    fwrite(og.header, 1, og.header_len, fp_out_ogg);
                    fwrite(og.body, 1, og.body_len, fp_out_ogg);

                    /* 流结束 */
                    if(ogg_page_eos(&og))
                        eos = 1;
                }
            }
        }
    }

    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_info_clear(&vi);

    free(readbuffer);
    fclose(fp_in_pcm);
    fclose(fp_out_ogg);

    printf("PCM -> Ogg Vorbis Success.\n");

    return 0;
}
