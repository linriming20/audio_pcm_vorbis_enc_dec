#### 1、libogg+libvorbis移植步骤

源码下载地址：[https://xiph.org/downloads/](https://xiph.org/downloads/) 

```bash
tar xzf libogg-1.3.5.tar.gz
cd libogg-1.3.5/
#./configure --prefix=$PWD/_install --disable-shared --enable-static --with-pic
./configure --prefix=$PWD/_install
make -j8
make install
strip  --strip-unneeded _install/lib/*

cd ../
tar xzf libvorbis-1.3.7.tar.gz 
cd libvorbis-1.3.7/
./configure --prefix=$PWD/_install --with-ogg=$PWD/../libogg-1.3.5/_install/
make -j8
make install
strip  --strip-unneeded _install/lib/*
```

```bash
$ tree libogg-1.3.5/_install/lib/ libogg-1.3.5/_install/include/ -h
libogg-1.3.5/_install/lib/
├── [ 31K]  libogg.a
├── [ 990]  libogg.la
├── [  15]  libogg.so -> libogg.so.0.8.5
├── [  15]  libogg.so.0 -> libogg.so.0.8.5
├── [ 34K]  libogg.so.0.8.5
└── [4.0K]  pkgconfig
    └── [ 340]  ogg.pc
libogg-1.3.5/_install/include/
└── [4.0K]  ogg
    ├── [ 538]  config_types.h
    ├── [8.2K]  ogg.h
    └── [4.7K]  os_types.h

2 directories, 9 files


$ tree libvorbis-1.3.7/_install/lib/ libvorbis-1.3.7/_install/include/ -h
libvorbis-1.3.7/_install/lib/
├── [292K]  libvorbis.a
├── [691K]  libvorbisenc.a
├── [1.3K]  libvorbisenc.la
├── [  22]  libvorbisenc.so -> libvorbisenc.so.2.0.12
├── [  22]  libvorbisenc.so.2 -> libvorbisenc.so.2.0.12
├── [690K]  libvorbisenc.so.2.0.12
├── [ 46K]  libvorbisfile.a
├── [1.3K]  libvorbisfile.la
├── [  22]  libvorbisfile.so -> libvorbisfile.so.3.3.8
├── [  22]  libvorbisfile.so.3 -> libvorbisfile.so.3.3.8
├── [ 38K]  libvorbisfile.so.3.3.8
├── [1.2K]  libvorbis.la
├── [  18]  libvorbis.so -> libvorbis.so.0.4.9
├── [  18]  libvorbis.so.0 -> libvorbis.so.0.4.9
├── [227K]  libvorbis.so.0.4.9
└── [4.0K]  pkgconfig
    ├── [ 446]  vorbisenc.pc
    ├── [ 472]  vorbisfile.pc
    └── [ 385]  vorbis.pc
libvorbis-1.3.7/_install/include/
└── [4.0K]  vorbis
    ├── [8.2K]  codec.h
    ├── [ 17K]  vorbisenc.h
    └── [7.8K]  vorbisfile.h

2 directories, 21 files
```

#### 2、说明

##### a. demo现状

- 参考`libvorbis-1.3.7/examples/`目录下实现的代码；
- `blog_readOggPage.c`是在参考文章中的代码基础上调整的；

- `main_pcm_2_ogg_vorbis.c`和`main_ogg_vorbis_2_pcm.c`可以正常编解码，并且能正常播放；


##### b. 关于verbois格式的说明

- Vorbis是一种有损音讯压缩格式，由Xiph.Org基金会所领导并开放源代码的一个免费的开源软件项目。
- Vorbis通常以Ogg作为容器格式，所以常合称为Ogg Vorbis。
- 目前Xiph.Org基金会建议使用延迟更低、音质更好的Opus编码来取代Vorbis。
- 32 kb/秒（-q-2）到500 kb/秒（-q10）的比特率。
- 采样率从8 kHz（窄带）到192 kHz（超频）。
- 支持采样精度 16bit\20bit\24bit\32bit。
- 采用可变比特率（VBR），动态调整比特率达到最佳的编码效果。
- 支持单声道、立体声、四声道和5.1环绕声道；支持多达255个音轨（多数据流的帧）。
- 可动态调节比特率，音频带宽和帧大小。
- Vorbis使用了一种灵活的格式，能够在文件格式已经固定下来后还能对音质进行明显的调节和新算法调校。
- 可以封装在多种媒体容器格式中，如Ogg（ .oga）、Matroska（ .mka）、WebM（ .webm）等。


#### 3、使用方法

##### a. 编译

```bash
make clean && make
```

##### b. 编码

```bash
$ ./pcm_2_ogg_vorbis 
Usage: 
         ./pcm_2_ogg_vorbis <in-pcm-file> <sample-rate> <channels> <samples once> <quality(-0.1~1.0)> <out-ogg-file>
examples: 
         ./pcm_2_ogg_vorbis ./audio/test_8000_16_1.pcm  8000 1 160 0.8 out1.ogg
         ./pcm_2_ogg_vorbis ./audio/test_44100_16_2.pcm 44100 2 1024 0.5 out2.ogg
```

##### c. 解码

```bash
$ ./ogg_vorbis_2_pcm 
Usage: 
         ./ogg_vorbis_2_pcm <in-ogg-vorbis-file> <out-pcm-file>
examples: 
         ./ogg_vorbis_2_pcm ./audio/out1.ogg out1.pcm
         ./ogg_vorbis_2_pcm ./audio/out2.ogg out2.pcm
```

#### 4、参考文章

- [【音视频 _ Ogg】Ogg封装格式详解——包含Ogg封装过程、数据包(packet)、页(page)、段(segment)等-CSDN博客](https://blog.csdn.net/wkd_007/article/details/134150061) 
- [【音视频 _ Ogg】libogg库详细介绍以及使用——附带libogg库解析.opus文件的C源码-CSDN博客](https://blog.csdn.net/wkd_007/article/details/134199191) 
- [Vorbis - 求闻百科，共笔求闻](https://www.qiuwenbaike.cn/wiki/Vorbis) 

#### 5、demo目录架构

```bash
$ tree
.
├── audio
│   ├── out1.ogg
│   ├── out2.ogg
│   ├── test_44100_16_2.pcm
│   └── test_8000_16_1.pcm
├── avfile
├── blog_readOggPage.c
├── docs
│   ├── Vorbis - 求闻百科，共笔求闻.mhtml
│   ├── 【音视频 _ Ogg】libogg库详细介绍以及使用——附带libogg库解析.opus文件的C源码-CSDN博客.mhtml
│   └── 【音视频 _ Ogg】Ogg封装格式详解——包含Ogg封装过程、数据包(packet)、页(page)、段(segment)等-CSDN博客.mhtml
├── include
│   ├── ogg
│   │   ├── config_types.h
│   │   ├── ogg.h
│   │   └── os_types.h
│   └── vorbis
│       ├── codec.h
│       ├── vorbisenc.h
│       └── vorbisfile.h
├── lib
│   ├── libogg.a
│   ├── libvorbis.a
│   ├── libvorbisenc.a
│   └── libvorbisfile.a
├── main_ogg_vorbis_2_pcm.c
├── main_pcm_2_ogg_vorbis.c
├── Makefile
├── opensource
│   └── libvorbis-1.3.7.tar.gz
├── README.md
├── reference_code
│   ├── chaining_example.c
│   ├── decoder_example.c
│   ├── encoder_example.c
│   ├── seeking_example.c
│   └── vorbisfile_example.c
└── tools

10 directories, 28 files
```
