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

const (
	// Types for variableInfo.
	varUnspecified = variableInfoType(0)
	varPrefix      = iota
	varSuffix      = iota
	varExplicit    = iota
	// Types for menuItems.
	menuItemVarInfo = menuItemType(1)
	menuItemYYY     = iota
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

type menuItem struct {
	miType       menuItemType
	variableInfo variableInfo // If miType == menuItemVarInfo
}

type askStatement struct {
}

type editableStatement struct {
}

func (m *menu) mergeItems(items []menuItem) error {
	for _, item := range items {
		if item.miType == menuItemVarInfo {
			if m.variableInfo.varType != varUnspecified {
				return fmt.Errorf("variable information already specified for menu")
			}
			m.variableInfo = item.variableInfo
		} else {
			// XXX check syntax, for dupe, etc.
			m.menuItems = append(m.menuItems, item)
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
