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
          release('crypto-tracker','2.0.0','PPSA99442',True),
          release('audiobook-reader','0.1.0','PPSA99784'),
          release('audiobook-reader','0.2.0','PPSA99784'),
          release('ps5-system-info','0.1.0','PPSA99786'),
          release('ps5-dreamcastx','0.1.0','PPSA99765')]
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    app=root/'homebrew/starbound-rebased'
    app.mkdir(parents=True)
    (app/'app.json').write_text(json.dumps({'titleId':'PPSA99642','name':'Starbound Rebased'}))
    audio=root/'homebrew/audiobook-reader'
    audio.mkdir(parents=True)
    (audio/'app.json').write_text(json.dumps({'titleId':'PPSA99784','name':'Audiobook Reader','slug':'audiobook-reader'}))
    info=root/'homebrew/ps5-system-info'
    info.mkdir(parents=True)
    (info/'app.json').write_text(json.dumps({'titleId':'PPSA99786','name':'PS5 System Info','slug':'ps5-system-info'}))
    dc=root/'homebrew/ps5-dreamcastx'
    dc.mkdir(parents=True)
    (dc/'app.json').write_text(json.dumps({'titleId':'PPSA99765','name':'PS5DreamcastX','slug':'ps5-dreamcastx'}))
    with patch.object(catalog,'ROOT',root), patch.object(catalog.urllib.request,'urlopen',
         side_effect=lambda *a,**k: BytesIO(json.dumps(releases).encode())):
        catalog.main()
        result=json.loads((root/'version.json').read_text())
        selected={entry['title_id']:entry['version'] for entry in result['apps']}
        assert selected=={'PPSA99442':'1.0.0','PPSA99642':'0.1.20','PPSA99784':'0.2.0','PPSA99786':'0.1.0','PPSA99765':'0.1.0'},selected
        # A raw.githubusercontent.com artifact does not have a GitHub
        # Release; it must not silently replace a compatible console entry.
        fallback=root/'testbuilds/ps5-dreamcastx.json'
        fallback.parent.mkdir(parents=True,exist_ok=True)
        fallback.write_text(json.dumps({
            'title_id':'PPSA99765','version':'0.1.4',
            'tag':'ps5-dreamcastx-v0.1.4',
            'zip_name':'PPSA99765.zip',
            'zip_url':'https://raw.githubusercontent.com/Yahballon/tmp_lap/testbuilds-artifacts/ps5-dreamcastx/v0.1.4/PPSA99765.zip',
            'size_bytes':16120744,
        }))
        catalog.main()
        compatible=json.loads((root/'version.json').read_text())
        dreamcast=next(x for x in compatible['apps'] if x['title_id']=='PPSA99765')
        assert dreamcast['version']=='0.1.0'
        assert '/releases/download/' in dreamcast['zip_url']
        assert '/releases/tag/' in dreamcast['release_url']
        assert all('/releases/download/' in entry['zip_url'] and
                   '/releases/tag/' in entry['release_url']
                   for entry in compatible['apps'])
        before=(root/'version.json').read_bytes()
        catalog.main()
        assert (root/'version.json').read_bytes()==before
print('PASS: Audiobook Reader, System Info and PS5DreamcastX included; newest semantic versions selected; prerelease filtering and deterministic output verified')
