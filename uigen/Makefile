# XXX go get -u golang.org/x/tools/cmd/goyacc
#goyacc="go tool yacc"
GOYACC=goyacc

uigen: lex.go uigen.go parse.y
	$(GOYACC) parse.y
	go build

clean:
	rm -f y.go y.output uigen
