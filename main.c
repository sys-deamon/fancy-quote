#include <stdio.h>
#include <ncursesw/curses.h>
#include <string.h>

int QuoteStartPointY(int cols, int quoteLen)
{
  // Rows middle
  int colsMid = cols/2;
  
  // Quote offset
  int offset = quoteLen / 2;

  // Calculations
  int startPoint = colsMid - offset;

  return startPoint;
}

int TerminalMidHeight(int rows)
{
  return rows / 2;
}

void PrintQuote(const char* quote, int quoteLen)
{
  // Get the terminal width and height
  int rows, cols = 0;
  getmaxyx(stdscr, rows, cols);

  // Get the starting point and move the cursor there
  move(TerminalMidHeight(rows), QuoteStartPointY(cols, quoteLen));

  // Print the quote
  addstr(quote);
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    {
      printf("Please read the man page for proper usage!\n");
      return 1;
    }
  
  // Initializing ncurses mode
  initscr();

  // Hide the cursor
  curs_set(0);
  
  // Clear the screen
  clear();

  // Move the cursor to the top left corner of the terminal
  move(0,0);

  // Print quote to screen
  int quoteLen = strlen(argv[1]);
  PrintQuote(argv[1], quoteLen);

  // Flush/Apply the changes to the actual terminal
  refresh();

  // Wait for any character input
  getch();
  
  // End ncurses mode and restore the previous terminal state
  endwin();
  return 0;
}
