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
	"display":    _DISPLAY,
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
%token <loc>		_DISPLAY
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

// No need to define %left/%right operator precedence, since we don't have
// any operators.

%%

// Grammar. We do little processing here - mostly just generating a syntax
// tree.

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
|	menus menu
	{
		for _, menu := range $1 {
			if menu.name == $2.name {
				yylex.Error("duplicate menu name")
				break
			}
		}
		$$ = append($1, $2)
	}

menu:
	_MENU _ROOT '{' menu_items '}'
	{
		$$ = menu{name: ""}
		err := $$.mergeItems($4)
		if err != nil {
			yylex.Error(err.Error())
		}
	}
|	_MENU _QUOTED '{' menu_items '}'
	{
		menuName := $<stringv>2
		if menuName == "" {
			yylex.Error("empty menu name")
		} else {
			$$ = menu{name: menuName}
			err := $$.mergeItems($4)
			if err != nil {
				yylex.Error(err.Error())
			}
		}
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
		$$ = menuItem{
			miType: miVariable,
			variableInfo: variableInfo{
				varType: varExplicit,
				name: $<stringv>2,
			},
		}
	}
|	_VARIABLE _PREFIX _QUOTED
	{
		$$ = menuItem{
			miType: miVariable,
			variableInfo: variableInfo{
				varType: varPrefix,
				name: $<stringv>3,
			},
		}
	}
|	_VARIABLE _SUFFIX _QUOTED
	{
		$$ = menuItem{
			miType: miVariable,
			variableInfo: variableInfo{
				varType: varSuffix,
				name: $<stringv>3,
			},
		}
	}

submenu_item:
	_SUBMENU _LABEL _QUOTED
	{
		$$ = menuItem{
			miType: miSubmenu,
			label: $<stringv>3,
			definition: $<stringv>3,
		}
	}
|	_SUBMENU _LABEL _QUOTED _DEFINITION _QUOTED
	{
		$$ = menuItem{
			miType: miSubmenu,
			label: $<stringv>3,
			definition: $<stringv>5,
		}
	}
|	_SUBMENU _INDEX _MIN _INT _MAX _INT _OFFSET _INT _LABEL _QUOTED _DEFINITION _QUOTED
	{
		$$ = menuItem{
			miType: miSubmenuIntRange,
			label: $<stringv>10,
			definition: $<stringv>12,
			intRange: intRange{
				lo: $<intv>4,
				hi: $<intv>6,
				offset: $<intv>8,
			},
		}
	}
|	_SUBMENU _INDEX _MIN _INT _MAX _INT _OFFSET _INT _LABEL _QUOTED
	{
		$$ = menuItem{
			miType: miSubmenuIntRange,
			label: $<stringv>10,
			definition: $<stringv>10,
			intRange: intRange{
				lo: $<intv>4,
				hi: $<intv>6,
				offset: $<intv>8,
			},
		}
	}

ask_item:
	_ASK _QUOTED '{' ask_statements '}'
	{
		$$ = menuItem{miType: miAsk, label: $<stringv>2}
		err := $$.mergeAsk($4)
		if err != nil {
			yylex.Error(err.Error())
		}
	}

ask_statements:
	ask_statement			{ $$ = []askStatement{$1}}
|	ask_statements ask_statement	{ $$ = append($1, $2) }

ask_statement:
	_DISPLAY _QUOTED
	{
		$$ = askStatement{asType: askDisplay, label: $<stringv>2}
	}
|	_ANSWER _QUOTED _THEN _CALL _QUOTED
	{
		answer := $<stringv>2
		action := $<stringv>5
		if answer == "" || action == "" {
			yylex.Error("invalid answer argument")
		}
		$$ = askStatement{
			asType: askAnswer,
			label: answer,
			action: action,
		}
	}
|	_ANSWER _QUOTED _THEN _RETURN
	{
		answer := $<stringv>2
		if answer == "" {
			yylex.Error("invalid answer argument")
		}
		$$ = askStatement{
			asType: askAnswer,
			label: answer,
			action: "",
		}
	}

editable_item:
	_EDITABLE _QUOTED '{' editable_statements '}'
	{
		$$ = menuItem{miType: miEditable, label: $<stringv>2}
		err := $$.mergeEditable($4)
		if err != nil {
			yylex.Error(err.Error())
		}
	}

editable_statements:
	editable_statement			{ $$ = []editableStatement{$1} }
|	editable_statements editable_statement	{ $$ = append($1, $2) }

editable_statement:
	_DISPLAY _QUOTED {
		$$ = editableStatement{esType: esDisplay, label: $<stringv>2}
	}
|	_OPTION _DEFINE _QUOTED _LABEL _QUOTED
	{
		$$ = editableStatement{
			esType: esOption,
			definition: $<stringv>3,
			label: $<stringv>5,
		}
	}
|	_INTEGER _RANGE _INT _TO _INT
	{
		$$ = editableStatement{
			esType: esIntRange,
			intRange: intRange{
				lo: $<intv>3,
				hi: $<intv>5,
			},
		}
	}
|	_INTEGER _RANGE _INT _TO _INT _OFFSET _INT
	{
		$$ = editableStatement{
			esType: esIntRange,
			intRange: intRange{
				lo: $<intv>3,
				hi: $<intv>5,
				offset: $<intv>7,
			},
		}
	}
|	_VARIABLE _QUOTED
	{
		$$ = editableStatement{
			esType: esVariable,
			variableInfo: variableInfo{
				varType: varExplicit,
				name: $<stringv>2,
			},
		}
	}
|	_VARIABLE _SUFFIX _QUOTED
	{
		$$ = editableStatement{
			esType: esVariable,
			variableInfo: variableInfo{
				varType: varSuffix,
				name: $<stringv>3,
			},
		}
	}

%%
