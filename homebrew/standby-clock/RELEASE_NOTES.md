# Standby Clock v0.1.1 — Live Weather + OLED Protection

Native PS5 homebrew by YAHBALLON.

- The previous HOME BREW / READY tile now shows **automatically detected local weather** (approximate public-IP location) using **ipapi.co** for coordinates and **Open-Meteo** for current temperature and WMO weather conditions. Updates every 15 minutes without blocking the UI.
- HTTPS certificate and hostname verification, strict bounded responses, no private API key, last valid local weather/cache fallback and clear offline state. No invented weather.
- **Fix:** AM/PM is no longer rendered below the large digital clock. In 12-hour mode it appears by the date on the right.
- **OLED burn-in mitigation:** full-screen 8-position pixel shifting every 90 seconds, dimming after 2/10 minutes of no input, additional night dimming from 23:00 to 07:00, and a 20-second black rest every half-hour. Alarms and active settings suppress dimming/rest. Defaults enabled; selectable in settings.
- Maintains the same Title-ID `PPSA99876`, older settings are migrated to the extended v2 layout.

**Limitations:** IP-derived city is approximate and may differ under VPN/ISP routing. Both providers see a network request from the PS5 public IP. Weather requires Internet, and currently uses English ASCII conditions. OLED protection is an application-level risk reduction only, not a guarantee against burn-in or a replacement for the TV's built-in protection. Weather and OLED features require validation on a real PS5.
