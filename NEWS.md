Version 0.6.3
 - fix: should use c++ syntax to check ".h" files
 - fix: can't copy contents in a readonly editor
 - fix: project's file not correctly syntaxed when open in editor
 - libturtle update: add fill() / setBackgroundColor() /setBackgroundImage() functions
 - fix: code fold calculation not correct, when editing code
 - fix: can't correctly find definition of the symbols in namespace
    
Version 0.6.2 
 - fix: The Enter key in the numpad doesn't work
 - fix: The compiled executable not fully write to the disk before run it
 - fix: settings object not correctly released when exit
 - fix: shouldn't check syntax when save modifications before compiling
 - fix: shouldn't scroll to the end of the last line when update compile logs
 - fix: can't debug project

Version 0.6.1
 - fix: editor deadlock

Version 0.6.0
 - fix: old data not displayed when editing code snippets
 - fix: shift-tab for unindent not work
 - fix: can't save code snippets modifications
 - fix: errors in code snippet processing
 - change: auto open a new editor at start
 - enhancement: todo view
 - add: about dialog
 - implement: correctly recognize clang (msys2 build)
 - enhancement: don't add encoding options when using clang to compile (clang only support utf-8)
 - enhancement: find occurence in project
 - implement: rename symbol in file
 - enhancement: replace in files
 - enhancement: rename symbol in project (using search symbol occurence and replace in files)
 - fix: search in files
 - implement: register file associations
 - implement: when startup , open file provided by command line options
 - implement: open files pasted by clipboard
 - fix: code fold parsing not correct
 - enhancement: support #include_next (and clang libc++)
 - fix:  hide popup windows when the editor is closed
 - enhancement: show pinyin when input chinese characters
 - fix: add mutex lock to prevent rare conditions when editor is modifying and the content is read
 - fix: makefile generated for static / dynamic library projects not right
 - fix: editors disappeared when close/close all
 - implement: config shortcuts
 - implement: handle windows logout message
 - fix: editor's inproject property not correctly setted (and may cause devcpp to crash when close project)
 - implement: print
 - implement: tools configuration
 - implement: default settings for code formatter
 - implement: remove all custom settings

Version 0.5.0
 - enhancement: support C++ using type alias;
 - fix: when press shift, completion popu window will hide
 - enhancement: options in debugger setting widget, to skip system/project/custom header&project files when step into
 - fix: icon not correctly displayed for global variables in the class browser 
 - enhancement: more charset selection in the edit menu
 - fix: can't correctly get system default encoding name when save file
 - fix: Tokenizer can't correctly handle array parameters
 - fix: debug actions enabled states not correct updated when processing debug mouse tooltips
 - enhancement: redesign charset selection in the project options dialog's file widget
 - fix: can't correctly load last open files / project with non-asii characters in path
 - fix: can't coorectly load last open project
 - fix: can't coorectly show code completion for array elements
 - enhancement: show caret when show code/header completions
 - fix: correctly display pointer info in watch console
 - implement: search in project
 - enhancement: view memory when debugging
 - implement: symbol usage count
 - implement: user code snippet / template
 - implement: auto generate javadoc-style docstring for functions
 - enhancement: use up/down key to navigate function parameter tooltip
 - enhancement: press esc to close function parameter tooltip
 - enhancement: code suggestion for unicode identifiers
 - implement: context menu for debug console
 - fix: errors in debug console
 - fix: speed up the parsing process of debugger
 - ehancement: check if debugger path contains non-ascii characters (this will prevent it from work

Version 0.2.1
 - fix: crash when load last opens

Version 0.2
 - fix : header file completion stop work when input '.'
 - change: continue to run / debug if there are compiling warnings (but no errors)
 - enhancement: auto load last open files at start
 - enhancement: class browser syntax colors and icons
 - enhancement: function tips
 - enhancement: project support
 - enhancement: paint color editor use system palette's disabled group color
 - fix: add watch not work when there's no editor openned;
 - enhancement: rainbow parenthesis
 - enhancement: run executable with parameters
 - add: widget for function tips
 - enhancement: options for editor tooltips
 - fix: editor folder process error