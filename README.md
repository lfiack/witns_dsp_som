# DSP System on Module from WITNS

## Make the project work
This project uses a submodule for the KiCAD library. To get it properly, use the `--recursive` option when cloning the repo :

```bash
git clone --recursive git@github.com:<path_to_your_project>
```

If you forgot the `--recursive` option (as I do quite often), you can type the following command :

```bash
git submodule update --init --recursive
```
