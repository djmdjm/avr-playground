package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"text/template"
)

const templateName = "uidata.c.t"

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

func checkMenus(menuList []menu) error {
	menus := map[string]*menu{}
	for i := range menuList {
		if menus[menuList[i].name] != nil {
			return fmt.Errorf("%v: duplicate menu %q, first instance at %v", menuList[i].loc, menuList[i].name, menus[menuList[i].name].loc)
		}
		menus[menuList[i].name] = &menuList[i]
	}
	// Check for missing submenus and ones that are unused.
	visited := map[string]bool{}
	for _, menu := range menus {
		for _, menuItem := range menu.menuItems {
			if menuItem.miType != miSubmenu && menuItem.miType != miSubmenuIntRange {
				continue
			}
			if menus[menuItem.definition] == nil {
				return fmt.Errorf("%v: missing menu %q", flag.Arg(0), menuItem.loc, menuItem.definition)
			}
			visited[menuItem.definition] = true
		}
	}
	for name, menu := range menus {
		if name != "" && !visited[name] {
			return fmt.Errorf("%v: menu %q not referenced", menu.loc, name)
		}
	}
	// XXX add cycle detection.
	return nil
}

type TemplateEditableItem struct {
	Name, Label string
}

type TemplateEditable struct {
	Display, Variable string
	Items             []TemplateEditableItem
	HasRange          bool
	RangeLow, RangeHi int64
}

type TemplateAskItem struct {
	Label, Action string
}

type TemplateAsk struct {
	Display string
	Items   []TemplateAskItem
}

type TemplateMenuItem struct {
	Name, Type string
}

type TemplateMenu struct {
	Name string
	Items     []TemplateMenuItem
	Asks      []TemplateAsk
	Editables []TemplateEditable
}

// TemplateUI is the data that is passed to the template to be rendered.
type TemplateUI struct {
	Menus []TemplateMenu
}

func prepareMenu(menu menu) (tm TemplateMenu) {
	tm.Name = menu.name
	for _, item := range menu.menuItems {
		switch item.miType {
		case miSubmenu:
		case miSubmenuIntRange:
		case miAsk:
		case miEditable:
		default:
			log.Fatalf("unknown menu type %v", item.miType)
		}
	}
	return
}

func prepare(menus []menu) (t TemplateUI) {
	for _, menu := range menus {
		t.Menus = append(t.Menus, prepareMenu(menu))
	}
	return
}

func main() {
	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatalf("no input file specified")
	}
	var tmpl = template.Must(template.ParseFiles(templateName))
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
		log.Fatalf("parse %v:%v", flag.Arg(0), err)
	}
	err = checkMenus(menus)
	if err != nil {
		log.Fatalf("check %v:%v", flag.Arg(0), err)
	}
	td := prepare(menus)
	//fmt.Printf("%#v\n", menus)
	err = tmpl.Execute(os.Stdout, td)
	if err != nil {
		log.Fatalf("render %v:%v", flag.Arg(0), err)
	}
}
