quakespasm-shalrathy home is https://github.com/shalrathy/quakespasm-shalrathy
mail: shalrathy@gmail.com

A port of QuakeSpasm http://quakespasm.sourceforge.net

Video demonstrating (outdated) changes: https://www.youtube.com/watch?v=HvL54gOdZDs

# Changes

## Extended HUD
Always-visible clock, kills, secrets, skill, map name. Enable with e.g. 'scr_extendedhud 5'.

`scr_extendedhud 0` (default) hides is.
`scr_extendedhud 5` shows three-line hud in lower-left corner scaled 5x.
`scr_extendedhud -2` shows one-line hud in lower-left corner scaled 2x.
`scr_extendedhud_loads 0` set "LOADS:" in the extended hud to 0.

## Tracers
Draw lines highlighting various things in maps. Cheats for finding secrets.

`trace_monsters 1` show lines to all remaining monsters.
`trace_secrets 1` show lines to all remaining secrets.
`trace_shootables 1` show lines to all shootable switches.
`trace_moving 1` show lines to moving doors.
`trace_buttons 1` show lines to pressable buttons.
`trace_shootables_targets 1` show lines from shootable switches to the things they activate.
`trace_buttons_targets 1` show lines from pressable buttons to the things they activate.
`trace_items 1` show lines to items.
`trace_any 1` show lines to edicts whose classnames contain the value of `trace_any_contains`.
`trace_any_contains key2` show lines to gold keys.
`trace_any_targets 1` show lines to edicts targeted from edicts matching `trace_any_contains`.
`trace_any_targetings 1` show lines to edicts targeting to edicts matching `trace_any_contains`.
`trace_edicts` prints `edicts` for edicts being traced.

For `trace_x`s a value of `0` (default) hides the lines. A value of `1` shows all
things in the map. A value of `1000` shows things within 1000 units of the player.

## No absolute paths
Does not print absolute file system paths when on save, load, screenshot, etc.

## Radar
Print a M/S in the top-right corner when close to a monster/secret.

`radar_secrets 1000` show S in top-right corner when a un-found secret is within 1000 units.
`radar_monster 1000` show M in top-right corner when a alive monster is within 1000 units.
`radar_scale 10` make the top-right letters 10x as big.

# Changelog

## 2021-06-20 quakespasm-0.92.3-shalrathy1
initial version with extended hud and tracers.

## 2021-06-21 quakespasm-0.92.3-shalrathy2
Don't print absolute paths.
Radar.

## 2021-06-25 quakespasm-0.92.3-shalrathy3
add trace_items and trace_any and trace_any_contains and trace_print.

## 2021-07-07 quakespasm-0.92.3-shalrathy4
add trace_any_targetings and rename trace_print to trace_edicts and scr_extendedhud_loads.

## 2021-07-22 quakespasm-0.92.3-shalrathy5
make tracers support target2, target3, target4 used by some mods.
