#!/usr/bin/env python3
"""Restyle the REAL upstream WKAL 0.4.0 installer page (not a static mockup).

Only replaces HTML/CSS and adds small UI-only status hooks. Original AppCache,
HTTP endpoints, failure/retry behavior, /install and /logs logic are preserved.
Run after checkout, before native build/staging the file registry.
"""
from __future__ import annotations
import argparse
import re
from pathlib import Path

CSS = r"""
:root{--cyan:#05e7ff;--blue:#1777ff;--mint:#16edb6;--purple:#9d57ff;--dim:#90bbd1;--edge:rgba(0,214,255,.73);--navy:#030b19}
*{box-sizing:border-box;margin:0;padding:0}
html,body{min-height:100%;width:100%}
body{color:#e8f8ff;font-family:Arial,"Helvetica Neue",Helvetica,sans-serif;background:#010817;background-image:linear-gradient(90deg,rgba(0,4,15,.90),rgba(0,8,26,.84) 61%,rgba(0,8,26,.52)),url('/background.png');background-position:center center;background-size:cover;background-attachment:fixed;overflow-x:hidden}
body:before{content:"";pointer-events:none;position:fixed;inset:0;z-index:0;background:repeating-linear-gradient(0deg,transparent 0,transparent 4px,rgba(0,169,255,.035) 5px),linear-gradient(180deg,rgba(0,2,10,.14),transparent 55%,rgba(0,2,16,.68))}
body:after{content:"";pointer-events:none;position:fixed;inset:0;z-index:0;background:linear-gradient(90deg,rgba(0,225,255,.045) 1px,transparent 1px);background-size:130px 100%;opacity:.55}
[hidden]{display:none!important}
.cyber{position:relative;z-index:1;width:95%;max-width:1900px;margin:0 auto;min-height:100vh;padding:2.0vh 0 1.2vh}
.mast{height:24vh;min-height:166px;position:relative;display:-webkit-flex;display:flex;align-items:center;justify-content:space-between;gap:1rem}
.brand{position:relative;z-index:2;max-width:70%}
.brand .wordmark{font-family:Impact,"Arial Black",Arial,sans-serif;font-size:clamp(42px,6.3vw,116px);font-style:italic;font-weight:900;line-height:.98;letter-spacing:-.065em;transform:skew(-6deg);text-shadow:0 3px 0 #004078,0 0 13px rgba(14,155,255,.95),6px 6px 0 rgba(0,37,76,.95);white-space:nowrap}
.wordmark em{color:#08dcff;font-style:inherit;text-shadow:0 0 25px #008dff,5px 5px 0 #173d94}
.subname{font-size:clamp(14px,1.8vw,29px);letter-spacing:.31em;margin-top:.65vh;margin-left:5%;color:#e4f3ff}
.mantra{color:var(--cyan);font-size:clamp(8px,.83vw,15px);font-weight:bold;letter-spacing:.22em;margin-top:1.1vh;margin-left:5%;white-space:nowrap}
.head-sign{align-self:flex-start;padding:1.1vh 1vw;text-align:right;background:rgba(0,11,27,.54);border-right:2px solid var(--cyan);box-shadow:0 0 20px rgba(0,170,255,.12)}
.head-sign strong{color:white;font-size:clamp(15px,1.8vw,28px);letter-spacing:.27em}
.head-sign span{display:block;margin-top:.6vh;color:#76b7dd;font-size:clamp(9px,.85vw,14px);letter-spacing:.35em}
.top-lock{position:absolute;right:11%;top:-4vh;height:28vh;width:30vw;max-width:530px;background:url('/background.png') no-repeat 100% 54%/auto 150%;filter:drop-shadow(0 0 14px #0378e8);opacity:.75;pointer-events:none;z-index:1;-webkit-mask-image:linear-gradient(90deg,transparent,#000 25%,#000 80%,transparent);mask-image:linear-gradient(90deg,transparent,#000 25%,#000 80%,transparent)}
.layout{display:grid;grid-template-columns:minmax(0,2.73fr) minmax(280px,1fr);grid-template-rows:auto auto;gap:1.1vh 1%;height:auto}
.left-stack{display:flex;flex-direction:column;gap:1.1vh;min-width:0}
.rail{display:flex;flex-direction:column;gap:1.1vh;min-width:0}
.panel{border:1px solid var(--edge);background:linear-gradient(120deg,rgba(1,8,24,.93),rgba(0,18,42,.86));box-shadow:inset 0 0 20px rgba(0,168,255,.10),0 0 13px rgba(0,183,255,.18);position:relative;overflow:hidden}
.panel:before{content:"";position:absolute;top:0;left:0;width:33%;height:2px;background:var(--cyan);box-shadow:0 0 12px var(--cyan)}
.panel:after{content:"";position:absolute;right:0;bottom:0;width:17%;height:2px;background:#3676ff;box-shadow:0 0 9px #3676ff}
.hero{min-height:23vh;padding:2.5vh 1.6vw;display:grid;grid-template-columns:15% 1fr 28%;gap:1%;align-items:center;background:linear-gradient(90deg,rgba(0,6,20,.97),rgba(0,14,39,.94) 61%,rgba(0,35,78,.46)),url('/background.png') no-repeat 95% 43%/auto 130%}
.seal{text-align:center;font-size:clamp(55px,7vw,115px);font-family:Arial,sans-serif;font-weight:900;line-height:1;color:var(--cyan);text-shadow:0 0 18px var(--cyan)}
.seal>span{display:inline-flex;align-items:center;justify-content:center;width:6.6vw;height:6.6vw;min-width:65px;min-height:65px;border:3px solid currentColor;border-radius:50%;box-shadow:inset 0 0 23px rgba(0,248,255,.23),0 0 19px currentColor;font-size:.63em}
body[data-stage="complete"] .seal{color:var(--mint)}body[data-stage="error"] .seal{color:#ff5f83}
.eyebrow{color:var(--cyan);letter-spacing:.22em;font-weight:bold;font-size:clamp(10px,.90vw,16px);margin-bottom:1.1vh}
#heroHeading{font-family:Impact,"Arial Black",Arial,sans-serif;font-style:italic;line-height:1.05;letter-spacing:.015em;font-size:clamp(28px,3.25vw,62px);color:#f7fbff;text-shadow:0 0 10px rgba(2,118,243,.85);white-space:nowrap}
#heroHeading b{color:var(--cyan)}body[data-stage="complete"] #heroHeading b{color:#26dffb}body[data-stage="error"] #heroHeading b{color:#ff6883}
.hero #status{font-size:clamp(13px,1.12vw,21px);font-weight:bold;margin-top:1.4vh;color:#9ceeff}
.hero #detail{margin-top:.5vh;line-height:1.35;color:#b0cfe1;font-size:clamp(11px,.96vw,18px)}
.hero-caption{margin-top:.7vh;color:#6c9cb9;font-size:clamp(10px,.78vw,15px)}
.pirate{justify-self:center;align-self:stretch;display:flex;justify-content:center;align-items:center;opacity:.96;max-width:260px;width:100%}
.pirate svg{width:100%;max-height:21vh;filter:drop-shadow(0 0 13px #00bbff)}
.track{min-height:15vh;padding:1.45vh 1vw 1.05vh}
.track-title,.panel-title{font-size:clamp(12px,1.04vw,19px);letter-spacing:.08em;font-weight:bold;display:flex;justify-content:space-between;align-items:center;gap:.6em;color:#dcefff}
.track-title small{color:#79a4c3;font-size:.80em;font-weight:normal;letter-spacing:.03em}
.steps{display:flex;justify-content:space-between;gap:1%;margin-top:1.2vh}
.step{width:20%;position:relative;text-align:center;color:#7892af}
.step:before{content:"";position:absolute;top:1.5vw;height:2px;left:-52%;right:52%;background:#0a5269;z-index:0}
.step:first-child:before{display:none}
.step .dot{position:relative;display:inline-flex;align-items:center;justify-content:center;width:3vw;height:3vw;min-width:36px;min-height:36px;max-width:65px;max-height:65px;border:2px solid #335877;border-radius:50%;background:#001327;color:#7892af;font-weight:bold;font-size:clamp(15px,1.25vw,24px);box-shadow:inset 0 0 12px rgba(0,132,255,.10)}
.step.is-current .dot{color:#fff;border-color:var(--cyan);box-shadow:0 0 16px rgba(0,212,255,.85),inset 0 0 16px rgba(0,169,255,.4)}
.step.is-done .dot{color:var(--mint);border-color:var(--mint);box-shadow:0 0 10px rgba(0,255,185,.42)}
.step.is-done:before,.step.is-current:before{background:var(--cyan);box-shadow:0 0 5px var(--cyan)}
.step-name{font-size:clamp(10px,.9vw,17px);font-weight:bold;margin-top:.7vh;color:#ddeaff}
.step-meta{margin-top:.35vh;font-size:clamp(9px,.78vw,15px);color:#7ea6c4}
.step.is-current .step-meta{color:var(--cyan)}.step.is-done .step-meta{color:var(--mint)}
#progressWrap{position:relative;width:100%;height:6px;border-radius:3px;background:#14314c;margin-top:1.35vh;overflow:hidden}
#progressBar{position:absolute;left:0;top:0;width:100%;height:100%;background:linear-gradient(90deg,#1460f3,#05edff);transform:scaleX(0);transform-origin:left;-webkit-transform:scaleX(0);-webkit-transform-origin:left;transition:transform .25s;-webkit-transition:-webkit-transform .25s}
#progressPct{position:absolute;right:0;top:0;color:white;font-size:1px;opacity:0}
.info{padding:1.4vh 1.15vw;min-height:20vh}
.info .panel-title,.ready .panel-title{padding-bottom:1.1vh;border-bottom:1px solid #17456c}
.specs{display:grid;grid-template-columns:38% 1fr;gap:.55vh .75vw;margin-top:1.45vh;font-size:clamp(11px,.94vw,18px)}
.specs dt{color:#829fba}.specs dd{color:#dfedff;overflow-wrap:anywhere}.bright{color:var(--mint)!important}
.ready{padding:1.35vh 1.15vw;min-height:18vh}
.ready ul{list-style:none;margin-top:1.3vh;font-size:clamp(11px,.9vw,17px)}
.ready li{display:flex;gap:.5rem;margin:0 0 .86vh;align-items:center;color:#adc6d7}
.ready li:before{content:"◇";color:var(--cyan);font-size:1.1em}
.ready .ready-state{margin-left:auto;color:var(--cyan);white-space:nowrap}
#overallIndicator{color:var(--cyan);font-size:.87em}
body[data-stage="complete"] #overallIndicator{color:var(--mint)}
body[data-stage="error"] #overallIndicator{color:#ff6a83}
.console{grid-column:1/-1;min-height:24vh;padding:1.2vh 1.25vw;display:flex;flex-direction:column}
.console-head{padding-bottom:.95vh;border-bottom:1px solid #145783;display:flex;justify-content:space-between;align-items:center;font-weight:bold;letter-spacing:.075em;font-size:clamp(12px,1.1vw,19px)}
.console-head .note{font-size:clamp(10px,.84vw,15px);letter-spacing:0;color:var(--cyan);font-weight:normal}
#logContainer{flex:1;margin-top:.8vh;max-height:21vh;min-height:15vh;overflow-y:auto;overflow-wrap:anywhere;white-space:pre-wrap;font-family:"Courier New",monospace;color:#b5d7e9;font-size:clamp(10px,.80vw,16px);line-height:1.46;scrollbar-color:#00c4ec #061527}
#logContainer:empty:before{content:"[SYSTEM] Waiting for WebKit installer logs...";color:#6494b7}
#logContainer .success{color:#18efa7}#logContainer .warning{color:#f2d66e}#logContainer .error{color:#ff6588}
.bottom-note{text-align:right;margin-top:.8vh;font-size:clamp(9px,.72vw,14px);color:#7599b3}
#cacheCorruptOverlay{position:fixed;z-index:100;inset:0;background:rgba(0,3,12,.93);display:flex;align-items:center;justify-content:center;padding:2rem}
#cacheCorruptOverlay .card{max-width:760px;width:100%;padding:2.5rem;border:2px solid #f4ab42;background:linear-gradient(160deg,#071626,#00101f);box-shadow:0 0 25px rgba(241,158,44,.28)}
#cacheCorruptOverlay h2{color:#ffbf55;margin-bottom:1.4rem;font-size:clamp(22px,2vw,36px)}
#cacheCorruptOverlay p{color:#d4dfe9;line-height:1.5;font-size:clamp(16px,1.2vw,21px)}
#cacheCorruptOverlay .actions{display:flex;gap:1rem;flex-wrap:wrap;margin-top:1.5rem}
#cacheCorruptOverlay .btn{padding:1rem 1.2rem;border:1px solid var(--cyan);color:#f5fbff;background:#073353;cursor:pointer;font-weight:bold;font-size:18px}
#cacheCorruptOverlay .btn.secondary{background:#1d2433;border-color:#64748b}
#cacheCorruptOverlay .actions[hidden]{display:none!important}
@media(max-width:850px){.cyber{width:96%;padding-top:1vh}.mast{min-height:125px;height:20vh}.brand{max-width:100%}.brand .wordmark{font-size:clamp(38px,7vw,70px)}.head-sign{display:none}.top-lock{display:none}.layout{display:flex;flex-direction:column}.hero{grid-template-columns:14% 1fr 23%;padding:2vh 1vw;min-height:22vh}.hero #heroHeading{font-size:clamp(23px,4vw,40px)}.rail{flex-direction:row}.rail .panel{flex:1;min-width:0}.track{min-height:135px}.console{min-height:20vh}.step:before{top:18px}.step .dot{width:36px;height:36px}}
@media(max-width:520px){.mast{height:18vh;min-height:105px}.brand .wordmark{font-size:clamp(29px,8vw,48px)}.subname{font-size:12px;letter-spacing:.18em}.mantra{font-size:7px;letter-spacing:.04em}.hero{grid-template-columns:20% 1fr}.pirate{display:none}.hero #heroHeading{font-size:clamp(23px,5.5vw,31px)}.seal>span{min-width:45px;min-height:45px}.steps{gap:0}.step-name{font-size:9px}.step-meta{font-size:8px}.step .dot{min-width:27px;min-height:27px;width:27px;height:27px;font-size:14px}.step:before{top:13px}.rail{flex-direction:column}.console{min-height:27vh}#logContainer{min-height:20vh}.bottom-note{text-align:left}}
@media(prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
"""

PIRATE_SVG = r"""<svg viewBox="0 0 240 210" role="img" aria-label="Cyber pirate emblem">
<defs><linearGradient id="cloak" x2="1" y2="1"><stop stop-color="#00214b"/><stop offset="1" stop-color="#000711"/></linearGradient></defs>
<path d="M120 6 C73 20 44 51 24 162 L5 205 L235 205 L214 162 C196 52 166 24 120 6Z" fill="url(#cloak)" stroke="#11a8ff" stroke-width="4"/>
<path d="M120 21 C82 36 68 72 59 121 M120 21 C160 40 180 82 184 122" fill="none" stroke="#0ae8ff" stroke-width="4" opacity=".83"/>
<path d="M70 100 Q120 63 170 100 L170 140 Q158 168 120 182 Q80 171 69 140Z" fill="#020c1b" stroke="#0bb5f4" stroke-width="2"/>
<path d="M77 109 L111 120 L97 138 L78 130 Z M163 109 L128 120 L143 139 L165 126Z" fill="#0ccfff"/>
<path d="M120 124 L110 150 L130 150Z" fill="#0be1ff"/>
<path d="M100 158 L142 158 L136 175 L106 175Z" fill="#c9edff"/>
<path d="M113 158 V173 M122 158 V173 M131 158 V173" stroke="#02294a" stroke-width="4"/>
<path d="M100 28 L112 43 L121 24 L133 43 L145 28 L143 51 L102 51 Z" fill="none" stroke="#02d7fc" stroke-width="4"/>
<path d="M44 177 L3 206 M196 177 L237 206" stroke="#00b2fa" stroke-width="7" opacity=".65"/>
</svg>"""

MARKUP = r"""
<div class="cyber" id="cyberUI">
  <header class="mast">
    <div class="top-lock" aria-hidden="true"></div>
    <div class="brand">
      <div class="wordmark">JAILBREAK <em>ME</em></div>
      <div class="subname">WebKit Launcher</div>
      <div class="mantra">OPEN SYSTEMS &nbsp;◆&nbsp; MORE FREEDOM &nbsp;◆&nbsp; FOR THE PLAYERS</div>
    </div>
    <div class="head-sign"><strong>PS5</strong><span>HOMEBREW EDITION</span></div>
  </header>
  <main class="layout">
    <section class="left-stack" aria-label="Automated installation status">
      <article class="panel hero">
        <div class="seal" aria-hidden="true"><span id="sealGlyph">⟳</span></div>
        <div class="hero-copy">
          <div class="eyebrow">JAILBREAK ME · WEBKIT AUTOLOADER v[[VERSION_PLACEHOLDER]]</div>
          <h1 id="heroHeading">AUTO <b>INSTALLER</b></h1>
          <p id="status" role="status">Initializing...</p>
          <p id="detail">Preparing the WebKit cache. No user input required.</p>
          <p class="hero-caption">AUTOMATIC SETUP · ORIGINAL WEBKIT EXPLOIT CHAIN UNCHANGED</p>
        </div>
        <div class="pirate">""" + PIRATE_SVG + r"""</div>
      </article>
      <section class="panel track" aria-label="Automatic setup progress">
        <div class="track-title">AUTO-INSTALL SEQUENCE <small id="phaseLabel">Starting installation...</small></div>
        <div class="steps">
          <div class="step is-current" id="stage1"><span class="dot">01</span><div class="step-name">INITIALIZE</div><div class="step-meta">Starting</div></div>
          <div class="step" id="stage2"><span class="dot">02</span><div class="step-name">NETWORK</div><div class="step-meta">Waiting</div></div>
          <div class="step" id="stage3"><span class="dot">03</span><div class="step-name">CACHE</div><div class="step-meta">Pending</div></div>
          <div class="step" id="stage4"><span class="dot">04</span><div class="step-name">INSTALL</div><div class="step-meta">Pending</div></div>
          <div class="step" id="stage5"><span class="dot">05</span><div class="step-name">FINALIZE</div><div class="step-meta">Pending</div></div>
        </div>
        <div id="progressWrap" hidden><div id="progressBar"></div><div id="progressPct">0%</div></div>
      </section>
    </section>
    <aside class="rail" aria-label="System and installation information">
      <section class="panel info">
        <div class="panel-title"><span>◇ SYSTEM INFORMATION</span><span>PS5</span></div>
        <dl class="specs">
          <dt>Console</dt><dd>PlayStation 5</dd>
          <dt>Firmware</dt><dd id="fwVersion">Detecting...</dd>
          <dt>Launcher</dt><dd>WebKit (Auto)</dd>
          <dt>Title ID</dt><dd>WKAG00001</dd>
          <dt>Web server</dt><dd>Local · 127.0.0.1</dd>
          <dt>Internet</dt><dd>Not required</dd>
        </dl>
      </section>
      <section class="panel ready">
        <div class="panel-title"><span>⌁ INSTALL STATUS</span><span id="overallIndicator">● ACTIVE</span></div>
        <ul>
          <li>Installer <span class="ready-state" id="installState">Initializing</span></li>
          <li>WebKit cache <span class="ready-state" id="cacheState">Checking</span></li>
          <li>Game shortcut <span class="ready-state" id="tileState">Pending</span></li>
          <li>Auto-launch <span class="ready-state">On tile launch</span></li>
        </ul>
      </section>
    </aside>
    <section class="panel console" aria-label="Live installation log">
      <div class="console-head"><span>⌲ LIVE INSTALLATION LOG</span><span class="note" id="consoleState">● STREAMING</span></div>
      <div id="logContainer" role="log" aria-live="polite" aria-relevant="additions text"></div>
    </section>
  </main>
  <footer class="bottom-note">JAILBREAK ME · WebKit Autoloader v[[VERSION_PLACEHOLDER]] by PLK · Open source: github.com/itsPLK/ps5-webkit-autoloader</footer>
</div>
"""

UI_JS = r"""
(function () {
  'use strict';
  var current = 1;
  var status = 'active';
  var phases = {
    network: [2, 'Local connection established'],
    cache: [3, 'Checking offline cache...'],
    download: [3, 'Downloading AppCache assets...'],
    install: [4, 'Registering Game shortcut...'],
    complete: [5, 'Installation finished'],
    slow: [3, 'Cache taking longer than expected'],
    corrupt: [3, 'WebKit cache recovery required'],
    error: [4, 'Installation failed']
  };
  var fw = document.getElementById('fwVersion');
  var match = navigator.userAgent.match(/PlayStation 5[\/\s]([0-9]+\.[0-9]+)/i);
  fw.textContent = match ? match[1] : 'Unavailable';
  function val(id, message) { document.getElementById(id).textContent = message; }
  function title(message) { document.getElementById('heroHeading').innerHTML = message; }
  window.wkalCyberStage = function (key) {
    var phase = phases[key]; if (!phase) return;
    var failure = key === 'error' || key === 'corrupt';
    if (!failure && key !== 'slow' && phase[0] < current) return;
    current = phase[0]; status = key;
    document.body.setAttribute('data-stage', failure ? 'error' : key);
    val('phaseLabel', phase[1]);
    val('installState',key === 'complete' ? 'Installed' :
         failure ? 'Action required' : key === 'install' ? 'Installing' : 'In progress');
    if (key === 'network') val('cacheState','Checking');
    if (key === 'cache') val('cacheState','Checking');
    if (key === 'download') val('cacheState','Downloading');
    if (key === 'install' || key === 'complete') val('cacheState','Cached');
    if (key === 'complete') val('tileState','Installed');
    val('consoleState', key === 'complete' ? '● INSTALLATION COMPLETE' :
       failure ? '● ACTION REQUIRED' : '● STREAMING');
    val('overallIndicator',key === 'complete' ? '● INSTALLED' :
        failure ? '● WARNING' : '● ACTIVE');
    val('sealGlyph',key === 'complete' ? '✓' : failure ? '!' : '⟳');
    if (key === 'complete') title('INSTALLATION <b>COMPLETE</b>');
    else if (failure) title('CACHE / INSTALL <b>ERROR</b>');
    else if (key === 'slow') title('STILL <b>CACHING</b>');
    else if (key === 'install') title('INSTALLING <b>SHORTCUT</b>');
    else title('AUTOMATIC <b>SETUP</b>');
    for (var i=1;i<=5;i++) {
      var item=document.getElementById('stage'+i);
      item.className='step'+(i < current || key === 'complete' ? ' is-done':i === current ? ' is-current':'');
      var meta = item.getElementsByClassName('step-meta')[0];
      meta.textContent=i < current || key === 'complete' ? 'Complete' :
          i === current ? (failure ? 'Error' : 'Working...') : 'Pending';
      var dot = item.getElementsByClassName('dot')[0];
      if (i < current || key === 'complete') dot.textContent='✓';
      else dot.textContent='0'+i;
    }
  };
})();
"""

def patch(root: Path) -> None:
    path = root / "frontend/installer-page/index.html"
    if not path.exists():
        raise FileNotFoundError("Missing original installer-page/index.html")
    raw = path.read_text(encoding="utf-8")
    if "function setInstalled()" not in raw or "function setCacheCorrupted()" not in raw:
        raise ValueError("Unexpected upstream v0.4.0 installer logic")
    if "wkalCyberStage" in raw:
        raise ValueError("Already themed")
    if raw.count("<style>") != 1 or raw.count("</style>") != 1:
        raise ValueError("Missing expected single stylesheet")
    raw = re.sub(r"<title>.*?</title>",
                 "<title>JAILBREAK ME · WebKit Launcher v[[VERSION_PLACEHOLDER]]</title>",
                 raw, count=1, flags=re.S)
    raw = re.sub(r"<style>.*?</style>", "<style>\n" + CSS + "\n</style>", raw, count=1, flags=re.S)
    raw = re.sub(r"(?s)<main>.*?</main>", MARKUP, raw, count=1)
    raw, n = re.subn(r"(?s)\s*<footer id=\"footer\">.*?</footer>", "", raw, count=1)
    if n != 1:
        raise ValueError("Original attribution footer not found")
    raw = raw.replace("<script>\n    (function () {", "<script>\n" + UI_JS + "\n</script>\n<script>\n    (function () {", 1)
    patches = [
        ("function setInstalled() {", "function setInstalled() {\n        window.wkalCyberStage('complete');"),
        ("function setFailed() {", "function setFailed() {\n        window.wkalCyberStage('error');"),
        ("function setSlow() {", "function setSlow() {\n        window.wkalCyberStage('slow');"),
        ("function setCaching() {", "function setCaching() {\n        window.wkalCyberStage('cache');"),
        ("function installApp() {", "function installApp() {\n        window.wkalCyberStage('install');"),
        ("function setCacheCorrupted() {", "function setCacheCorrupted() {\n        window.wkalCyberStage('corrupt');"),
        ("function appendLog(msg) {", "function appendLog(msg) {"),
        ("div.textContent = msg;", "div.textContent = msg;\n        if (/ERROR|failed|corrupt/i.test(msg)) div.className='error';\n        else if (/SUCCESS|installed successfully|cached successfully/i.test(msg)) div.className='success';\n        else if (/WARNING|WARN/i.test(msg)) div.className='warning';"),
        ("isDownloading = true;\n          appendLog(\"[AppCache] Downloading cache...\");", "isDownloading = true;\n          window.wkalCyberStage('download');\n          appendLog(\"[AppCache] Downloading cache...\");"),
    ]
    for orig,new in patches:
        if raw.count(orig) != 1:
            raise ValueError(f"Upstream event hook differs ({raw.count(orig)}): {orig}")
        raw = raw.replace(orig,new,1)
    raw = raw.replace("style.color = '#22c55e'", "style.color = '#16edb6'")
    assert raw.count('id="logContainer"') == 1
    assert raw.count('id="status"') == 1
    assert raw.count('id="progressBar"') == 1
    assert "/install" in raw and "/logs?pos=" in raw and "applicationCache" in raw
    assert "clear-webkit-data" in raw and "goToAutoloader" in raw
    path.write_text(raw, encoding="utf-8")

    # The installer master page is cached by Sony WebKit AppCache.  The upstream
    # build version is still 0.4.0 and an existing console may already have
    # /?v=0.4.0 cached; a UI-only cache-buster ensures the new interface is
    # actually fetched without changing the exploit payload chain.
    launcher = root / "src/main.c"
    source = launcher.read_text(encoding="utf-8")
    url_changes = (
        ('"http://127.0.0.1:%d/?v=%s%s&retry=1"',
         '"http://127.0.0.1:%d/?v=%s%s&ui=cyberpirate1&retry=1"'),
        ('"http://127.0.0.1:%d/?v=%s%s"',
         '"http://127.0.0.1:%d/?v=%s%s&ui=cyberpirate1"'),
    )
    for needle, replacement in url_changes:
        if source.count(needle) != 1:
            raise ValueError(f"Missing original installer browser URL: {needle}")
        source = source.replace(needle, replacement, 1)
    launcher.write_text(source, encoding="utf-8")
    print(f"Applied real JAILBREAK ME cyber-pirate installer UI ({len(raw)} bytes) and fresh AppCache master URL; original endpoints/logic retained.")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("upstream", type=Path)
    patch(p.parse_args().upstream)
