# Pix2Ascii
Simple Image to Ascii graphics converter written in C


![](https://raw.githubusercontent.com/Blackdeer1524/Pix2Ascii/master/Media/CurrentProjectState.gif)

## Requirements
 * FFmpeg
 * ncurses     
 * [Optional] video4linux (Camera support)

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

flags:

* -f [media path]
  
* -c : (camera support)
  
* -set [sharp | optimal | standard | long] : ascii set
  
* -method [average | yuv] : grayscale conversion methods
  
* -color : enable color support
  
* -nl : loop video; -1 for infinite loop
  
