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

releases=[release('ps5nesx','1.0.9','PPSA99204'),
          release('zquestclassic-ps5','0.1.0','PPSA99127',True),
          release('zquestclassic-ps5','0.1.1','PPSA99127',True,True),
          release('starbound-rebased','0.1.19','PPSA99642'),
          release('starbound-rebased','0.1.20','PPSA99642',True),
          release('starbound-rebased','0.1.21','PPSA99642',True,True),
          release('crypto-tracker','1.0.0','PPSA99442'),
          release('crypto-tracker','2.0.0','PPSA99442',True),
          release('audiobook-reader','0.1.0','PPSA99784'),
          release('audiobook-reader','0.2.0','PPSA99784'),
          release('ps5-system-info','0.1.0','PPSA99786'),
          release('bible-reader','0.1.0','PPSA99793'),
          release('ps5-dreamcastx','0.1.0','PPSA99765'),
          release('godot-ps5-probe','0.1.0','PPSA99842',True),
          release('godot-ps5-probe','0.2.0','PPSA99842',True,True)]
standalone=[{
    'tag_name':'v0.1.24','prerelease':False,'draft':False,
    'html_url':'https://github.com/Yahballon/PSJukebox/releases/tag/v0.1.24',
    'published_at':'2026-10-18T18:54:10Z',
    'assets':[{'name':'PPSA99947.zip','state':'uploaded','size':21797369,
               'browser_download_url':'https://github.com/Yahballon/PSJukebox/releases/download/v0.1.24/PPSA99947.zip'}]
}]
def fake_open(req,timeout=30):
    return BytesIO(json.dumps(standalone if '/Yahballon/PSJukebox/' in req.full_url else releases).encode())

with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    # PS5NESX must remain discoverable alongside the other releases.
    zquest=root/'homebrew/zquestclassic-ps5'
    zquest.mkdir(parents=True)
    (zquest/'app.json').write_text(json.dumps({'titleId':'PPSA99127','name':'ZQuest Classic PS5','slug':'zquestclassic-ps5'}))
    nes=root/'homebrew/ps5nesx'
    nes.mkdir(parents=True)
    (nes/'app.json').write_text(json.dumps({'titleId':'PPSA99204','name':'PS5NESX','slug':'ps5nesx'}))
    app=root/'homebrew/starbound-rebased'
    app.mkdir(parents=True)
    (app/'app.json').write_text(json.dumps({'titleId':'PPSA99642','name':'Starbound Rebased'}))
    audio=root/'homebrew/audiobook-reader'
    audio.mkdir(parents=True)
    (audio/'app.json').write_text(json.dumps({'titleId':'PPSA99784','name':'Audiobook Reader','slug':'audiobook-reader'}))
    bible=root/'homebrew/bible-reader'
    bible.mkdir(parents=True)
    (bible/'app.json').write_text(json.dumps({'titleId':'PPSA99793','name':'Bible Reader','slug':'bible-reader'}))
    info=root/'homebrew/ps5-system-info'
    info.mkdir(parents=True)
    (info/'app.json').write_text(json.dumps({'titleId':'PPSA99786','name':'PS5 System Info','slug':'ps5-system-info'}))
    dc=root/'homebrew/ps5-dreamcastx'
    dc.mkdir(parents=True)
    (dc/'app.json').write_text(json.dumps({'titleId':'PPSA99765','name':'PS5DreamcastX','slug':'ps5-dreamcastx'}))
    godot=root/'homebrew/godot-ps5'
    godot.mkdir(parents=True)
    (godot/'app.json').write_text(json.dumps({'titleId':'PPSA99842','name':'Godot PS5 Probe','slug':'godot-ps5','engineIntegrated':False}))
    p=root/'homebrew/psjukebox'
    p.mkdir(parents=True)
    (p/'app.json').write_text(json.dumps({'titleId':'PPSA99947','name':'PSJukebox','slug':'psjukebox'}))
    with patch.object(catalog,'ROOT',root), patch.object(catalog.urllib.request,'urlopen',side_effect=fake_open):
        catalog.main()
        result=json.loads((root/'version.json').read_text())
        selected={entry['title_id']:entry['version'] for entry in result['apps']}
        assert selected['PPSA99204']=='1.0.9'
        assert selected['PPSA99127']=='0.1.0' # Opt-in prototype, never an unlisted prerelease
        assert result['app_count']==len(result['apps'])
        assert selected=={'PPSA99842':'0.1.0','PPSA99127':'0.1.0','PPSA99204':'1.0.9','PPSA99442':'1.0.0','PPSA99642':'0.1.20','PPSA99784':'0.2.0','PPSA99786':'0.1.0','PPSA99765':'0.1.0','PPSA99793':'0.1.0','PPSA99947':'0.1.24'},selected
        godot_entry=next(e for e in result['apps'] if e['title_id']=='PPSA99842')
        assert godot_entry['name']=='Godot PS5 Probe'
        assert godot_entry['tag']=='godot-ps5-probe-v0.1.0'
        assert godot_entry['zip_url']=='https://github.com/Yahballon/tmp_lap/releases/download/godot-ps5-probe-v0.1.0/PPSA99842.zip'
        jukebox=next(e for e in result['apps'] if e['title_id']=='PPSA99947')
        assert jukebox['zip_url']=='https://github.com/Yahballon/PSJukebox/releases/download/v0.1.24/PPSA99947.zip'
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
print('PASS: TESTBUILDS catalog includes Godot PS5 Probe prerelease and preserves stable releases')
