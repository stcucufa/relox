import { describe, expect, test } from "bun:test";
import { evaluate } from "./eval.js";

const tests = {
    "-1 + 2 * 3 - 4": -1 + 2 * 3 - 4,
    "(1 + 2) * -3 - 4": (1 + 2) * -3 - 4,
    "2 ** 3 ** 4": 2 ** 3 ** 4,
    "\"\\\"foo\\\"\" * \"bar\"": "\"foo\"" + "bar",
    "\"length of foo = ${|\"foo\"|}, false = ${!true}\"": "length of foo = 3, false = false",
    "!(1 + 2 < 3 + 4)": !(1 + 2 < 3 + 4),
    "|-|\"foo\" * \"bar\"||": 6,
    "|'123|": 3,
    "\"hell\" * \"o\" ** 17": "hellooooooooooooooooo",
    "!true": false,
    "-∞": -Infinity,
};

describe("Evaluate expressions", () => {
    for (const [input, output] of Object.entries(tests)) {
        test(input, () => { expect(evaluate(input)).toEqual(output); });
    }
});

test("Runtime error", () => {
    expect(() => evaluate(`"foo" + "bar"`)).toThrow();
});
