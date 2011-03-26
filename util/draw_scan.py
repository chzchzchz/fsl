#!/usr/bin/python
import os
import sys
import Image
import ImageColor
import colorsys

#Usage:
# ./draw_scan.py scan_data disk_size img.png [dimx dimy]

color_cache = dict()
colors = [	"orange", 'white', "red", "green", "blue",
		"gray", "yellow", "purple", 'magenta', 'lime',
		'teal', 'aqua', 'navy', 'olive', 'maroon',
		'fuschia', 'silver' ]

def getColor(type_name):
	global color_cache
	global color_idx
	global total_colors
	if type_name not in color_cache:
		if total_colors < len(colors):
			color_cache[type_name] = ImageColor.getrgb(colors[color_idx])
			color_idx = color_idx + 1
		else:
			rgb = colorsys.hls_to_rgb(float(color_idx)/total_colors,0.5,1.0)
			rgb = (int(rgb[0]*255),int(rgb[1]*255),int(rgb[2]*255))
			color_cache[type_name] = rgb
			color_idx = color_idx + 1

	return color_cache[type_name]

def countTypes(fname):
	typename_cache = dict()
	f = open(sys.argv[ARG_IDX_DISKMAP], 'r')
	for l in f.readlines():
		e = eval(l)
		typename_cache[e['name']] = 1
	f.close()
	return len(typename_cache)

color_idx = 0
ARG_IDX_DISKMAP = 1
ARG_IDX_DISKSZ = 2
ARG_IDX_OUTIMG = 3
ARG_IDX_PX_W = 4
ARG_IDX_PX_H = 5

print sys.argv[ARG_IDX_DISKMAP]

if len(sys.argv) == 6:
	pixels_w=int(sys.argv[ARG_IDX_PX_W])
	pixels_h=int(sys.argv[ARG_IDX_PX_H])
else:
	pixels_w=1024
	pixels_h=1024

disk_bytes=int(sys.argv[ARG_IDX_DISKSZ])
bytes_per_pixel=disk_bytes/(pixels_w*pixels_h)

if bytes_per_pixel < 0:
	print "Fewer bytes than one per pixel for " + sys.argv[ARG_IDX_DISKMAP]
	sys.exit(0)

out_size = pixels_w,pixels_h
im = Image.new("RGB", out_size, ImageColor.getrgb('black'))

print "BPP:" + str(bytes_per_pixel)

total_colors = countTypes(sys.argv[ARG_IDX_DISKMAP])

ents=list()
f = open(sys.argv[ARG_IDX_DISKMAP], 'r')
for l in f.readlines():
	ents.append(eval(l))
f.close()


for e in ents:
	num_bits=e['size']
	type_name=e['name']
	poff_bits = e['poff']

	num_bytes = num_bits/8
	poff_bytes = poff_bits/8
	pixel_c = (num_bytes + bytes_per_pixel-1) / bytes_per_pixel

	color = getColor(type_name)
	px_base = poff_bytes/bytes_per_pixel
	for i in range(pixel_c):
		pixel_off=px_base+i
		x = pixel_off % pixels_w
		y = pixel_off / pixels_w
		im.putpixel((x,y), color)

#	print str(poff_bytes)+" bytes/ ("+str(poff_bits)+")@num pixels: " + str(pixel_c)

im.save(sys.argv[ARG_IDX_OUTIMG])

for k in color_cache.keys():
	print k +" : "+ str(color_cache[k])
