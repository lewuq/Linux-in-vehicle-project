# Linux In-Vehicle Project

[中文](#中文说明) | [English](#english)

## 中文说明

### 项目简介

这是一个基于 Linux 和 Qt 开发的车载终端项目，用于展示多媒体、环境监测、天气信息、地图定位和基础语音交互等功能。项目整体面向嵌入式车载场景，适合作为课程设计、毕业设计或功能原型演示。

### 主要功能

- 音乐播放与搜索
- 视频播放界面
- 天气信息展示
- 地图与定位模块
- 传感器与环境监测
- 时钟、设置与基础语音交互

### 项目结构

- `Music/`：音乐播放与搜索相关界面和逻辑
- `Video/`：视频播放相关资源
- `Weather/`：天气页面、样式与资源
- `Map/`：地图展示、GPS 与串口相关代码
- `Monitor/`：传感器采集与监控模块

### 开发环境

- Linux
- Qt Widgets
- C++
- qmake / Makefile

### 构建运行

可使用 Qt Creator 打开 `VehicleTerminal.pro` 进行构建，也可以在命令行中使用 qmake 和 make 编译。

### 说明

本项目以界面展示和功能整合为主，适合用于车载终端应用的原型开发与课程实践。

---

## English

### Overview

This project is an in-vehicle terminal application built with Linux and Qt. It provides a compact demonstration of multimedia playback, environmental monitoring, weather display, map positioning, and basic speech interaction for an embedded automotive-style interface.

### Main Features

- Music playback and search
- Video player interface
- Weather information display
- Map and positioning module
- Sensor and environment monitoring
- Clock, settings, and basic speech interaction

### Project Structure

- `Music/`: music playback and search UI logic
- `Video/`: video-related resources
- `Weather/`: weather pages, styles, and assets
- `Map/`: map display, GPS, and serial communication code
- `Monitor/`: sensor collection and monitoring module

### Environment

- Linux
- Qt Widgets
- C++
- qmake / Makefile

### Build

You can open `VehicleTerminal.pro` with Qt Creator to build the project, or compile it from the command line with qmake and make.

### Notes

This repository focuses on UI presentation and functional integration, making it suitable for coursework, graduation projects, and embedded prototype demonstrations.
