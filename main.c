#include <stdio.h>
#include <ncursesw/curses.h>
#include <string.h>

#include <locale.h>
#include <langinfo.h>

int MidCol(int cols, int quoteLen)
{
  // Calculate Columns middle
  int colsMid = cols / 2;
  
  // Quote offset
  int offset = quoteLen / 2;

  return colsMid - offset;
}

int MidRow(int rows)
{
  return rows / 2;
}

void PrintQuote(const char* quote, int quoteLen)
{
  // Get the terminal width and height
  int totalScreenRows, totalScreenCols = 0;
  getmaxyx(stdscr, totalScreenRows, totalScreenCols);

  // Get the mid point of terminal and move the cursor there
  move(MidRow(totalScreenRows), MidCol(totalScreenCols, quoteLen));

  // Print the quote
  addstr(quote);
}

int main(int argc, char *argv[])
{
  // sets the locale encoder to en_US.UTF-8
  setlocale(LC_ALL, "en_US.UTF-8");

  // Debug statement for checking current set encoder
  // printf("codeset = %s\n", nl_langinfo(CODESET));

  if (argc < 2)
    {
      printf("Please read the man page for proper usage!\n");
      return 1;
    }
  
  // Initializing ncurses mode
  initscr();
  
  // Clear the screen
  clear();

  // Move the cursor to the top left corner of the terminal
  move(0,0);

  // Calculate quote length
  int quoteLen = strlen(argv[1]);

  // Print the quote to screen
  PrintQuote(argv[1], quoteLen);

  // Hide the cursor
  curs_set(0);
  
  // Flush/Apply the changes to the actual terminal
  refresh();

  // Wait for any character input
  getch();
  
  // End ncurses mode and restore the previous terminal state
  endwin();
  return 0;
}