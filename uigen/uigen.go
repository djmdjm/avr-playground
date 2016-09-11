package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"text/template"
	"unicode"
)

//go:generate go tool yacc -o parse.go -p parser parse.y

var templatePath = flag.String("template", "uidata.c.t", "path to template")

func (m *menu) mergeItems(items []menuItem) error {
	for _, mi := range items {
		if mi.miType == miVariable {
			m.variableInfo = mi.variableInfo
		} else {
			m.menuItems = append(m.menuItems, mi)
		}
	}
	// Check for duplicates.
	labelNames := map[string]bool{}
	for _, mi := range m.menuItems {
		if labelNames[mi.label] {
			return fmt.Errorf("duplicate menu item %q", mi.label)
		}
		labelNames[mi.label] = true
	}
	return nil
}

func (mi *menuItem) mergeAsk(statements []askStatement) error {
	for _, as := range statements {
		if as.asType == askDisplay {
			if mi.display != "" {
				return fmt.Errorf("display already specified for ask statement")
			}
			mi.display = as.label
		} else if as.asType == askAnswer {
			mi.asks = append(mi.asks, as)
		} else {
			return fmt.Errorf("unknown ask item %v", as.asType)
		}
	}
	// Check for duplicates.
	labelNames := map[string]bool{}
	for _, as := range statements {
		if labelNames[as.label] {
			return fmt.Errorf("duplicate ask label %q", as.label)
		}
		labelNames[as.label] = true
	}
	return nil
}

func (mi *menuItem) mergeEditable(statements []editableStatement) error {
	for _, es := range statements {
		switch es.esType {
		case esVariable:
			mi.variableInfo = es.variableInfo
		case esDisplay:
			if mi.display != "" {
				return fmt.Errorf("display already specified for editable statement")
			}
			mi.display = es.label
		case esOption:
			// more syntax checks?
			mi.options = append(mi.options, es)
		case esIntRange:
			// range validity check?
			mi.intRange = es.intRange
		default:
			return fmt.Errorf("unknown editable item %v", es.esType)
		}
	}
	// Check for duplicates.
	labelNames := map[string]bool{}
	definitionNames := map[string]bool{}
	seenIntRange := false
	for _, es := range statements {
		switch es.esType {
		case esIntRange:
			if seenIntRange {
				return fmt.Errorf("multiple integer range options in editable statement")
			}
			seenIntRange = true
		case esOption:
			if labelNames[es.label] {
				return fmt.Errorf("duplicate editable label %q", es.label)
			}
			if definitionNames[es.definition] {
				return fmt.Errorf("duplicate editable definition %q", es.definition)
			}
			labelNames[es.label] = true
			definitionNames[es.definition] = true
		}
	}
	return nil
}

func checkStatements(statements []statement) error {
	menus := map[string]*menu{}
	for i, st := range statements {
		switch st.sType {
		case sBoilerplate:
			if len(st.boilerplate.value) == 0 {
				return fmt.Errorf("%v: empty boilerplate definition", st.loc)
			}
		case sMenu:
			if menus[st.menu.name] != nil {
				return fmt.Errorf("%v: duplicate menu %q, first instance at %v", st.loc, st.menu.name, menus[st.menu.name].loc)
			}
			menus[st.menu.name] = &statements[i].menu
		default:
			return fmt.Errorf("unknown statement type %v at %v", st.sType, st.loc)
		}
	}
	// Check for missing submenus and ones that are unused.
	if menus["root"] == nil {
		return fmt.Errorf("%v: missing root menu", flag.Arg(0))
	}
	visited := map[string]bool{}
	for _, menu := range menus {
		for _, mi := range menu.menuItems {
			if mi.miType != miSubmenu && mi.miType != miSubmenuIntRange {
				continue
			}
			child := menus[mi.definition]
			if child == nil {
				return fmt.Errorf("%v: missing menu %q", flag.Arg(0), mi.loc, mi.definition)
			}
			if child.parent != "" {
				return fmt.Errorf("%v: menu %q has multiple parents: %q, %q", flag.Arg(0), child.loc, child.parent, menu.name)
			}
			child.parent = menu.name
			visited[mi.definition] = true
		}
	}
	for name, menu := range menus {
		if name != "root" && !visited[name] {
			return fmt.Errorf("%v: menu %q not referenced", menu.loc, name)
		}
	}
	// XXX add cycle detection.
	return nil
}

type Boilerplate struct {
	Text string
}

type TemplateEditableItem struct {
	Name, Label, Definition string
}

type TemplateEditable struct {
	Name, Label       string
	Get, Set          string
	Every             bool
	Items             []TemplateEditableItem
	HasRange          bool
	RangeLow, RangeHi int64
}

type TemplateSubmenuRange struct {
	Name, Label, Definition string
	Set                     string
	RangeLow, RangeHi       int64
}

type TemplateAskItem struct {
	Name, Label, Action, ReturnMenu string
}

type TemplateAsk struct {
	Name, Label string
	Items       []TemplateAskItem
}

type TemplateMenuItem struct {
	Name, Label, Type string
}

type TemplateMenu struct {
	Label, Name, Parent string
	Items               []TemplateMenuItem
	Asks                []TemplateAsk
	Editables           []TemplateEditable
	SubmenuRanges       []TemplateSubmenuRange
}

// TemplateUI is the data that is passed to the template to be rendered.
type TemplateUI struct {
	Menus       []TemplateMenu
	Boilerplate []Boilerplate
}

// makeIdentifer tries to turn an arbitrary string into a valid C identifer.
func makeIdentifier(format string, a ...interface{}) string {
	s := fmt.Sprintf(format, a...)
	var r []rune
	for _, c := range s {
		c = unicode.ToLower(c)
		if isValidIdentifierChar(c) {
			r = append(r, c)
			continue
		}
		if unicode.IsSpace(c) || unicode.IsSymbol(c) {
			r = append(r, '_')
			continue
		}
		// Other characters are simply omitted.
	}
	return string(r)
}

func prepareMenu(menu menu) (tm TemplateMenu, err error) {
	tm.Name = menu.name
	tm.Parent = menu.parent
	for i, mi := range menu.menuItems {
		ident := fmt.Sprintf("%v_%v", tm.Name, i)
		switch mi.miType {
		case miSubmenu:
			tmi := TemplateMenuItem{
				Label: mi.label,
				Type:  "submenu",
				Name:  mi.definition,
			}
			tm.Items = append(tm.Items, tmi)
		case miSubmenuIntRange:
			tsr := TemplateSubmenuRange{
				Label:      mi.label,
				Definition: mi.definition,
				Name:       makeIdentifier("%v_submenu_range_%v", ident, mi.label),
				Set:        mi.variableInfo.set,
				RangeLow:   mi.intRange.lo,
				RangeHi:    mi.intRange.hi,
			}
			tmi := TemplateMenuItem{
				Label: mi.label,
				Type:  "submenurange",
				Name:  makeIdentifier("%v_submenu_range_%v", ident, mi.label),
			}
			tm.SubmenuRanges = append(tm.SubmenuRanges, tsr)
			tm.Items = append(tm.Items, tmi)
		case miAsk:
			ta := TemplateAsk{
				Label: mi.label,
				Name:  makeIdentifier("%v_ask_%v", ident, mi.label),
			}
			tmi := TemplateMenuItem{
				Label: mi.label,
				Type:  "ask",
				Name:  ta.Name,
			}
			for j, as := range mi.asks {
				if as.asType != askAnswer {
					err = fmt.Errorf("Unexpected ask type")
					return
				}
				tai := TemplateAskItem{
					Name:       makeIdentifier("%v_%v_%v", ta.Name, j, as.label),
					Label:      as.label,
					Action:     as.action,
					ReturnMenu: tm.Name,
				}
				ta.Items = append(ta.Items, tai)
			}
			tm.Asks = append(tm.Asks, ta)
			tm.Items = append(tm.Items, tmi)
		case miEditable:
			te := TemplateEditable{
				Label:    mi.label,
				Name:     makeIdentifier("%v_editable_%v", ident, mi.label),
				RangeLow: mi.intRange.lo,
				RangeHi:  mi.intRange.hi,
				HasRange: mi.intRange.lo < mi.intRange.hi,
				Get:      mi.variableInfo.get,
				Set:      mi.variableInfo.set,
				Every:    mi.variableInfo.every,
			}
			tmi := TemplateMenuItem{
				Label: mi.label,
				Type:  "editable",
				Name:  te.Name,
			}
			for j, es := range mi.options {
				if es.esType != esOption {
					err = fmt.Errorf("Unexpected editable item type %v at %v", es.esType, es.loc)
					return
				}
				tei := TemplateEditableItem{
					Name:       makeIdentifier("%v_%v_%v", te.Name, j, es.definition),
					Label:      es.label,
					Definition: es.definition,
				}
				te.Items = append(te.Items, tei)
			}
			tm.Editables = append(tm.Editables, te)
			tm.Items = append(tm.Items, tmi)
		default:
			log.Fatalf("unknown menu type %v", mi.miType)
		}
	}
	return
}

func prepareBoilerplate(bp boilerplate) (ret Boilerplate, err error) {
	switch bp.bType {
	case bFile:
		var contents []byte
		contents, err = ioutil.ReadFile(bp.value)
		if err != nil {
			return
		}
		ret = Boilerplate{Text: string(contents)}
	case bInclude:
		if bp.value[0] == '<' {
			ret = Boilerplate{Text: fmt.Sprintf("#include %v", bp.value)}
		} else {
			ret = Boilerplate{Text: fmt.Sprintf("#include %q", bp.value)}
		}
	case bText:
		ret = Boilerplate{Text: bp.value}
	}
	return
}

// prepare flattens the syntax tree down to a form better suited for
// rendering via the template.
func prepare(statements []statement) (t TemplateUI, err error) {
	for _, st := range statements {
		switch st.sType {
		case sMenu:
			var preparedMenu TemplateMenu
			preparedMenu, err = prepareMenu(st.menu)
			if err != nil {
				return
			}
			t.Menus = append(t.Menus, preparedMenu)
		case sBoilerplate:
			var preparedBoilerplate Boilerplate
			preparedBoilerplate, err = prepareBoilerplate(st.boilerplate)
			if err != nil {
				return
			}
			t.Boilerplate = append(t.Boilerplate, preparedBoilerplate)
		}
	}
	return
}

func main() {
	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatalf("no input file specified")
	}
	var tmpl = template.Must(template.ParseFiles(*templatePath))
	f, err := os.Open(flag.Arg(0))
	if err != nil {
		log.Fatalf("open %q: %v", flag.Arg(0), err)
	}
	text, err := ioutil.ReadAll(f)
	if err != nil {
		log.Fatalf("read %q: %v", flag.Arg(0), err)
	}
	f.Close()
	statements, err := parse(text)
	if err != nil {
		log.Fatalf("parse %v:%v", flag.Arg(0), err)
	}
	//fmt.Printf("%#v\n", menus)
	err = checkStatements(statements)
	if err != nil {
		log.Fatalf("check %v:%v", flag.Arg(0), err)
	}
	td, err := prepare(statements)
	if err != nil {
		log.Fatalf("prepare: %v:%v", flag.Arg(0), err)
	}
	err = tmpl.Execute(os.Stdout, td)
	if err != nil {
		log.Fatalf("render %v:%v", flag.Arg(0), err)
	}
}
