# gst_dec_display

A lightweight GStreamer-based RTSP/video playback application for embedded Linux platforms.

---

##  Overview

`gst_dec_display` is a command-line video playback tool built on **GStreamer 1.x**, designed for
embedded Linux environments (e.g. Rockchip SoCs).  
It supports:

- RTSP video streaming
- Local media file playback
- Wayland / KMS video output
- Software decode (`decodebin`)
- Hardware decode (e.g. `mppvideodec`, optional)

This project is suitable for **BSP development**, **automotive IVI/HUD**, and **embedded multimedia pipelines**.

---

##  Features

- RTSP H.264/H.265 streaming support
- Dynamic pad handling for `rtspsrc` and `decodebin`
- Wayland display support (`waylandsink`)
- KMS/DRM display support (`kmssink`)
- Optional loop playback
- Clean shutdown with signal handling
- Modular pipeline construction

---


## Example

- Use the connector-id to play local videos on the specified screen(ksmmink mode).

```
	./dst_dec_display -c 198 -l /home/root/phud_demo.mp4	
```

- Use the connector-id to play rtsp stream on the specified screen(ksmmink mode).

```
	./dst_dec_display -c 198 -u rtsp://172.29.4.196:8554/phud_demo
```

- Play a single RTSP video stream on three screens.(waylandsink)

```
	./dst_dec_display -i rtsp://172.29.4.196:8554/phud_demo
```

- gst_dec_diplay usage help

```
	root@RK3576-Tronlong:~# ./gst_dec_display --help
	Usage: gst_dec_display [options]
	Options:
	   -c | --connector-id     Select the connector-id.
	   -p | --plane-id         Select the plane-id.
	   -x | --h26x             Select h264 or h265 parse
	   -l | --location         The file path
	   -u | --url              The RTSP url
	   -i | --uri              Use wayland not kmssink, set uri
	   -r | --replay           Replay video
	   -v | --version          Version Info.
	   --help                  Show this message.


	e.g. :
		   ./gst_dec_display -c 208 -p 57  -x h264 -l test.mp4
```

##  Project Structure


```text
.
├── build
│   └── gst_dec_display
├── inc
│   └── parameter_parser.h
├── log
├── Makefile
├── README.md
└── src
    ├── gst_dec_display.c
    └── parameter_parser.c
