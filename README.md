<div align="center">
	<h1>
		fb-dm
	</h1>
</div>

<div align="center">
	Name not finalized
</div>

## Why do this?
I saw this post [reddit post](https://www.reddit.com/r/Gentoo/comments/1g63itl/\
does_anyone_know_how_to_recreate_this/) that showed a neat little image and prompt
during the boot process. As a Touhou fan and someone who likes to make things look
the way I want I made this.

Previously I was using a bash script and [imgfb](https://github.com/sophuric/imgfb) 
to achieve this which worked but only somewhat. It is now being written in C with 
minimal dependencies so I can learn play around with it.

## Installation
To install simply clone, make and make install:
```
git clone https://git.ethanbtst.ca/fb-dm && cd fb-dm
make
sudo make install
```

After, enable `fb-dm@tty1.service`, then disable `getty@tty1.service`

## Customization
To customize edit `/opt/fb-dm/config.ini`

By default the images shown are the penguins found when booting the
system (if enabled when building the kernel). The big image will be
your distro (only Arch right now).

Modifiable values:\
top_text takes a string\
hostname/kernel takes `true/false` to enable/disable it\
data/time takes a `strftime` format 


Default config: 
```
small_pic = path to image
big_pic   = distro or path to image

top_text  = string
hostname  = bool
kernel    = bool
date      = strftime format
time      = strftime format
```


## Screenshots
![Default](./assets/default.png)
![Custom](./assets/custom.png)

## Todo
- Add selection of wayland/x11 sessions (Currently starts shell and users need to startx/wayland session equivalent)
- Add jpeg/png support and scaling of images (Images need to be scaled already)
- Better customization
- Refactor
- Make a better makefile
- Something else I want