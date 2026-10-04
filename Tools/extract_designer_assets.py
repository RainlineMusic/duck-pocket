"""Derive static bodies and outline labels from the supplied 800 x 865 SVG.

Only unchanging material is rasterised. Value arcs, indicators, text values and
audio traces are drawn by JUCE and remain interactive. Requires Inkscape.
"""
from pathlib import Path
import copy
import subprocess
import xml.etree.ElementTree as ET

root_dir = Path(__file__).resolve().parents[1]
source = root_dir / 'Assets/Designer/Reference.svg'
root = ET.parse(source).getroot()
scene = root[0]
ns = '{http://www.w3.org/2000/svg}'
ET.register_namespace('', 'http://www.w3.org/2000/svg')

def asset(name, indices, bounds, material=False):
    x, y, w, h = bounds
    out = ET.Element(ns + 'svg', width=str(w), height=str(h), viewBox=f'0 0 {w} {h}', fill='none')
    if material:
        out.append(copy.deepcopy(root[1]))
    group = ET.SubElement(out, ns + 'g', transform=f'translate({-x} {-y})')
    for i in indices:
        element = copy.deepcopy(scene[i])
        if not material:
            for e in element.iter():
                e.attrib.pop('filter', None)
        group.append(element)
    path = source.parent / (name + '.svg')
    ET.ElementTree(out).write(path, encoding='utf-8', xml_declaration=True)
    if material:
        subprocess.run(['inkscape', str(path), '--export-type=png', f'--export-filename={path.with_suffix(".png")}', f'--export-width={w*4}'], check=True)

asset('DialLarge', [12,13,14], (60,108,240,250), True)
asset('DialSmall', [32,33,37], (334,68,132,135), True)
asset('TitleInfluence', [15], (60,108,240,250))
asset('TitleDuration', [28], (503,108,240,250))
asset('TitleOutput', [38], (334,68,132,135))
asset('TitleMS', [48], (334,207,132,135))
asset('Logo', list(range(2,12))+list(range(132,137)), (0,0,92,48))
