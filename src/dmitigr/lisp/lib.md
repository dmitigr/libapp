# Igrilisp Standard Library

## Functions

- [let](#function-let)

## Function `let`

Establishes the `BINDINGS` and evaluates a `BODY`.

### Synopsis

`(let BINDINGS BODY)`

### Parameters

- `BINDINGS` - a form which is evaluated first. The result of evaluation must
be a tuple of LVAR-VALUE pairs (or empty tuple). Next, each VALUE of each pair
is evaluated and the result is assigned to LVAR;
- `BODY` - a form to evaluate after the enviroment establishment.

### Result

The result of the `BODY` evaluation.
