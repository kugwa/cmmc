# About

This is a C Minus Minus compiler. See [pattern](pattern)/\*.cmm to check the syntax of C Minus Minus.

# Build

```
./autogen.sh
make
```

Note: flex and bison are required.

# Testing

```
./parser pattern/should-pass.cmm
```

This command will compile [pattern/should-pass.cmm](pattern/should-pass.cmm) and generate output.s.

# Runing

[cross compile](https://askubuntu.com/questions/250696/how-to-cross-compile-for-arm)

