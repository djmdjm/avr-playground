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
// include "constants.h"
// boilerplate file "license.txt"
// boilerplate "/* don't edit me! */"
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
// XXX only editables are integers; need strings too?
// }

%{
package main

import (
	"fmt"
)

// keywords holds the language keywords and their symbolic values.
var keywords = map[string]int{
	"answer":      _ANSWER,
	"ask":         _ASK,
	"boilerplate": _BOILERPLATE,
	"call":        _CALL,
	"define":      _DEFINE,
	"definition":  _DEFINITION,
	"display":     _DISPLAY,
	"editable":    _EDITABLE,
	"file":        _FILE,
	"get":         _GET,
	"index":       _INDEX,
	"include":     _INCLUDE,
	"integer":     _INTEGER,
	"label":       _LABEL,
	"max":         _MAX,
	"menu":        _MENU,
	"min":         _MIN,
	"option":      _OPTION,
	"range":       _RANGE,
	"return":      _RETURN,
	"root":        _ROOT,
	"set":         _SET,
	"submenu":     _SUBMENU,
	"to":          _TO,
	"variable":    _VARIABLE,
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
	statement		statement
	statements		[]statement
	boilerplate		boilerplate
	menu			menu
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
%token <loc>		_BOILERPLATE
%token <loc>		_CALL
%token <loc>		_DEFINE
%token <loc>		_DEFINITION
%token <loc>		_DISPLAY
%token <loc>		_FILE
%token <loc>		_EDITABLE
%token <loc>		_INDEX
%token <loc>		_INCLUDE
%token <loc>		_INTEGER
%token <loc>		_LABEL
%token <loc>		_MAX
%token <loc>		_MENU
%token <loc>		_MIN
%token <loc>		_OPTION
%token <loc>		_PREFIX
%token <loc>		_RANGE
%token <loc>		_RETURN
%token <loc>		_ROOT
%token <loc>		_SUBMENU
%token <loc>		_GET
%token <loc>		_SET
%token <loc>		_THEN    	// "=>"
%token <loc>		_TO
%token <loc>		_VARIABLE

// Parsed values.
%token <loc>		_FLOAT
%token <loc>		_INT
%token <loc>		_QUOTED

// Specify the union fields for parser productions. 'type' is a misnomer :/
%type <statement>		statement
%type <statements>		statements
%type <boilerplate>		boilerplate
%type <menu>			menu
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
	statements _EOF
	{
		yylex.(*state).statements = $1
		return 0
	}

// Top level of the file consists of one or more "menu" or
// "boilerplate" statements.
statements:
     	statement		{ $$ = []statement{$1} }
|	statements statement
	{
		$$ = append($1, $2)
	}

statement:
	menu
	{
		$$ = statement{loc: $<loc>1, sType: sMenu, menu: $1}
	}
|	boilerplate
	{
		$$ = statement{loc: $<loc>1, sType: sBoilerplate, boilerplate: $1}
	}

boilerplate:
	_INCLUDE _QUOTED
	{
		$$ = boilerplate{loc: $1, bType: bInclude, value: $<stringv>2}
	}
|	_BOILERPLATE _FILE _QUOTED
	{
		$$ = boilerplate{loc: $1, bType: bFile, value: $<stringv>3}
	}
|	_BOILERPLATE _QUOTED
	{
		$$ = boilerplate{loc: $1, bType: bText, value: $<stringv>2}
	}


menu:
	_MENU _ROOT '{' menu_items '}'
	{
		$$ = menu{loc: $1, name: "root"}
		err := $$.mergeItems($4)
		if err != nil {
			yylex.Error(err.Error())
		}
	}
|	_MENU _QUOTED '{' menu_items '}'
	{
		menuName := $<stringv>2
		if !isValidIdentifier(menuName) {
			yylex.Error("invalid menu name")
		} else {
			$$ = menu{loc: $1, name: menuName}
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
|	_VARIABLE _GET _QUOTED _SET _QUOTED
	{
		$$ = menuItem{
			loc: $1,
			miType: miVariable,
			variableInfo: variableInfo{
				get: $<stringv>3,
				set: $<stringv>5,
			},
		}
	}

submenu_item:
	_SUBMENU _LABEL _QUOTED
	{
		$$ = menuItem{
			loc: $1,
			miType: miSubmenu,
			label: $<stringv>3,
			definition: $<stringv>3,
		}
	}
|	_SUBMENU _LABEL _QUOTED _DEFINITION _QUOTED
	{
		$$ = menuItem{
			loc: $1,
			miType: miSubmenu,
			label: $<stringv>3,
			definition: $<stringv>5,
		}
	}
|	_SUBMENU _INDEX _MIN _INT _MAX _INT _LABEL _QUOTED _DEFINITION _QUOTED _SET _QUOTED
	{
		$$ = menuItem{
			loc: $1,
			miType: miSubmenuIntRange,
			label: $<stringv>8,
			definition: $<stringv>10,
			intRange: intRange{
				lo: $<intv>4,
				hi: $<intv>6,
			},
			variableInfo: variableInfo{
				set: $<stringv>12,
			},
		}
	}
|	_SUBMENU _INDEX _MIN _INT _MAX _INT _LABEL _QUOTED _SET _QUOTED
	{
		$$ = menuItem{
			loc: $1,
			miType: miSubmenuIntRange,
			label: $<stringv>8,
			definition: $<stringv>8,
			intRange: intRange{
				lo: $<intv>4,
				hi: $<intv>6,
			},
			variableInfo: variableInfo{
				set: $<stringv>10,
			},
		}
	}

ask_item:
	_ASK _QUOTED '{' ask_statements '}'
	{
		$$ = menuItem{loc: $1, miType: miAsk, label: $<stringv>2}
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
		$$ = askStatement{
			loc: $1,
			asType: askDisplay,
			label: $<stringv>2,
		}
	}
|	_ANSWER _QUOTED _THEN _CALL _QUOTED
	{
		answer := $<stringv>2
		action := $<stringv>5
		if answer == "" || action == "" {
			yylex.Error("invalid answer argument")
		}
		$$ = askStatement{
			loc: $1,
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
			loc: $1,
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
		$$ = editableStatement{
			loc: $1,
			esType: esDisplay,
			label: $<stringv>2,
		}
	}
|	_OPTION _DEFINE _QUOTED _LABEL _QUOTED
	{
		$$ = editableStatement{
			loc: $1,
			esType: esOption,
			definition: $<stringv>3,
			label: $<stringv>5,
		}
	}
|	_INTEGER _RANGE _INT _TO _INT
	{
		$$ = editableStatement{
			loc: $1,
			esType: esIntRange,
			intRange: intRange{
				lo: $<intv>3,
				hi: $<intv>5,
			},
		}
	}
|	_VARIABLE _GET _QUOTED _SET _QUOTED
	{
		$$ = editableStatement{
			loc: $1,
			esType: esVariable,
			variableInfo: variableInfo{
				get: $<stringv>3,
				set: $<stringv>5,
			},
		}
	}

%%

// Structures that constitute the parse tree.

type variableInfoType int
type menuItemType int
type askStatementType int
type editableStatementType int
type statementType int
type boilerplateType int

const (
	// Types for menuItems.
	_                       = iota
	miVariable menuItemType = iota
	miSubmenu
	miSubmenuIntRange
	miAsk
	miEditable
	// Types for askStatement
	_                           = iota
	askDisplay askStatementType = iota
	askAnswer
	// Types for editableStatement
	_                               = iota
	esDisplay editableStatementType = iota
	esOption
	esIntRange
	esVariable
	// Types for statement
	_                   = iota
	sMenu statementType = iota
	sBoilerplate
	// Types for boilerplate text
	_                     = iota
	bText boilerplateType = iota
	bInclude
	bFile
)

type variableInfo struct {
	get, set string
}

type intRange struct {
	lo, hi int64
}

type askStatement struct {
	loc    location
	asType askStatementType
	label  string
	action string
}

type editableStatement struct {
	loc          location
	esType       editableStatementType
	label        string
	definition   string
	intRange     intRange
	variableInfo variableInfo
}

type menuItem struct {
	loc          location
	miType       menuItemType
	variableInfo variableInfo        // For miVariable
	label        string              // For all but miVariable.
	display      string              // For all but miVariable.
	definition   string              // For miSubmenu and miSubmenuIntRange
	intRange     intRange            // For miSubmenuIntRange
	asks         []askStatement      // For miAsk
	options      []editableStatement // For miEditable

}

type menu struct {
	loc          location
	name         string
	variableInfo variableInfo
	menuItems    []menuItem
}

type boilerplate struct {
	loc   location
	bType boilerplateType
	value string
}

type statement struct {
	loc         location
	sType       statementType
	menu        menu
	boilerplate boilerplate
}

func (loc location) String() string {
	return fmt.Sprintf("%v:%v", loc.line, loc.char)
}
