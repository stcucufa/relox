// Tokens are either a symbol (e.g. Token.Open for an open parenthesis,
// Token.End for the end of input) or a value (boolean, number or string).
const Token = Object.fromEntries([
    "Open", "Close",
    "Plus", "Minus", "Star", "StarStar", "Solidus",
    "LE", "LT", "GE", "GT", "Equal", "NE", "Bang",
    "Bar", "Quote",
    "End"
].map(name => ([name, Symbol.for(name)])));

// Get the type of a token (itself for a symbol, but "boolean", "number" or
// "string" for values.
const typeOf = token => typeof token === "symbol" ? Symbol.keyFor(token) : typeof token;

// Generate tokens from an input string.
function* tokenize(input) {

    // Advance and return true, if the next character matches c, return false
    // otherwise.
    function isNext(c) {
        if (input[0] == c) {
            input = input.substring(1);
            return true;
        }
        return false;
    }

    // Skip whitespace and comments and advance to the end of the next token.
    // Return Token.End when the input is exhausted.
    while (input.length > 0) {
        input = input.replace(/^(\s*(\/\/[^\n]*\n)*)+/, "");
        const fullInput = input;
        const c = input[0];
        input = input.substring(1);
        switch (c) {
            case "(": yield Token.Open; break;
            case ")": yield Token.Close; break;
            case "+": yield Token.Plus; break;
            case "-": yield Token.Minus; break;
            case "*": yield isNext("*") ? Token.StarStar : Token.Star; break;
            case "/": yield Token.Solidus; break;
            case "<": yield isNext("=") ? Token.LE : Token.LT; break;
            case ">": yield isNext("=") ? Token.GE : Token.GT; break;
            case "=": yield Token.Equal; break;
            case "!": yield isNext("=") ? Token.NE : Token.Bang; break;
            case "|": yield Token.Bar; break;
            case "'": yield Token.Quote; break;
            case `"`: {
                // String
                const match = input.match(/^([^"]*)"/);
                if (match) {
                    input = input.substring(match[0].length);
                    yield match[1];
                } else {
                    throw Error(`Unfinished string near "${input}".`);
                }
                break;
            }
            default: {
                // Number
                let match = fullInput.match(/^\d+(\.\d+)?/);
                if (match) {
                    input = fullInput.substring(match[0].length);
                    yield parseFloat(match[0]);
                } else if (match = fullInput.match(/^\w+/)) {
                    // Identifier, boolean or keyword
                    input = fullInput.substring(match[0].length);
                    const id = match[0];
                    yield id === "true" ? true : id === "false" ? false : id;
                } else {
                    throw Error(`Unexpected character at "${input}".`);
                }
            }
        }
    }
    return Token.End;
}

// Identity (for value nuds).
const id = x => x;

// Attempt to apply an unary operator to a value based on its type. Pass a list
// of (type, function) pairs and return a function that applies f to x if the
// type of x matches. If no type matches, throw a runtime error.
const unary = (...fs) => function(x) {
    for (const [tx, f] of fs) {
        if (typeof x === tx) {
            return f(x);
        }
    }
    throw Error(`Wrong parameter for op (expected ${ fs.map(([type]) => type).join(" or ") }, got ${
        typeOf(x) }`);
};

// Same for binary functions.
const binary = (...fs) => function(x, y) {
    for (const [_, tx, ty, f] of fs) {
        if (typeof x === tx && typeof y === ty) {
            return f(x, y);
        }
    }
    throw Error(`Wrong parameters for ops (expected ${
        fs.map(([op, tx, ty]) => `${op} (${tx}/${ty})`).join(" or ")
    }, got ${x} and ${y}`);
};

// Repeat a string x n times.
function repeat(x, n) {
    const m = Math.max(0, Math.round(n));
    let r = "";
    for (let i = 0; i < m; ++i) {
        r += x;
    }
    return r;
}

// Parse and evaluate an input string. Use a Pratt parser for parsing the
// expression; the nuds/leds evaluate the input as the parser advances. Return
// the eventual value.
export function evaluate(input) {

    // Get an iterator for tokens, keeping track of the current and previous.
    let previousToken, currentToken;
    const tokens = tokenize(input);

    // Advance to the next token, optionally enforcing an expected type.
    function advance(expected) {
        if (expected && typeOf(expected) !== typeOf(currentToken)) {
            throw Error(`Expected ${typeOf(expected)}, got ${typeOf(currentToken)}`);
        }
        previousToken = currentToken;
        currentToken = tokens.next().value;
    }

    // Null denotation for tokens. Literals return their own value, and unary
    // operators.
    const Nud = {
        Minus: () => unary(["number", x => -x])(expression(Precedence.unary)),
        Bang: () => !expression(Precedence.unary),
        Bar() {
            const x = expression();
            advance(Token.Bar);
            return unary(["number", x => Math.abs(x)], ["string", x => x.length])(x);
        },
        Quote: () => expression(Precedence.unary).toString(),
        Open() {
            const value = expression();
            advance(Token.Close);
            return value;
        },
        number: id,
        string: id,
        boolean: id,
    };

    // Left denotation of operators. Most are left-associative, which is the
    // default; exponent (StarStar) is right-associative so 
    const Led = {
        Plus: (x, op) => binary(
            ["addition", "number", "number", (x, y) => x + y]
        )(x, expression(Precedence[typeOf(op)])),
        Minus: (x, op) => binary(
            ["subtraction", "number", "number", (x, y) => x - y]
        )(x, expression(Precedence[typeOf(op)])),
        Star: (x, op) => binary(
            ["multiplication", "number", "number", (x, y) => x * y],
            ["concatenation", "string", "string", (x, y) => x + y]
        )(x, expression(Precedence[typeOf(op)])),
        Solidus: (x, op) => binary(
            ["division", "number", "number", (x, y) => x / y]
        )(x, expression(Precedence[typeOf(op)])),
        StarStar: (x, op) => binary(
            ["exponentiation", "number", "number", (x, y) => x ** y],
            ["repetition", "string", "number", (x, y) => repeat(x, y)]
        )(x, expression(Precedence[typeOf(op) - 1])),
        LT: (x, op) => binary(
            ["comparison (less than)", "number", "number", (x, y) => x < y]
        )(x, expression(Precedence[typeOf(op)])),
        LE: (x, op) => binary(
            ["comparison (less or equal)", "number", "number", (x, y) => x <= y]
        )(x, expression(Precedence[typeOf(op)])),
        GT: (x, op) => binary(
            ["comparison (greater than)", "number", "number", (x, y) => x > y]
        )(x, expression(Precedence[typeOf(op)])),
        GE: (x, op) => binary(
            ["comparison (greater or equal)", "number", "number", (x, y) => x >= y]
        )(x, expression(Precedence[typeOf(op)])),
        Equal: (x, op) => x === expression(Precedence[typeOf(op)]),
        NE: (x, op) => x !== expression(Precedence[typeOf(op)]),
    };

    // Precedence levels of operators: interpolation is used for string
    // interpolation (between a string prefix/infix/suffix and an expression)
    // and unary for unary operators. The higher the value, the tighter the
    // binding.
    const Precedence = {
        interpolation: 1,
        Equal: 2, NE: 2,
        LT: 3, LE: 3, GT: 3, GE: 3,
        Plus: 4, Minus: 4,
        Star: 5, Solidus: 5,
        StarStar: 6,
        unary: 7,
    };

    // The core of the parser loop. Apply the null denotation to the current
    // token and advance; then while the current token has a higher precedence
    // than the target precedence, advance and apply the left denotation of the
    // previous token.
    function expression(precedence = 0) {
        const nud = Nud[typeOf(currentToken)];
        if (!nud) {
            throw Error("Expected nud");
        }
        advance();
        let x = nud(previousToken);
        while (Precedence[typeOf(currentToken)] > precedence) {
            advance();
            const led = Led[typeOf(previousToken)];
            if (led) {
                x = led(x, previousToken);
            } else {
                throw Error("Expected led");
            }
        }
        return x;
    }

    // Move to the first token and parse an expression; then check that we
    // reached the end of input.
    advance();
    const result = expression();
    advance(Token.End);
    return result;
}
