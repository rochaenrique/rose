# TODO

## Now
 - Watch Window
   - Tabs be watch window with set expr
     - Maybe change views to be a "context for expr ui rule"

 - Structured expression editor (ui?)

 - eval filtering function

 - show highlighted term when filtering lister
   - Maybe the simplest api is: 
   - ui_text_with_highlight(ui, text, highlight_range()) 
     - function deals with showing the highlight
     - no need for fancy strings or stuff

   - DONE? (needs work)
     - Lister
     - NewEvent/FocusEvent
     - Calendar

 - popups to use cell lister
   - Add a single popup in the UI state that "can" be the center
   - CONTINUE CLEANUP

## Fixes
 - Make nill ui node
   - Return nil ui node in ui_node_from_key to avoid checks
 - Color picker hsv
 - Fix inputs weird focus / event bubling
 - Font size not working
 - Real tree removal in Panel tree
 - If Profile -> Change All ui recursive functions to use a TreeIterator

## Tests  
 - Add simple status (OPEN, CLOSED), and some UI

 - Generic markdown-like file parsing and integration (like this one)
   - Also TODO(erb): things

 - Add first citizen schedule

 - Events
   - Event parenting?

 - Focus event panel (first real thing)
   - Date Input
     - reference other events dates
   - Smart input
     - Completion
       - reference other events
     - Output some kind of expression tree

 - Sorting & grouping
 - Searching everywhere

## App design ideas
 - Very ergonomic quick add
   - With some set defaults and quick to change
   - Maybe have many quick-add templates and differend binds for each

 - Extensibility/Customizablity
   - Rose Expr Aliases / functions -> allow user to specify simple table-like aliases to functions or identifiers
     - example1: "today" -> filter(events, now().date))
     - example2: "overdue" -> filter(events, dt_range(min_time(), now().date))
   - Rose Custom Commands / plugins -> allow user to link a dll (or package 'tcc' compiler) and add their own commands that will be treated as other commands in the app (with bindings and stuff)
     - Also enables the user to add much more complex Rose Expr Aliases - directly in c code??

 - IMPORTANT:
   - By far my bigest problem with other calendar apps is reorganization. If I have a week almost packed out and suddenly I decide to move something I was meant to do today to later on, say the end of the week, then I have to manually reorganize every other event a bit forward. Also, small changes of a schedule make it really anoying because I have to reorder a bunch of stuff. This should all be automatic or at least MUCH more intuitive to do.

 - Audio / Music sessions
   - Metronome music player / timer
   - Access a "playlist" and make focus sessions timed to albums or playlists

 - Default Every Day times (Work/Commute/Lunch/Dinner)
   - To estimate correctly how many hours in a day
   - Flexible settings like weekly, weekends or not
   - Sleep/Wake should only be "lines" since the "mark a single point" 
   - Semanticly "work day", meaning it can go over midnight for example, and still be considered the current "work day". 
   - A "work day" should be considered "from the time I wake up" to "the time I go to sleep". Also, there should be an option to "balance out days" or "preserve sleep/work/commute hourse"
     - Example: if I regularly I mark the end of my day at midnight and one day I set a task from 00:00 - 01:00, it is still in the same "work day", and this can or not "reschedule"/"move around" tasks in the next day, for example, I might wake up an hour later If I can, or work an hour less the next day, etc. This brings up a notion of "negotiable"/"mutable/immutable" events in the day. 
   - Notion of "non-negotiable" and "negotiable" events in the day.
     - For example: Usually I would like to wake up at 7:00, to have my morning routine during 7:00-8:00, then commute 8:00-9:00, and I have class 9:00. I cannot move class around, and I would like to (in a regular schedule) have my morning routine during 7:00-8:00, and it is "non-negotiable" that I finish my morning routine at 8:00 so I can commute 8:00-9:00. But if I work 30 minutes (or even more) the night before, I could then squish my morning routine from 7:30-8:00. This is possible because my morning routine is "squishable", and in days where at wake at 7:00 it actually goes: morning(7:00-7:45), commute(7:45-8:30), wait_for_class(8:30-9:00). Still, I shouldn't have to specify all of this as a user, I should only say that "If it is really necessary, squish my morning routine", and it should happend automatically

 - Insert Filter
   - Besides a regular view filter, which can or not auto-set properties when inserting a new thing (based on the filter)
   - Have just an insert filter
     - Inital properties could be the view filter, which can be activated or deactivated 

 - Postpone Planning / Semi Planned events
   - Make it possible to ONLY set a duration and a time interval (day/week/month) for an event and just leave it there.
   - This event can then be automatically "scheduled"
   - Event will discount day hours, showing a progress bar like in the day maybe?
   - It is sort of a "backlog" but with more scheduling work done to it
   - These should be most of "day events"

 - Smart duplicate / Event Reproductivty 
   - Have tagged options in each significant field, so when duplicating or creating many, some increment rules are applied
   - This could also be something simpler like batch rename or batch insert
   - For example: "Revision Tema 4 Hoy", could be "Revision Tema %T=4%, %D=Today%", so when I create another one, %T% and %D% can be incrementend beause of their semantic meaning, and this can become "Revision Tema %T=5%, %D=Tomorrow%"

 - Work Slots / Windows
   - As implied by the rest of the ideas
   - There are time windows where I plan to be productive in general (in my desk / library / office, doesn't really matter, but something of this kind). 
   - Maybe allow a category like "I wan't to work on university stuff" or "I wan't to work on my side job", etc. 
   - Tasks in the category (or without a filter) can and then they are slotted and "scheduled" (like OS job scheduling) in the work window. 
   - Also, when moving a task to a day, imply that its desired to complete in the work windows of that day, and there fore, it have a small button like "fit and reorganize work slot?" once the task is moved to a day. 
   - Tasks in work slot can be sorted by priority or time since planning or overdueness, or duration (try and minimize user input on the heuristic to sort, since its really annoying). The best heuristic is probably "task I least wan't to do"

 - Auto complete schedule events
   - Things like "class", "wake up", "flight" should be auto completed since they can't be moved or postponed.
   - These shouldn't really be considered tasks, maybe something more like "scheduled things" or "time events" or just "immutable events"

 - Productive app (in general)
   - Try minizime user input and noise always and for ever!
   - Use things like filepilot context menu combobox (shows favorite options and all options can be toggled / searched for)

## Low Level Layers / Specific features
 - Inteligent search 
   - Allow basic incremental search to have character aliaes. (like google docs)
     - For example: search with 'cao' should have 'ção' as a result. Where 'c->ç', 'a->ã' are aliases, but not the other way around.

 - Profile & Test
   - Profiler and in-frame profile with UI
     - Handle recursive functions
     - Emit to flame graph viewer
     - Render flame graph viewer in debug UI
   - Arena memory checker with UI
   - Profile & Memory leaks check 

 - Research SRGB

 - Bind textures according to GL_MAX_TEXTURE_IMAGE_UNITS

 - FNT Layer
   - BAD: You're scaling bitmap glyphs to arbitrary sizes.
   - BAD: Your text isn't pixel aligned.
   - BAD: Your glyphs have no atlas padding.
   - ?????Bake many font sizes in a single atlas????
   - Restrict api to 'em_per_pixel' and 'pixel_height' (is what a "loaded atlas means")

 - On resize draw / Minimize, maximize
   - Maybe this envolved changing then entry point system, as well as declaring a
     single frame draw function so this is easily called.

 - Move all globals to headers ??????

 - 'builder.c'
   - Some system to test different backends with #defines
   - Enable or not DEBUG macro in "debug" build
   - Make clang output full paths
   - Compile on GCC
   - Improve flags system