package main

// Lexer for UI generator.

import (
	"fmt"
	//	"log"
	"strconv"
	"unicode"
	"unicode/utf8"
)

// state holds the lexer state (including the text being processed) and the
// resultant parse tree.
type state struct {
	// Lexing state.
	err       error    // lex/parse error; usually nil.
	text      []byte   // entire input.
	loc       location // human-readable location, for error messages.
	p         int      // current character location in input.
	pStart    int      // start of current token/word.
	lastToken string   // most recently returned token, for error messages.

	// Parser state.
	menus []menu // top level of parse tree.
}

// LexError is the error type for errors encountered during lexing.
type LexError struct {
	msg string
	loc location
}

func (err *LexError) Error() string {
	return fmt.Sprintf("%v: lex error: %s", err.loc, err.msg)
}

// lexError records a lexing error.
func (st *state) lexError(f string, a ...interface{}) {
	st.err = &LexError{msg: fmt.Sprintf(f, a...), loc: st.loc}
}

// ParseError is the error type for parsing errors.
type ParseError struct {
	msg string
	loc location
}

func (err *ParseError) Error() string {
	return fmt.Sprintf("%v: parse error: %s", err.loc, err.msg)
}

// Error records a parse error. It is part of the lexer interface
// required by the generated parser.
func (st *state) Error(msg string) {
	// Only record first error.
	if st.err == nil {
		st.err = &ParseError{msg: msg, loc: st.loc}
	}
}

// has checks whether the input has at least 'n' characters left.
func (st *state) has(n int) bool {
	return st.p+n <= len(st.text)
}

// eof checks whether the input has been exhausted.
func (st *state) eof() bool {
	return !st.has(1)
}

// next returns the next input character, advancing the input pointer to consume
// the character.
func (st *state) next() int {
	if st.eof() {
		// Shouldn't happen.
		panic("EOF during next()")
	}
	c, sz := utf8.DecodeRune(st.text[st.p:])
	st.p += sz
	// Update location.
	if c == '\n' {
		st.loc.line++
		st.loc.char = 0
	} else {
		st.loc.char++
	}
	return int(c)
}

// peek looks ahead one input character without advancing the character pointer.
func (st *state) peek() int {
	if st.eof() {
		// At EOF.
		return 0
	}
	r, _ := utf8.DecodeRune(st.text[st.p:])
	return int(r)
}

// startToken records the start of a token in the input. Once the token
// has been fully read by next() then endToken() must be called.
func (st *state) startToken(yy *yySymType) {
	yy.loc = st.loc
	st.pStart = st.p
	yy.token = ""
}

// endToken records the end of a token started by startToken()
func (st *state) endToken(yy *yySymType) {
	if yy.token != "" {
		// No token is active. (XXX error here?)
		return
	}
	tok := string(st.text[st.pStart:st.p])
	yy.token = tok
	st.lastToken = tok
}

// hexDigit converts a hex digit to a 4-bit integer.
// Returns -1 on invalid input.
func hexDigit(a int) int {
	if a >= '0' && a <= '9' {
		return a - '0'
	} else if a >= 'a' && a <= 'f' {
		return 10 + a - 'a'
	} else if a >= 'A' && a <= 'F' {
		return 10 + a - 'F'
	} else {
		return -1
	}
}

// hexByte reads two hex digits from the input, returning an 8-bit integer.
// Returns -1 on invalid input or eof.
func (st *state) hexByte() int {
	if !st.has(2) {
		return -1
	}
	av := hexDigit(st.next())
	bv := hexDigit(st.next())
	if av == -1 || bv == -1 {
		return -1
	}
	return (av << 4) | bv
}

// Lex obtains the next token from the input. It is part of the lexer interface
// required by the generated parser.
func (st *state) Lex(yy *yySymType) int {
	// If an error has occurred already then don't return any more tokens.
	if st.err != nil {
		return _EOF
	}

	// Skip whitespace and comments.
	for !st.eof() {
		c := st.peek()
		if c == '#' {
			// Comment; consume to EOL.
			for !st.eof() && st.next() != '\n' {
			}
		} else if unicode.IsSpace(rune(c)) {
			st.next()
		} else {
			break
		}
	}

	// Begin parsing a token.
	st.startToken(yy)
	defer st.endToken(yy)

	// End of file.
	if st.eof() {
		st.lastToken = "<EOF>"
		return _EOF
	}

	switch c := st.peek(); c {
	// Braces
	case '{', '}':
		st.next()
		return c

	case '=': // beginning of "=>"?
		st.next()
		if st.peek() == '>' {
			st.next()
			return _THEN
		}
		return c

	// Quoted string; dequote and de-escape it.
	case '"':
		quote := c
		st.next()
		unquoted := []byte{}
		for {
			if st.eof() {
				st.loc = yy.loc
				st.lexError("EOF in quoted string")
				return _EOF
			}
			c = st.next()
			if c == '\\' {
				// Escaped character.
				if st.eof() {
					st.lexError("EOF in quoted escape sequence")
					return _EOF
				}
				c = st.next()
				if c == '\\' || c == quote {
					unquoted = append(unquoted, byte(c))
				} else if c == 'x' {
					// '\xYY' hex-literal character.
					c = st.hexByte()
					if c == -1 {
						st.lexError("Invalid hex escape in quoted string")
						return _EOF
					}
					unquoted = append(unquoted, byte(c))
				} else {
					st.lexError(fmt.Sprintf("Invalid escape '%c' in quoted string", c))
					return _EOF
				}
			} else if c == quote {
				break
			} else {
				unquoted = append(unquoted, byte(c))
			}
		}
		yy.stringv = string(unquoted)
		return _QUOTED
	}

	// Find the end of the current token.
	for {
		c := rune(st.peek())
		if unicode.IsSpace(c) || !(unicode.IsLetter(c) || unicode.IsNumber(c) || c == '-') {
			break
		}
		st.next()
	}

	// Is this a keyword?
	st.endToken(yy)
	k, ok := keywords[yy.token]
	if ok {
		return k
	}

	// If not, it might be an integer.
	i, err := strconv.ParseInt(yy.token, 0, 64)
	if err == nil {
		yy.intv = i
		return _INT
	}

	// There are no bare identifiers in this language, so this is an error.
	st.lexError("unrecognised token %q", yy.token)
	return _EOF
}

// parse calls the yacc-generated parser to parse the specified text.
func parse(text []byte) ([]menu, error) {
	st := &state{
		text: text,
		loc:  location{line: 1},
	}
	// To turn on maximum debugging:
	// yyDebug = 4
	yyParse(st)
	return st.menus, st.err
}
