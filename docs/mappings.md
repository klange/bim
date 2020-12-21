## Normal

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<backspace>` | `cursor_left_with_wrap` | Move the cursor one position left, wrapping to the previous line. |
| `V` | `enter_line_selection` | Enter line selection mode. |
| `v` | `enter_char_selection` | Enter character selection mode. |
| `^V` | `enter_col_selection` | Enter column selection mode. |
| `O` | `prepend_and_insert` | Insert a new line before the current line and enter insert mode. |
| `o` | `append_and_insert` | Insert a new line after the current line and enter insert mode. |
| `a` | `insert_after_cursor` | Place the cursor after the selected character and enter insert mode. |
| `s` | `delete_forward_and_insert` | Delete the character under the cursor and enter insert mode. |
| `x` | `delete_forward` | Delete the character under the cursor. |
| `P` | `paste` | Paste yanked text before (`P`) or after (`p`) the cursor. |
| `p` | `paste` | Paste yanked text before (`P`) or after (`p`) the cursor. |
| `r` | `replace_char` | Replace a single character. |
| `A` | `insert_at_end` | Move the cursor to the end of the current line and enter insert mode. |
| `u` | `undo_history` | Undo history until the last breakpoint. |
| `^R` | `redo_history` | Redo history until the next breakpoint. |
| `^L` | `redraw_all` | Repaint the screen. |
| `^G` | `goto_definition` | Jump to the definition of the word under under cursor. |
| `i` | `enter_insert` | Enter insert mode. |
| `R` | `enter_replace` | Enter replace mode. |
| `<shift-up>` | `enter_line_selection_and_cursor_up` | Enter line selection and move the cursor up one line. |
| `<shift-down>` | `enter_line_selection_and_cursor_down` | Enter line selection and move the cursor down one line. |
| `<alt-up>` | `previous_tab` | Switch the previous tab |
| `<alt-down>` | `next_tab` | Switch to the next tab |

## Insert

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `leave_insert` | Leave insert modes and return to normal mode. |
| `<del>` | `delete_forward` | Delete the character under the cursor. |
| `^C` | `leave_insert` | Leave insert modes and return to normal mode. |
| `<backspace>` | `smart_backspace` | Delete the preceding character, with special handling for indentation. |
| `<enter>` | `insert_line_feed` | Insert a line break, splitting the current line into two. |
| `^O` | `perform_omni_completion` | (temporary) Perform smart symbol completion from ctags. |
| `^V` | `insert_char` | Insert one character. |
| `^W` | `delete_word` | Delete the previous word. |
| `<tab>` | `smart_tab` | Insert a tab or spaces depending on indent mode. (Use ^V <tab> to guarantee a literal tab) |
| `/` | `smart_comment_end` | Insert a `/` ending a C-style comment. |
| `}` | `smart_brace_end` | Insert a closing brace and smartly position it if it is the first character on a line. |
| `<paste-begin>` | `paste_begin` | Begin bracketed paste; disable indentation, completion, etc. |
| `<paste-end>` | `paste_end` | End bracketed paste; restore indentation, completion, etc. |

## Replace

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `leave_insert` | Leave insert modes and return to normal mode. |
| `<del>` | `delete_forward` | Delete the character under the cursor. |
| `<backspace>` | `cursor_left_with_wrap` | Move the cursor one position left, wrapping to the previous line. |
| `<enter>` | `insert_line_feed` | Insert a line break, splitting the current line into two. |

## Line Selection

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `leave_selection` | Leave selection modes and return to normal mode. |
| `^C` | `leave_selection` | Leave selection modes and return to normal mode. |
| `V` | `leave_selection` | Leave selection modes and return to normal mode. |
| `v` | `switch_selection_mode` | Swap between LINE and CHAR selection modes. |
| `y` | `yank_lines` | Copy lines into the paste buffer. |
| `<backspace>` | `cursor_left_with_wrap` | Move the cursor one position left, wrapping to the previous line. |
| `<tab>` | `adjust_indent` | Adjust the indentation on the selected lines (`<tab>` for deeper, `<shift-tab>` for shallower). |
| `<shift-tab>` | `adjust_indent` | Adjust the indentation on the selected lines (`<tab>` for deeper, `<shift-tab>` for shallower). |
| `D` | `delete_and_yank_lines` | Delete and yank the selected lines. |
| `d` | `delete_and_yank_lines` | Delete and yank the selected lines. |
| `x` | `delete_and_yank_lines` | Delete and yank the selected lines. |
| `s` | `delete_lines_and_enter_insert` | Delete and yank the selected lines and then enter insert mode. |
| `r` | `replace_chars_in_line` | Replace characters in the selected lines. |
| `<shift-up>` | `cursor_up` | Move the cursor up one line. |
| `<shift-down>` | `cursor_down` | Move the cursor one line down. |

## Char Selection

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `leave_selection` | Leave selection modes and return to normal mode. |
| `^C` | `leave_selection` | Leave selection modes and return to normal mode. |
| `v` | `leave_selection` | Leave selection modes and return to normal mode. |
| `V` | `switch_selection_mode` | Swap between LINE and CHAR selection modes. |
| `y` | `yank_characters` | Yank the selected characters to the paste buffer. |
| `<backspace>` | `cursor_left_with_wrap` | Move the cursor one position left, wrapping to the previous line. |
| `D` | `delete_and_yank_chars` | Delete and yank the selected characters. |
| `d` | `delete_and_yank_chars` | Delete and yank the selected characters. |
| `x` | `delete_and_yank_chars` | Delete and yank the selected characters. |
| `s` | `delete_chars_and_enter_insert` | Delete and yank the selected characters and then enter insert mode. |
| `r` | `replace_chars` | Replace the selected characters. |
| `A` | `insert_at_end_of_selection` | Move the cursor to the end of the selection and enter insert mode. |

## Col Selection

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `leave_selection` | Leave selection modes and return to normal mode. |
| `^C` | `leave_selection` | Leave selection modes and return to normal mode. |
| `^V` | `leave_selection` | Leave selection modes and return to normal mode. |
| `I` | `enter_col_insert` | Enter column insert mode. |
| `a` | `enter_col_insert_after` | Enter column insert mode after the selected column. |
| `d` | `delete_at_column` | Delete from the current column backwards (`<backspace>`) or forwards (`<del>`). |

## Col Insert

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `leave_selection` | Leave selection modes and return to normal mode. |
| `^C` | `leave_selection` | Leave selection modes and return to normal mode. |
| `<backspace>` | `delete_at_column` | Delete from the current column backwards (`<backspace>`) or forwards (`<del>`). |
| `<del>` | `delete_at_column` | Delete from the current column backwards (`<backspace>`) or forwards (`<del>`). |
| `<enter>` | `(unbound)` | (unbound) |
| `^W` | `(unbound)` | (unbound) |
| `^V` | `insert_char_at_column` | Insert a character on all lines at the current column. |
| `<left>` | `column_left` | Move the column cursor left. |
| `<right>` | `column_right` | Move the column cursor right. |

## Navigation (Select)

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `^B` | `go_page_up` | Jump up a screenfull. |
| `^F` | `go_page_down` | Jump down a screenfull. |
| `:` | `enter_command` | Enter command input mode. |
| `/` | `enter_search` | Enter search mode. |
| `?` | `enter_search` | Enter search mode. |
| `n` | `search_next` | Jump to the next search match. |
| `N` | `search_prev` | Jump to the preceding search match. |
| `j` | `cursor_down` | Move the cursor one line down. |
| `k` | `cursor_up` | Move the cursor up one line. |
| `h` | `cursor_left` | Move the cursor one character to the left. |
| `l` | `cursor_right` | Move the cursor one character to the right. |
| `b` | `word_left` | Move the cursor left to the previous word. |
| `w` | `word_right` | Move the cursor right to the start of the next word. |
| `B` | `big_word_left` | Move the cursor left to the previous WORD. |
| `W` | `big_word_right` | Move the cursor right to the start of the next WORD. |
| `<` | `shift_horizontally` | Shift the current line or screen view horizontally, depending on settings. |
| `>` | `shift_horizontally` | Shift the current line or screen view horizontally, depending on settings. |
| `f` | `find_character_forward` | Find a character forward on the current line and place the cursor on (`f`) or before (`t`) it. |
| `F` | `find_character_backward` | Find a character backward on the current line and place the cursor on (`F`) or after (`T`) it. |
| `t` | `find_character_forward` | Find a character forward on the current line and place the cursor on (`f`) or before (`t`) it. |
| `T` | `find_character_backward` | Find a character backward on the current line and place the cursor on (`F`) or after (`T`) it. |
| `G` | `goto_line` | Jump to the requested line. |
| `*` | `search_under_cursor` | Search for the word currently under the cursor. |
| `<space>` | `go_page_down` | Jump down a screenfull. |
| `%` | `jump_to_matching_bracket` | Find and jump to the matching bracket for the character under the cursor. |
| `{` | `jump_to_previous_blank` | Jump to the preceding blank line before the cursor. |
| `}` | `jump_to_next_blank` | Jump to the next blank line after the cursor. |
| `$` | `cursor_end` | Move the cursor to the end of the line, or past the end in insert mode. |
| `<pipe>` | `cursor_home` | Move the cursor to the beginning of the line. |
| `<enter>` | `next_line_non_whitespace` | Jump to the first non-whitespace character in the next next line. |
| `^` | `first_non_whitespace` | Jump to the first non-whitespace character in the current line. |
| `0` | `cursor_home` | Move the cursor to the beginning of the line. |

## Escape (Select, Insert)

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<f1>` | `toggle_numbers` | Toggle the display of line numbers. |
| `<f2>` | `toggle_indent` | Toggle smart indentation. |
| `<f3>` | `toggle_gutter` | Toggle the display of the revision status gutter. |
| `<f4>` | `toggle_smartcomplete` | Toggle smart completion. |
| `<mouse>` | `handle_mouse` | Process mouse actions. |
| `<mouse-sgr>` | `handle_mouse_sgr` | Process SGR-style mouse actions. |
| `<up>` | `cursor_up` | Move the cursor up one line. |
| `<down>` | `cursor_down` | Move the cursor one line down. |
| `<right>` | `cursor_right` | Move the cursor one character to the right. |
| `<ctrl-right>` | `big_word_right` | Move the cursor right to the start of the next WORD. |
| `<shift-right>` | `word_right` | Move the cursor right to the start of the next word. |
| `<alt-right>` | `expand_split_right` | Move the view split divider to the right. |
| `<alt-shift-right>` | `use_right_buffer` | Switch to the right split view. |
| `<left>` | `cursor_left` | Move the cursor one character to the left. |
| `<ctrl-left>` | `big_word_left` | Move the cursor left to the previous WORD. |
| `<shift-left>` | `word_left` | Move the cursor left to the previous word. |
| `<alt-left>` | `expand_split_left` | Move the view split divider to the left. |
| `<alt-shift-left>` | `use_left_buffer` | Switch to the left split view. |
| `<home>` | `cursor_home` | Move the cursor to the beginning of the line. |
| `<end>` | `cursor_end` | Move the cursor to the end of the line, or past the end in insert mode. |
| `<page-up>` | `go_page_up` | Jump up a screenfull. |
| `<page-down>` | `go_page_down` | Jump down a screenfull. |
| `^Z` | `suspend` | Suspend bim and the rest of the job it was run in. |

## Command

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<enter>` | `command_accept` | Accept the command input and run the requested command. |
| `<tab>` | `command_tab_complete_buffer` | Complete command names and arguments in the input buffer. |
| `<up>` | `command_scroll_history` | Scroll through command input history. |
| `<down>` | `command_scroll_history` | Scroll through command input history. |

## Search

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<enter>` | `search_accept` | Accept the search term and return to the previous mode. |
| `<up>` | `(unbound)` | (unbound) |
| `<down>` | `(unbound)` | (unbound) |

## Input (Command, Search)

| **Key** | **Action** | **Description** |
|---------|------------|-----------------|
| `<escape>` | `command_discard` | Discard the input buffer and cancel command or search. |
| `^C` | `command_discard` | Discard the input buffer and cancel command or search. |
| `<backspace>` | `command_backspace` | Erase the character before the cursor in the input buffer. |
| `^W` | `command_word_delete` | Delete the previous word from the input buffer. |
| `<mouse>` | `eat_mouse` | (temporary) Read, but ignore mouse input. |
| `<left>` | `command_cursor_left` | Move the cursor one character left in the input buffer. |
| `<ctrl-left>` | `command_word_left` | Move to the start of the previous word in the input buffer. |
| `<right>` | `command_cursor_right` | Move the cursor one character right in the input buffer. |
| `<ctrl-right>` | `command_word_right` | Move to the start of the next word in the input buffer. |
| `<home>` | `command_cursor_home` | Move the cursor to the start of the input buffer. |
| `<end>` | `command_cursor_end` | Move the cursor to the end of the input buffer. |

