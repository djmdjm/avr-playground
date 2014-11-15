// UI generator for small LCD screens.

// The language is simple:
//
// One or more "menu" items, which may contain "variable" statements (that
// specify the variable / variable prefix that is being edited at this level
// of menu), "editable" statements (that define editable fields and their
// allowed values) or "ask" statements defining multiple choice menus that
// call functions on being answered.
//
// An example:
//
// menu root {
//         variable prefix "config."
//         editable "mode" {
//                 variable suffix "mode"
//                 option define "MODE_CLEAN" label "clean"        
//                 option define "MODE_DRY" label "dry"        
//         }
//         ask "start" {
//                 answer "Y" => call "start"
//                 answer "N" => return
//         }
//         submenu "CONFIG"
// }
// menu "CONFIG" {
//         editable "channel" {
//                 variable suffix "channel"
//                 integer range 0 to 16
//         }
// XXX pre/postamble sections for boilerplate like licenses?
// }

%{
package main

// keywords holds the language keywords and their symbolic values.
var keywords = map[string]int{
	"answer":     _ANSWER,
	"ask":        _ASK,
	"call":       _CALL,
	"define":     _DEFINE,
	"definition": _DEFINITION,
	"editable":   _EDITABLE,
	"index":      _INDEX,
	"integer":    _INTEGER,
	"label":      _LABEL,
	"max":        _MAX,
	"menu":       _MENU,
	"min":        _MIN,
	"offset":     _OFFSET,
	"option":     _OPTION,
	"prefix":     _PREFIX,
	"range":      _RANGE,
	"return":     _RETURN,
	"root":       _ROOT,
	"submenu":    _SUBMENU,
	"suffix":     _SUFFIX,
	"to":         _TO,
	"variable":   _VARIABLE,
}

// location is a human-readable location in the lexer/parser input.
// NB. line starts at 1.
type location struct {
	line int // Line number.
	char int // Character number in line.
}

%}

// These fields end up in struct yySymType.
%union {
	// Input tokens
	token		string     // Verbatim input text.
	loc		location   // Human-readable location of token

	// Token contents.
	stringv		string     // String value (without quotes).
	intv    	int64      // Integer value.

	// Types for parser productions.
	menu			menu
	menus			[]menu
	menuItem		menuItem
	menuItems		[]menuItem
	askStatement		askStatement
	askStatements		[]askStatement
	editableStatement	editableStatement
	editableStatements	[]editableStatement
}

// Specify tokens coming from the lexer. In declarations, they are
// referred to by position: $1, $2, $3, etc. which yield the type
// defined here, but it is also possible to refer to specific fields
// using a syntax like $<stringv>1. 

// Error during lexing.
%token <loc>		_ERROR

// End-of-file.
%token <loc>		_EOF

// Block begin / end.
%token <loc>		'{'
%token <loc>		'}'

// Keywords.
%token <loc>		_ANSWER
%token <loc>		_ASK
%token <loc>		_CALL
%token <loc>		_DEFINE
%token <loc>		_DEFINITION
%token <loc>		_EDITABLE
%token <loc>		_INDEX
%token <loc>		_INTEGER
%token <loc>		_LABEL
%token <loc>		_MAX
%token <loc>		_MENU
%token <loc>		_MIN
%token <loc>		_OFFSET
%token <loc>		_OPTION
%token <loc>		_PREFIX
%token <loc>		_RANGE
%token <loc>		_RETURN
%token <loc>		_ROOT
%token <loc>		_SUBMENU
%token <loc>		_SUFFIX
%token <loc>		_THEN    	// "=>"
%token <loc>		_TO
%token <loc>		_VARIABLE

// Parsed values.
%token <loc>		_FLOAT
%token <loc>		_INT
%token <loc>		_QUOTED

// Specify the union fields for parser productions. 'type' is a misnomer :/
%type <menu>			menu
%type <menus>			menus
%type <menuItems>		menu_items
%type <menuItem>		menu_item ask_item submenu_item editable_item
%type <askStatements>		ask_statements
%type <askStatement>		ask_statement
%type <editableStatements>	editable_statements
%type <editableStatement>	editable_statement

// Operator precedence. We don't really have any operators though.

%left			_QUOTED _FLOAT _INT

%%

// Grammar.

// Whole file.
file:
	menus _EOF
	{
		yylex.(*state).menus = $1
		return 0
	}

// Top level of the file consists of one or more "menu" statements.
menus:
     	menu			{ $$ = []menu{$1} }
|	menus menu		{ $$ = append($1, $2) }

menu:
	_MENU _ROOT '{' menu_items '}'
	{
		// XXX
	}
|	_MENU _QUOTED '{' menu_items '}'
	{
		// XXX
	}

// Menu contents are one or more statements.
menu_items:
	menu_item		{ $$ = []menuItem{$1} }
|	menu_items menu_item	{ $$ = append($1, $2) }

// Menu statements are one of "variable", "ask", "submenu" or "editable".
menu_item:
	ask_item		{ $$ = $1 }
|	editable_item		{ $$ = $1 }
|	submenu_item		{ $$ = $1 }
|	_VARIABLE _QUOTED
	{
		// XXX
	}
|	_VARIABLE _PREFIX _QUOTED
	{
		// XXX
	}
|	_VARIABLE _SUFFIX _QUOTED
	{
		// XXX
	}

submenu_item:
	_SUBMENU _LABEL _QUOTED
	{
		// XXX
	}
|	_SUBMENU _LABEL _QUOTED _DEFINITION _QUOTED
	{
		// XXX
	}
|	_SUBMENU _INDEX _MIN _INT _MAX _INT _OFFSET _INT _DEFINITION _QUOTED
	{
		// XXX
	}

ask_item:
	_ASK _QUOTED '{' ask_statements '}'
	{
		// XXX
	}

ask_statements:
	ask_statement			{ $$ = []askStatement{$1}}
|	ask_statements ask_statement	{ $$ = append($1, $2) }

ask_statement:
	_LABEL _QUOTED
	{
		// XXX
	}
|	_ANSWER _QUOTED _THEN _CALL _QUOTED
	{
		// XXX
	}
|	_ANSWER _QUOTED _THEN _RETURN
	{
		// XXX
	}

editable_item:
	_EDITABLE _QUOTED '{' editable_statements '}'
	{
		// XXX
	}

editable_statements:
	editable_statement			{ $$ = []editableStatement{$1} }
|	editable_statements editable_statement	{ $$ = append($1, $2) }

editable_statement:
	_OPTION _DEFINE _QUOTED _LABEL _QUOTED
	{
		// XXX
	}
|	_INTEGER _RANGE _INT _TO _INT
	{
		// XXX
	}
|	_VARIABLE _QUOTED
	{
		// XXX
	}
|	_VARIABLE _SUFFIX _QUOTED
	{
		// XXX
	}

%%
