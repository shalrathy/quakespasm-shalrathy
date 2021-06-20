A port of QuakeSpasm http://quakespasm.sourceforge.net

# Changes

## Extended HUD
Always-visible clock, kills, secrets, skill, map name. Enable with e.g. 'scr_extendedhud 5'.

`scr_extendedhud 0` (default) hides is.
`scr_extendedhud 5` shows three-line hud in lower-left corner scaled 5x.
`scr_extendedhud -2` shows one-line hud in lower-left corner scaled 2x.

## Tracers
Draw lines highlighting various things in maps. Cheats for finding secrets.

`trace_monsters 1` show lines to all remaining monsters.
`trace_secrets 1` show lines to all remaining secrets.
`trace_shootables 1` show lines to all shootable switches.
`trace_moving 1` show lines to moving doors. Shows where that sound came from.
`trace_buttons 1` show lines to pressable buttons.
`trace_shootables_targets 1` show lines from shootable switches to the things they activate.
`trace_buttons_targets 1` show lines from pressable buttons to the things they activate.

A value of `0` (default) hides the lines. A value of `1` shows all
things in the map. A value of `1000` shows things within 1000 units.
