#define _XOPEN_SOURCE 700

#include <locale.h>
#include <ncursesw/curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

typedef struct
{
  wchar_t *text;
  size_t length;
  int width;
} QuoteSegment;

static int
MidCol(int cols, int textWidth)
{
  if (cols <= 0)
    {
      return 0;
    }

  if (textWidth > cols)
    {
      textWidth = cols;
    }

  int colsMid = cols / 2;
  int offset = textWidth / 2;
  int col = colsMid - offset;

  return col < 0 ? 0 : col;
}

static int
MidRow(int rows, int blockHeight)
{
  if (rows <= 0)
    {
      return 0;
    }

  if (blockHeight > rows)
    {
      blockHeight = rows;
    }

  int row = (rows - blockHeight) / 2;

  return row < 0 ? 0 : row;
}

static void
AddSegment(QuoteSegment **segments, size_t *count, size_t *capacity, wchar_t *text,
           size_t length, int width)
{
  if (*count == *capacity)
    {
      size_t newCapacity = *capacity == 0 ? 8 : *capacity * 2;
      QuoteSegment *resized = realloc(*segments, newCapacity * sizeof(QuoteSegment));
      if (resized == NULL)
        {
          endwin();
          fprintf(stderr, "Failed to allocate memory for quote segments.\n");
          exit(EXIT_FAILURE);
        }
      *segments = resized;
      *capacity = newCapacity;
    }

  (*segments)[*count].text = text;
  (*segments)[*count].length = length;
  (*segments)[*count].width = width < 0 ? 0 : width;
  (*count)++;
}

static bool
IsOnlyWhitespace(const wchar_t *line)
{
  while (*line != L'\0')
    {
      if (!iswspace(*line))
        {
          return false;
        }
      line++;
    }
  return true;
}

static int
CharWidth(wchar_t ch)
{
  int width = wcwidth(ch);
  return width < 0 ? 1 : width;
}

static void
CollectSegmentsForLine(wchar_t *line, int cols, QuoteSegment **segments,
                       size_t *count, size_t *capacity)
{
  if (cols <= 0)
    {
      AddSegment(segments, count, capacity, line, 0, 0);
      return;
    }

  if (*line == L'\0' || IsOnlyWhitespace(line))
    {
      AddSegment(segments, count, capacity, line, 0, 0);
      return;
    }

  wchar_t *segmentStart = line;
  while (*segmentStart != L'\0')
    {
      wchar_t *cursor = segmentStart;
      wchar_t *lastSpace = NULL;
      int width = 0;
      int widthAtLastSpace = 0;

      while (*cursor != L'\0')
        {
          int charWidth = CharWidth(*cursor);

          if (width + charWidth > cols && width > 0)
            {
              break;
            }

          if (width == 0 && charWidth > cols)
            {
              width = charWidth;
              cursor++;
              break;
            }

          width += charWidth;

          if (iswspace(*cursor))
            {
              lastSpace = cursor;
              widthAtLastSpace = width;
            }

          cursor++;

          if (width >= cols)
            {
              break;
            }
        }

      wchar_t *segmentEnd = cursor;
      int segmentWidth = width;

      if (*cursor != L'\0' && lastSpace != NULL && lastSpace != segmentStart)
        {
          segmentEnd = lastSpace;
          int spaceWidth = CharWidth(*lastSpace);
          segmentWidth = widthAtLastSpace - spaceWidth;
          cursor = lastSpace + 1;
        }

      if (segmentEnd == segmentStart)
        {
          if (*segmentEnd == L'\0')
            {
              break;
            }

          segmentEnd++;
          segmentWidth = CharWidth(*segmentStart);
          cursor = segmentEnd;
        }

      wchar_t *trimEnd = segmentEnd;
      while (trimEnd > segmentStart && iswspace(*(trimEnd - 1)))
        {
          trimEnd--;
          segmentWidth -= CharWidth(*trimEnd);
        }

      size_t segmentLen = (size_t)(trimEnd - segmentStart);
      AddSegment(segments, count, capacity, segmentStart, segmentLen,
                 segmentWidth < 0 ? 0 : segmentWidth);

      segmentStart = cursor;
      while (iswspace(*segmentStart) && *segmentStart != L'\0')
        {
          segmentStart++;
        }
    }
}

static QuoteSegment *
SplitQuoteIntoSegments(wchar_t *quote, int cols, size_t *outCount)
{
  QuoteSegment *segments = NULL;
  size_t count = 0;
  size_t capacity = 0;

  wchar_t *lineStart = quote;
  for (wchar_t *cursor = quote;; ++cursor)
    {
      if (*cursor == L'\n' || *cursor == L'\0')
        {
          wchar_t saved = *cursor;
          *cursor = L'\0';
          CollectSegmentsForLine(lineStart, cols, &segments, &count, &capacity);
          *cursor = saved;

          if (saved == L'\0')
            {
              break;
            }

          lineStart = cursor + 1;
        }
    }

  *outCount = count;
  return segments;
}

static void
PrintSegments(const QuoteSegment *segments, size_t count)
{
  int totalRows = 0;
  int totalCols = 0;
  getmaxyx(stdscr, totalRows, totalCols);

  int startRow = MidRow(totalRows, (int)count);
  int currentRow = startRow;

  for (size_t i = 0; i < count && currentRow < totalRows; ++i)
    {
      if (segments[i].length == 0)
        {
          currentRow++;
          continue;
        }

      int col = MidCol(totalCols, segments[i].width);
      mvaddnwstr(currentRow, col, segments[i].text, segments[i].length);
      currentRow++;
    }
}

static void
FreeSegments(QuoteSegment *segments)
{
  free(segments);
}

static char *
JoinArgs(int argc, char *argv[])
{
  size_t totalLength = 0;

  for (int i = 1; i < argc; ++i)
    {
      totalLength += strlen(argv[i]);
      if (i < argc - 1)
        {
          totalLength++;
        }
    }

  char *result = malloc(totalLength + 1);
  if (result == NULL)
    {
      fprintf(stderr, "Failed to allocate memory for the quote.\n");
      exit(EXIT_FAILURE);
    }

  size_t position = 0;
  for (int i = 1; i < argc; ++i)
    {
      size_t len = strlen(argv[i]);
      memcpy(result + position, argv[i], len);
      position += len;

      if (i < argc - 1)
        {
          result[position++] = ' ';
        }
    }

  result[position] = '\0';
  return result;
}

static wchar_t *
ConvertToWide(const char *quote)
{
  size_t length = mbstowcs(NULL, quote, 0);

  if (length == (size_t)-1)
    {
      // Fallback to manual conversion
      length = strlen(quote);
    }

  wchar_t *wideQuote = malloc((length + 1) * sizeof(wchar_t));
  if (wideQuote == NULL)
    {
      fprintf(stderr, "Failed to allocate memory for wide quote.\n");
      exit(EXIT_FAILURE);
    }

  size_t converted = mbstowcs(wideQuote, quote, length + 1);
  if (converted == (size_t)-1)
    {
      for (size_t i = 0; i < length; ++i)
        {
          wideQuote[i] = (unsigned char)quote[i];
        }
      wideQuote[length] = L'\0';
    }

  return wideQuote;
}

static char *
ReadQuoteFromStdin(void)
{
  size_t capacity = 256;
  size_t length = 0;
  char *buffer = malloc(capacity);

  if (buffer == NULL)
    {
      fprintf(stderr, "Failed to allocate memory for the quote.\n");
      exit(EXIT_FAILURE);
    }

  int ch = 0;
  while ((ch = fgetc(stdin)) != EOF)
    {
      if (length + 1 >= capacity)
        {
          size_t newCapacity = capacity * 2;
          char *resized = realloc(buffer, newCapacity);
          if (resized == NULL)
            {
              free(buffer);
              fprintf(stderr, "Failed to allocate memory for the quote.\n");
              exit(EXIT_FAILURE);
            }
          buffer = resized;
          capacity = newCapacity;
        }

      buffer[length++] = (char)ch;
    }

  if (length == 0)
    {
      free(buffer);
      return NULL;
    }

  while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
    {
      length--;
    }

  buffer[length] = '\0';
  return buffer;
}

static void
PrintUsage(const char *programName)
{
  fprintf(stderr, "Usage: %s <quote>\n", programName);
  fprintf(stderr, "       %s --help\n", programName);
  fprintf(stderr, "       %s --version\n", programName);
  fprintf(stderr, "       (pipe text to %s to read from standard input)\n", programName);
}

static void
PrintVersion(void)
{
  printf("fancyquote 1.0\n");
}

int
main(int argc, char *argv[])
{
  if (argc > 1)
    {
      if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
          PrintUsage(argv[0]);
          return 0;
        }

      if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
        {
          PrintVersion();
          return 0;
        }
    }

  bool waitForKey = isatty(STDIN_FILENO);

  if (setlocale(LC_ALL, "") == NULL)
    {
      if (setlocale(LC_ALL, "en_US.UTF-8") == NULL)
        {
          fprintf(stderr,
                  "Warning: failed to set locale, continuing with default settings.\n");
        }
    }

  char *quote = NULL;
  if (argc >= 2)
    {
      quote = JoinArgs(argc, argv);
    }
  else
    {
      quote = ReadQuoteFromStdin();
      if (quote == NULL)
        {
          PrintUsage(argv[0]);
          return 1;
        }
    }

  wchar_t *wideQuote = ConvertToWide(quote);

  initscr();
  clear();
  noecho();
  curs_set(0);

  int totalCols = getmaxx(stdscr);

  size_t segmentCount = 0;
  QuoteSegment *segments = SplitQuoteIntoSegments(wideQuote, totalCols, &segmentCount);

  if (segmentCount > 0)
    {
      PrintSegments(segments, segmentCount);
    }

  refresh();
  if (waitForKey)
    {
      getch();
    }
  endwin();

  FreeSegments(segments);
  free(wideQuote);
  free(quote);

  return 0;
}
