# Fancy Quote

A minimal terminal eye candy.

# Features

- Automatically uses your current UTF-8 locale when available (falls back to `en_US.UTF-8`).
- Accepts multi-word quotes without quoting each argument, gracefully wraps long lines, and reads from standard input when no arguments are provided.
- Centers multi-line and wide character quotes using the wide-character ncurses API.
- Provides `--help` and `--version` flags for quick reference.
- Draws a centered frame around the quote and can print an author banner above it via `-a/--author`.
- Lets you style the quote, frame, and author label independently with `-ct/--colortext`, `-cr/--colorrectangle`, and `-ca/--colorauth`.

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
./fancyquote Hello there General Kenobi
```

Add an author banner and custom colors:

```shell
./fancyquote -a "Obi-Wan Kenobi" -ct yellow -cr blue -ca white "Hello there"
```

Pass `--` to treat the following arguments as quote text even if they start with dashes, and specify colors using the standard ncurses names (`black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`) or a numeric color index supported by your terminal. If your terminal cannot apply a requested color, the program keeps running and prints a warning after exit.

Quotes containing newlines can be piped in as well:

```shell
printf 'It\'s over, Anakin.\nI have the high ground.' | ./fancyquote
```

When running with piped input, the app renders the quote and exits automatically.

Press any key to close the program and return to the previous session.

# Screenshots
![screenshot](./demo.png)
