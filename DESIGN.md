# OpenPlay: design

OpenPlay 0.1, 6 October 2026. The mock-ups (draft 2, eight screens, every
OpenGadTools theme) are at https://claude.ai/artifact/MxJ5abGyH28nNYKPxLHAu3
(private until shared). The rules are Open Apps Look and Feel.

## 1. What it is

A player for anything datatypes.library opens. It never decodes a format
itself: the datatypes do (openamigaimage's openpicture, opensound, openvideo,
webp and webm, and the OS's own). So the player stays small, and every new
datatype is a new format for it.

| Kind | Group | What OpenPlay does |
| --- | --- | --- |
| Picture | GID_PICTURE | Shows it; Play is a slideshow, six seconds a picture |
| Sound, tune, module | GID_SOUND, GID_MUSIC | Plays, pauses, stops (DTM_TRIGGER) |
| Film, animation | GID_ANIMATION, GID_MOVIE | The same; the last frame moves on to the next item; Save CDXL |
| Text, document | GID_TEXT, GID_DOCUMENT | Shows it |

## 2. The window

```
[Open] | [Previous] [Play] [Stop] [Next] | [Repeat] [Full screen]   [Playlist] [Info] [Save CDXL]
+-----------------------------------------------+   Playlist        7 items
| the datatype object, as a gadget              |   Boing 2026.mp4
|                                               |   Lake at dusk.heic
+-----------------------------------------------+   [Add...] [Remove] [Save...]
1:36 =======O-------------------------- 4:12
[VIDEO] openvideo.datatype - 640 x 480 - decoded by media.decode/1 through the Nursery - opened in 2.0 s
```

- **On Workbench by default.** View › Own screen (and an `OWNSCREEN` ToolType)
  opens a public screen like Workbench's for OpenPlay alone. Full screen is
  always a screen of its own, with the film or picture alone; Esc, F or a
  click goes back.
- **Title:** `OpenPlay · <file>`.
- **The main action, Play, is in the accent**, and becomes Pause while
  playing (Slideshow for pictures).
- **Toggles** (Repeat, Playlist) draw pressed while on.
- **The status line** names the datatype and what decoded the file. Over a
  toolbar button it names the button and its key, since toolbar help bubbles
  aren't in OpenGadTools yet (DESIGN.md 2h, gap 4).

## 3. Buttons

Icons and text, icons, or text, as in every Open app. The style comes from,
first to last: `BUTTONS=` (Shell or ToolType), View › Buttons (kept in
`ENVARC:OpenPlay/Buttons`), then Look prefs' `buttons` line
(`ogt_buttons_style()`). View › Buttons › As in Look prefs goes back to the
last.

The player icons (play, pause, stop, previous, next, repeat, full screen,
playlist, info, convert, slideshow) are OpenGadTools' (`ogt_icons`), so
every Open app shares them.

## 4. The look

The theme, light or dark (or by the clock), the accent and Lite come from
`ENV:OpenGadTools/Look` as OpenLook reads them; older set-ups'
`ENV:OpenGadTools/Theme` works too; with neither, Open light. OpenPlay draws
nothing in its own colours: the media area is black for pictures and films,
the theme's list colour for sound.

## 5. What it remembers

`ENVARC:OpenPlay/`: `Buttons`, `Playlist` (shown or not), `Repeat`,
`OwnScreen` and `Window` (its place and size on Workbench).

## 6. Next

1. Films: playing waits on openvideo.datatype (it runs through every frame
   at once and hangs on close; reported to the datatypes work).
2. The compact window for tunes from the mock-ups: title, author, songs and
   a scope.
3. A seek bar you can drag (ADTM_LOCATE for films, the sample position for
   sound).
4. Help bubbles and underlined keys on the toolbar, when OpenGadTools has
   them.
5. Thumbnails in the playlist, made by the same datatypes at small size.
