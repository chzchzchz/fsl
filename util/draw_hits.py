#!/usr/bin/python
import os
import sys
import Image
import ImageColor
import math

#Usage:
# ./draw_scan.py scan_data disk_size img.png
hit_data_fname=sys.argv[1]

if (sys.argv[2])[-1] == 'M':
	hit_disk_size = int((sys.argv[2])[:-1]) * 1024 * 1024
else:
	hit_disk_size=int(sys.argv[2])

hit_img_fname=sys.argv[3]

BOTTOM_LEGEND_PX=50
pixels_per_side=1000
disk_bytes=hit_disk_size
bytes_per_pixel=disk_bytes/(pixels_per_side**2)
out_size = pixels_per_side,(pixels_per_side+1+BOTTOM_LEGEND_PX)
im = Image.new("RGB", out_size, ImageColor.getrgb('black'))

print "BPP:" + str(bytes_per_pixel)

ents=list()
f = open(hit_data_fname, 'r')
for l in f.readlines():
	ents.append(eval(l))
f.close()

hit_count_min = min(map(lambda x : x['hits'], ents))
hit_count_max = max(map(lambda x : x['hits'], ents))
hit_count_logmin = math.floor(math.log(hit_count_min)/math.log(2))
hit_count_logrange = math.floor(math.log((hit_count_max - hit_count_min)+1)/math.log(2))

color_table = [
	'rgb(125,0,255)',#violet
	'rgb(190,0,255)',
	'rgb(0,0,255)',#blue
	'rgb(0,62,255)',
	'rgb(0,125,255)',#oceana
	'rgb(0,190,255)',
	'rgb(0,255,255)',#cyan
	'rgb(0,255,190)',
	'rgb(0,255,125)',#turquoise
	'rgb(0,255,62)',
	'rgb(0,255,0)', #green
	'rgb(62,255,0)',
	'rgb(125,255,0)', #spring green
	'rgb(190,255,0)',
	'rgb(255,255,0)', #yellow
	'rgb(255,190,0)',
	'rgb(255,125,0)', #orange
	'rgb(255,62,0)',
	'rgb(255,0,0)', #red
]

def getColor(hit_c):
	global hit_count_max
	global hit_count_min
	global hit_count_logmin
	global hit_count_logrange
	global color_table

	log_v = math.floor(math.log(hit_c)/math.log(2))
	log_rebase = log_v - hit_count_logmin
	color_intensity=log_rebase*(255 / hit_count_logrange)
	color_rgb = color_table[int(log_rebase)]
	return ImageColor.getrgb(color_table[int(log_rebase)])


for e in ents:
	hit_c = e['hits']
	num_bytes = 1 + e['last_byte_offset'] - e['start_byte_offset']
	poff_bytes = e['start_byte_offset']

	pixel_c = (num_bytes + bytes_per_pixel-1) / bytes_per_pixel
	color = getColor(hit_c)

	for i in range(pixel_c):
		pixel_off=(poff_bytes/bytes_per_pixel)+i
		x = pixel_off % pixels_per_side
		y = pixel_off / pixels_per_side
		im.putpixel((x,y), color)

# draw border
for x in range(pixels_per_side):
	im.putpixel((x,pixels_per_side), ImageColor.getrgb('blue'))


# draw logrange
for i in range(int(hit_count_logrange)):
	x_start = i*(pixels_per_side/hit_count_logrange)
	color = getColor(2**(i+hit_count_logmin))
	for x_idx in range(1+int(pixels_per_side/hit_count_logrange)):
		for y in range(BOTTOM_LEGEND_PX):
			im.putpixel(
				(int(x_idx+x_start),int(pixels_per_side+1+y)),
				color)

print "Min Hits: " + str(hit_count_min)
print "Max Hits: " + str(hit_count_max)

im.save(hit_img_fname)
