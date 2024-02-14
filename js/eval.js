// Tokens are either a symbol (e.g. Token.OpenParen for an open parenthesis,
// Token.End for the end of input) or a value (boolean, number or string).
const Token = Object.fromEntries([
    "Boolean", "Number", "String", "Identifier",
    "OpenParen", "CloseParen", "OpenBrace", "CloseBrace",
    "Plus", "Minus", "Star", "StarStar", "Solidus",
    "LE", "LT", "GE", "GT", "Equal", "NE", "Bang",
    "Bar", "Quote", "Unquote",
    "In", "Let",
    "End"
].map(name => ([name, Symbol.for(name)])));

const Keyword = {
    false: [Token.Boolean, false],
    "let": [Token.Let],
    "in": [Token.In],
    true: [Token.Boolean, true],
};

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

    // Keep track of string nesting for interpolation (e.g. "hell${"o" ** 17}")
    let nesting = 0;

    // Skip whitespace and comments and advance to the end of the next token.
    // Return Token.End when the input is exhausted.
    while (input.length > 0) {
        input = input.replace(/^(\s*(\/\/[^\n]*\n)*)+/, "");
        const fullInput = input;
        const c = input[0];
        input = input.substring(1);
        switch (c) {

            // Simple tokens
            case "(": yield [Token.OpenParen]; break;
            case ")": yield [Token.CloseParen]; break;
            case "{": yield [Token.OpenBrace]; break;
            case "+": yield [Token.Plus]; break;
            case "-": yield [Token.Minus]; break;
            case "*": yield isNext("*") ? [Token.StarStar] : [Token.Star]; break;
            case "/": yield [Token.Solidus]; break;
            case "<": yield isNext("=") ? [Token.LE] : [Token.LT]; break;
            case ">": yield isNext("=") ? [Token.GE] : [Token.GT]; break;
            case "=": yield [Token.Equal]; break;
            case "!": yield isNext("=") ? [Token.NE] : [Token.Bang]; break;
            case "|": yield [Token.Bar]; break;
            case "'": yield [Token.Quote]; break;
            case "∞": yield [Token.Number, Infinity]; break;

            // String: allow interpolation ("x = ${x}") with nesting
            // ("hell${"o" ** 17}, world!") where the string is broken up and
            // stars/quotes are injected into the token stream (so the first
            // example will produce "x = ", Star, Quote, x, "".
            case "}": {
                if (nesting > 0) {
                    const match = input.match(/^((?:[^"\\\$]|\\.|\$(?!\{))*)("|\$\{)/);
                    if (match) {
                        yield [Token.Star];
                        input = input.substring(match[0].length);
                        yield [Token.String, match[1].replace(/\\(.)/g, "$1")];
                        if (match[2] === "${") {
                            yield [Token.Star];
                            yield [Token.Quote];
                        } else {
                            nesting -= 1;
                        }
                    } else {
                        throw Error(`Unfinished string near "${input}".`);
                    }
                } else {
                    // Just a regular closing brace.
                    yield [Token.CloseBrace];
                }
                break;
            }
            case `"`: {
                const match = fullInput.match(/^"((?:[^"\\\$]|\\.|\$(?!\{))*)("|\$\{)/);
                if (match) {
                    input = fullInput.substring(match[0].length);
                    yield [Token.String, match[1].replace(/\\(.)/g, "$1")];
                    if (match[2] === "${") {
                        yield [Token.Star];
                        yield [Token.Quote];
                        nesting += 1;
                    }
                } else {
                    throw Error(`Unfinished string near "${input}".`);
                }
                break;
            }

            // Number (begins with a digit) or identifiers.
            default: {
                let match = fullInput.match(/^\d+(\.\d+)?/);
                if (match) {
                    input = fullInput.substring(match[0].length);
                    yield [Token.Number, parseFloat(match[0])];
                } else if (match = fullInput.match(/^\w+/)) {
                    input = fullInput.substring(match[0].length);
                    const id = match[0];
                    if (Object.hasOwn(Keyword, id)) {
                        yield Keyword[id];
                    } else {
                        yield [Token.Identifier, id];
                    }
                } else {
                    throw Error(`Unexpected character at "${input}".`);
                }
            }
        }
    }
    return [Token.End];
}

// Value of a literal token (bool, number, string)
const tokenValue = (_, [__, x]) => x;

// Attempt to apply an unary operator to a value based on its type. Pass a list
// of (op, type, function) pairs and return a function that applies f to x if
// the type of x matches. If no type matches, throw a runtime error.
const unary = (...fs) => function(x) {
    for (const [_, tx, f] of fs) {
        if (typeof x === tx) {
            return f(x);
        }
    }
    throw Error(`Wrong parameter for op (expected ${
        fs.map(([op, type]) => `${op} (${type})`).join(" or ")
    }, got ${x}`);
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

// Parse and evaluate an input string in a given environment. Use a Pratt
// parser for parsing the expression; the nuds/leds evaluate the input as the
// parser advances. Return the eventual value.
export function evaluate(input, env = {}) {

    // Get an iterator for tokens, keeping track of the current and previous.
    let previousToken, currentToken;
    const tokens = tokenize(input);

    // Advance to the next token, optionally enforcing an expected type.
    function advance(expected) {
        if (expected && expected !== currentToken[0]) {
            throw Error(`Expected ${Symbol.keyFor(expected)}, got ${Symbol.keyFor(currentToken[0])}`);
        }
        previousToken = currentToken;
        currentToken = tokens.next().value;
        return previousToken;
    }

    // Null denotation for tokens. Literals return their own value, and unary
    // operators.
    const Nud = {
        Minus: env => unary(["negation", "number", x => -x])(expression(env, Precedence.Unary)),
        Bang: env => !expression(env, Precedence.Unary),
        Bar(env) {
            const x = expression(env);
            advance(Token.Bar);
            return unary(["abs", "number", x => Math.abs(x)], ["length", "string", x => x.length])(x);
        },
        Quote: env => expression(env, Precedence.Unary).toString(),
        OpenParen(env) {
            const value = expression(env);
            advance(Token.CloseParen);
            return value;
        },

        Let: env => {
            const name = advance(Token.Identifier)[1];
            advance(Token.Equal);
            const value = expression(env);
            advance(Token.In);
            env = Object.create(env);
            env[name] = value;
            return expression(env);
        },

        Identifier(env, [_, id]) {
            if (!(id in env)) {
                throw new Error(`Unbound identifier ${id}.`);
            }
            return env[id];
        },

        Number: tokenValue,
        String: tokenValue,
        Boolean: tokenValue,
    };

    // Left denotation of operators. Most are left-associative, which is the
    // default; exponent (StarStar) is right-associative so 
    const Led = {
        Plus: (env, x, op) => binary(
            ["addition", "number", "number", (x, y) => x + y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        Minus: (env, x, op) => binary(
            ["subtraction", "number", "number", (x, y) => x - y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        Star: (env, x, op) => binary(
            ["multiplication", "number", "number", (x, y) => x * y],
            ["concatenation", "string", "string", (x, y) => x + y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        Solidus: (env, x, op) => binary(
            ["division", "number", "number", (x, y) => x / y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        StarStar: (env, x, op) => binary(
            ["exponentiation", "number", "number", (x, y) => x ** y],
            ["repetition", "string", "number", (x, y) => repeat(x, y)]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])] - 1)),
        LT: (env, x, op) => binary(
            ["comparison (less than)", "number", "number", (x, y) => x < y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        LE: (env, x, op) => binary(
            ["comparison (less or equal)", "number", "number", (x, y) => x <= y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        GT: (env, x, op) => binary(
            ["comparison (greater than)", "number", "number", (x, y) => x > y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        GE: (env, x, op) => binary(
            ["comparison (greater or equal)", "number", "number", (x, y) => x >= y]
        )(x, expression(env, Precedence[Symbol.keyFor(op[0])])),
        Equal: (env, x, op) => x === expression(env, Precedence[Symbol.keyFor(op[0])]),
        NE: (env, x, op) => x !== expression(env, Precedence[Symbol.keyFor(op[0])]),
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
        Unary: 7,
    };

    // The core of the parser loop. Apply the null denotation to the current
    // token and advance; then while the current token has a higher precedence
    // than the target precedence, advance and apply the left denotation of the
    // previous token.
    function expression(env, precedence = 0) {
        const nud = Nud[Symbol.keyFor(currentToken[0])];
        if (!nud) {
            throw Error(`Expected nud for ${Symbol.keyFor(currentToken[0])}.`);
        }
        advance();
        let x = nud(env, previousToken);
        while (Precedence[Symbol.keyFor(currentToken[0])] > precedence) {
            advance();
            const led = Led[Symbol.keyFor(previousToken[0])];
            if (led) {
                x = led(env, x, previousToken);
            } else {
                throw Error(`Expected led for ${Symbol.keyFor(previousToken[0])}.`);
            }
        }
        return x;
    }

    // Move to the first token and parse an expression; then check that we
    // reached the end of input.
    advance();
    const result = expression(env);
    advance(Token.End);
    return result;
}
