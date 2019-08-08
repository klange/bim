#!/usr/bin/env python3
"""
Tool for automatically generating all of the theme screenshots.
You need gnome-screenshot, xdotool, and xfce4-terminal to use this.
"""
import subprocess
import time

sample_c = """/* A sample C source to demonstrate syntax highlighting. */
#include <stdio.h>

int main(int argc, char * argv[]) {
	printf("Hello, %s!\\n", "world");
	return 42;
}
"""

with open("/tmp/sample.c","w") as f:
	f.write(sample_c)

# Create the terminal
terminal = subprocess.Popen(['xfce4-terminal','--disable-server','-e','bim /tmp/sample.c'])

time.sleep(0.5)

# Find the terminal
wids = subprocess.check_output(['xdotool','search','--pid',str(terminal.pid).encode()]).decode().split('\n')
wid = wids[1]

# Set up the cursor
subprocess.run(['xdotool','windowfocus',wid,'type',":5\n"])
subprocess.run(['xdotool','type',':4'])
subprocess.run(['xdotool','key','KP_Enter'])
subprocess.run(['xdotool','key','Right','Right','Right','Right','Right','Right','Right','Right'])

# See what themes are supported.
themes = subprocess.check_output(['bim','--version'],stderr=subprocess.STDOUT).decode().split('\n')[2].replace(' Available color themes: ','').split(' ')

for theme in themes:
	# Set the theme to this them
	subprocess.run(['xdotool','type',':theme {}'.format(theme)])
	subprocess.run(['xdotool','key','KP_Enter'])
	time.sleep(0.5)
	subprocess.run(['gnome-screenshot','-w','-f','theme_{}.png'.format(theme) if theme != 'sunsmoke' else 'screenshot.png'])

# Take the cat screenshot
subprocess.run(['xdotool','type',':!bash'])
subprocess.run(['xdotool','key','KP_Enter'])
subprocess.run(['xdotool','type','clear; cd /tmp'])
subprocess.run(['xdotool','key','KP_Enter'])
time.sleep(0.5)
subprocess.run(['xdotool','type','bim -C sample.c'])
subprocess.run(['xdotool','key','KP_Enter'])
time.sleep(0.5)
subprocess.run(['xdotool','type','bim -c sample.c'])
subprocess.run(['xdotool','key','KP_Enter'])
time.sleep(0.5)
subprocess.run(['gnome-screenshot','-w','-f','screenshot_cat.png'])
subprocess.run(['xdotool','type','exit'])
subprocess.run(['xdotool','key','KP_Enter'])
subprocess.run(['xdotool','key','KP_Enter'])
subprocess.run(['xdotool','type',':q!'])
subprocess.run(['xdotool','key','KP_Enter'])
