# Pix2Ascii
Simple Image to Ascii graphics converter written in C


![](https://raw.githubusercontent.com/Blackdeer1524/Pix2Ascii/master/Media/CurrentProjectState.gif)

## Flags
### Video source flags
 * **-f "file path"**: read from video/image file
 * **-c**: read from camera
### Optional flags
 * **-method [average | yuv]**: RGB2Grayscale conversion method. **average** by deault 
 * **-set [sharp | long | optimal | standard]**: defines character set. **optimal** by default
 * **-color**: colorization flag. **turned off** by default
 * **-nl**: number of video loops to create (-1 for infinite loop). **0** by default

## Requirements
 * **FFmpeg**
 * **ncurses**     
 * [Optional] **video4linux** (Camera support)

## Installation

 * sudo apt install ffmpeg
 * sudo apt-get install libncursesw5-dev
 * sudo apt install v4l-utils

## Usage

Compile with cmake and run 

Example:
```console
~$ ./program -f <filepath> 
```
  
