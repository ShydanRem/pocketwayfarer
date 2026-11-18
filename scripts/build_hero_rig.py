#!/usr/bin/env python3
"""Register generated modular components; no painting or provider call here."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from build_art import Emitter, rgb565

SOURCE = ROOT / 'assets/hero-rig/source'
OUT = ROOT / 'firmware/rpg/ui/art'


def magenta(rgb):
    r, g, b = [rgb[:, :, i].astype(float) for i in range(3)]
    return (r > g*1.8+20) & (b > g*1.8+20) & (r > b*.65) & (b > r*.65) & (r > 70)


def cells(filename, rows, cols):
    im = Image.open(SOURCE/filename)
    assert im.mode == 'RGBA'
    xbounds = json.loads((SOURCE/'layout.json').read_text())['hats_column_bounds'] if filename=='hats.png' else [round(c*im.width/cols) for c in range(cols+1)]
    result = []
    for r in range(rows):
        for c in range(cols):
            cell = im.crop((xbounds[c], round(r*im.height/rows),xbounds[c+1], round((r+1)*im.height/rows)))
            rgba = np.array(cell)
            alpha = rgba[:, :, 3] >= 128
            assert alpha.any() and not (alpha[0].any() or alpha[-1].any() or alpha[:, 0].any() or alpha[:, -1].any()), f'clipped {filename} {r},{c}'
            result.append(cell)
    return result


def peg(im, neck=False, band=None):
    a = np.array(im)
    m = magenta(a[:, :, :3]) & (a[:, :, 3] >= 128)
    if band:
        yy=np.indices(m.shape)[0];m &= (yy>=im.height*band[0]) & (yy<im.height*band[1])
    if neck:
        # The highest marker is the neck; other markers belong to hands/skin.
        ys, xs = np.where(m)
        assert len(xs)
        m &= np.indices(m.shape)[0] < int(ys.min())+max(5,round(im.height*.07))
    ys, xs = np.where(m)
    assert len(xs) >= 4, 'missing registration peg'
    return float(np.mean(xs)), float(np.mean(ys))


def strip_peg(rgba,anchor):
    yy,xx=np.indices(rgba.shape[:2]);radius=max(14,rgba.shape[0]*.06)
    marker=magenta(rgba[:,:,:3]) & ((xx-anchor[0])**2+(yy-anchor[1])**2<=radius**2)
    rgba[marker,3]=0


def register(im, scale, anchor, target=(44,32)):
    # A single inverse affine resample keeps registration independent of bbox.
    return np.array(im.transform((88,88), Image.Transform.AFFINE,
                    (1/scale,0,anchor[0]-target[0]/scale,0,1/scale,anchor[1]-target[1]/scale),
                    resample=Image.Resampling.NEAREST))


def role(c, kind):
    r = ((c >> 11) & 31)*255/31
    g = ((c >> 5) & 63)*255/63
    b = (c & 31)*255/31
    marker = r > g*1.8+20 and b > g*1.8+20 and r > b*.65 and b > r*.65 and r > 70
    if kind == 'hair': return 3 if max(r,g,b) > 42 and max(r,g,b)-min(r,g,b) < 65 else 0
    if marker: return 1
    if kind == 'gear':
        if max(r,g,b)>65 and max(r,g,b)-min(r,g,b)<32 and min(r,g,b)>max(r,g,b)*.72: return 4
        if r>g*1.15 and g>b*1.15 and r>60: return 5
    if kind == 'head':
        if b > r*1.25 and g > r*1.2 and b > 55: return 2
        if g > r*1.15 and g > b*1.15 and g > 50: return 3
        if r > b*1.2 and r > g*1.05 and r > 64: return 1
    return 0


class RigEmitter(Emitter):
    def layer(self, name, rgba, kind):
        mask = rgba[:,:,3] >= 128
        ys,xs = np.where(mask)
        assert len(xs)
        x0,y0,x1,y1 = int(xs.min()),int(ys.min()),int(xs.max())+1,int(ys.max())+1
        crop=rgba[y0:y1,x0:x1].copy()
        crop[crop[:,:,3] < 128,:3]=0
        q=Image.fromarray(crop[:,:,:3]).quantize(colors=112,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE).convert('RGB')
        crop[:,:,:3]=np.array(q)
        sprite=self.sprite(name,crop)
        colors={}
        for y in range(crop.shape[0]):
            for x in range(crop.shape[1]):
                if crop[y,x,3] < 128: continue
                c=rgb565(*[int(v) for v in crop[y,x,:3]])
                if c not in colors: colors[c]=len(colors)+1
        roles=[0]+[role(c,kind) for c in colors]
        self.body.append('static const uint8_t '+name+'_roles[] = {'+','.join(map(str,roles))+'};')
        return '{'+sprite+f',{x0},{y0},{name}_roles'+'}'


def build():
    e=RigEmitter();outfits=cells('outfits.png',2,4)+cells('advanced-a-outfits.png',2,4)
    heads=cells('heads.png',8,4);hair=cells('hair.png',2,3)
    costume_rows=[];mounts=[];transforms=[]
    for i,im in enumerate(outfits):
        a=np.array(im);ys,xs=np.where(a[:,:,3]>=128);anchor=peg(im,True)
        scale=52/(int(ys.max())+1-anchor[1])
        a=register(im,scale,anchor)
        assert not (a[0,:,3]>=128).any() and not (a[86:,:,3]>=128).any(), 'body registration outside frame'
        costume_rows.append(e.layer(f'rig_costume_{i}',a,'costume'))
        m=magenta(a[:,:,:3]) & (a[:,:,3]>=128)
        points=[]
        for left in (True,False):
            yy,xx=np.indices(m.shape)
            region=m & (yy>=49) & (yy<68) & ((xx<44) if left else (xx>44))
            ys,xs=np.where(region)
            # Gauntlets have no exposed skin; registration follows the shared rig.
            points.append([round(float(np.mean(xs))) if len(xs) else (32 if left else 57),
                           round(float(np.mean(ys))) if len(ys) else 56])
        mounts.append(points);transforms.append(dict(component=f'costume{i}',anchor=anchor,scale=scale))
    # All expressions share one head scale; pegs lock each neck to the same joint.
    anchors=[peg(im,band=(.75,1)) for im in heads]
    distances=[]
    for im,anchor in zip(heads,anchors):
        ys,_=np.where(np.array(im)[:,:,3]>=128);distances.append(anchor[1]-int(ys.min()))
    hs=18/max(distances)
    head_rows=[]
    for race in range(4):
        variants=[]
        for variant in range(2):
            expr=[]
            for frame in range(4):
                i=(race*2+variant)*4+frame
                a=register(heads[i],hs,anchors[i]);expr.append(e.layer(f'rig_head_{race}_{variant}_{frame}',a,'head'))
            variants.append('{'+','.join(expr)+'}')
        head_rows.append('{'+','.join(variants)+'}')
    hair_anchors=[peg(im) for im in hair]
    distances=[]
    for im,anchor in zip(hair,hair_anchors):
        a=np.array(im);m=(a[:,:,3]>=128)&~magenta(a[:,:,:3]);ys,_=np.where(m);distances.append(anchor[1]-int(ys.min()))
    hair_scale=18/float(np.median(distances))
    hair_rows=[]
    for i,(im,anchor) in enumerate(zip(hair,hair_anchors)):
        a=np.array(im);strip_peg(a,anchor)
        a=register(Image.fromarray(a),hair_scale,anchor)
        hair_rows.append(e.layer(f'rig_hair_{i}',a,'hair'))
    hats=cells('hats.png',2,4)+cells('advanced-a-hats.png',2,4);hat_rows=[]
    for i,im in enumerate(hats):
        anchor=peg(im,band=(.78,1));a=np.array(im);strip_peg(a,anchor)
        a=register(Image.fromarray(a),.058,anchor)
        hat_rows.append(e.layer(f'rig_hat_{i}',a,'fixed'))
    equipment=cells('gear.png',4,4);gear_rows=[];knuckles=[]
    target_heights=[40,36,44,42,25,36,12,36,46,28,34,25,19]
    for i,im in enumerate(equipment):
        if i==6:
            for side in range(2):
                sub=im.crop((round(side*im.width/2),0,round((side+1)*im.width/2),im.height))
                anchor=peg(sub,band=(.45,.74));a=np.array(sub);strip_peg(a,anchor);m=a[:,:,3]>=128;ys,_=np.where(m)
                scale=12/(int(ys.max())-int(ys.min())+1)
                knuckles.append(e.layer(f'rig_knuckle_{side}',register(Image.fromarray(a),scale,anchor,(44,56)),'gear'))
            gear_rows.append(knuckles[0]);continue
        anchor=peg(im,band=(.45,.74) if i<13 else (.7,1));a=np.array(im);strip_peg(a,anchor);m=a[:,:,3]>=128;ys,_=np.where(m)
        scale=target_heights[i]/(int(ys.max())-int(ys.min())+1) if i<13 else .10
        a=register(Image.fromarray(a),scale,anchor,(44,56) if i<13 else (44,32))
        gear_rows.append(e.layer(f'rig_gear_{i}',a,'gear'))
    header='''/* Generated by scripts/build_hero_rig.py. */
#pragma once
#include "sprite.h"
#include "rpg.h"
typedef struct { wf_sprite s; int16_t x,y; const uint8_t *roles; } hero_rig_layer;
#define HERO_RIG_CLASS_COUNT 16
extern const uint8_t rig_class_families[HERO_RIG_CLASS_COUNT];
extern const uint8_t rig_class_tiers[HERO_RIG_CLASS_COUNT];
extern const hero_rig_layer rig_costumes[HERO_RIG_CLASS_COUNT];
extern const hero_rig_layer rig_heads[RACE_COUNT][2][4];
extern const hero_rig_layer rig_hair[6];
extern const uint8_t rig_hand_mounts[HERO_RIG_CLASS_COUNT][2][2];
extern const hero_rig_layer rig_hats[HERO_RIG_CLASS_COUNT];
extern const hero_rig_layer rig_gear[16];
extern const hero_rig_layer rig_knuckles[2];
'''
    source='/* Generated by scripts/build_hero_rig.py. */\n#include "hero_rig_art.h"\n'+'\n'.join(e.body)+'\n'
    families=list(range(8))+[f for f in range(4) for _ in range(2)]
    tiers=[0]*8+[1]*8
    source+='const uint8_t rig_class_families[HERO_RIG_CLASS_COUNT] = {'+','.join(map(str,families))+'};\n'
    source+='const uint8_t rig_class_tiers[HERO_RIG_CLASS_COUNT] = {'+','.join(map(str,tiers))+'};\n'
    source+='const hero_rig_layer rig_costumes[HERO_RIG_CLASS_COUNT] = {'+','.join(costume_rows)+'};\n'
    source+='const hero_rig_layer rig_heads[RACE_COUNT][2][4] = {'+','.join(head_rows)+'};\n'
    source+='const hero_rig_layer rig_hair[6] = {'+','.join(hair_rows)+'};\n'
    source+='const uint8_t rig_hand_mounts[HERO_RIG_CLASS_COUNT][2][2] = '+str(mounts).replace('[','{').replace(']','}')+';\n'
    source+='const hero_rig_layer rig_hats[HERO_RIG_CLASS_COUNT] = {'+','.join(hat_rows)+'};\n'
    source+='const hero_rig_layer rig_gear[16] = {'+','.join(gear_rows)+'};\n'
    source+='const hero_rig_layer rig_knuckles[2] = {'+','.join(knuckles)+'};\n'
    manifest=dict(grid=[88,88],feet=84,neck=[44,32],class_ids=list(range(16)),class_families=families,class_tiers=tiers,
        heads_scale=hs,hair_scale=hair_scale,body_transforms=transforms,hand_mounts=mounts,
        sources={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(SOURCE.iterdir()) if p.suffix in ('.png','.json','.txt')},
        transforms=['registration pegs','inverse affine nearest-neighbor','alpha128','palette112','RGB565','semantic palette channels'],
        semantic_roles={'0':'original','1':'skin','2':'iris','3':'hair/fur','4':'metal tier','5':'wood/leather tier'},physical_device_tested=False)
    return {OUT/'hero_rig_art.h':header,OUT/'hero_rig_art.c':source,ROOT/'assets/hero-rig/manifest.json':json.dumps(manifest,indent=2)+'\n'}


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--check',action='store_true');args=ap.parse_args()
    for path,data in build().items():
        if args.check: assert path.exists() and path.read_text()==data, 'stale '+str(path.relative_to(ROOT))
        else: path.parent.mkdir(parents=True,exist_ok=True);path.write_text(data)
    print('hero rig:16 exact-class costumes,32 heads,6 hairstyles '+('up to date' if args.check else 'registered'))


if __name__=='__main__': main()
