package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

type menu struct {
}

type menuItem struct {
}

type askStatement struct {
}

type editableStatement struct {
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
	fmt.Println(string(text))
	menus, err := parse(text)
	if err != nil {
		log.Fatalf("parse %q: %v", flag.Arg(0), err)
	}
	fmt.Printf("%#v\n", menus)
}
