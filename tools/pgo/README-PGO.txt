Hortensia PGO training build
==========================

This build is slower than the normal one: it counts which code runs while you play, so the
next Hortensia can be compiled for exactly that (profile-guided optimization, PGO).

1. Use it like the normal Hortensia (same keys, firmware, games and settings: it shares
   %APPDATA%\hortensia).
2. Play what you want Hortensia to be fast at, for example 10 minutes of Mario Kart World
   (a few races, docked and handheld) and a few minutes of Ocarina of Time and Donkey Kong
   Bananza. Expect lower fps than usual: that is normal for this build.
3. Close Hortensia normally (File > Exit). The profile is also saved every 30 seconds.
4. Double-click merge-profile.bat. It writes hortensia.profdata next to it.
5. Send hortensia.profdata (about 15 MB). It replaces eglantine.profdata in the repository.
