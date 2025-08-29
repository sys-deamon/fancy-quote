# Fancy Quote

---

A minimal terminal eye candy.

---

## Usage

*NOTE!* The installer only works for Debian based distributions, support for other will be added later cuz im neub right now. However you can just for now compile the source code yourself.

```shell
git clone <URL>
```

go into the folder and execute the `installer`

```shell
installer
```

This will first update your package database and install *ncursesw* libraries.

You can then compile the program with:

```shell
make
```

After that just run the executable:

```shell
./fancy-quote "Hello World"
```

press any key to close the program and return to previous session.