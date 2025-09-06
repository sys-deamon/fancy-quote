# Fancy Quote

A minimal terminal eye candy.

---
# Features

- Uses `en_us.UTF-8` encoder.

>Make sure your terminal is set to the same encoder. You can check it by executing `locale` in your terminal.

## Usage

*NOTE!* The installer only works for Debian based distributions, support for other will be added later cuz im neub right now. However you can just for now compile the source code yourself.

```shell
git clone <URL>
```

go into the folder and execute the `installer`

```shell
installer
```

This will first update your package database and install *ncurses* libraries.

You can then compile the program with:

```shell
make
```

After that just run the executable:

```shell
./fancy-quote "Hello World"
```

press any key to close the program and return to previous session.