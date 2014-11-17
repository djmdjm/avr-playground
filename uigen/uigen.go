package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

type variableInfoType int
type menuItemType int
type askStatementType int
type editableStatementType int

const (
	// Types for variableInfo.
	varUnspecified = variableInfoType(0)
	varPrefix      = iota
	varSuffix      = iota
	varExplicit    = iota
	// Types for menuItems.
	miVariable        = menuItemType(1)
	miSubmenu         = iota
	miSubmenuIntRange = iota
	miAsk             = iota
	miEditable        = iota
	// Types for askStatement
	askDisplay = askStatementType(1)
	askAnswer  = iota
	// Types for editableStatement
	esDisplay  = editableStatementType(1)
	esOption   = iota
	esIntRange = iota
	esVariable = iota
)

type variableInfo struct {
	varType variableInfoType
	name    string
}

type menu struct {
	name         string
	variableInfo variableInfo
	menuItems    []menuItem
}

type intRange struct {
	lo, hi, offset int64
}

type askStatement struct {
	asType askStatementType
	label  string
	action string
}

type editableStatement struct {
	esType       editableStatementType
	label        string
	definition   string
	intRange     intRange
	variableInfo variableInfo
}

type menuItem struct {
	miType       menuItemType
	variableInfo variableInfo        // For miVariable
	label        string              // For all but miVariable.
	display      string              // For all but miVariable.
	definition   string              // For miSubmenu and miSubmenuIntRange
	intRange     intRange            // For miSubmenuIntRange
	asks         []askStatement      // For miAsk
	options      []editableStatement // For miEditable

}

func (m *menu) mergeItems(items []menuItem) error {
	for _, item := range items {
		if item.miType == miVariable {
			if m.variableInfo.varType != varUnspecified {
				return fmt.Errorf("variable information already specified for menu")
			}
			m.variableInfo = item.variableInfo
		} else {
			m.menuItems = append(m.menuItems, item)
		}
	}
	// Check for duplicates.
	labelNames := map[string]bool{}
	for _, item := range m.menuItems {
		if labelNames[item.label] {
			return fmt.Errorf("duplicate menu item %q", item.label)
		}
		labelNames[item.label] = true
	}
	return nil
}

func (mi *menuItem) mergeAsk(statements []askStatement) error {
	for _, statement := range statements {
		if statement.asType == askDisplay {
			if mi.display != "" {
				return fmt.Errorf("display already specified for ask statement")
			}
			mi.display = statement.label
		} else if statement.asType == askAnswer {
			mi.asks = append(mi.asks, statement)
		} else {
			return fmt.Errorf("unknown ask item %v", statement.asType)
		}
	}
	// Check for duplicates.
	labelNames := map[string]bool{}
	for _, statement := range statements {
		if labelNames[statement.label] {
			return fmt.Errorf("duplicate ask label %q", statement.label)
		}
		labelNames[statement.label] = true
	}
	return nil
}

func (mi *menuItem) mergeEditable(statements []editableStatement) error {
	for _, statement := range statements {
		if statement.esType == esVariable {
			if mi.variableInfo.varType != varUnspecified {
				return fmt.Errorf("variable information already specified for editable")
			}
			mi.variableInfo = statement.variableInfo
		} else if statement.esType == esDisplay {
			if mi.display != "" {
				return fmt.Errorf("display already specified for editable statement")
			}
			mi.display = statement.label
		} else if statement.esType == esOption || statement.esType == esIntRange {
			// XXX check syntax, for dupe, etc.
			mi.options = append(mi.options, statement)
		} else {
			return fmt.Errorf("unknown editable item %v", statement.esType)
		}
	}
	// Check for duplicates.
	labelNames := map[string]bool{}
	definitionNames := map[string]bool{}
	seenIntRange := false
	for _, statement := range statements {
		if statement.esType == esIntRange {
			if seenIntRange {
				return fmt.Errorf("multiple integer range options in editable statement")
			}
			seenIntRange = true
		} else if statement.esType == esOption {
			if labelNames[statement.label] {
				return fmt.Errorf("duplicate editable label %q", statement.label)
			}
			if definitionNames[statement.definition] {
				return fmt.Errorf("duplicate editable definition %q", statement.definition)
			}
			labelNames[statement.label] = true
			definitionNames[statement.definition] = true
		}
	}
	return nil
}

func main() {
	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatalf("no input file specified")
	}
	f, err := os.Open(flag.Arg(0))
	if err != nil {
		log.Fatalf("open %q: %v", flag.Arg(0), err)
	}
	text, err := ioutil.ReadAll(f)
	if err != nil {
		log.Fatalf("read %q: %v", flag.Arg(0), err)
	}
	f.Close()
	menus, err := parse(text)
	if err != nil {
		log.Fatalf("parse %q: %v", flag.Arg(0), err)
	}
	fmt.Printf("%#v\n", menus)
}
