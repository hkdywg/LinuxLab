#!/usr/bin/env python3

import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')

from gi.repository import Gst, GstRtspServer, GObject

Gst.init(None)

class MyFactory(GstRtspServer.RTSPMediaFactory):
    def __init__(self, filepath):
        super(MyFactory, self).__init__()
        self.set_shared(True)
        self.filepath = filepath

    def do_create_element(self, url):
        # 构建 RTSP 管道
        # 这里假设视频是 H.264 mp4
        pipe = f"filesrc location={self.filepath} ! qtdemux ! h264parse ! rtph264pay name=pay0 pt=96"
        return Gst.parse_launch(pipe)

def main():
    server = GstRtspServer.RTSPServer()
    server.set_address("172.29.4.196")  # 使用本地 IP
    server.set_service("8554")

    mounts = server.get_mount_points()

    # 三路视频文件
    video_files = [
        "/home/yinwg/ywg_workspace/PHUD/rtsp_server/video_resource/output_3840x640.mp4",
        "/home/yinwg/ywg_workspace/PHUD/rtsp_server/video_resource/video2.mp4",
        "/home/yinwg/ywg_workspace/PHUD/rtsp_server/video_resource/video3.mp4",
    ]

    # 对应三路 RTSP mount point
    mount_points = ["/video1", "/video2", "/video3"]

    # 创建工厂并挂载
    for filepath, mount_point in zip(video_files, mount_points):
        factory = MyFactory(filepath)
        mounts.add_factory(mount_point, factory)
        print(f"Added RTSP stream: rtsp://172.29.4.196:8554{mount_point}")

    server.attach(None)
    print("RTSP Server running...")
    loop = GObject.MainLoop()
    loop.run()

if __name__ == "__main__":
    main()

