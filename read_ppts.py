import zipfile, xml.etree.ElementTree as ET, re, os

def get_slide_text(z, slide_name):
    with z.open(slide_name) as f:
        tree = ET.parse(f)
    text = ''
    for elem in tree.iter():
        if elem.tag.endswith('}t') and elem.text:
            text += elem.text
        if elem.tag.endswith('}br'):
            text += '\n'
        if elem.tag.endswith('}p'):
            text += '\n'
    return text

base = r'c:\Users\corey\Desktop\编译实验'
ppts = [
    ('2026第一次实验.pptx', 'Exp1'),
    ('第二次实验2026.pptx', 'Exp2'),
    ('2026第三次实验.pptx', 'Exp3'),
    ('2026第四次实验.pptx', 'Exp4'),
    ('第五次实验2026.pptx', 'Exp5'),
    ('第六次实验2026.pptx', 'Exp6'),
]

for pptx, name in ppts:
    path = os.path.join(base, pptx)
    try:
        with zipfile.ZipFile(path, 'r') as z:
            slides = sorted([f for f in z.namelist() if re.match(r'ppt/slides/slide\d+\.xml', f)],
                          key=lambda x: int(re.search(r'(\d+)', x.split('/')[-1]).group(1)))
            print('='*60)
            print('  %s: %s (%d slides)' % (name, pptx, len(slides)))
            print('='*60)
            for s in slides:
                text = get_slide_text(z, s)
                lines = [l.strip() for l in text.split('\n') if l.strip()]
                slide_num = re.search(r'(\d+)', s.split('/')[-1]).group(1)
                print('\n--- Slide %s ---' % slide_num)
                for l in lines[:25]:
                    print('  %s' % l)
    except Exception as e:
        print('%s: Error - %s' % (name, str(e)))
