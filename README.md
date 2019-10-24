## 使用
当修改某些内容，或第一次运行时需要编译工程

    ./make.sh
运行项目

    ./run.sh
所以当clone本工程后，需要为上面两个脚本添加可执行权限。
或者手动指定配置文件

    ./build/circle CONFIG_FILE_PATH

## 配置文件
配置文件位于`./cfg`目录下，运行程序不指定配置文件时默认使用`./cfg/config.json`
初始配置文件为

    {
        "test": {
            "video_path": "0"
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
            "gamma": 2,
            "red_low_1": [170, 150, 230],
            "red_high_1": [180, 255, 256],
            "red_low_2": [0, 150, 230],
            "red_high_2": [20, 255, 256],
            "green_low": [],
            "green_high": [],
            "blue_low": [],
            "blue_high": []
        },
        "control_parameters": {
            "k_p": 10,
            "k_i": 10,
            "k_d": 10
        }
    }