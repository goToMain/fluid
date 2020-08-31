# Fluid - A liquid template renderer

Fluid is a tool used to parse/render liquid templated source files to plain
text files. A typical use case is HTML files with liquid tags. Read more about
liquid grammar [here][1].

## Dependencies

  - cmake (host)
  - [LibYAML][2] (host)

## Build

``` bash
git clone https://github.com/goToMain/fluid --recurse-submodules
cd fluid
mkdir build && cd build
cmake ..
make
```

## Contribution

This project is actively developed and maintained. Contributions are always
welcome. Create PR/Issues at [https://github.com/goToMain/fluid][3]

## License

This software is distributed under the terms of Apache-2.0 license. If you don't
know what that means/implies, you can consider it is as "free as in beer".

[1]: https://shopify.github.io/liquid/
[2]: https://github.com/yaml/libyaml
[3]: https://github.com/goToMain/fluid
