# generate_spritesheet.py
# author: 	lshoek	
# summary: 	Generates a simple spritesheet from a folder of square images. 
#			All input images should be of the same size.
# flags:	Use --dir to set an input folder.
#			Use --size to resize the output spritesheet.
# 			Use --dim to set the number of rows/cols (Only squared numbers currently)
#				e.g. --dim 5 does 5^5=25 images.

import numpy as np
import argparse
import os
import sys
from PIL import Image, ImageOps, ImageFont, ImageDraw

parser = argparse.ArgumentParser()
parser.add_argument('--dir', type=str, default=os.getcwd())
parser.add_argument('--dim', type=int, default=2)
parser.add_argument('--size', type=int, default=-1)
args = parser.parse_args()

indir = os.path.join(args.dir)
imlist = os.listdir(indir)
dim = args.dim
outfile_name = 'sheet.png'

list_im = [[ImageOps.expand(Image.open(os.path.join(indir, imlist[i+dim*j]))) for i in range(dim)] for j in range(dim)]

imgs_arr = [np.vstack(list_im[i]) for i in range(dim)]
imgs_comb = [Image.fromarray(imgs_arr[i]) for i in range(dim)]
im_grid = Image.fromarray(np.hstack(imgs_comb))

if args.size != -1: 
  im_grid = im_grid.resize((args.size, args.size))

im_grid.save(os.path.join(outfile_name))

print(f'Saved {os.path.join(os.getcwd(), outfile_name)}')
