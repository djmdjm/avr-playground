{{$top := .}}{{/* Template file for UI definitions*/}}
/* This file is generated by uigen; DO NOT EDIT */

{{range .Boilerplate}}
{{if $top.Header}}{{if .InHeader}}{{.Text}}{{end}}{{else}}{{if .InSource}}{{.Text}}{{end}}{{end}}{{end}}

{{if $top.Header}}
{{if $top.HeaderGuard}}#ifndef {{$top.HeaderGuard}}
#define {{$top.HeaderGuard}}{{end}}
#include <stddef.h>
#include <stdint.h>

struct menu;

struct editable_item {
	const char *label;	/* NULL if integer range */
	int value;
};

struct editable {
	const char *display;
	int (*get)(void);
	void (*set)(int);
	int set_every;
	int range_lo, range_hi;	/* If no integer range, then hi<lo */
	size_t n;
	const struct editable_item *items;
};

struct ask_item {
	const char *label;
	void (*action)(void);	/* NULL to return to previous menu */
};

struct ask {
	const char *display;
	size_t n;
	const struct ask_item *items;
};

struct submenu_range {
	const char *label;
	void (*set)(int);
	int range_lo, range_hi;	/* If no integer range, then hi<lo */
	const struct menu *definition;
};

struct menu_item {
	const char *label;
	enum { M_EDITABLE, M_ASK, M_SUBMENU, M_SUBMENU_RANGE } mi_type;
	const union {
		const struct editable *editable;
		const struct ask *ask;
		const struct menu *submenu;
		const struct submenu_range *submenu_range;
	} item;
};

struct menu {
	size_t n;
	const struct menu_item * const items;
	const struct menu *parent;
};

#define MENU_MAX_DEPTH	{{.MenuDepth}}
extern const struct menu *menu_root;

{{if $top.HeaderGuard}}#endif /* {{$top.HeaderGuard}} */{{end}}
{{else}}{{/* $top.Header */}}

/* Forward declaration of menus */
{{range .Menus}}static const struct menu {{.Name}};
{{end}}
{{range .Menus}}

{{range .Editables}}
{{if .Items}}
static const struct editable_item {{.Name}}_items[] = {
{{range .Items}}	{ {{printf "%q" .Label}}, {{.Definition}} },
{{end}}};{{end}}

static const struct editable {{.Name}} = {
	{{printf "%q" .Label}},
	{{.Get}},
	{{.Set}},
	{{if .Every}}1{{else}}0{{end}},
	{{if .HasRange}}{{.RangeLow}}, {{.RangeHi}},
	{{else}}0, -1, /* no integer range */
	{{end}}{{len .Items}},
	{{if .Items}}{{.Name}}_items{{else}}NULL{{end}},
};
{{end}}
{{range .SubmenuRanges}}
static const struct submenu_range {{.Name}} = {
	{{printf "%q" .Label}},
	{{.Set}},
	{{.RangeLow}}, {{.RangeHi}},
	&{{.Definition}},
};
{{end}}
{{range .Asks}}

{{if .Items}}
{{range .Items}}{{if .Action}}void {{.Action}}(void);
{{end}}{{end}}
static const struct ask_item {{.Name}}_items[] = {
{{range .Items}}	{ {{printf "%q" .Label}}, {{if .Action}}{{.Action}}{{else}}NULL{{end}} },
{{end}}};
{{end}}
static const struct ask {{.Name}} = {
	{{printf "%q" .Label}},
	{{len .Items}},
	{{if .Items}}{{.Name}}_items{{else}}NULL{{end}},
};
{{end}}

static const struct menu_item {{.Name}}_items[] = {
{{range .Items}}	{
		.label = {{printf "%q" .Label}},
{{if eq .Type "editable"}}		.mi_type = M_EDITABLE,
		.item = { .editable = &{{.Name}} },
{{else if eq .Type "ask"}}		.mi_type = M_ASK,
		.item = { .ask = &{{.Name}} }
{{else if eq .Type "submenu"}}		.mi_type = M_SUBMENU,
		.item = { .submenu = &{{.Name}} }
{{else if eq .Type "submenurange"}}		.mi_type = M_SUBMENU_RANGE,
		.item = { .submenu_range = &{{.Name}} }
{{else}}#error unknown menu item type "{{.Type}}"
{{end}}
	},
{{end}}
};

static const struct menu {{.Name}} = {
	{{len .Items}}, {{.Name}}_items,
	{{if .Parent}}&{{.Parent}}{{else}}NULL{{end}}
};

{{end}}

const struct menu *menu_root = &root;

{{end}}{{/* $top.Header */}}
