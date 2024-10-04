Red Panda C++ Version 3.2

  - change: The way to calcuate astyle path.
  - fix: Scroll bar arrow size not correct in the dark themes.
  - fix: Don't auto scroll to the caret after undo/redo.
  - fix: "bits/stdc++" is not openned in readonly mode.
  - fix: astyle path error when reformat.
  - fix: Slow when paste/replace bulk contents.
  - fix: Crash in windows 7. (by CyanoHao)
  - fix: While Control is pressed, can't start Drag&Drop by mouse.
  - enhancement: Auto detect gdb ACP (by CyanoHao)
  - fix:  When debugging project, the executing source file is not auto switched to. （#476）
  - enhancement: Support Raw string literal with LR/UR/uR/u8R prefix.
  - change: Copy/Export as html using less restrictive header.
  - enhancement: Better gcc info detection (by CyanoHao)
  - enhancement: Copy/Export as html with line numbers.


Red Panda C++ Version 3.1

  - fix: Can't correctly select in column mode.
  - fix: Can't correctly parse template parameters that contains "->", like "std::queue<std::function<auto()->void>>";
  - fix: Shouldn't warn "xxx.s is modifed" when "Generate assembly" is rerun.
  - fix: Shouldn't warn "Makefile is modifed" when "View Makefile" is rerun.
  - fix: In compiler options page, Can't save default stack size to 0MB.
  - enhancement: Support national flag emojis.
  - fix: Visibility for the interrupt action is not correctly updated.
  - enhancement: Handle problems info from competitive-companion in background thread.
  - enhancement: Handle time/memory limits in problems info from competitive-companion in background thread.
  - enhancement: When problems info from competitive-companion received, show tips in the status bar.  
  - fix: Layout for function tips.
  - enhancement: More elements in the demo of editor color theme optiont page.
  - fix: Mingw32-make doesn't work correctly if there are bash in the path.
  - fix: All color scheme names are incorrectly displayed as bold, if the current one is a customed one.
  - fix: Variables defined by using alias can't show completion info.
  - enhancement: Support operator() overload.
  - change: rename all "ansi" encoding to "system default".

Red Panda C++ Version 3.0

  - enhancement: New chinese translation for invalid filename messagebox. (by XY0797@github.com)
  - enhancement: Limit the minimum font size in options dialog to 5. (by XY0797@github.com)
  - enhancement: After a new file is created in filesystem panel, auto select and rename it. (by XY0797@github.com)
  - enhancement: Select file basename when rename in the filesystem panel. (by XY0797@github.com)
  - change: Don't use "Microsoft Yahei" as the default non-ascii font in non-chinese environment.
  - enhancement: Support unicode characters > 0xFFFF
  - enhancement: Support unicode ZWJ and ZWNJ.
  - enhancement: Support unicode combining characters.
  - enhancement: Don't force fixed-width when using non fixed-width fonts.
  - change: Replace non-ascii font with fallback font.
  - enhancement: Display ascii control chars.
  - fix: Parser: invalidating file may lost class inheritance infos.
  - fix: Function argument infos are not correctly parsed.
  - enhancement: Migrate external calls from command string to argv array to improve safety and security.
  - enhancement: Support POSIX shell-like escaping in user inputs for compiler arguments.
  - fix: (Hopefully) properly escape filenames and arguments in makefile generation.
  - enhancement: Beautify display for spaces and linebreaks.
  - fix: Insert line after comments may auto add an extra '*'.
  - fix: Can't show function tips for std::ios::sync_with_stdio.
  - fix: Wrong indent for the line after the pasted context.
  - Enhancement: When '{' is inputted and there are contents selected, auto add line breaks and indents.
  - fix: Selected lines doesn't draw line break glyphs.
  - fix: issue #215 (Caret may be drawn in the gutter.)
  - change: Force use utf8 as the exec encoding for fmtlib in the auto link options page.
  - fix: After spaces in comments and strings, symbol completion for '{' and '(' are wrong.
  - fix: Issue #230 Crash when input " in the txt files.
  - enhancement: Unique look&feel for the underline shown while ctrl+mouse over #include line.
  - enhancement: Better look&feel for the wave underline shown for syntax errors.
  - fix: "float" in #include "float.h" is wrong syntax colored.
  - enhancement: Unify syntax color for #include header name
  - enhancement: Issue #229 Press Enter/Return in the tree view in files panel will open the file.
  - enhancement: Internal optimization for loading/editing files.
  - enhancement: Show space glyphs in C/C++ char literals.
  - enhancement: Optimization for string/raw string/char literal status check while completing symbols in c/c++ files.
  - enhancement: Windows installer Hi-DPI support.
  - fix: Delete/Insert in column editing mode.
  - enhancement: Issue #196 Support C++ using alias in  syntax highlighting/code completion/function tips.
  - enhancement: Support annonymous class
  - fix: Using alias for global symbols are not correctly handled.
  - enhancement: Support "enum struct" Scoped enumerations.
  - fix: Function tips contains functions that not in the scope.
  - fix: Hint for bold text (<b></b>) are not correctly handled in the function tips.
  - enhancement: Improve lldb-mi compatibility.
  - fix: Failed to evaluate expressions while debugging, if the expression has spaces in it.
  - fix: When debugging, can't watch expressions that has spaces in it.
  - enhancement: Font list in the options / editor / font panel( by CyanoHao  ). 
  - enhancement: Text are vertically center aligned in lines( by CyanoHao  ).
  - fix: In the debugger console, Auto-wrapped lines  can't be correctly selected.
  - enhancement: Auto choose a better font for theme choosing dialog in the first run.
  - fix: Debugger console's background not correctly cleared before redrawn.
  - enhancement: Make output in the debug console cleaner.
  - enhancement: Execute the last debug command in the debug console if ENTER pressed.
  - change: When debugging, don't auto set focus to the editor.
  - enhancement: Folding button scales with editor font.
  - fix: Shouldn't show header completion popup in #include line comments.
  - change: Invert scroll direction in horizontal, like in vertical.
  - fix: Caret unseen when move to a long line end by press END.
  - fix: No icons for inherited class private members.
  - fix: Ctrl+Return insert linebreak shouldn't scroll unnecessarilly.
  - enhancement: Move caret to line begin would scroll to the begin if possible.
  - fix: Filename in tables in the debug panel are not correctly eroded.
  - enhancement: Tooltip info for the stacktrace table in the debug panel.
  - fix: '*=' is treadted as '*' when parsing. 
  - fix: Can't correctly retrieve function parameters type.
  - fix: Auto type induction for expression contains '[]' are not correct.
  - fix: Option 'Pause after run in console' for tools doesn't work.
  - fix: Filename that contains '&' doesn't correctly displayed in the editor tab.
  - enhancement: Type induction for "auto &&" vars.
  - enhancement: Syntax highlighting for c++ attributes.
  - enhancement: Show "std::function" in the completion list.
  - enhancement: Improvement in italic font support.
  - fix: History not correctly loaded with up/down arrow key in the debug console.
  - enhancement: Improve lambda expression support.
  - enhancement: Show type completion hint after "constexpr"/"extern"/"static"/"consteval"/"constinit"/"const"/"volatile"/"inline" etc.
  - enhancement: Restore line position after file is modified outside and reloaded.
  - fix: Caret on '('/',' in string/comment shouldn't invoke function info tips.
  - fix: Function name not correctly found if it and the '(' is not in one line;
  - fix: Register names in the cpu info are not in correct order.
  - enhancement: Auto type induction for new / temp class object.
  - enhancement: Vertically scroll by pixel.
  - enhancement: Display (gdb) prompt in debug console after it's cleared.
  - fix: Output of "disas" is not shown in debug console.
  - fix: Display not correctly updated after select all in debug console.
  - change: Set focus to "find next" button when find/replace dialog is openned.
  - change: Don't set focus to "close" button after searched in the find/replace dialog
  - change: Set focus to "find" button when "find in files..." dialog is openned.
  - enhancement: Correct tab orders for all setting pages/dialogs.
  - enhancement: Shortcut key for buttons in find/replace and "find in files" dialogs.  
  - enhancement: Auto define macro "_DEBUG" for "Debug" compiler set(like visual studio).
  - enhancement: Suggest macro names after "#ifdef"/"#ifndef"/"#undef".
  - enhancement: If contents from stderr are logged into "Tools Output" panel, add problem case name info to the log. 
  - fix: In split screen mode, editor on the right can't be correctly found by commands.
  - fix: Remove duplicated macro defines make it's lost in the parse result.
  - fix: An undefined macro is still missing the the parse result after #undef is removed.
  - fix: If a class method is overloaded, only one of them is inherited by it's children.
  - enhancement: Adjust function tip pos to prevent it from run outside the right window edge.
  - enhancement: Open ".def" (Module definition file) file in editor when double click it in the project view.
  - enhancement: When a dll project has .def file, use it when generating the dll file.  
  - fix: "project name".exe.manifest is auto removed when build the project.
  - fix: "0x3.12p+1" is treadted as a plus expression when reformatting code. ( by 绣球135@qq ）
  - change: Don't turn on the code format option "indent class" by default.
  - enhancement: Add compiler set by choose the executable.
  - fix: Compile info for project doesn't have name of the project executable.
  - enhancement: Highlight words in the string/comments.
  - fix: If there are only 1 line in the editor, shift+down can't select it.
  - enhancement: By default, use monospaced font to display register values in the CPU Info dialog.
  - fix: Negative values in register like AH/AL are wrongs displayed as 32/64-bit number.
  - Change: Change background color for highlighted buttons in the default theme.
  - enhancement: Make colors in code suggestion popup consistent with the editor.
  - enhancement: Make colors in header suggestion popup consistent with the editor.
  - fix: C++ source after ';' are treated as comments in cpu info window.
  - enhancement: Support "extern template" in code parser.
  - enhancement: Set shortcuts for tools menu item.
  - enhancement: Enhancement for custom tools.
  - fix: Can't correctly undo/redo "Delete current line".
  - fix: Breakpoint condition expression that contains spaces doesn't work.
  - enhancement: Double click on breakpoint table's condition cell to modify it.
  - fix: Don't show function prototype tip for function name that contains namespace alias.
  - fix: Can't save changes in project options -> compiler set , after base compiler set was changed.
  - fix: Project options -> file doesn't work.
  - fix: Don't show function prototype tip for function name that contains more than one namespace;
  - fix: Compiler set options "Check for stack smashing attacks (-fstack-protector)" was not correctly applied when compiling.
  - fix: can't jump to definition/declaration for symbols in using alias statement like "using ::printf".
  - fix: Don't show completion suggestion for members of variable which type name has namespace alias;
  - fix: Theme manager not correctly inited in options dialog / environment / appearance.  
  - enhancement: Size of icons in the completion popup changes with the editor font size.
  - change: Completion popup size settings are based on editor's char width/line height.
  - change: Remove "limit for copy" and "limit for undo" options.
  - fix: Can't find the correct type if current symbol is member of a class that has constructors.
  - fix: Alias a namespace to itself will create infinite loop.
  - fix: Can't find symbols indirectly included by other files.
  - enhancement: Function tip's width changes with editor width.
  - fix: '<' / '>' not shown in function tips.
  - enhancement: In debug console, Ctrl+C/Ctrl+X/Ctrl+V conflicts with application action.
  - enhancement: Auto hide Edit/Selection/Code/Refactor menu if no file openning.
  - enhancement: Auto hide Project menu if no project openning.
  - fix: Toggle breakpoint by shortcut may use wrong line.
  - fix: Size of the icons in problem and problem set panel are not correct.
  - fix: Shouldn't consider preceeding '&'/'*' when popping completion suggest list for variable members.
  - fix: Positions of current matching parenthesis not correctly updated.
  - fix: Can't show correct completion info for vars declared with template parameters ending with ">>".
  - enhancement: Auto type induction for "std::make_shared"/"std::make_unique".
  - enhancement: sdcc project compiler: compile source file in subfolders.
  - fix: project options -> compiler set -> static link & auto convert charset options not correctly loaded.
  - change: Don't generate project resource files for sdcc project.
  - fix: Name of the macro for project private resource header is not correct.
  - fix: In sdcc project, sdcc keywords are not in completion suggest list.
  - fix: In sdcc project, parser are not correctly inited as sdcc parser.
  - fix: Temp object + member function call is wrongly parsed as constructor.
  - enhancement: Improve how to manage themes in Options → general → appearance.
  - change: Use official astyle program.
  - enhancement: New code format option: "Remove superfluous empty lines exceeding"
  - enhancement: New code format option: "Remove superfluous spaces"
  - change: Remove code format option: "Delete continuous empty lines"
  - fix: Current editor wouldn't get parsed, when it's switched from another editor being parsed.
  - enhancement: Support macro in #include preprocessing statements.
  - fix: In options -> code format -> Program, Choose astyle path button doesn't work.
  - fix: project not correctly reparsed after rename unit.
  - enhancement: support C++ 17 structured binding in stl map containers foreach loop.
  - fix: Crash when has source line like "std::cout << (3+4*4>5*(4+3)-1 && (4-3>5)) <<std::endl;".
  - fix: The memory usage displayed after program execution is wrong.
  - enhancement: New compiler option "stack size" in the link subpage.
  - change: Set "Ctrl+G" as the shortcut for "Goto line..."
  - change: Set "Ctrl+B" as the shortcut for "Toggle Bookmark"
  - fix: Fail to evaluate expressions if macro can't be expanded.
  - enhancement: New menu item "Code completion" in "Code" menu.
  - fix: Can't compile / run assembly files in gcc 13/14 .
  - enhancement: Show full filepath in the tooltip of editor tab.
    
Red Panda C++ Version 2.26

  - enhancement: Code suggestion for embedded std::vectors.
  - change: Use ctrl+mouseMove event to highlight jumpable symbols (instead of ctrl+tooltip).
  - enhancement: Auto adjust position of the suggestion popup window.
  - enhancement: Windows XP support ( by cyano.CN  )
  - fix: __attribute__ is not correctly handled if it is after 'static'.
  - enhancement: Parse files that contains C++ 20 'concept' keyword. (No code suggesion for concepts now)
  - enhancement: Parse files that contains C++ 20 'requires' keyword.
  - fix: Code suggestions in namespace.
  - enhancement: Code suggestions for namespace alias.
  - fix: Correctly handle statements like 'using xxx::operator()'.
  - fix: Link in the project options dialog / precompiled header pages is not clickable.
  - change: Don't change caret position when ctrl+click.
  - fix: Should cd to working directory when debugging.
  - change: Ensure the line just below caret is visible while moving caret.
  - change: Set mouse cursor to hand pointing when it's on gutter.
  - enhancement: Basic support for parsing variadic macros(macros that use __VA_ARGS__).
  - enhancement: Better support for expanding macros with complex parameters.
  - fix: Macros that defined by the compiler are not correctly syntax-colored and tooltiped.
  - fix: Code suggestion for identifiers after '*' (eg. 3 * item->price) can't correct.
  - fix: C++ compiler atrribute '[[xxx]]' are not correctly handled.
  - fix: If the integrated gcc compiler is add to path, auto find compilers will find in twice. (Windows)
  - enhancement: When induce type info for return value, try to select the overloaded one that doesn't have an "auto" type.
  - enhancement: Hide symbols that contains "<>" in code suggestions.
  - enhancement: Slightly reduce memory usage.
  - change: In Options -> Language -> Generate Assembly, option "Don't generate SEH directives" default to True.
  - change: In Options —> Editor -> Code Suggestion, option "Hide symbols starting with underscore" default to True.
  - fix: Crash if include a non-exist header file in the source.
  - fix: Line numbers for problem case input/output/expected texteditors are not vertically centered.
  - enhancement: E-ink color scheme.
  - fix: Use the system default encoding for input when running problem cases.
  - change: Use qt.conf to use freetype font engine. User can use the windows default font engine by remove this file.
  - fix: Click on the line begin may toggle breakpoint.
  - change: Don't auto add; when completing '{' for lines starting with 'struct/union/enum' and ending with ')'
  - Enhancement: Better support for macros.
  - Enhancement: Better type induction for auto in foreach loop of maps.
  - Enhancement: Better contrast for scroller slider in dark theme.
  - Enhancement: Using lua script in themes.
  - Enhancement: Add compiler hint add-on interface for packager.
  - Enhancement: Loose some limit about platform/architecture (such as ASan).
  - Enhancement: add support for Windows user-wide installation.
  - Enhancement: add qmake variable to control preference of UTF-8 compatible OpenConsole.exe on Windows.
  - Enhancement: add Windows arm64 package.
  - Fix: Force to use debug server when debugging with lldb-mi to fix input/output on Windows.
  - Fix: Can't goto definition/declaration into files that not saved.
  - Fix: Expression that starts with full scoped variables might be treated as var definition.
  - Enhancement: Don't auto-indent in raw string.
  - Fix: Function list is not correctly retrived for full-scoped functions.
  - Enhancement: Improved Raw string support
  - Enhancement: New option for compiler set "Don't localize gcc output messages"
  - Enhancement: Optimization for drawing scrollbars.
  - Enhancement: Issue #213 Expands macro when finding function tips.

Red Panda C++ Version 2.25

  - fix: Symbol completion of '(' before selection may fail, if cursor is at the beginning of the selection.
  - change: Symbol completion of '{' won't insert extra new lines.
  - fix: "move selection up/down" of whole lines selection are no correctly handled.
  - enhancement: Improvement of terminal support ( by cyano.CN )
  - enhancement: ANSI escape sequences Support in windows 10/11 ( by cyano.CN )
  - enhancement: Option "Enable ANSI escape sequences Support" in Settings -> Executor 
  - change: Use freetype as the fontengine in windows ( by cyano.CN )
  - fix: Custom compile options is not used when retrieve macros defined by the compiler.
  - fix: Processing for #if/#elif/#else is not correct.
  - Change: Empty project template won't auto create main.c/main.cpp
  - enhancement: When creating project, warn user if the project folder is not empty.
  - fix: Press '>' after '-' don't show completion suggestion info.
  - fix: Icon position not correct under hiDPI devices and zoom factor >= 200%.
  - enhancement: After compiler settings changed, run/debug current file will auto recompile.
  - ehhancement: Show selected char counts in status bar.
  - enhancement: Differentiate /* and /** when calculate auto indents.
  - fix: crash when using ibus as the input method ( cyano.CN ).
  - fix: Correctly handle project templates that have wrong unit counts.
  - fix: Project recompiles for every run if auto increase build number is turned on.
  - fix: Auto increase build number for project is not correctly processed.
  
Red Panda C++ Version 2.24

  - fix: members of elements of stl maps are not correctly suggested.
  - fix: memory view's cell size is too wide in linux.
  - fix: Code completion doesn't work if "min id length to show completion" is not 1.
  - fix: english typos. (thanks for sangiye0@github)
  - fix: Goto definition/declaration may choose wrong symbol when multiple files are opened and symbols have the same name.
  - fix: "UTF-8 BOM" can't be correctly loaded as project file's encoding.
  - fix: Project file's encoding is not correctly updated after converted manually.
  - enhancement: Press left/right arrow will move caret to the begin/end of the selection.
  - enhancement: Press up/down arrow will move caret up/down from the begin/end of the selection.
  - enhancement: Show progress dialog if the time for searching compilers is too long.
  - fix: Dummy struct/enum symbols shouldn't be shown in the completion suggestion.
  - enhancement: Support optional enum name.
  - enhancement: Support optional enum type.
  - enhancement: Support simple const expression evaluation for enum values.
  - fix: Accessibilty for inherited members are not correct calculated in multiple inheritance.
  - fix: Can't parse full class name when handle inheritance.
  - fix: Can't parse virtual inherit.  
  - fix: Filename in the gcc 13.1 error messages when building project is using wrong encoding.
  - change: Git support is disabled in the distributed buildings. 
  - fix: Wrong code suggestion while inputing numbers in assembly files.
  - fix: Defines in all files are wrongly cleared when reparsing.
  - change: New file created by file template is set as unmodified by default.
  - change: Remove option "clear all symbols when current editor is hidden".
  - fix: When opening multiple files, only the active file should be parsed.
  - fix: Wrong compiler settings if xcode is not installed in mac os.
  - enhancement: Name for new files will not be different from files openned.
  - fix: Crash if close file while auto syntax checking.
  - enhancement: Support sdcc compiler.
  - enhancement: Autowrap tool output text.
  - fix: Press up/down arrow key in the option dialog's left panel won't switch page.
  - fix: Can't suggest header filename starting with numbers.
  - enhancement: Better layout for compiler options page.
  - enhancement: False branches are displayed as comments.
  - enhancement: Support SDCC Project.
  - enhancement: 3 compare mode for problem cases.
  - fix: Can't find other compilers that in the same folder with gcc.

Red Panda C++ Version 2.23

  - fix: When selection is availalbe, Ctrl+Click shouldn't jump to declaration/definition.
  - enhancement: Code completion for '->' operator on std iterators.
  - enhancement: Tooltip support for '->' operator on std iterators.
  - enhancement: Close other editors.
  - fix: Goto definition/Goto declaration/Info tips can't be correctly triggered when mouse pointer is at the last half character of current word.
  - fix: Use "/" as path seperator when starting app by double clicking c/c++ files in the explorer.
  - enhancement: differenciate -> and . when displaying completion suggestion infos.
  - enhancement: improve code completion for std iterators defined with "using namespace std"
  - enhancement: improve pointer calculation when inferencing type info
  - enhancement: improve parsing for multiple vars defined in one line
  - enhancement: improve parsing result for function parameters like 'Node (&node)[10]'
  - fix: Can't copy by ctrl+dray&drop to current selection's begin/end
  - enhancement: Support debug executable files generated by mingw-w64 gcc 13.1 and filepath contains non-ascii chars.
  - enhancement: When deleteing files in the files/project view, try moving to the trash bin instead.
  - fix: GNU assembly files (.s) are not shown in the files view.
  - fix: "typedef struct" that don't have definition of the struct is not correctly parsed.
  - enhancement: correctly highlight multiline raw string literals.
  - enhancement: correctly highlight multiline string literals.
  - change: remove "Assembly" color scheme item (it's not used anymore).
  - fix: crash when parsing files containing inline assembly code.
  - fix: crash when source files contains macro definitions like "#define cfun (cfun + 0)"
  - improvement: Correctly expands multi-line macros when parsing;
  - improvement: Correctly eppands macros when real param string contains '(' or  ')'.
  - enhancement: add "OI Wiki" and "turtle graphics tutorial" in help menu for zh_CN locale.
  - fix: Replace panel should be hidden after finding occurrencies.
  - enhancement: Show code completion suggestion after "typedef" and "const".
  - fix: GLFW project template.
  - fix: Inherited class/struct members are not correctly shown in the completion suggestions.
  - enhancement: Sort symbols by their declaration pos in the Class Browser, if not sort by alpha order.
  - fix: Keyword asm is not correctly parsed.
  - fix: Tips for problem is not correctly displayed.
  - enhancement: Folder mode in "File in files" dialog.
  - enhancement: When open a file, test if it contains binary contains.
  - enhancement: Correctly reformat C++ three-way comparision operator "<=>"
  - enhancement: Auto insert spaces between #include and <> when reformat
  - enhancement: Auto insert spaces between #include and "" when reformat
  - fix: Click editor's gutter won't toggle breakpoint in KDE debian 12 
  - fix: "Toggle breakpoint " in the editor gutter's context menu doesn't work.
  - fix: Shouldn't auto indent lines starts with "\\".
  - enhancement: When problem case's expected output is not too large (<= 5000 line), highlight text in the first different line in the expected output.
  - enhancement: Highlight text in the first different line using the error color.
  - enhancement: Add the option "redirect stderr to the Tools output panel" in the options dialog -> executor -> problem set page.
  - fix: Can't correctly uncomment multiple "//" comment lines that doesn't have spaces at linestarts.
  - fix: Autoindent for "{" is not correct.  
  - change: Don't print repeated values in gdb individually
  - enhancement: Don't show "\000" of string values in the debug local variables panel.

Red Panda C++ Version 2.22

  - fix: Crash at startup when current problem in the problem set is connected with source file.
  - fix: Double-clicking on touchpad can't select current word.
  - fix: foreach-loops are not correctly parsed.
  - fix: '^' is not correctly handled as operator.
  - fix: lambda expression  is not correctly handled.
  - fix: '__extension__' should be ignored when parsing C/C++ codes.
  - enhancement: show completion for return type of lambda expressions.
  - enhancement: support function arguments like "int (&t)[]"  
  - change: Don't show error dialog when bookmark/debug configuration json files are empty.
  - upgrade raylib to 4.5, raygui to 3.6  
  - enhancement: support -std=c++2d gcc parameter
  - fix: vertice shader(.vs) and fragment shader(.fs) files can't be openned by double click in the project browser.
  - enhancement: Add various menu items for cursor actions using Home/End/Page Up/Page Down keys.
  - enhancement: Filter names in the shortcut config page of options dialog.
  - fix: Typedef and using alias is not correctly handled in expression evaluation.

Red Panda C++ Version 2.21

  - change: The option "Check for stack smashing attacks (-fstack-protector)" is turned off by default in the Debug compiler set settings.
  - fix: Project makefile generated for C files is not correct.
  - fix: Horizontal scroll by touchpad is not working.
  - fix: Horizontal scroll by touchpad is inversed.
  - fix: Error message when save bookmarks.
  - enhancement: Auto skip ; and , when input.
  - enhancement: Add 'characters' column in the file properties dialog.
  - enhancement: Just keeping two digits after the decimal point for file size in the file properties dialog.

Red Panda C++ Version 2.20
 
  - change: Remove the compiler set option "Syntax error when object larger than"
  - fix: Projects created by some templates are not correct when editor's default encoding is not utf8.
  - fix: File/Project visit histories are not correctly saved when clearing.
  - fix: Octal numeric escape sequences is not correctly syntax highlighted.
  - enhancement: Refine suggestion info when try debug and the compiler settings are not correct.
  - enhancement: Open the options dialog/project options dialog when user want to correct compiler settings for debug.
  - enhancement: Open project's option dialog instead of the option dialog, when click the compiler set settings button in the toolbar and the current editor is for project.
  - enhancement: Reset project compile options when change compiler set in the project options dialog.

Red Panda C++ Version 2.19

  - fix: Crash when directive line ends with '\' and at the last line.
  - fix: The option "Minimal indent for a continuous conditional beloning to a conditional header:" for formatter is not correct.
  - fix: Crash when a project is removed from the disk while it is openned in RedPanda-C++.
  - fix: The option "Open CPU info dialog when signal received" can't be correctly set in the options dialog's debugger page.
  - fix: Crash when drag the selection beyond the end of the document.
  - enhancement: Drag the selection beyond the end of the document, and move/copy it beyond the last line.
  - enhancement: Open Containing folder will auto select the file in windows file explore.
  - fix: Class constructor & destructor is not correctly handled.
  - fix: Remove multiple files in the project panel is not correctly handled.
  - fix: Reformat code when select contents in column mode will mess up the document.
  - enhancement: Add "save as" icon to the toolbar.
  - enhancement: Use key sequences as shortcut to actions in the options dialog's environment->shortcut page.
  - change: Use ctrl+shift+S as the shortcut for "save as".
  - change: Use ctrl+K,ctrl+S as the shortcut for "save all".
  - fix: "Run all problem cases" with project is not correctly handled.
  - fix: When adding files to project and there'are duplicates, the warning info is not complete.
  - enhancement: Improve code completion suggestion for arrays.
  - fix: File's real encoding is not correctly calculated when save it using system default encoding.

Red Panda C++ Version 2.18

  - fix: macos icon size overgrown (by RigoLigo).
  - enhancement: Code completion for embedded stl containers.
  - enhancement: Slightly speed up code parsing.
  - enhancement: Sort header completion infos by suffix-trimmed filename.
  - fix: Code completion info for stl::map/std::unordered_map is not correct.
  - enhancement: Warn user and stop compile if project has missing files.
  - enhancement: Warn user when exit and save settings failed.
  - change: Remove compiler set options that's rarely used.
  - enhancement: Add option in the compiler set settings, to generate syntax error for large stack objects. （Enable for Debug settings by default)
  - enhancement: Add option in the compiler set settings, to generate protection code for stack smashing attack. （Enable for Debug settings by default)
  - enhancement: Add option in the compiler set settings, to enable address sanitizer. Not available in windows.（Enable for Debug settings by default)
  - fix: The comboxbox to input search keyword in the search dialog is case insensitive.
  - fix: The comboxbox to input replace text in the search dialog is case insensitive.
  - fix: The comboxbox to input search keyword in the search in files dialog is case insensitive.
  - fix: The comboxbox to input address expression in the debug panel's memory view is case insensitive.
  - fix: The comboxbox to input evaluation expression in the debug panel is case insensitive.
  - fix: The comboxbox to input replace text in the search panel is case insensitive.
  - fix: None initialized std::vector is not correctly displayed in the gdb of the gcc distributed with redpanda-c++ (Windows 64bit).
  - fix: Don't show completion info when input parameters for function definitions.
  - fix: Don't show function info tips when typing class variable definitions.
  - enhancement: Add option in the debug settings, to limit the length of the ouput generated by gdb for arrays.
  - enhancement: Show shortcut info in toolbar's tooltip.
  - change: Use F11 as the shortcut for "Run". (It's the old shortcut for "Compile&Run")
  - fix: Crash when directive line ends with '\' and at the last line.


Red Panda C++ Version 2.17

  - enhancement: Add X86_64 AVX/AVX instruction descriptions to asm syntaxer.
  - enhancement: Update x86 Assembly manual link to the newest website.
  - enhancement: Add "New Text File" in the File menu
  - enhancement: Add "address" in the memory view's mouse tip.
  - enhancement: Show mousetip for numbers in the GNU assembly file. 
  - enhancement: Open offline gnu as/x86 assembly manual if exists.
  - fix: Hex number with 'f' in not is not correctly colored.
  - fix: After project's default encoding is changed in the project options dialog, all project files' encoding are wrongly setted to the new encoding.(They should be "Project default")
  - enhancement: Make project's default encoding setting in the project options dialog more user friendly.
  - fix: In project options dialog's file page, Project's default encoding name is not updated when it's changed.
  - enhancement: Improve the compatibility with Dev-C++ for project configuations saved by Redpanda-C++.
  - enhancement: Syntax color support for binaray integer literals.
  - enhancement: Syntax color support for suffix in integer/float literals.
  - fix: Cpu info window is auto openned, when debug using gdb-server.
  - enhancement: Shift+Up in the first line will expand selection to the beginning of the line.
  - enhancement: Shift+Down in the last line will expand selection to the end of the line.
  - enhancement: If no selection, Ctrl+C (Copy) auto selects the current line and put the cursor to the beginning.
  - fix: Chinese characters in the source code is not correctly displayed in the CPU info window.
  - fix: Can't undo & save after copy by drag with mouse.
  - fix: '::' is not correctly handled when skip to next ':' in the parser.
  - fix: '::' is not correctly handled when parsing class definitions.
  - enhancement: Don't show operator overloading functions in the complete suggestions
  - enhancement: Correctly hanlde operator overloading functions like "operator ClassA"

Red Panda C++ Version 2.16

  - fix: Project files that not in the project folder is not correctly handled in makefile.
  - fix: Can't debug project when project is saved after it's compiled.
  - fix: Icons for buttons in the cpu info dialog is not correctly set.
  - fix: Can't locate the corresponding line in the generated asm file under linux.
  - enhancement: Add cfi directives for asm syntaxer in linux.
  - change: Editor option "Scroll past end of line" default to false.
  - emhancement: Improve display of disassembled codes in the cpu info dialog.
  - fix: Can't correctly parse function pointer var definition.
  - enhancement: Improve support for function pointer typedefs.
  - enhancement: Improve support for function pointer vars.
  - enhancement: When first display two editor panes, auto make them the same width
  - change: Don't rebuild the whole project when run/debug, if only contents of project unit file is modified.
  - fix: rebuild may not work, if project's parallel build option is enabled.
  - enhancement: Add "Close window" and "Move to other view" in the "Window" menu
  - enhancement: Auto open CPU info dialog, if the program in debug is stopped at a position that have no source file.
  - enhancement: "add watchpoint" when debug. It's hitted when the watch variable is modified, or it's out of scope.
  - enhancement: Switch current call stack frame in the CPU info dialog
  - fix: Shouldn't try evaluate value of the selection in the cpu info dialog.
  - enhancement: Show oct/bin/bin value in the memory view's tooltip.
  - fix: Hex float point literal is not correctly colored.
  - fix: Problem's memory limit unit can't be correctly saved.

Red Panda C++ Version 2.15

  - fix: Static class members is not correctly recognized as static.
  - fix: Function with reference type return value is not correctly parsed.
  - enhancement: Add description tooltips for x86 registers in the cpu info dialog.
  - fix: Search dialog shouldn't have "prompt when replace".
  - change: Default value for the debugger debugger panel "memory view's columns" is changed from 8 to 16.
  - change: Default value for the debugger debugger panel "memory view's rows" is changed from 8 to 16.
  - enhancement: Display hex value as ascii chars in the debugger panel memory view tab.
  - fix: Word on the last line's end can't be searched.
  - enhancement: Auto close other search/replace dialogs when start to search/replace.
  - change: Remove "prompt when replace" in the replace.
  - fix: Search/replace with regex is not correctly handled.
  - enhancement: Show descriptions mouse tip for assebmly instructions. (editor / cpu info dialog)
  - fix: When completing resigter names, an extra '%' is wrongly added.  
  - enhancement: Syntax check for assembly files.
  - enhancement: Add "Languages" page group in the options dialog.
  - enhancement: Add "ASM Generation" page in the options dialog.
  - change: Move "Custom C/C++ keywords" from group "Editor" to "Lanauges" in the options dialog.
  - change: Rename "Folder" page to "Folder / Reset default settings" in the options dialog.
  - enhancement: Generate asm with/without SEH directives.
  - enhancement: Generate asm using intel style/att style.
  - enhancement: make description for jump/cmov/setb instructions more explicit. (used for signed or unsigned)
  - fix: Lead and end spaces in search/replace text is wrongly trimmed.
  - change: Merge search and replace to one dialog.
  - fix: Search dialog's "Match whole word" option doesn't work with "Use Regular expresion".
  - fix：Search dialog's "Close after search" option doesn't work.
  - change: Fill the search dialog with the current selection if it's available.

Red Panda C++ Version 2.14

  - change: Remove all breakpoints of the current non-project file, when it is closed.
  - fix: Enum value defines is not correctly parsed.
  - enhancement: Use differenct source file for each language in project templates
  - fix: Ctrl+click is too sensitive.
  - enhancement: Check and remove all non-exist breakpoints before debug a project
  - change: Remove nasm support
  - change: Don't stop debug when breakpoint can't be set
  - fix: "Generate assembly" menu item is wrongly enabled for new GNU assembly files 
  - enhancement: New file templates for C / C++ / GAS files
  - enhancement: Keep project compile warning & error infos in the issues table, before project file is edited.

Red Panda C++ Version 2.13
  
  - fix: Only C/C++/GAS files can set breakpoints.
  - Enhancement: Don't show breakpoints/watch related menuitems in context menu for non-C/C++/GAS files.
  - Enhancement: Disable reformat code for non-C/C++ files.
  - Enhancement: Support C11 anonymous struct/union
  - fix: Can't debug when debug a file while other file has breakpoints
  - change: Don't save breakpoints for non-project files
  - Enhancement: Correctly init panel sizes when first run.

Red Panda C++ Version 2.12

  - fix: Can't correctly load project's custom compile options, if it contains more than one line contents.
  - fix: Crash when create or open txt files in project.
  - enhancement: Code folding for #if/#endif
  - enhancement: When folding "if", don't fold "else";
  - fix: Confirm if recompile, when start to debug and project files has modifications.
  - fix: Crash when debug project that has nasm files.
  - enhancement: Generate debug info for nasm files in Linux/MacOS.
  - enhancement: Compile/Run/Debug GAS source files.
  - enhancement: Compile/Debug GAS source files in project.
  - enhancement: Keyword completion for asm/GAS files.
  - enhancement: If GAS source file has "_start" label, compile it with "-nostartfiles".
  - fix: New non-saved filenames is wrongly saved in the last openfiles list.
  - fix: File is parsed before editor is fully created.
  - enhancement: New GAS File in the File Menu
  - change: rename "New File" to "New C/C++ File"    
  - change: The default disassemble style of CPU Dialog is "AT&T" now.
  - fix: Can't compile files with chinese characters in filenames using winlibs mingw gcc
  - fix: If current editor is empty, parser will parse the file's content on the disk instead from the editor.
  - fix: Can't show completion when cursor is after "char["
  - change: Don't confirm rebuild/recompile when run/debug.
  - fix: Can't parse enum values.
  - fix: Can't correctly show enum values in the class browser.
  - fix: Can't correctly create project, if template's encoding setting is not valid.
  - enhancement: New "embed assembly" template.
  - enhancement: New "Hello GAS" and "GAS and C" templates for linux and win64.
  - fix: Wrong selection position after delete in column mode.
  - enhancement: Syntax highlight and basic code completion for lua.
  - enhancement: Basic code completion for xmake.lua.
  - fix: Parser not correctly released if save a c file to non-c file.
  - enhancement: Improve auto indent for embedding no-brace statements like for-for-if.
  - enhancement: Toggle comment for asm/makefile/lua files.
  - enhancement: Delay for tooltips.
  - enhancement: "Tool tips delay" option in Options/editor/Tooltips
  - change: Remove "Compile & Run" menu item. It's replaced by "Run".
  - enhancement: Show "..." instead of "...}" when folding #if/#endif
  - fix: Correctly handle high-precision mouse wheel / touchpad in editors.
  - enhancement: Greatly reduce time to open/edit big files.
  - enhancement: Reduce flicker when editing big files. 
  - enhancement: If executable doesn't have symbol table, inform user and stop.
  - enhancement: If breakpoint is setted but executable doesn't have debug info ，inform user and stop.
  - enhancement: If current compiler set has "strip addition infos(-s)" enabled, inform user and stop.
  - enhancement: Auto create project custom executable folder if not existing.

Red Panda C++ Version 2.11

  - fix: Can't correctly handle definitions for "operator,"
  - enhancement: Auto suggest keyword "operator" when define functions.
  - enhancement: Differentiate class and constructors in syntax color and jupming to declarations.
  - enhancement: Improve parsing for operator overloading.
  - fix: Parser can't correctly differentiate function and var initialization.
  - fix: Respect encoding "Project default" when search/find occurrencies/open project units.
  - enhancement: Show progress dialog when search/find occurrencies in large projects.
  - enhancement: Improve auto indent.
  - enhancement: Change the way to calculate execution time.
  - enhancement: Auto reload openned project files that use "Project Default" as the encoding, when the project encoding setting is changed in the project options dialog.
  - fix: Correctly handle files whose name contains spaces in the generated makefile.
  - fix: Correctly handle custom obj folder in the generated makefile.
  - enhancement: Support compile asm files using nasm in the project.
  - fix: Project parser should not parse non-c/cpp files.
  - enhancement: Add "assembler" tab in the project options dialog's custom compiler parameters.
  - enhancement: Auto find nasm when detecting new compiler sets/adding gcc compiler sets.
  - fix: preprocessors is not correctly suggested.
  - fix: javadoc-style docstring is not correctly suggested
  - enhancement: Better syntax color for asm files.
  - enhancement: Add nasm.exe in the gcc distributed with RedPanda-CPP
  - enhancement: Add assembly templates 


Red Panda C++ Version 2.10

  - fix: When restored from minimization, info on statusbar not correctly restored.
  - enhancement: Changes of "auto backup editing contents" is applied immediately.
  - enhancement: Don't create temp backup for readonly files.
  - enhancement: Add "Help"/"Submit Iusses".
  - enhancement: Add "Help"/"Document" for Simplified Chinese users.
  - enhancement: Code Completion now respect compiler set's language standard settings.
  - enhancement: Save project files' real encoding;
  - enhancement: Use project files' real encoding information when generating the makefile.
  - fix: If buttons in the options dialog / compiler / compiler set page is pressed, they won't release.
  - enhancement: Confirm before remove a compiler set.
  - enhancement: If there is "cppreference.chm" or "cppreference-%locale_name%.chm"(like cppreference-zh_CN.chm) in the redpanda C++'s app folder, open it instead of the cppreference website.
  - enhancement: Use lldb-mi as the debugger.
  - enhancement: Set lldb-mi as the debugger program for clang, when finding compiler set in folders and gdb doesn't exist.
  - fix: Settings in Options/Tools/General is messed up when switching items in the list.
  - fix: Infos in the status bar not correctly updated when editor closed.
  - change: Project's encoding shouldn't be set to "auto detect"
  - fix: Can't correctly set project file's encoding back to 'UTF-8'/'ANSI' in the project options dialog/files setting page.
  - enhancement: Simplified chinese translations for encoding names.
  - fix: Crash when there are preprocessing directives like '#if 0/0' or '#if 0%0'
  - enhancement: Pause autosave timer when autosave new files.

Red Panda C++ Version 2.9

  - enhancement: set caret to the corresponding line in the editor after "run"/"generate assembly"
  - fix: syntax highlighting for cpp style line comment is not correct.
  - fix: Save may crash app if the encoding codec is failed to load.
  - enhancement: support open and save utf-16/utf-32 BOM files. (but gcc can't compile)
  - enhancement: Add "auto backup editing contents" option in options/editor/auto save. Turned off by default.
  - enhancement: If the "auto backup editing contents" option is turned on, auto save editing contents 3 seconds after input stopped. Auto delete when editor successfully closed)
  - fix: rename project file will wrongly set it's encoding to 'ASCII';
  - fix: Project's file encoding is wrongly set to 'AUTO' when load project.

Red Panda C++ Version 2.8

  - fix: Crash when editing makefile
  - enhancement: Add "Resources" in project option's dialog's custom compiler parameter page
  - fix: Crash while input using input method in makefile 
  - enhancement: "Run" / "Generate Assembly" for project source files
  - fix: Can't set project icon to "app.ico" in the project folder, if the project doesn't has icon.
  - fix: Resource compilation items is missing in the auto generated makefile, if the project's icon is removed and re-added.
  - fix: Action "Run all problem cases" is triggered twice by one clicked.
  - enhancement: "Switch Header/Source" in editor title bar context menu.
  - enhancement: "Toggle readonly" in the Edit menu.
  - fix: Error When save project units' encoding settings.
  - enhancement: Waiting for syntax parsers to finish before saving files, to prevent data lost caused by syntax parsering crash.
  - fix: Restore main window and cpu info window will set wrong font in the cpu info.
  - enhancement: Let encoding options in the statusbar more explicit.
  - fix: Crash when find occurrences in a project that has missing files.
  - enhancement: Print current selection can be used in the print dialog.
  - enhancement: Print syntax colored content.
  - enhancement: Correctly handle tab in the exported RTF.
  - change: Disable undo limit by default.
  - fix: "Goto declaration" / "Goto definition" / "Find occurences" not correctly disabled for non-c/c++ files.
  - fix: Can't save new file using filename with custom suffix.
  - fix: alt+shift+left/right can't select
  - fix: Input any content will exit column mode.
  - fix: Result of scope calculation not right if a for statement immediately follows another for statement.
  - fix: Function parameters that is pointer,reference or array can't be correctly parsed.
  - fix: In column mode, selection that contain lines with different length will cause error.
  - enhancement: Rename symbols won't remove all breakpoints/bookmarks
  - enhancement: Batch replace won't remove all breakpoints/bookmarks
  - enhancement: Execute parameters can be used in debug.
  - enhancement: Add "Open files in editor" in the search panel
  - enhancement: Auto disable the "in project" option in the "search in files" dialog, if no project is opened.
  - enhancement: Auto disable the "search again" button in the search panel if the current search history item is search in the project, and no project is opened.

Red Panda C++ Version 2.7

  - enhancement: Remove multiple problems in the problem set view
  - enhancement: Clear the problem view after a new problem set created
  - enhancement: "Trim trailing spaces" (Before saving a file) in options / editor / misc
  - enhancement: "Trim trailing spaces" in code menu
  - change: Don't auto disable compile and debug buttons for compiler sets that don't have compiler/debugger programs.
  - enhancement: Better error messages for missing compile/debug/make programs.
  - fix: Lost compiler set settings if a compiler set's bin dirs is empty.
  - enhancement: Better error message when trying to debug with Release compile set.
  - enhancement: Add missing space char color settings in color schemes
  - enhancement: Export FPS (free problem set) files.
  - enhancement: Run all cases button not correct disabled when no case exits.
  - enhancement: Speed up remove problems.
  - fix: "Compile" button disabled after app start with an empty new file.
  - enhancement: Don't add "-g3" option when generate assembely.
  - enhancement: Generate assembly is not correctly disabled when current file is not C/C++.
  - change: Disable  "Copy Limit" int "options"/"editor"/"Copy/Export" by default.
  - fix: Project's "static link" option is overwrited by global compiler set settings, when project options dialog is opened.
  - fix: Icon size not correct under macOS high DPI / zoom factor settings.
  - enhancement: "Icon zoom" in options / environment / appearance
  - enhancement: "Line Spacing" in options / editor / font
  - enhancement: "Show whitespaces" in options / editor / font
  - enhancement: Auto add "lib" to the output of static/dynamic library projects, if project name don't start with "lib".
  - fix: Makefile error when "Use precompiled header" is enabled in the project option dialog.
  - enhancement: "Convert HTML for - Input" / "Convert HTML for - Expected" in "Options" - "Executor" - "Problem Set"
  - fix: Unit for memory limit is not correctly loaded when open problem properties dialog.
  - enhancement: Auto open the properties dialog, after add a new problem.

Red Panda C++ Version 2.6

  - enhancement: Highlighter for makefiles
  - fix: QSortFilterProxyModel not correctly cleared when exiting and project closed. (ASSERT fails in DEBUG mode.)
  - enhancement: Windows installers now use UNICODE encoding.
  - fix: Can't correctly show code suggestions after "template <"
  - enhancement: Better code completion support for macros
  - fix: Paste not enabled when create a new file and system clipboard is empty.
  - enhancement: Auto rebuild when project's compiler set changed.
  - enhancement: When current file is the project's makefile, show project's compiler set in the toolbar.
  - enhancement: Prevent error of "del" to stop make when rebuild project.
  - enhancement: Import FPS (free problem set) files.
  - enhancement: Show current problem's description in the problem list's mouse tip.
  - enhancement: Show memory usage for problem cases (windows only).
  - enhancement: Show memory usage after console program exited.
  - fix: If clang and g++ are in the same folder, only the compiler sets for gcc are auto generated.
  - fix: Buttons in options -> compiler -> compiler set -> programs are not usable.
  - enhancement: Don't check existence of gcc/g++/make/gdb at startup.
  - enhancement: Auto disable "compile" button if gcc doesn't exist.
  - enhancement: Auto disable "debug" button if gdb doesn't exist.
  - enhancement: Auto disable "compile" button for project if make doesn't exist.
  - fix: Crash when scroll file which has more than 65535 lines.
  - fix: Can't scroll to lines greater than 65535.

Red Panda C++ Version 2.5

  - enhancement: New color scheme Monokai (contributed by 小龙Dev(XiaoLoong@github))
  - enhancemnet: Add "Reserve word for Types" item in color scheme
  - enhancement: Auto save / load problem set
  - enhancement: Project's custom compile include/lib/bin directory is under folder of the app, save them using the path relative to the app
  - enhancement: Slightly reduce memory usage   
  - enhancement: Options -> editor -> custom C/C++ type keywords page
  - change: Default value of option "Editors share one code analyzer" is ON
  - change: Default value of option "Auto clear symbols in hidden editors" is OFF
  - enhancement: Show completion suggest for "namespace" after "using"
  - fix: MinGW-w64 gcc displayed as "MinGW GCC"
  - enhancement: Deduce type info for "auto" in some simple cases for stl containers.
  - fix: Crash when no semicolon or left brace after the keyword "namespace"
  - fix: Can't correctly show completion suggest for type with template parameters
  - enhancement: Show compltion suggest for std::pair::first and std::pair second
  - enhancement: Disable "run" and "debug" actions when current project is static or dynamic library
  - enhancement: Add "Generate Assembly" in "Run" Menu
  - enhancement: Improve highlighter for asm
  - enhancement: Use asm highlighter in cpu window
  - fix: "AT&T" radio button not correctly checked in cpu window 
  - enhancement: Remove blank lines in the register list of cpu window.
  - fix: Cpu window's size not correctly saved, if it is not closed before app exits.
  - fix: Can't restore cpu window's splitter position.

Red Panda C++ Version 2.4

  - fix: Contents in class browser not correctly updated when close the last editor for project. 
  - fix: When all editors closed, switch browser mode dosen't correct update the class browser;
  - fix: "check when open/save" and "check when caret line changed" in Options Dialog / Editor / Syntax Check don't work
  - fix: Crash when editing a function at the end of file without ; or {
  - enhancement: Add the "parsing TODOs" option in Options Dialog / Editor / Misc
  - enhancement: Remove todos/bookmarks/breakpoints when deleting file from project
  - enhancement: Rename filenames in todos/bookmarks/breakpoints  when renaming project file
  - enhancement: Rename filenames in bookmarks/breakpoints after a file is save-ased.
  - fix: Can't goto definition of classes and namespaces displayed in the class browser on whole project mode.
  - fix: macro defines parsed before not correctly applied in the succeeding parse.
  - fix: function pointers not correctly handle in code parser;
  - fix: var assignment not correctly handled in code parser;
  - fix: function args not correctly handled in code parser;
  - fix: crash when alt+mouse drag selection
  - enhancement: show completion tips for when define a function that already has a declaration.
  - enhancement: Use relative paths to save project settings
  - fix: Layout for project options dialog's general page is not correct.
  - fix: modifitions in the project options dialogs's dll host page is not correctly saved.
  - enhancement: In the project options dialog, autoset the default folder in the openning dialog when choosing file/directory paths.
  - fix: Escape suquences like \uxxxx and \Uxxxxxxxx in strings are not correctly highlighted.
  - enhancement: Search / replace dialogs redesigned.
  - fix: inline functions are not correctly parsed;
  - fix: &operator= functions are not correctly parsed;
  - fix: Code Formatter's "add indent to continueous lines" option is not correctly saved.
  - fix: _Pragma is not correctly handled;
  - enhancement: improve parse result for STL <random>
  - change:  the default value for UI font size : 11
  - change:  the default value for add leading zeros to line numbers : false
  - upgrade integrated rturtle. fix: nothing is drawed when set background color to BLACK
  - upgrade integrate fmtlib. fix: imcompatible with GBK encoding

Red Panda C++ Version 2.3

  - fix: When start parsing and exit app, app may crash
  - enhancement: add "Allow parallel build" option in project option dialog's custom compile options page
  - fix: crash when rename project file
  - fix: When remove project file, symbols in it not correctly removed from code parser
  - fix: infos in class browser (structure panel) not correctly updated when add/create/remove/rename project files


Red Panda C++ Version 2.2

  - enhancement: basic code completion support for C++ lambdas
  - enhancement: slightly reduce parsing time
  - fix: Wrong charset name returned when saving file
  - fix: 'using =' / 'namespace =' not correctly handled
  - fix: Pressing '*' at begin of line will crash app
  - enhancement: switch header/source in editor's context menu
  - enhancement: base class dropdown list in new class dialog now works 
  - fix: Edting / show context menu when code analyzer is turned off may crash app.
  - fix: Show context menu when edting non c/c++ file may crash app.
  - fix: Project Options Dialog's Files panel will crash app.
  - fix: Memory usage of undo system is not correctly calculated, which may cause undo items lost
  - fix: Set max undo memory usage to 0 don't really remove the limit for undo
  - fix: Set max undo times to 0 don't really remove the limit for undo
  - fix: Keep the newest undo info regardless of undo memory usage
  - fix: inline functions not correctly handled by parser
  - fix: &operator= not correctly handled by parser

Red Panda C++ Version 2.1

  - fix: editors that not in the editing panel shouldn't trigger switch breakpoint
  - fix: editors that not in the editing panel shouldn't show context menu
  - enhancement: add "editors share one code parser" in "options" / "editor" / "code completion", to reduce memory usage.
        Turned off by default on PCs with memory > 4G; Force turned on PCs with memory < 1G.
  - enhancement: add "goto block start"/"goto block end" in "Code" menu
  - add fmtlib to the gcc compiler's lib distributed with RedPanda IDE windows version
  - add default autolink for fmtlib in Windows 
  - reduce size of the executable of win-git-askpass tool
  - change: remove "Optimize for the following machine" and "Optimize less, while maintaining full compatibility" options in the compiler setting panel, which are obseleted.
  - change: escape spaces in the executabe path under linux.
  - fix: Before run  a project's executable, we should check timestamp for project files AND modification states of files openned in editor.
  - change: Don't turn on "Show some more warnings (-Wextra)" option by default for DEBUG compiler set 
  - fix: Changes mainwindows's compiler set combobox not correctly handled for project
  - change: Don't localize autogenerated name for new files and new project (new msys2 gcc compiler can't correctly handle non-ascii chars in filenames)
  - change: rename "file" Menu -> "New Source File" to "New File" 

Red Panda C++ Version 2.0

  - redesign the project parser, more efficient and correct
  - enhancement: todo parser for project
  - fix: save/load bookmark doesn't work
  - fix: if project has custom makefile but not enabled, project won't auto generate makefile.
  - fix: File path of Issues in project compilation is relative, and can't be correctly marked in the editors.
  - fix: editor & class browser not correct updated when editor is switched but not focused
  - enhancement: show all project statements in the class browser
  - fix: namespace members defined in multiple places not correctly merged in the class browser
  - fix: correctly display statements whose parent is not in the current file
  - fix: statements is the class browser is correctly sorted
  - enhancement: Weither double click on the class browser should goto definition/declaration,  depends on the current cursor position
  - enhancement: keep current position in the class browser after contents modified
  - fix: "." and ".." in included header paths not correctly handled
  - reduce memory usage when deciding file types
  - enhancement: refresh project view for git status won't redraw project structure
  - enhancement: auto save project options after the compilerset option for project resetted 
  - enhancement: "." and ".." in paths of issues not correctly handled
  - enhancement: auto locate the last opened file in the project view after project creation
  - enhancement: separate compiler's language standard option for C / C++
  - fix: compiler settings not correctly handled when create makefile
  - enhancement: auto locate current open file in the project view panel
  - enhancement: when closing project, prevent all editors that belongs to the project check syntax and parse todos.
  - enhancement: add "auto reformat when saving codes" in "Options" / "Editor" / "Misc" (off by default)
  - enhancement: use "todo" and "fixme" as the keyword for TODO comments
  - fix: rules for obj missing in the makefile generated for project 
  - enhancement: before run a project'executable, check if there's project file  newer than the executable
  - enhancement: when create a new folder in the files view, auto select that folder and rename it
  - enhancement: when new header in the project view, auto select basename in the filename dialog
  - enhancement: when add file in the project view, auto select basename in the filename dialog
  - change: Don't generate localized filename when new header/add file in the project view
  - fix: Restore project's original compiler set if user choose 'No' in the confirm project compiler set change dialog.
  - fix: Encoding info in the status bar not correctly updated when save a new file
  - enhancement: auto sort TODO items 
  - fix: Correctly set file's real encoding to ASCII after saving
  - fix: selection's position not correctly set after input a char / insert string (and causes error under OVERWRITE mode)
  - fix: editors that not in the editing panel should not be syntax checked/ todo parsed/ code analyzed
  - fix: editors that not in the editing panel should not trigger breakpoint/bookmark/watch switch

Red Panda C++ Version 1.5

  - fix: project files that lies in project include folder is wrongly openned in Read-only mode
  - enhancement: add/new/remove/rename project files won't rebuild project tree
  - fix: gliches in UI's left panel in some OS
  - fix: correctly restore project layout when reopen it
  - change: new syntax for project layout files
  - change: clear tools output panel when start to compile
  - change: don't show syntax check messages in the tools output panel (to reduce longtime memory usage)
  - fix: minor memory leaks when set itemmodels
  - fix: threads for code parsing doesn't correctly released when parsing finished ( so and the parsers they use)
  - enhancement: save project's bookmark in it's own bookmark file
  - enhancement: project and non-project files use different bookmark view (auto switch when switch editors)
  - enhancement: auto merge when save bookmarks.
  - enhancement: add option "max undo memory usage" in the options / editor / misc page
  - fix: icons in options dialogs  not correctly updated when change icon set
  - enhancement: set compilation stage in the options / compiler set pages
  - enhancement: set custom compilation output suffix in the options / compiler set pages
  - enhancement: project and non-project files use different breakpoint and watchvar view (auto switch when not debugging and editor switched)
  - enhancement: save project's breakpoint and watchvar in it's own debug file.
  - enhancement: delete a watch expression don't reload who watch var view
  - enhancement: auto save/restore debug panel's current tab
  - fix: correctly restore left(explorer) panel's current tab
  - enhancement: auto close non-modified new editor after file/project openned;
  - fix: project files openned by double click in bookmark/breakpoint panel may cause app crash when closed.
  - fix: When open a project that's already openned, shouldn't close it.
  - enhancement: When open a project, let user choose weither open it in new window or replace the already openned project
  - fix: editor tooltip for #include_next is not correctly calculated
  - fix: ctrl+click on #include_next header name doesn't open the right file
  - enhancement: parser used for non-project C files won't search header files in C++ include folders.
  - fix: toggle block comment/delete to word begin/delete to word end are not correctly disabled when editor not open
  - fix: index out of range in cpp highlighter
  - fix: memory leak in code folding processing
  - change: add/remove/new project file won't save all openned project files.
  - fix: save all project files shouldn't trigger syntax check in inactive editors 

Red Panda C++ Version 1.4

  - fix: "Encode in UTF-8" is not correctly checked, when the editor is openned using UTF-8 encoding.
  - fix: crash when create non C/C++ source file in project
  - fix: can't open text project file in the editor
  - change: when create non-text project file, don't auto open it
  - fix: the project compiler options is not correctly read  when open old dev-c++ project 
  - fix: astyle.exe can't correctly format files that using non-ascii identifier
  - fix: when "cleary symbol table of hidden editors" is turned on, content in the editor reshown is not correctly parsed

Red Panda C++ Version 1.3

  - enhancement: don't parse all openned files when start up
  - enhancement: don't parse files when close all and exit
  - change: reduce time intervals for selection by mouse
  - enhancement: change orders of the problems in the problem set panel by drag&drop
  - enhancement: change orders of the problem cases in the problem panel by drag&drop
  - fix: the size of horizontal caret is wrong

Red Panda C++ Version 1.2

  - enhancement: Portuguese Translation ( Thanks for crcpucmg@github)
  - fix: files in network drive is opened in readonly mode
  - change: reorganization templates in subfolders
  - enhancement: create template from project
  - fix: can't correctly set project icon
  - fix: can't set shortcut that contains shift and non-alphabet characters

Red Panda C++ Version 1.1.6

  - fix: block indent doesn't work
  - fix: selection is not correctly set after input in column mode 
  - fix: in #if, defined without () not correctly processed
  - enhancement: don't show cpp defines when editing c files
  - enhancement: choose default language when first run
  - fix: Drag&Drop no correctly disabled for readonly editors
  - enhancement: disable column mode in readonly editors
  - fix: inefficient loop when render long lines
  - fix: indents for "default" are not the same with "case"
  - fix: (wrongly) use the default font to calculate  non-ascii characters' width
  - change: switch positions of problem case output and expected output

Red Panda C++ Version 1.1.5

  - change: uncheck "hide unsupported files" in files view shouldn't gray out non-c files
  - enhancement: double clicking a non-text file in the files view, will open it with external program
  - enhancement: double clicking a non-text file in the project's view, will open it with external program
  - fix: correctly update the start postion of selection after code completion
  - enhancement: add a demo template for raylib/rdrawing predefined colors
  - enhancement: add select current word command in the Selection menu
  - change: add Selection menu
  - enhancement: add memory view rows/columns settings in the settings dialog -> debugger -> general panel
  - enhancement: add "Go to Line..." in the Code menu
  - fix: "Timeout for problem case" can't be rechecked, in the Settings Dialog -> executor -> problem set panel.
  - fix: bug in the project template
  - change: sort local identifiers before keywords in the auto completion popup
  - fix: can't create folder in files view, if nothing is selected
  - fix: can't find the gcc compiler, if there are gcc and clang compilers in the same folder

Red Panda C++ Version 1.1.4

  - enhancement: prohibit move selection up/down under column mode
  - enhancement: prohibit move selection up/down when the last line in selection is a folded code blocks
  - enhancement: check validity of selection in column mode when moving caret by keyboard
  - enhancement: check validity of selection in column mode when moving caret by mouse
  - enhancement: only allow insert linebreak at the end of folded code block
  - enhancement: only allow delete whole folded code block
  - refactor of undo system
  - fix: calculation of the code block ranges when inserting/deleting
  - fix: undo chains
  - enhancement: prevent group undo when caret position changed
  - fix: undo link break may lose leading spaces
  - fix: correctly restore editor's modified status when undo/redo
  - enhancement: set current index to the folder after new folder created in the files view
  - enhancement: resort files in the files view after rename

Red Panda C++ Version 1.1.3

  - fix: wrong auto indent calculation for comments
  - enhancement: position caret at end of the line of folded code block
  - enhancement: copy the whole folded code block
  - enhancement: delete the whole folded code block
  - fix: correctly update the folding state of code block, when deleted
  - change: just show one function hint for overloaded functions
  - update raylib to 4.2-dev
  - update raylib-drawing to 1.1
  - add "raylib manual" in the help menu

Red Panda C++ Version 1.1.2
  - enhancement: use different color to differenciate folder and headers in completion popup window
  - enhancement: auto add "/" to folder when completing #include headers
  - enhancement: add the option "Set Encoding for the Executable" to project's compiler options
  - fix: can't correctly compile when link params are seperated by line breaks
  - fix: select all shouldn't set file's modified flag
  - enhancement: add (return)type info for functions/varaibles/typedefs in the class browser panel
  - enhancement: autolink add "force utf8" property (mainly for raylib)
  - change: position caret to (1,1) when create a new file using editor's new file template

Red Panda C++ Version 1.1.1
  - enhancement: adjust the appearance of problem case's input/output/expected control
  - change: swap position of problem case's output and expected input controls
  - enhancement: when problem case panel is positioned at right, problem case's input, output and expected controls is layouted vertically
  - enhancement: add ignore spaces checkbox in problem cases panel
  - fix: can't paste contents copied from Clion/IDEA/PyCharm
  - fix: project don't have compiler set bin folder setting
  - fix: when run/debug the executable, add current compiler set's bin folders to path
  - fix: when open in shell, add current compiler set's bin folders to path
  - fix: when debug the executable using gdb server, add current compiler set's bin folders to path
  - fix: reduce height of the message panel when dragging from right to bottom
  - fix: when messages panel is docked at right, its width not correctly restored when restart.

Red Panda C++ Version 1.1.0
  - enhancement: when ctrl+mouse cursor hovered an identifier or header name, use underline to highlight it
  - enhancement: mark editor as modified, if the editing file is changed by other applications.
  - enhancement: When the editing files is changed by other applications, only show one notification dialog for each file.
  - fix: c files added to a project will be compiled as c++ file.
  - enhancement: restore caret position after batch replace
  - enhancement: rename in files view's context menu
  - enhancement: delete in files view's context menu
  - change: drag&drop in files view default to move
  - fix: rename macro doesn't work in project
  - fix: undo doesn't work correctly after rename symbole & reformat
  - fix: can't remove a shortcut
  - enhancement: hide all menu actions in the option dialog's shortcut panel
  - enhancement: add 'run all problem cases' / 'run current problem case' / 'batch set cases' to the option dialog's shortcut panel
  - enhancement: more templates for raylib
  - fix: compiler settings not correctly saved

Red Panda C++ Version 1.0.10
  - fix: modify watch doesn't work
  - fix: make behavior consistent in adding compiler bindirs to Path (thanks for brokencuph@github)
  - enhancement: basic MacOS support ( thanks for RigoLigoRLC@github)
  - fix: #define followed by tab not correctly parsed
  - enhancement: don't auto add () when completing C++ io manipulators ( std::endl, std::fixed, etc.)
  - fix: can't goto to definition of std::endl
  - fix: errors in the calculation of cut limit
  - enhancement: new turtle library based on raylib ( so it can be used under linux)
  - fix: autolink calculation not stable

Red Panda C++ Version 1.0.9
  - fix: selection in column mode not correctly drawn when has wide chars in it
  - fix: delete & insert in column mode not correctly handled
  - fix: input with ime in column mode not correctly handled
  - fix: copy & paste in column mode not correctly handled
  - fix: crash when project name is selected in the project view and try create new project file
  - change: panels can be relocated
  - fix: tab icon not correct restore when hide and show a panel
  - fix: the hiding state of the tools output panel is not correctly saved
  - enhancement: add "toggle explorer panel" and "toggle messages panel" in "view" menu
  - fix: cursor is wrongly positioned when insert code snippets that don't have placeholders
  - fix: "run current cases" dosen't correctly display real output 

Red Panda C++ Version 1.0.8
  - enhancement: auto complete '#undef'
  - enhancement: redesign components for compiler commandline arguments processing
  - fix: selection calculation error when editing in column mode
  - enhancement: add compiler commandline argument for "-E" (only preprocessing)
  - enhancement: auto set output suffix to ".expanded.cpp" when compiler commandline argument for "-E" is turned on
  - enhancement: auto set output suffix to ".s" when compiler commandline argument for "-S" is turned on
  - enhancement: show error message when user set a shortcut that's already being used.
  - enhancement: adjust scheme colors for "dark" and "high contrast" themes
  - enhancement: can debug files that has non-ascii chars in its path and is compiled by clang
  - fix: when debugging project, default compiler set is wrongly used

Red Panda C++ Version 1.0.7
  - change: use Shift+Enter to break line
  - change: highlight whole #define statement using one color
  - enhancement: don't highlight '\' as error
  - enhancement: hide add charset  option in project options dialog's compiler set page, when project compiler set is clang
  - fix: When generating project's makefile for clang, don't add -fexec-charset / -finput-charset command line options
  - fix: index of the longest line not correctly updated when inputting with auto completion open
  - enhancement: support UTF-8 BOM files
  - enhancement: add new tool button for "compiler options"
  - fix: keyword 'final' in inhertid class definition is not correctly processed
  - change: stop generating 'profile' compiler set

Red Panda C++ Version 1.0.6
  - fix: gcc compiler set name is not correct in Linux
  - enhancement: hide add charset option when the currect compiler set is clang
  - enhancement: auto check the c project option in the new project dialog
  - change: use "app.ico" as default name for the project icon file
  - fix: c file should use CC to build in the auto generated makefile
  - enhancement: package script for msys2 clang
  - enhancement: auto set problem case's expected output file which has "ans" as the suffix, when batch set cases
  - fix: use utf8 as the encoding for clang's error output
  - fix: correctly parse link error message for clang
  
Red Panda C++ Version 1.0.5
  - enhancement: add autolink and project template for sqlite3
  - enhancement: add sqlite3 lib to the gcc in distribution
  - enhancement: improve the matching of function declaration and definitions
  - fix: research button doesn't show find in files dialog
  - enhancement: add project template for libmysqlclient(libmariadbclient)
  - enhancement: add libmysqlclient to the x86-64 version gcc in distribution
  - enhancement: select and delete multiple watches
  - enhancement: add project templates for tcp server / tcp client
  - enhancement: only show function tips when cursor is after ',' or '('.
  - enhancement: when auto complete function names, only append '(' if before identifier or "/'
  - update highconstrast icon set
  - fix: index of the longest line not correctly updated when insert/delete multiple lines ( which will cause selection errors)
  
Red Panda C++ Version 1.0.4
  - fix: hide function tips, when move or resize the main window
  - enhancement: add help link for regular expression in search dialog
  - enhancement: remember current problem set's filename
  - enhancement: F1 shorcut opens offcial website
  - enhancement: don't auto complete '(', if the next non-space char is '(' or ident char
  - enhancement: if a project's unit encoding is the same with project's encoding, don't save its encoding
  - fix: files will be saved to default encoding inspite of its original encoding
  - fix: parenthesis skip doesn't work when editing non-c/c++ files
  - enhancement: prefer local headers over system headers when complete #include header path
  - fix: tab/shift+tab not correctly handled in options dialog's code template page
  - enhancement: batch set cases ( in problem case table's context menu )
  - enhancement: add Portugese translation
  - fix: crash when eval statements like "fsm::stack fsm;"
  - enhancement: add Traditional Chinese translation
  - fix: index of the longest line not correctly updated ( which will cause selection errors)
  - fix: scroll bar not correctly updated when collapse/uncollapse folders
  - fix: parse error for definition of functions whose return type is pointer
  - enhancement: add library in project option dialog's compile options page

Red Panda C++ Version 1.0.3
  - fix: when oj problem grabbed by competitive companion received,
    the app is restored to normal state, no matter it's current state.
  - enhancement: input shortcut in the option dialog's general -> shortcut page by pressing keys.
  - enhancement: shift+ctrl+down/up to move currenlt selection lines up / down
  - fix: can't compile under linux
  - enhancement: support Devcie Pixel Ratio ( for linux )
  - fix: crash when editing txt file and input symbol at the beginning of a line
  - fix: ctrl+shift+end doesn't select
  - fix: don't show tips in the editor, when selecting by mouse
  - fix: auto syntax check doesn't work for new files
  - change: don't auto jump to the first syntax error location when compile
  - enhancement: don't show folders that doesn't contain files in the project view
  - enhancement: redesigned new project unit dialog
  - fix: some dialog's icon not correctly set
  - fix: can't build project that has source files in subfolders
  - fix: can't build project that has custom object folder
  - fix: buttons in the project option dialog's output page don't work
  - fix: don't add non-project header files to makefile's object rules
  - change: add glm library in the integrated gcc

Red Panda C++ Version 1.0.2
  - enhancement: press tab in column mode won't exit column mode
  - enhancement: refine behavior of undo input space char
  - enhancement: better display when input with IM under column mode
  - enhancement: better display current lines under column mode
  - change: test to use utf-8 as the default encoding (prepare to use libclang to implement parser)
  - fix: auto syntax check fail, if the file is not gbk and includes files encoded with utf8
  - enhancement: timeout for problem case test
  - enhancement: slightly reduce start up time
  - enhancement: use icon to indicate missing project files in the project view
  - enhancement: display problem case running time 
  - enhancement: set problem case input/expected output file
  - enhancement: auto position cursor in expected with output's cursor
  - enhancement: display line number in problem case's input/output/expected input controls
  - enhancement: only tag the first inconstantency when running problem case, to greatly reduce compare & display time
  - fix: can't stop a freeze program that has stdin redirected.
  - enhancement: context menu for problem cases table 
  - fix: error in auto generate makefile under linux
  - fix: when open a project, and it's missing compiler set getten reset, it's modification flag is not correctly set.
  - fix: vector vars can't be expanded in the watch panel
  - change: use qt's mingw 8.1 (32bit) and 11.2 (64bit) in distributions, to provide better compatibility with simplified chinese windows.
  - fix: crash when rename an openned file, and choose "no" when ask if keep the editor open
  - change: only auto complete symbol '(' when at line end, or there are spaces or right ')' '}' ']'after it
  - fix: mouse drag may fail when start drag at the right half of the selection's last character

Red Panda C++ Version 1.0.1
  - fix: only convert project icon file when it's filename doesn't end with ".ico"
  - fix: hide function tip when scroll
  - fix: short cut for goto definition/declaration doesn't work
  - enhancement: press alt to switch to column selection mode while selection by mouse dragging in editor
  - fix: order for parameters generated by auto link may not correct
  - fix: corresponding '>' not correctly removed when deleting '<' in #include line 
  - enhancement: shortcut for goto definition/declaration
  - change: ctrl+click symbol will goto definition, instead of got declaration
  - fix: when size of undo items is greater than the limit, old items should be poped in group
  - enhancement: max undo size in option dialog's editor->misc tab
  - fix: when editor font is too small, fold signs on the gutter are not correctly displayed
  - fix: expand fold signs on the gutter are not correct
  - enhancement: auto restore mainwindow when open files in one instance
  - fix: the problem & problem set panel can't be correctly , if problem set is enabled
  - fix: disable code completion doesn't correctly disable project parser
  - enhancement: slightly reduce memory usage for code parser
  - enhancement: switch capslock won't cancel code completion
  - enhancement: double click on item in code completion list will use it to complete
  - fix: goto declaration by ctrl+click will incorrectly select contents
  - fix: input may cause error, if selection in column mode and begin/end at the same column
  - enhancement: draw cursor for column mode
  - enahcnement: edit/delete in multiline ( column mode), press ese to exit

Red Panda C++ Version 1.0.0
  - fix: calculation for code snippets's tab stop positions is not correct
  - fix: Refresh files view shouldn'tchange open/save dialog's default folder
  - enhancement: "locate in files view" will request user's confirmation when change the working folder
  - enhancement: adjust tab order in the find dialog
  - enhancement: highlight hits in the find panel's result list
  - enhancement: optimize startup time
  - fix: batch replace in file doesn't respect item check states in the find panel
  - enhancement: option for default file encoding in "option" dialog's "editor"->"misc" tab
  - enhancement: auto detect "gbk" encoding when running in zh_CN locale under Linux
  - enhancement: disable encoding submenu when editor closed
  - enhancement: clear infos in the status bar when editor closed
  - fix: wrong selection when drag & dropped in editor
  - enhancement: toggle block comment
  - fix: syntax color of #include header filenames not correct
  - enhancement: disable "code completion" will disable enhanced syntax highlight
  - enhancement: match bracket
  - enhancement: **Linux** convert to "gbk"/"gb18030" encodings when run under "zh_CN" locale
  - fix: when no selection, copy/cut should auto select whole line with the line break
  - fix: redo cut with no selection while make whole line selected
  - fix: correctly reset caret when redo cut with no selection
  - enhancement: close editor when middle button clicked on it's title tab
  - fix: error when insert text in column mode 
  - fix: error when delete contents in column mode on lines that has wide-chars
  - fix: error when create folder in files view
  - fix: "ok" button should be disabled when no template selected in new project dialog
  - fix: saveas an openned project file shouldn't be treated as rename
  - enhancement: auto add parentheis when complete function like MARCOs
  - fix: wrong font size of exported RTF file 
  - fix: correct tokenize statements like "using ::memcpy";
  - fix: wrong font size of exported HTML file 
  - fix: parse error in avxintrin.h 
  - fix: switch disassembly mode doesn't update contents
  - fix: if there is a Red Panda C++ process running program, other Red Panda C++ processes can't run program correctly.
  - enhancement: ctrl+enter insert a new line at the end of current line
  - enhancement: create file in files view
  - fix: hits in the search view not correctly displayed (overlapped with others)
  - enhancement: auto convert project icon to ico format
  - fix: correctly reparse modified project files when rename symbol
  - change: remove shortcuts for line/column mode

Red Panda C++ Version 0.14.5
  - fix: the "gnu c++ 20" option in compiler set options is wrong
  - enhancement: option "open files in the same red panda C++ instance", in options->environment->file associations
  - enhancement: hide unsupported files in files view
  - fix: can't correctly set break conditions
  - fix: crash when copy to non-c files
  - fix: fonts in cpu window is not correctly set, when dpi changed
  - enhancement: enable group undo
  - enhancement: add option "hide symbols start with underscore" and "hide synbols start with two underscore"
  - fix: can't rename project files that not openned in editor
  - enhancement: group undo will stop at spaces
  - fix: menu font size is wrong when dpi changed
  - enhancement: better processing of symbol completion
  - enhancement: better support of ligatures
  - enhancement: use the expression evaluation logic to handle "goto declaration"/"goto definition" 
  - enhancement: reduce startup time by about 1 second.
  - enhancement: add option "mouse selection/drag scroll speed" in the options dialog's "Editor" / "general" tab.
  - fix: the scroll speed of mouse selection/drag is too fast.
  - fix: the scroll behavior of mouse dragging on the editor's edge is not correct
  - fix: calculation of caret position is not in consistence.
  - fix: undo one symbol completion as a whole operation
  - fix: crash when open a project that contains custom folder
  - enhancement: symbol completion when editor has selection 
  - fix: save project's layout shouldn't modify the project file
  - enhancement: use expression processing in syntax highlighting for identifiers
  - fix: if a function's declaration can't be found, it will be wrongly highlighted as variable
  - change: "locate in files view" won't change the working folder, if current file is in subfolders of the working folder
  - enhancement: hide function tips, when input method is visible

Red Panda C++ Version 0.14.4
  - enhancement: git - log
  - fix: error in templates
  - enhancement: git - reset
  - fix: header completion error when header name contains '+'
  - enhancement: clear history in file -> recent menu
  - enhancement: close project in project view's context menu
  - enhancement: auto find compiler sets when run for the first time
  - enhancement: git - remotes
  - enhancement: rename "open folder" to "choose working folder"
  - enhancement: let user choose app theme when first run
  - enhancement: git - pull / push / fetch

Red Panda C++ Version 0.14.3
  - fix: wrong code completion font size, when screen dpi changed
  - enhancement: replace Files View Panel's path lineedit control with combo box
  - enhancement: custome icons for project view
  - fix: convert to encoding setting in compiler set option not correctly handled
  - change: rename "compile log" panel to "tools output"
  - fix: debug panel can't be correctly show/hide
  - enhancement: redesign tools output's context menu, add "clear" menu item
  - enhancement: tools -> git  in the options dialog
  - enhancement: auto detect git in PATH
  - enhancement: git - create repository
  - enhancement: git - add files
  - enhancement: git - commit
  - enhancement: git - restore
  - enhancement: git - branch / switch
  - enhancement: git - merge
  - fix: compiler set index not correctly saved, when remove compiler sets in options dialog
  - enhancement: when create a repository in a project, auto add it's files to the repository
  - enhancement: when add files to project, auto add it to git (if the project has a git repository)
  - enhancement: when save a file, and it's under files view's current folder,  auto add it to git (if it has a git repository)
  - enhancement: new file icons for high contrast icon set
  - fix: left and bottom panel size not correct when DPI changed
  - fix: icons in files view not changed, when icon set is changed
  

Red Panda C++ Version 0.14.2
  - enhancement: file system view mode for project
  - enhancement: remove / rename / create new folder in the files view
  - fix: crash when there are catch blocks in the upper most scope
  - fix: can't read project templates when path has non-ascii chars
  - fix: huge build size for c++ files

Red Panda C++ Version 0.14.1
  - enhancement: custom theme
  - fix: failed to show function tip, when there are parameters having '[' and ']'
  - enhancement: display localized theme name in the option dialog
  - enhancement: show custom theme folder in options dialog -> enviroment -> folders
  - enhancement: display localzed icon set name in the option dialog
  - enhancement: new sky blue icon set, contributed by Alan-CRL
  - enhancement: show caret at once, when edition finished
  - enhancement: new header dialog for project
  - enhancement: new contrast icon set, contributed by Alan-CRL
  - enhancement: new contrast theme, contributed by Alan-CRL
  - enhancement: theme now have default icon set
  - fix: wrong icons for file associations
  - fix: editor's font size set by ctrl+mouse wheel will be reset by open the option dialog
  - fix: actions not correctly disabled when compile
  - fix: can't differentiate disabled and enabled buttons, when using contrast icon set
  - fix: when running problem cases, the output textbox might be wrongly cleared.
  - fix: typo error in the parser
  - fix: typing after symbols like 'std::string' shouldn't show code completion suggestions

Red Panda C++ Version 0.14.0
  - enhancement: custom icon set ( in the configuration folder)
  - enhancement: show custom icon set folder in options -> enviroment -> folders 
  - enhancement: new class ( to project) wizard
  - enhancement: greatly speed up code completion 
  - fix: code folding calculation not correct when some codes are folded and editing after them
  - enhancement: code completion ui redesigned
  - fix: mainwindow action's short cut doesn't work,  if the action is not in menu or toolbar
  - fix: when run all cases for a problem, processing of output is slow

Red Panda C++ Version 0.13.4
  - fix: when copy comments, don't auto indent
  - enhancement: auto add a new line when press enter between '/*' and '*/'
  - fix: code completion popup won't show  members of 'this'
  - fix: can't show private & protected members of 'this'
  - fix: function name like 'A::B' is not correctly parsed
  - fix: static members are not correct shown after Classname + '::'
  - enhancement: show parameter tips for class constructors
  - enhancement: when there are tips showing, don't show mouse tips
  - enhancement: setting non-ascii font for editors
  - enhancement: correct handle windows dpi change event
  - enhancement: code completion find words with char in the middle

Red Panda C++ Version 0.13.3
  - enhancement: restore editor position after rename symbol
  - enhancement: restore editor position after reformat code
  - fix: If project's compiler set is not the same with the default compiler set, parser for the project doesn't use the project's compiler set
  - fix: If project's compiler set is not the same with the default compiler set, auto openned project's file will use wrong compiler set to do syntax check.
  - change: symbols that exactly match are sorted to the front in the code suggestion popup list
  - fix: symbols defind locally should be sorted to the front in the code suggestion popup list
  - fix: when show function tips, can't correctly calcuate the current position in the function param list
  - fix: app will become very slow when processing very long lines.
  - enhancement: If console pauser doesn't exist, warn and stop running programs.
  - fix: app crash when ctrl+click on a #include statement that point to a directory instead of header file.
  - fix: ctrl+click on the enum value will jump to the wrong line in it's definition file
  - fix: line info in the mouse tip of statement not correct
  - fix: editor crash when no highlighter is assigned (the editing file is a not c/cpp source file);
  - fix: ')' not correctly skip in the editor when no highlighter is assigned (the editing file is a not c/cpp source file);
  - fix: Undo in the editor will lose line indents when no highlighter is assigned (the editing file is a not c/cpp source file);
  - enhancement: highlighter for GLSL (OpenGL Shading Language)
  - add a new template for raylib shader apps
  - fix: project files' charset settings doesn't work correctly
  - enhancement: add exec charset option to compiler set settings
  - enhancement: delete to word begin /delete to word end
  - fix: when open a file, all blank lines's indents are removed.
  - fix: indent lines displayed at wrong position, when there are folded lines
  - fix: if editor's active line color is disabled, caret's position may not be correct redrawn
  - fix: insert code snippets will crash, if current compiler set's include dir list is not empty and lib dir list is empty
  - fix: search around option can't be disabled
  - enhancement: show a confirm dialog when search/replace around
  - enhancement: auto zoom ui when screen's zoom factor changed (windows)
  - enhancement: parser not called when open a file, if option "clean parser symbols when hidden" is turned on.
  
Red Panda C++ Version 0.13.2
  - fix: "delete and exit" button in the environtment / folder option page doesn't work correctly 
  - fix: crash when closing the options dialog under Ubuntu 20.04 LTS ( no memory leak now)
  - enhancement: can add non-code file in templates
  - enhancement: if there's no selection when copy/cut, select currect line by default
  - enhancement: support ligatures in fonts like fira code ( disabled by default, can be turned on in options dialog's editor font page)
  - enhancement: add "minimum id length required to show code completion" to the options dialog's editor code completion page
  - enhancement: modify values in the memory view while debugging
  - enhancement: auto update watch, local and memory view after expression evaluated
  - enhancement: auto update watch, local and memory view after memory modified
  - enhancement: modify values in the watch view by double click
  - fix: crash when refactor symbol and cursor is at the end of the identifier
  - fix: refactor symbol doesn't work for 1-length identifiers
  - enhancement: redirect stdio to a file while debugging ( must use gdb server mode to debug)
  - fix: parser can't correctly handle variable definitions that don't have spaces like 'int*x';
  - fix: parser can't correctly handle function parameters like 'int *x' 
  - fix: caret dispears when at '\t' under Windows  7
  - enhancement: ctrl+up/down scrolls in the editor
  - enhancement: add "wrap around" option to find/replace
  - fix: project's icon setting is not correctly saved
  - fix: project's type setting won't be saved
  - fix: If project's compiler set is not the same with the default compiler set, auto openned project's file will use wrong compiler set to do syntax check.
  - fix: open a project file through "File"->"Open" will not correctly connect it with the project internally
  - fix: wrong project program directory parameter is sent to the debugger
  - enhancement: better behavior of mouse tips
  - fix: in linux, projects no need of winres to be built

Red Panda C++ Version 0.13.1
 - enhancement: support localization info in project templates
 - change: template / project files use utf-8 encoding instead of ANSI
 - fix: .rc file shouldn't be syntax checked
 - enhancement: auto save/restore size of the new project dialog
 - fix: new project dialog's tab bar should fill all empty spaces
 - enhancement: add raylib to autolinks
 - enhancement: distribute raylib with integrated gcc

Red Panda C++ Version 0.12.7
 - change: make current build system follow FHS specifications
 - fix: crash when close settings dialog in Ubuntu 20.04 (but we'll leak memory now...)
 - enhancement: add raylib.h to autolink
 - fix: shouldn't generate default autolink settings in linux
 - fix: shouldn't auto add /bin/gcc to compiler sets
 - fix: if a dir duplicates in PATH, don't add it to compiler sets repeatedly
 - enhancement: add "--sanitize=address" to compile option in the Debug compiler set in Linux 
 - enhancement: auto sort files in the project view

Red Panda C++ Version 0.12.6
 - fix: heartbeat for gdb server async command shouldn't disable actions
 - fix: problem cases doesn't use svg icons
 - fix: problem's title info not updated after running cases 
 - enhancement: open the corresponding source file from problem's context menu
 - fix: debugger's "continue" button not correctly disabled
 - change: use libicu instead of ConvertUTF.c under Linux
 - change depends to qt 5.12 instead of 5.15

Red Panda C++ Version 0.12.5
 - fix: compile error in linux
 - fix: can't receive gdb async output for commands
 - fix: can't reformat code
 - enhancement: add option for setting astyle path
 - fix: wrong file wildcard (*.*) in linux
 - fix: open terminal in linux
 - fix: wrong executable filename for source files in linux
 - enhancement: console pauser for linux 
 - enhancement: redirect input to program in linux
 - enhancement: detach pausing console window
 - rename to Red Pand C++

Version 0.12.4 For Dev-C++ 7 Beta
 - change: add copyright infos to each source file
 - fix: watch and local infos not updated when changing current frame in the call stack panel
 - enhancement: pause the debugging program (The debugger should work under gdb server mode, which is turned off by default in windows)

Version 0.12.3 For Dev-C++ 7 Beta
 - enhancement: basic linux compatibility
 - enhancement: debug with gdb server

Version 0.12.2 For Dev-C++ 7 Beta
 - enhancement: auto find compiler sets in the PATH 
 - change: path for iconsets
 - enhancement: select icon sets in options dialog ( but we  have only 1 icon set now...)

Version 0.12.1 For Dev-C++ 7 Beta
 - fix: error when drag&drop in editors

Version 0.12.0 For Dev-C++ 7 Beta
 - enhancement: enable run/debug/compile when console program finished but pausing.

Version 0.11.5 For Dev-C++ 7 Beta
 - fix: step into instruction and step over instruction not correctly disabled when cpu dialog is created
 - enhancement: icons in all dialogs auto change size with fonts 
 - enhancement: save/restore sizes of CPU dialog and settings dialog

Version 0.11.4 For Dev-C++ 7 Beta
 - fix: compiler set's custom link parameters  not used when compiling
 - fix: code completion doesn't work when input inside () or []
 - fix: auto indent processing error when input '{' in the middle of if statement
 - fix: left and right gutter offset settings not  correctly saved
 - fix: symbol completion for '<>' in the preprocessor line not work
 - enhancement: new svg icons set
 - enhancement: the size of icons in the main window zooms with font size

Version 0.11.3 For Dev-C++ 7 Beta
 - fix: use pixel size for fonts, to fit different dpi in multiple displays
 - enhancement: use the new expression parser to parse info for tips
 - enhancement: better highlight processing for preprocess directives 
 - enhancement: use the new expression parser to implement rename symbol
 - fix: rename symbol shouldn't remove empty lines

Version 0.11.2 For Dev-C++ 7 Beta
 - fix: button "run all problem cases" not disabled when compiling or debugging
 - enhancement: set font for problem case input/output textedits
 - enhancement: when run program with problem cases, updates output immediately (note: stdout of the program running with problem cases is fully buffered,
 so we need to fflush after each time output to stdout, or use setbuf(stdout,NULL) to turn the buffer off)
 - fix: current line of the disassembly in the cpu window not correctly setted
 - enhancement: add "step into one machine instruction" and "step over one machine instruction" in the cpu window
 - fix: can't correctly set TDM-GCC compiler
 - fix: auto add 32-bit compiler sets for TDM64-GCC

Version 0.11.1 For Dev-C++ 7 Beta
 - enhancement: Problem's test case shouldn't accept rich text inputs
 - enhancement: recalc layout info for code editors when dpi changed

Version 0.11.0 For Dev-C++ 7 Beta
 - enhancement: redesign the expression parser for code completion
 - fix: "make as default language" option in the project wizard doesn't work
 - fix: "ake as default language" option in the project wizard doesn't work
 - fix: typo errors in settings dialog
 - enhancement: console pauser clears STDIN buffer before show "press any key to continue..."
 - fix: path in macros should use system's path separator
 - fix: custom tools doesn't work
 - enhancement: add a demo for custom tool 

Version 0.10.4 For Dev-C++ 7 Beta
 - fix: can't correctly undo/redo indent 
 - fix: can't correctly undo/redo unindent
 - change: press tab when there are selections will do indent
 - change: press shift+tab when there are selections will do unindent
 - enhancement: press home will switch between begin of line and the position of fisrt non-space char
 - enhancement: press end will switch between end of line and the position of last non-space char 
 - enhancement: use "Microsoft Yahei" as the default UI font whe running in Simplified Chinese Windows

Version 0.10.3 For Dev-C++ 7 Beta
 - enhancement: treat files ended with ".C" or ".CPP"  as C++ files
 - enhancement: add option "ignore spaces when validating problem cases" to the "Executor"/"Problem Set" option tab.

Version 0.10.2 For Dev-C++ 7 Beta
 - fix: select by mouse can't correctly set mouse's column position
 - fix: dragging out of the editor and back will cause error
 - fix: dragging text from lines in the front to lines back will cause error
 - fix: dragging text onto itself should do nothing
 - fix：license info in the about dialog should be readonly
 - enhancement: change project name in the project view

Version 0.10.1 For Dev-C++ 7 Beta
 - fix: can't correctly expand watch expression that has spaces in it
 - fix: can't correctly display stl containers in watch
 - fix: the last line in the debug console is not correctly displayed
 - enhancement: scroll while dragging text in the editor
 - fix: dragging out of the editor shouldn't reset the caret back 

Version 0.10.0 For Dev-C++ 7 Beta
 - enhancement: use gdb/mi interface to  communicate with gdb debug session
 - enhancement: better display of watch vars
 - fix: project's modified flag not cleared after saved

Version 0.9.4 For Dev-C++ 7 Beta
 - fix: code formatter's "indent type" option not correctly saved

Version 0.9.3 For Dev-C++ 7 Beta
 - fix: the count in the title of issues view isn't correct
 - fix: columns calculation not correct when paint lines containing chinese characters
 - fix: restore caret position after reformat code
 - enhancement: ask user to rebuild project, when run/debug the project and it has been modified
 - fix: correct set the enabled state of "delete line"/"insert line"/"delete word"/"delete to BOL"/"delete to EOL" menu items
 - fix: undo "delete word"/"delete to BOL"/"delete to EOL" correct reset caret position

Version 0.9.2 For Dev-C++ 7 Beta
 - fix: gutter of the disassembly code control in the cpu info dialog is grayed
 - fix: problem set & problem views not correctly hidden when disabled in the executor / problem set options 
 - fix: executor / problem set options not correctly saved
 - fix: option "Move caret to the first non-space char in the current line when press HOME key" dosen't work fine.
 - fix: ctrl+left can't correctly move to the beginning of the last word
 - enhancement: add "delete line"/"duplicate line"/"delete word"/"delete to EOL"/"delete to BOL" in the edit menu
 - fix: crash when run "Project" / "Clean Make files"
 - fix: when make project and del non-existing files, shouldn't show error messages

Version 0.9.1 For Dev-C++ 7 Beta
 - enhancement: code completion suggestion for "__func__" variable
 - fix: ide failed to start, if there are errors in the compiler set settings
 - fix: numpad's enter key doesn't work
 - enhancement: code completion suggestion for phrase after long/short/signed/unsigned
 - enhancement: save/load default projects folder
 - enhancement: add editor general options "highlight current word" and "highlight matching braces"

Version 0.9.0 For Dev-C++ 7 Beta
 - fix: control keys in the numpad doesn't work in the editor
 - fix: project layout infos are wrongly saved to registry 
 - fix: project layout infos are not correctly saved/loaded

Version 0.8.11 For Dev-C++ 7 Beta
 - fix: text color for cpu info dialog not correctly setted

Version 0.8.10 For Dev-C++ 7 Beta
 - fix: Shouldn't update auto link settings, if the header name to be modified is unchanged
 - fix: add unit to project not correctly set new unit file's encoding
 - fix: correctly set encoding for the new added project unit file
 - fix: if there's a project openned, new file should ask user if he want to add the new file to the project
 - fix: when adding a file openned in the editor to the project, properties of it are not correctly setted.
 - enhancement: when remove a file from the project, also ask if user want to remove it from disk
 - fix: double click a project's .dev file in the Files panel should load the project

Version 0.8.9 For Dev-C++ 7 Beta
 - fix: text color of labels in statusbar not correctly updated when change theme

Version 0.8.8 For Dev-C++ 7 Beta
 - enhancement: drag & drop text in the editor
 - enhancement: auto calcuate caret line size basing on font size
 - enhancement: shift+mouse wheel to scroll horizontally 
 - fix: greatly reduces paste time 
 - fix: auto indent shouldn't use preprocessor's indent to calculate 
 - fix: option "don't add leading zeros to line numbers" not work
 - fix: "collapse all" and "uncollapse all" doesn't work

Version 0.8.7 For Dev-C++ 7 Beta
 - enhancement: auto indent line to column 1 when enter '#' at beginning of line
 - fix: when enter '{' or '}' at beginning of line, auto indent will remove all contents of the line
 - fix: auto indent should be turned off when reformat code
 - fix: auto indent should be turned off when replace in code 

Version 0.8.6 For Dev-C++ 7 Beta
 - enhancement: greatly reduces memory usage for symbol parsing ( memory needed for bits/stdc++.h reduced from 150m+ to 80m+)
 - fix: currect compiler set not correctly updated when switch between normal file and project file
 - fix: editor auto save settings not saved and applied
 - fix: only auto save files that has new modifications 
 - fix: correctly auto save files with it's own name

Version 0.8.5 For Dev-C++ 7 Beta
 - enhancement: use lighter color to draw menu seperators
 - enhancement: differentiate selected and unselected tab bars

Version 0.8.4 For Dev-C++ 7 Beta
 - enhancement: auto save/load the default open folder in the configuration file
 - fix: shouldn't auto add '()' when char succeeding the completed function name is '('
 - fix: can't show code completion popup if symbol is proceed with an operator '~' ( and it's not a destructor)
 - fix: can't show code completion popup when define MACRO
 - fix: can't debug files with chinese characters in the path

Version 0.8.3 For Dev-C++ 7 Beta
 - enhancement: View menu
 - enhancement: hide/show statusbar
 - enhancement: hide/show left/bottom tool window bars
 - enhancement: hide/show individual left/bottom tool window

Version 0.8.2 For Dev-C++ 7 Beta
 - fix: highlighter can't correctly find the end of ANSI C-style Comments
 - enhancement: add default color scheme to themes. Change theme option will change color scheme too.
 - fix: when changing options in the option dialog's color scheme panle, color of the demo editor won't be not correctly updated
 - enhancement: auto clear parsed symbols when the editor is hidden ( to reduce memory usage of un-active editors)
 - fix: when inputing in the editor, correctly set the position of the input method panel
 - fix: correctly display watch & local variable names when debugging

Version 0.8.1 For Dev-C++ 7 Beta
 - fix: ConsolePaurser.exe only exits when press ENTER
 - fix: input/output/expected textedit in the problem view shouldn't autowrap lines
 - fix: Red Panda C++ will freeze when receiving contents from Competitve Companion in chrome/edge
 - enhancement: when problem from competitive companion received, activate RedPanda C++ if it's minimized.
 - enhancement: when problem from competitive companion received, show the problem and problem set views.
 - enhancement: set problem's answer source file 
 - enhancement: open the problem's answer source file in editor
 - fix: if the proceeding line ends with ':' in comments, current line should not indent
 - enhancement: right click the problem set name label to rename it
 - change: memory view and locals view use debug console's font settings
 - fix: one line 'while' statement dosen't correctly indents
 - fix: line start with  '{' that follow an un-ended 'if'/'for' statement is not correctly un-indented
 - fix: multi-line comments indents calculation
 - fix: Installer should install the app in "program files", not "program files (x86)"
 - fix: symbol completion for '/*' not work
 - fix: javadoc-style docstring indents calculation
 - fix: indents calculation for the line succeeding "*/"

Version 0.8 For Dev-C++ 7 Beta
 - fix: find in the current file is not correcly saved in the search history
 - fix: hit info not correctly displayed in the search result view
 - fix: If find in files found no hits, search result view will not be shown.
 - fix: wront indents when paste one line content
 - fix: Results of "find symbol usage" in project not correctly set in the search result view
 - change: turn on gcc compiler's "-pipe" option by default, to use pipe instead of temp files in compiliation (and make the life of SSD longer)
 - fix: correctly save input histories for the find combo box in the Find dialog
 - fix: can't correctly test if it's not running in green mode

Version 0.7.8
 - enhancement: In problem view's output control, indicates which line is different with the expected
 - fix: current input/expected not correctly applied when save/run problem cases
 - fix: colors of the syntax issues view are not correctly set using the current color sheme
 - change: The error color of color scheme "vs code" 
 - add: "C Reference" in the help menu
 - fix: Custom editor colors shouldn't be tested for high contrast with the default background color
 - fix: Custom color settings not correctly displayed in the options widget
 - enhancement: add hit counts in the search result view
 - fix: editor actions' state not correctly updated after close editors.
 - fix: When replace in the editor, "Yes to All" and "No" button doesn't work correctly.
 - fix: crash when editing non-c/c++ files
 - enhancement: set the alpha value of scheme colors
 - enhancement: can use symbols' own foreground color to draw selection or the current line
 - enhancement: can use different colors to highlight the current word and the selections
 - enhancement: can set editor's default background / foreground color. They must be setted to make the custom color schemes correctly.
 - enhancement: can set the color for the current line's number in the gutter
 - all predefined color schemes updated. 
 - enhancement: check syntax/parse symbols when modifed and cursor's line changed.
 - enhancement: edit problem properties
 - enhancement: show problem description in the problem name lable's tooltip

Version 0.7.7
 - enhancement: Problem Set 
 - enhancement: Competitive Companion Support
 - change: "save" action will be enabled no matter contents in the current editor is modified or not
 - fix: focus not correctly set when the current editor is closed
 - fix: can't parse old c-style enum variable definition like "enum Test test;"
 - fix: remove the file change monitor if it's remove from the disk
 - fix: don't test if a file is writable before save to it (because qt can't do that test reliably).
 - fix: when search in project, files opened for search shouldn't be parsed for symbols.
 - fix: when search in project, the search history is not correctly updated.

Version 0.7.6
 - change: don't auto insert a new line when input an enter between '(' and ')' or between '[' and ']' (indent instead)
 - enhancement: the line containing '}' will use the indents of the matching '{' line, instead of just unindent one level
 - enhancement: the line containing 'public:' / 'private:' / 'protected:' / 'case *:' will use of indents of the surrounding '{' line, instead of just unindent one level
 - enhancement: correctly handle auto indents for multi-level embedding complex statements like 'for(...) if (...) printf();
 - change: Don't use 'pause' in the console pauser, in case of privilege problems.
 - enhancement: correctly handle auto indents for statement span many lines;
 - enhancment: only use colors have good contrasts with the background in the class browser and code completion suggestion window
 - fix: bottom and left panel properties not correctly saved when hiding the main window
 - fix: When debugging, if value of the variable pointed by the mouse cursor is too long, tooltip will fill the whole screen.

Version 0.7.5
 - enhancement: more accurate auto indent calculation
 - change: remove "add indent" option in the editor general options widget ( It's merged with "auto indent" option)
 - enhancement: auto insert a new line when input an enter between '(' and ')' or between '[' and ']'
 - fix: correctly updates cursor position when pasting from clipboard
 - enhancement: auto unindent when input protected: public: private: case *:
 - enhancement: can use PageDown / PageUp / Home / End to scroll in the auto completion popup

Version 0.7.4
 - fix: when debug a project, and have breakpoints that not in opened editors, dev-cpp will crash
 - fix: when a file is parsing in background, exit dev-cpp will crash
 - fix: "tab to spaces" option in the editor general options widget doesn't work
 - fix: when remove all breakpoints in the debug breakpoint view,  debug tags in the opened editors are not correctly updated.
 - change: when start debuging, show local view instead of the debug console.
 - update bundled compiler to msys2 mingw-w64 gcc 11.2 and gdb 10.2
 - update bundled xege to the lastest git build

Version 0.7.3
 - enhancement: icons in project view
 - fix: sometimes option widget will show confirm dialog even not changed
 - enhancement: only editor area will receive file drop events
 - enhancement: change project file's folder by drag and drop in the project view
 - enhancement: open project file by drag it to the editor area
 - fix: the "add bookmark" menu item is not correctly disabled on a bookmarked line
 - enhancement: "use utf8 by default" in editor's misc setting
 - fix: syntax issues not correctly cleared when the file was saved as another name.
 - enhancement: when running a program, redirect a data file to its stdin
 - fix: can't correctly handle '&&' and '||' in the #if directive (and correctly parse windows.h header file)
 - fix: crash when create an empty project
 - fix: syntax issues' filepath info not correct when build projects 
 - fix: compiler autolinks options widget don't show autolink infos
 - fix: autolink parameters are repeated when compile single files
 - enhancement: prompt for filename when create new project unit file
 - fix: options not correctly set when change compiler set in the project settings
 - change: reset compiler settings when change the project compiler set
 - enhancement: use project's compiler set type info to find a nearest system compiler set, when the project compiler set is not valid.
 - fix: toolbar's compiler set info not correctly updated when change it in the project settings dialog.

Version 0.7.2
 - fix: rainbow parenthesis stop functioning when change editor's general options
 - fix: issue count not correctly displayed when syntax check/compile finished
 - fix: function declaration's parameters not correctly parsed, if it have a definition which have different parameter names
 - fix: file path seperator used in the app is not unified, and cause errors somtimes.


Version 0.7.1
 - fix: can't add bookmark at a breakpoint line
 - fix: app name in the title bar not translated
 - use new app icon

Version 0.7.0
 - fix: Backspace still works in readonly mode
 - fix: save as file dialog's operation mode is not correct
 - enhancement: fill indents in the editor (Turned off by default)
 - enhancement: new file template
 - fix: when an editor is created, its caret will be displayed even it doesn't have focus
 - enhancement: set mouse wheel scroll speed in the editor general option tab ( 3 lines by default)
 - fix: don't highlight '#' with spaces preceeding it as error
 - fix: correctly handle integer with 'L' suffix in #if directives ( so <thread> can be correctly parsed )
 - enhancement: bookmark view
 - enhancement: autosave/load bookmarks
 - enhancement: autosave/load breakpoints 
 - enhancement: autosave/load watches
 - implement: files view
 - fix: app's title not update when editor closed

Version 0.6.8
 - enhancement: add link to cppreference in the help menu
 - fix: add mutex lock to prevent editor crash in rare conditions
 - fix: In the create project dialog, the browser button doesn't work
 - enhancement: use QStyle to implement the dark style, and better control of the style's look and feel 
 - enhancement: add link to EGE website, if locale is zh_CN

Version 0.6.7
 - fix: messages send to the gdb process's standard error are not received
 - adjust: the max value of the debug console's vertical scrollbar.
 - fix: shfit+click not correctly set selection's end
 - fix: ctrl+home/end not correctly set cursor to start/end of the editor
 - enhancement: click the encoding info in the statusbar will show encoding menu

Version 0.6.6
 - fix: crash when create new file
 - implement: two editor view

Version 0.6.5
 - implement: export as rtf / export as html
 - fix: the contents copied/exported are not correctly syntax colored
 - fix: stop execution if the source file is not compiled and user choose not to compile it
 - fix: not correctly parse gdb's output
 - fix: path not correctly setted for the debugger process
 - fix: indent line not correctly drawed
 - enhancement: use rainbox color to draw indent guide lines
 - implement: highlight matching brackets

Version 0.6.4
 - fix: code completion popup not show after '->' inputted
 - fix: font styles in the color scheme settings not in effect
 - fix: editor's font style shouldn't affect gutter's font style
 - change: enable copy as HTML by default
 - fix: unneeded empty lines when copy as HTML

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
