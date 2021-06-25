A port of QuakeSpasm http://quakespasm.sourceforge.net

Video demonstrating changes: https://www.youtube.com/watch?v=HvL54gOdZDs

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
`trace_items 1` show lines to items.
`trace_any 1` show lines to edicts whose classnames contain the value of `trace_any_contains`.
`trace_any_contains key2` show lines to gold keys.

A value of `0` (default) hides the lines. A value of `1` shows all
things in the map. A value of `1000` shows things within 1000 units.

## No absolute paths
Does not print absolute file system paths when on save, load, screenshot, etc.

## Radar
Print a M/S in the top-right corner when close to a monster/secret.

`radar_secrets 1000` show S in top-right corner when a un-found secret is within 1000 units.
`radar_monster 1000` show M in top-right corner when a alive monster is within 1000 units.
`radar_scale 10` make the top-right letters 10x as big.

# Changelog

## quakespasm-0.92.3-shalrathy1
initial version with extended hud and tracers.

## quakespasm-0.92.3-shalrathy2
Don't print absolute paths.
Radar.

## quakespasm-0.92.3-shalrathy3
add trace_items and trace_any and trace_any_contains
