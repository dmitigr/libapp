# Igrilisp the Language

## Strings and text

*String* - is a sequence of fixed n-byte sized numbers. By default, string
is a sequence of bytes (i.e. sequence of 1-byte sized numbers). Strings can
be created from *string literals*, for example:

```
'This is a string literal'.
```

*Text* - is a sequence of *letters* encoded according to some encoding. Text
can be created from a string literal by using function `text`, for example:

```
(text 'This is a text' :encoding #utf-8)
```

## Makers and modifiers

There are 2 types of functions:
  - *makers* - produces new instances
    ```
    (tuple-append $tup 1 2 3) ; returns new tuple of 3 integers
    ```
  - *modifiers* - modifies the existing instances
    ```
    (tuple-append= $tup 1 2 3) ; modifies $tup, returns the result

The name of a modifier ends with the suffix `=`.

## Predicates

Predicates - are functions which returns boolean values. The name of a predicate
ends with the suffix `?`.

## Aliases

Some of the often used functions have aliaes for convenience:

|Name              |Alias    |
|:-----------------|:--------|
|math-add          |add      |
|math-sub          |sub      |
|math-mul          |mul      |
|math-div          |div      |
|math-mod          |mod      |
|string-cat        |cat      |
|string-cat=       |cat=     |
