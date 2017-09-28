# prompeg decoder

>The project aiming to solve the well-known wireless multicast problem by forward error correction(FEC). The goal is to improve the performance of  wireless multicast video stream.

This program acts as a proxy server that receives prompeg video stream from remote FFmpeg server. It will recover the lost packet with the FEC stream, redirect the recovered stream to localhost, which is playable with FFplay.


***The project is currently under development***

## Requirement
- Server:
    - FFmpeg (required version above 3.3)
- Client:
    - OS: Linux
    - FFplay


ffmpeg server command:
```
    ffmpeg -re -i yourVideoFileName.mp4 \
    -vcodec libx264 \
    -profile:v main \
    -preset faster \
    -tune zerolatency \
    -b:v 3000k \
    -g 48 -refs 1 \
    -me_method epzs -me_range 16 \
    -intra-refresh 1 \
    -f rtp_mpegts -strict -2 \
    -fec prompeg=l=10:d=10 \
    rtp://233.0.41.102:20000
```

ffplay client command:
```
ffplay rtp://127.0.0.1:8000
```
