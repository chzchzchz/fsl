#!/usr/bin/python
import os
import sys
import Image
import ImageColor

#Usage:
# ./draw_scan.py scan_data disk_size img.png

color_cache = dict()
colors = [	"white", 'lime', "red", "green", "blue",
		"gray", "yellow", "purple", 'maroon',
		'teal', 'aqua', 'navy', 'olive']
color_idx = 0

pixels_per_side=1024
disk_bytes=int(sys.argv[2])
bytes_per_pixel=disk_bytes/(pixels_per_side**2)
out_size = pixels_per_side,pixels_per_side
im = Image.new("RGB", out_size, ImageColor.getrgb('black'))

print "BPP:" + str(bytes_per_pixel)

ents=list()
f = open(sys.argv[1], 'r')
for l in f.readlines():
	ents.append(eval(l))
f.close()

def getColor(type_name):
	global color_cache
	global color_idx
	if type_name not in color_cache:
		color_cache[type_name] = colors[color_idx]
		color_idx = color_idx + 1
	return color_cache[type_name]


for e in ents:
	num_bits=e['size']
	type_name=e['name']
	poff_bits = e['poff']

	num_bytes = num_bits/8
	poff_bytes = poff_bits/8
	pixel_c = (num_bytes + bytes_per_pixel-1) / bytes_per_pixel

	color = getColor(type_name)

	for i in range(pixel_c):
		pixel_off=(poff_bytes/bytes_per_pixel)+i
		x = pixel_off % pixels_per_side
		y = pixel_off / pixels_per_side
		im.putpixel((x,y), ImageColor.getrgb(color))

#	print str(poff_bytes)+" bytes/ ("+str(poff_bits)+")@num pixels: " + str(pixel_c)

im.save(sys.argv[3])

for k in color_cache.keys():
	print k +" : "+ color_cache[k]
