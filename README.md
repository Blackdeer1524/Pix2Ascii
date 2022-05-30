# Pix2Ascii
Simple Image to Ascii graphics converter written in C


![](https://raw.githubusercontent.com/Blackdeer1524/Pix2Ascii/master/Media/CurrentProjectState.gif)

## Flags
### Video source flags
 * **-f "file path"**: read from video/image file
 * **-c**: read from camera
### Optional flags
 * **-h**: get help
 * **-method [average | yuv]**: RGB2Grayscale conversion method. **average** by deault 
 * **-set [sharp | long | optimal | standard]**: defines character set. **optimal** by default
 * **-method [average | yuv]**: RGB channels combining method. **average** by default.
 * **-nl**: number of video loops to create (-1 for infinite loop). **0** by default
 * **-player [0 - off; 1 - only video; 2 - only audio; 3 - video and audio]**. Start ffplay simultaneously with the program (mainly for debug purposes). **off** by default.
 * **-filter [naive | gauss]**. Convolution filter type. **naive** by default
   * **naive**: simple pixel average
   * **gauss**: gaussian convolution filter
 * **-maxw**: sets maximum produced **width**
 * **-maxh**: sets maximum produced **height**
 * **--color**: terminal colorization flag. **turned off** by default
 * **--keep-aspect**: Enable aspect ratio. **turned off** by default

## Requirements
 * **FFmpeg**
 * **ncurses**     
 * [Optional] **video4linux** (Camera support)

## Installation

 * sudo apt install ffmpeg
 * sudo apt-get install libncursesw5-dev
 * sudo apt install v4l-utils
