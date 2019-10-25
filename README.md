## 使用
当修改某些内容，或第一次运行时需要编译工程

    ./make.sh
运行项目

    ./run.sh
所以当clone本工程后，需要为上面两个脚本添加可执行权限。
或者手动指定配置文件

    ./build/circle CONFIG_FILE_PATH

要下载`./video`目录下的视频，需要git lsf支持，安装参考[git-lfs](https://git-lfs.github.com/)

## 配置文件
配置文件位于`./cfg`目录下，运行程序不指定配置文件时默认使用`./cfg/config.json`
初始配置文件为

    {
        "test": {
            "video_path": "./video/20160212_003550.avi"
        },
        "camera": {
            "frame_width": 640,
            "frame_height": 480,
            "frame_fps": 30,
            "brightness": 0.5,
            "contrast": 0.5,
            "saturation": 0.46875,
            "hue": 0.5,
            "exposure": 5
        },
        "video": {
            "frame_fps": 27.0
        },
        "image_processing": {
            "gamma": 2.0,
            "red_low_1": [170, 150, 230],
            "red_high_1": [180, 256, 256],
            "red_low_2": [0, 150, 230],
            "red_high_2": [20, 256, 256],
            "green_low": [35, 150, 230],
            "green_high": [77, 256, 256],
            "blue_low": [100, 150, 230],
            "blue_high": [124, 256, 256],
            "circle1_param": {
                "dp": 4.0,
                "minDist": 100,
                "param1": 100,
                "param2": 160,
                "minRadius": 0,
                "maxRadius": 0
            },
            "circle2_param": {
                "dp": 4.0,
                "minDist": 100,
                "param1": 100,
                "param2": 160,
                "minRadius": 0,
                "maxRadius": 0
            },
            "circle3_param": {
                "dp": 4.0,
                "minDist": 100,
                "param1": 100,
                "param2": 160,
                "minRadius": 0,
                "maxRadius": 0
            }
        },
        "control_parameters": {
            "k_p": 10,
            "k_i": 10,
            "k_d": 10
        }
    }