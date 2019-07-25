
| **Keybind** | **Vi**                | **Bim**                   |
|-------------|-----------------------|---------------------------|
| `a`         | enter INSERT after    | same                      |
| `b`         | back word             | same                      |
| `c`         | change command        | *unbound*                 |
| `d`         | delete command        | *only with selection*     |
| `f`         | find character        | same                      |
| `g`         | *unbound*             | *unbound*                 |
| `h`         | move left             | same                      |
| `i`         | enter INSERT before   | same                      |
| `j`         | move down             | same                      |
| `k`         | move up               | same                      |
| `l`         | move right            | same                      |
| `m`         | mark line             | *unbound*                 |
| `n`         | repeat last search    | same, *but always searches forward* |
| `o`         | insert on new line    | same                      |
| `p`         | put buffer after      | same                      |
| `q`         | *unbound*             | *unbound*                 |
| `r`         | replace one character | same                      |
| `s`         | delete and insert     | same                      |
| `t`         | `f` but move before   | same                      |
| `u`         | undo                  | same                      |
| `v`         | *unbound*             | select characters         |
| `w`         | forward one word      | same                      |
| `x`         | delete one character  | same                      |
| `y`         | yank                  | *only with selection*     |
| `z`         | position current line | *unbound*                 |
| `A`         | insert at end         | same                      |
| `B`         | back one WORD         | same                      |
| `C`         | change to end of line | *unbound*                 |
| `D`         | delete to end of line | *unbound*                 |
| `E`         | move to end of word   | *unbound*                 |
| `F`         | backwards `f`         | same                      |
| `G`         | go to line or eof     | end of file only          |
| `H`         | first line on screen  | *unbound*                 |
| `I`         | insert before first space | *unbound*             |
| `J`         | join line with next   | *unbound*                 |
| `K`         | *unbound*             | *unbound*                 |
| `L`         | last line on screen   | *unbound*                 |
| `M`         | middle of screen      | *unbound*                 |
| `N`         | repeat search backwards | same, *but always searches backwards* |
| `O`         | insert on new line above | same                   |
| `P`         | put buffer before     | same                      |
| `Q`         | leave visual          | *unbound*                 |
| `R`         | replace mode          | same                      |
| `S`         | delete line, enter insert | *unbound*             |
| `T`         | `t` but backwards     | same                      |
| `U`         | restore cursor        | *unbound*                 |
| `V`         | *unbound*             | select lines              |
| `Y`         | yank line             | *unbound*                 |
| `Z`         | part of save-and-exit | *unbound*                 |
| `0`         | start of line         | same                      |
| `1`-`9`     | enter line prefix     | *unbound*                 |
| (space)     | move right            | jump forward several lines |
| `!`         | shell command         | *needs command mode*      |
| `@`         | eval                  | *unbound*                 |
| `#`         | *unbound*             | *unbound*                 |
| `$`         | end of line           | same                      |
| `%`         | jump to closest brace on line, or its match | *only jump to matching brace* |
| `^`         | first non whitespace  | same                      |
| `&`         | repeat last substitution | *unbound*              |
| `*`         | *unbound*             | search for word under cursor |
| `(`         | previous sentence     | *unbound*                 |
| `)`         | next sentence         | *unbound*                 |
| `\`         | *unbound*             | *unbound*                 |
| pipe        | start of line         | same                      |
| `-`         | first non-whitespace of previous line | *unbound* |
| `_`         | same as `^`?          | *unbound*                 |
| `=`         | *unbound*             | *unbound*                 |
| `+`         | first non-whitespace of next line | *unbound*     |
| `[`         | previous `{}`         | *unbound; also does something else in vim* |
| `]`         | next `{}`             | *unbound; also does something else in vim* |
| `{`         | previous blank line   | same                      |
| `}`         | next blank line       | same                      |
| `;`         | repeat last `fFtT`    | *unbound*                 |
| `'`         | move to first non-whitespace on marked line | *unbound* |
| backtick    | move to memorized column on marked line | *unbound* |
| `:`         | command mode          | same                      |
| `"`         | numbered buffer       | *unbound; not sure what this does in vim* |
| `~`         | swap case, move forward | *unbound*               |
| `,`         | reverse last `fFtT`?  | *unbound*                 |
| `.`         | repeat change         | *unbound*                 |
| `/`         | search forward        | same                      |
| `<`         | unindent              | *unbound, could implement; use shift-tab with line selection* |
| `>`         | indent                | *unbound, could implement; use tab with line selection* |
| `?`         | search backwards      | same                      |
| `^A`        | *unbound*             | *unbound*                 |
| `^B`        | up one screen         | same                      |
| `^C`        | *unbound*             | *unbound*, exits insert modes |
| `^D`        | down half a screen    | *unbound, might implement* |
| `^E`        | scroll up             | *unbound; vim doesn't do this?* |
| `^F`        | down on screen        | same                      |
| `^G`        | show status           | *unbound, redundant; status always shown* |
| `^H`        | backspace             | same, *only deletes in insert* |
| `^I` (tab)  | *unbound*             | *unbound, but see other modes* |
| `^J`        | line down             | *unbound; is that not the same as regular j?* |
| `^K`        | *unbound*             | *unbound; vim deletes to end* |
| `^L`        | refresh screen        | same                      |
| `^M` (cr)   | first non-whitespace of next line | same          |
| `^N`        | down one line         | *unbound*                 |
| `^O`        | *unbound*             | *unbound*                 |
| `^Q`        | XON                   | *unbound*                 |
| `^P`        | up one line           | *unbound*                 |
| `^R`        | *unbound*             | redo from history         |
| `^S`        | XOFF                  | *unbound*                 |
| `^T`        | go to last tag jump   | *unbound*                 |
| `^V`        | *unbound*             | *unbound*                 |
| `^W`        | *unbound*             | *unbound*                 |
| `^X`        | *unbound*             | *unbound*                 |
| `^Y`        | scroll down           | *unbound*                 |
| `^[`        | cancel                | same                      |
| `^\`        | leave visual          | *unbound*                 |
| `^]`        | lookup tags (go to definition) | *unbound, try `^O` in insert* |
| `^^`        | switch file buffers   | *unbound*                 |
| `^_`        | *unbound*             | *unbound*                 |
| `^?` (del)  | *unbound*             | *unbound; vim deletes and enters insert?* |
