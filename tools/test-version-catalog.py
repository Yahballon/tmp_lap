#!/usr/bin/env python3
"""Verify release selection end-to-end without contacting GitHub."""
import importlib.util
import json
import tempfile
from pathlib import Path
from unittest.mock import patch
from io import BytesIO

spec=importlib.util.spec_from_file_location('catalog',Path(__file__).with_name('update-version.py'))
catalog=importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)

def release(slug,version,title,prerelease=False,draft=False):
    tag=f'{slug}-v{version}'
    return {'tag_name':tag,'prerelease':prerelease,'draft':draft,
            'html_url':f'https://github.com/Yahballon/tmp_lap/releases/tag/{tag}',
            'published_at':'2026-10-14T17:00:00Z',
            'assets':[{'name':title+'.zip','state':'uploaded','size':1000,
                       'browser_download_url':f'https://github.com/Yahballon/tmp_lap/releases/download/{tag}/{title}.zip'}]}

releases=[release('starbound-rebased','0.1.19','PPSA99642'),
          release('starbound-rebased','0.1.20','PPSA99642',True),
          release('starbound-rebased','0.1.21','PPSA99642',True,True),
          release('crypto-tracker','1.0.0','PPSA99442'),
          release('crypto-tracker','2.0.0','PPSA99442',True)]
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    app=root/'homebrew/starbound-rebased'
    app.mkdir(parents=True)
    (app/'app.json').write_text(json.dumps({'titleId':'PPSA99642','name':'Starbound Rebased'}))
    with patch.object(catalog,'ROOT',root), patch.object(catalog.urllib.request,'urlopen',
         side_effect=lambda *a,**k: BytesIO(json.dumps(releases).encode())):
        catalog.main()
        result=json.loads((root/'version.json').read_text())
        selected={entry['title_id']:entry['version'] for entry in result['apps']}
        assert selected=={'PPSA99442':'1.0.0','PPSA99642':'0.1.20'},selected
        before=(root/'version.json').read_bytes()
        catalog.main()
        assert (root/'version.json').read_bytes()==before
print('PASS: Relayfall prerelease selected; drafts/other prereleases excluded; catalog is deterministic')
