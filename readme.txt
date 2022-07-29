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
`trace_monsters_targetings 1` show lines to edicts targeting traced monsters.
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
`trace_select_add` select currently traced edicts. Selected edicts are always shown.
`trace_select_clear` unselect all selected edicts.
`trace_select_targets` expand selected edicts to include targets of selected edicts.
`trace_select_targetings` expand selected edicts to include edicts targeting a selected edicts.
`trace_edicts` prints `edicts` for edicts being traced.
`trace_bboxes 1` show r_showbboxes for traced edicts.

For `trace_x`s a value of `0` (default) hides the lines. A value of `1` shows all
things in the map. A value of `1000` shows things within 1000 units of the player.

## No absolute paths
Does not print absolute file system paths when on save, load, screenshot, etc.

## Radar
Print a M/S in the top-right corner when close to a monster/secret.

`radar_secrets 1000` show S in top-right corner when an un-found secret is within 1000 units.
`radar_monsters 1000` show M in top-right corner when a live monster is within 1000 units.
`radar_scale 10` make the top-right letters 10x as big.

## Speed
Show stats and change physics to understand and train wall-running, circle jumping, and bunny hopping.

`scr_speed 1` show player speed (current, avg, and max since last standstill)
`scr_speed 2` also show plot of speed over latest number of seconds
`scr_speed 3` also show plots of speed increase of different view angles
`scr_speed 4` also show plot view angle difference away from most speedy view angle over latest number of seconds
`scr_speed 14` also show jump info under cursor about each jump.
    E.g. `<^ 14 - 6 = 496 +7 30%` which means that for this jump:
      <    Strafe left key was down
      ^    Forward key was down
      14   Speed was increased by 14 due to turning in the air
      -6   Speed was decreased by 6 due to ground friction
      496  The current speed is 496
      +7   Which is 7 faster than the previous jump
      30%  This jump increased speed 30% of what was maximum possible with perfect view angles at every frame
    A new line of text is added every jump. Text lines disappear after scr_speed_history/10 seconds.
`scr_speed 24` add extra value to jump info text showing view angle degrees away from best speed on ground.
`scr_speed_scale 5` increase size of scr_speed text and plots
`scr_speed_history 100` change time plots record, 100 is 10 seconds
`scr_speed_angles 180` change shown view angles in plots, 180 means 180 degrees left and right
`scr_speed_minspeed 400` set min speed to 400 in the speed time plot (default "no" uses min of y values)
`scr_speed_maxspeed 700` set max speed to 700 in the speed time plot (default "no" uses max of y values)
`scr_speed_scale_minspeed -5` setmin speed to -5 in the angle plots (default "no" uses min of y values)
`scr_speed_scale_maxspeed 20` setmin speed to 20 in the angle plots (default "no" uses max of y values)
`sv_bunnyhopqw 1` emulate qw bunny hopping by not applying friction between bunny hops (0 to disable)
`sv_speedhelppower 1` set angle giving best speed when landing (`sv_speedhelppower 10` only changes angle up to 10 degrees)
`sv_speedhelpbunny 1` set angle giving best speed when in the air (`sv_speedhelpbunny 10` only changes angle up to 10 degrees)
`sv_speedhelpview 0` dont update visual angle when updating speed help angles
`host_framerate 0.01` slow down game. lower numbers slows game down more

Guide on how to use speed visuals at
http://shalrathy.github.io/quake-movement-speed-training/quake-movement-speed-training.html

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

## 2021-08-08 quakespasm-0.94.1-shalrathy5
make tracers support target2, target3, target4 used by some mods.
add trace_bboxes.
remove absolute path from new localization code.

## 2022-01-03 quakespasm-0.94.3-shalrathy6
updated quakespasm base to 0.94.3.
trace_monsters and radar_monsters shows monsters that have not spawned in yet (SMP and AD mod features).
added trace_monsters_targetings.
trace_secrets no longer shows discovered item-pick-up secrets in AD mod.

## 2022-07-24 quakespasm-0.94.3-shalrathy7
Add speed commands: scr_speed, scr_speed_history, scr_speed_minspeed, scr_speed_angles,
scr_speed_scale, sv_slowmo, sv_bunnyhopqw.
Add trace_select commands: trace_select_add, trace_select_clear, trace_select_targets, trace_select_targetings.

## 2022-07-27 quakespasm-0.94.3-shalrathy8
Add speed commands: scr_speed_maxspeed, scr_speed_scale_minspeed,
scr_speed_scale_maxspeed, sv_speedhelppower, sv_speedhelpbunny, sv_speedhelpview. Added
speed jump info lines.
