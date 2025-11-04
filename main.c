#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <locale.h>
#include <ncursesw/curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

typedef struct
{
  wchar_t *text;
  size_t length;
  int width;
} QuoteSegment;

typedef struct
{
  char *author;
  char *textColorName;
  char *rectangleColorName;
  char *authorColorName;
  int firstQuoteIndex;
  bool hasQuoteFromArgs;
} CLIOptions;

typedef enum
{
  PARSE_OK,
  PARSE_SHOW_HELP,
  PARSE_SHOW_VERSION,
  PARSE_ERROR
} ParseResult;

typedef struct
{
  bool specified;
  short color;
} ColorChoice;

static char *
DuplicateString(const char *input)
{
  if (input == NULL)
    {
      return NULL;
    }

  char *copy = strdup(input);
  if (copy == NULL)
    {
      fprintf(stderr, "Failed to allocate memory for option value.\n");
      exit(EXIT_FAILURE);
    }

  return copy;
}

static void
FreeCLIOptions(CLIOptions *options)
{
  if (options == NULL)
    {
      return;
    }

  free(options->author);
  options->author = NULL;
  free(options->textColorName);
  options->textColorName = NULL;
  free(options->rectangleColorName);
  options->rectangleColorName = NULL;
  free(options->authorColorName);
  options->authorColorName = NULL;
}

static ParseResult
ParseArguments(int argc, char *argv[], CLIOptions *options)
{
  if (options == NULL)
    {
      return PARSE_ERROR;
    }

  options->author = NULL;
  options->textColorName = NULL;
  options->rectangleColorName = NULL;
  options->authorColorName = NULL;
  options->firstQuoteIndex = argc;
  options->hasQuoteFromArgs = false;

  bool endOfOptions = false;

  for (int i = 1; i < argc; ++i)
    {
      char *arg = argv[i];

      if (!endOfOptions && strcmp(arg, "--") == 0)
        {
          if (i + 1 < argc)
            {
              options->firstQuoteIndex = i + 1;
              options->hasQuoteFromArgs = true;
            }
          else
            {
              options->firstQuoteIndex = argc;
              options->hasQuoteFromArgs = false;
            }
          break;
        }

      if (!endOfOptions && arg[0] == '-' && arg[1] != '\0')
        {
          if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
            {
              return PARSE_SHOW_HELP;
            }

          if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0)
            {
              return PARSE_SHOW_VERSION;
            }

          if (strcmp(arg, "-a") == 0 || strcmp(arg, "--author") == 0)
            {
              if (i + 1 >= argc)
                {
                  fprintf(stderr, "Option %s requires an argument.\n", arg);
                  return PARSE_ERROR;
                }

              free(options->author);
              options->author = DuplicateString(argv[++i]);
              continue;
            }

          if (strncmp(arg, "--author=", 9) == 0)
            {
              free(options->author);
              options->author = DuplicateString(arg + 9);
              continue;
            }

          if (strcmp(arg, "-ct") == 0 || strcmp(arg, "--colortext") == 0)
            {
              if (i + 1 >= argc)
                {
                  fprintf(stderr, "Option %s requires an argument.\n", arg);
                  return PARSE_ERROR;
                }

              free(options->textColorName);
              options->textColorName = DuplicateString(argv[++i]);
              continue;
            }

          if (strncmp(arg, "--colortext=", 12) == 0)
            {
              free(options->textColorName);
              options->textColorName = DuplicateString(arg + 12);
              continue;
            }

          if (strcmp(arg, "-cr") == 0 || strcmp(arg, "--colorrectangle") == 0)
            {
              if (i + 1 >= argc)
                {
                  fprintf(stderr, "Option %s requires an argument.\n", arg);
                  return PARSE_ERROR;
                }

              free(options->rectangleColorName);
              options->rectangleColorName = DuplicateString(argv[++i]);
              continue;
            }

          if (strncmp(arg, "--colorrectangle=", 18) == 0)
            {
              free(options->rectangleColorName);
              options->rectangleColorName = DuplicateString(arg + 18);
              continue;
            }

          if (strcmp(arg, "-ca") == 0 || strcmp(arg, "--colorauth") == 0)
            {
              if (i + 1 >= argc)
                {
                  fprintf(stderr, "Option %s requires an argument.\n", arg);
                  return PARSE_ERROR;
                }

              free(options->authorColorName);
              options->authorColorName = DuplicateString(argv[++i]);
              continue;
            }

          if (strncmp(arg, "--colorauth=", 13) == 0)
            {
              free(options->authorColorName);
              options->authorColorName = DuplicateString(arg + 13);
              continue;
            }

          fprintf(stderr, "Unknown option: %s\n", arg);
          return PARSE_ERROR;
        }

      options->firstQuoteIndex = i;
      options->hasQuoteFromArgs = true;
      break;
    }

  return PARSE_OK;
}

static bool
ParseColorName(const char *name, short *outColor)
{
  if (name == NULL || outColor == NULL)
    {
      return false;
    }

  static const struct
  {
    const char *name;
    short value;
  } colorMap[] = {
    { "black", COLOR_BLACK },   { "red", COLOR_RED },
    { "green", COLOR_GREEN },   { "yellow", COLOR_YELLOW },
    { "blue", COLOR_BLUE },     { "magenta", COLOR_MAGENTA },
    { "cyan", COLOR_CYAN },     { "white", COLOR_WHITE },
    { "default", -1 },          { "grey", COLOR_WHITE },
    { "gray", COLOR_WHITE }
  };

  for (size_t i = 0; i < sizeof(colorMap) / sizeof(colorMap[0]); ++i)
    {
      if (strcasecmp(name, colorMap[i].name) == 0)
        {
          *outColor = colorMap[i].value;
          return true;
        }
    }

  char *end = NULL;
  long numeric = strtol(name, &end, 10);
  if (end != name && *end == '\0' && numeric >= -1 && numeric <= 255)
    {
      *outColor = (short)numeric;
      return true;
    }

  return false;
}

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

static int
MeasureWide(const wchar_t *text, size_t length)
{
  if (text == NULL)
    {
      return 0;
    }

  int width = 0;
  for (size_t i = 0; i < length && text[i] != L'\0'; ++i)
    {
      width += CharWidth(text[i]);
    }

  return width;
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
RenderQuoteWithFrame(const QuoteSegment *segments, size_t count, const wchar_t *author,
                     size_t authorLength, short textPair, short rectanglePair,
                     short authorPair)
{
  int totalRows = 0;
  int totalCols = 0;
  getmaxyx(stdscr, totalRows, totalCols);

  int maxWidth = 0;
  for (size_t i = 0; i < count; ++i)
    {
      if (segments[i].width > maxWidth)
        {
          maxWidth = segments[i].width;
        }
    }

  size_t authorLabelLength = 0;
  int authorLabelWidth = 0;
  wchar_t *authorLabel = NULL;

  if (author != NULL && authorLength > 0)
    {
      authorLabelLength = authorLength + 2;
      authorLabel = malloc((authorLabelLength + 1) * sizeof(wchar_t));
      if (authorLabel == NULL)
        {
          endwin();
          fprintf(stderr, "Failed to allocate memory for author label.\n");
          exit(EXIT_FAILURE);
        }

      authorLabel[0] = L' ';
      wmemcpy(authorLabel + 1, author, authorLength);
      authorLabel[authorLabelLength - 1] = L' ';
      authorLabel[authorLabelLength] = L'\0';
      authorLabelWidth = MeasureWide(authorLabel, authorLabelLength);
    }

  int contentRows = count > 0 ? (int)count : 1;
  int contentWidth = maxWidth;
  if (authorLabelWidth > contentWidth)
    {
      contentWidth = authorLabelWidth;
    }

  int horizontalPadding = 2;
  while (horizontalPadding > 0
         && contentWidth + horizontalPadding * 2 + 2 > totalCols)
    {
      horizontalPadding--;
    }

  int verticalPadding = 1;
  while (verticalPadding > 0
         && contentRows + verticalPadding * 2 + 2 > totalRows)
    {
      verticalPadding--;
    }

  int innerWidth = contentWidth + horizontalPadding * 2;
  int innerHeight = contentRows + verticalPadding * 2;

  int boxWidth = innerWidth + 2;
  int boxHeight = innerHeight + 2;

  if (boxWidth > totalCols)
    {
      boxWidth = totalCols;
      innerWidth = boxWidth - 2;
    }

  if (boxHeight > totalRows)
    {
      boxHeight = totalRows;
      innerHeight = boxHeight - 2;
    }

  if (boxWidth < 2)
    {
      boxWidth = 2;
      innerWidth = boxWidth - 2;
    }

  if (boxHeight < 2)
    {
      boxHeight = 2;
      innerHeight = boxHeight - 2;
    }

  int startRow = MidRow(totalRows, boxHeight);
  int startCol = MidCol(totalCols, boxWidth);

  if (rectanglePair > 0)
    {
      attron(COLOR_PAIR(rectanglePair));
    }

  mvaddch(startRow, startCol, ACS_ULCORNER);
  mvaddch(startRow, startCol + boxWidth - 1, ACS_URCORNER);
  mvaddch(startRow + boxHeight - 1, startCol, ACS_LLCORNER);
  mvaddch(startRow + boxHeight - 1, startCol + boxWidth - 1, ACS_LRCORNER);

  if (boxWidth > 2)
    {
      mvhline(startRow, startCol + 1, ACS_HLINE, boxWidth - 2);
      mvhline(startRow + boxHeight - 1, startCol + 1, ACS_HLINE, boxWidth - 2);
    }

  for (int row = 1; row < boxHeight - 1; ++row)
    {
      mvaddch(startRow + row, startCol, ACS_VLINE);
      mvaddch(startRow + row, startCol + boxWidth - 1, ACS_VLINE);
    }

  if (rectanglePair > 0)
    {
      attroff(COLOR_PAIR(rectanglePair));
    }

  int innerLeft = startCol + 1;
  int innerTop = startRow + 1;

  int totalPaddingCols = innerWidth - contentWidth;
  if (totalPaddingCols < 0)
    {
      totalPaddingCols = 0;
    }
  int leftPadding = totalPaddingCols / 2;

  int totalPaddingRows = innerHeight - contentRows;
  if (totalPaddingRows < 0)
    {
      totalPaddingRows = 0;
    }
  int topPadding = totalPaddingRows / 2;

  if (authorLabel != NULL)
    {
      int labelOffset = (innerWidth - authorLabelWidth) / 2;
      if (labelOffset < 0)
        {
          labelOffset = 0;
        }

      if (authorPair > 0)
        {
          attron(COLOR_PAIR(authorPair));
        }

      mvaddnwstr(startRow, innerLeft + labelOffset, authorLabel,
                 (int)authorLabelLength);

      if (authorPair > 0)
        {
          attroff(COLOR_PAIR(authorPair));
        }
    }

  if (segments != NULL && count > 0)
    {
      int baseCol = innerLeft + leftPadding;
      int textRow = innerTop + topPadding;

      for (size_t i = 0; i < count && textRow < startRow + boxHeight - 1; ++i)
        {
          if (segments[i].length == 0)
            {
              textRow++;
              continue;
            }

          int lineOffset = (contentWidth - segments[i].width) / 2;
          if (lineOffset < 0)
            {
              lineOffset = 0;
            }

          int lineCol = baseCol + lineOffset;
          if (lineCol < innerLeft)
            {
              lineCol = innerLeft;
            }

          if (textPair > 0)
            {
              attron(COLOR_PAIR(textPair));
            }

          mvaddnwstr(textRow, lineCol, segments[i].text, segments[i].length);

          if (textPair > 0)
            {
              attroff(COLOR_PAIR(textPair));
            }

          textRow++;
        }
    }

  free(authorLabel);
}


static void
FreeSegments(QuoteSegment *segments)
{
  free(segments);
}

static char *
JoinArgs(int argc, char *argv[], int startIndex)
{
  size_t totalLength = 0;

  if (startIndex < 0 || startIndex >= argc)
    {
      return NULL;
    }

  for (int i = startIndex; i < argc; ++i)
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
  for (int i = startIndex; i < argc; ++i)
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
  fprintf(stderr, "Usage: %s [OPTIONS] [QUOTE...]\n", programName);
  fprintf(stderr, "       %s --help\n", programName);
  fprintf(stderr, "       %s --version\n", programName);
  fprintf(stderr, "       (pipe text to %s to read from standard input)\n", programName);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  -a,  --author <name>           Display an author centered above the quote.\n");
  fprintf(stderr, "  -ct, --colortext <color>      Set the quote text color.\n");
  fprintf(stderr, "  -cr, --colorrectangle <color> Set the border color.\n");
  fprintf(stderr, "  -ca, --colorauth <color>      Set the author label color.\n");
  fprintf(stderr, "      --                        Treat following arguments as quote text.\n");
  fprintf(stderr,
          "\nColors accept standard names (black, red, green, yellow, blue, magenta, cyan, "
          "white),\n");
  fprintf(stderr,
          "or numeric ncurses color indexes. Use 'default' to keep the terminal's "
          "palette.\n");
}

static void
PrintVersion(void)
{
  printf("fancyquote 1.0\n");
}

int
main(int argc, char *argv[])
{
  bool waitForKey = isatty(STDIN_FILENO);

  if (setlocale(LC_ALL, "") == NULL)
    {
      if (setlocale(LC_ALL, "en_US.UTF-8") == NULL)
        {
          fprintf(stderr,
                  "Warning: failed to set locale, continuing with default settings.\n");
        }
    }

  CLIOptions options;
  ParseResult parseResult = ParseArguments(argc, argv, &options);
  if (parseResult == PARSE_SHOW_HELP)
    {
      PrintUsage(argv[0]);
      FreeCLIOptions(&options);
      return 0;
    }

  if (parseResult == PARSE_SHOW_VERSION)
    {
      PrintVersion();
      FreeCLIOptions(&options);
      return 0;
    }

  if (parseResult == PARSE_ERROR)
    {
      PrintUsage(argv[0]);
      FreeCLIOptions(&options);
      return 1;
    }

  char *quote = NULL;
  if (options.hasQuoteFromArgs)
    {
      quote = JoinArgs(argc, argv, options.firstQuoteIndex);
    }
  else
    {
      quote = ReadQuoteFromStdin();
      if (quote == NULL)
        {
          PrintUsage(argv[0]);
          FreeCLIOptions(&options);
          return 1;
        }
    }

  if (quote == NULL)
    {
      fprintf(stderr, "No quote provided.\n");
      FreeCLIOptions(&options);
      return 1;
    }

  ColorChoice textColor = { false, 0 };
  ColorChoice rectangleColor = { false, 0 };
  ColorChoice authorColor = { false, 0 };

  if (options.textColorName != NULL)
    {
      if (!ParseColorName(options.textColorName, &textColor.color))
        {
          fprintf(stderr, "Unknown text color: %s\n", options.textColorName);
          FreeCLIOptions(&options);
          free(quote);
          return 1;
        }
      textColor.specified = true;
    }

  if (options.rectangleColorName != NULL)
    {
      if (!ParseColorName(options.rectangleColorName, &rectangleColor.color))
        {
          fprintf(stderr, "Unknown rectangle color: %s\n", options.rectangleColorName);
          FreeCLIOptions(&options);
          free(quote);
          return 1;
        }
      rectangleColor.specified = true;
    }

  if (options.authorColorName != NULL)
    {
      if (!ParseColorName(options.authorColorName, &authorColor.color))
        {
          fprintf(stderr, "Unknown author color: %s\n", options.authorColorName);
          FreeCLIOptions(&options);
          free(quote);
          return 1;
        }
      authorColor.specified = true;
    }

  wchar_t *wideQuote = ConvertToWide(quote);
  wchar_t *wideAuthor = NULL;
  size_t authorLength = 0;

  if (options.author != NULL && options.author[0] != '\0')
    {
      wideAuthor = ConvertToWide(options.author);
      authorLength = wcslen(wideAuthor);
    }

  initscr();
  clear();
  noecho();
  curs_set(0);

  bool warnAboutColors = false;
  short textPair = 0;
  short rectanglePair = 0;
  short authorPair = 0;

  if (has_colors())
    {
      start_color();
      bool supportsDefault = use_default_colors() != ERR;
      short nextPair = 1;
      short background = supportsDefault ? -1 : COLOR_BLACK;

      if (textColor.specified && textColor.color != -1)
        {
          if ((textColor.color >= 0 && textColor.color >= COLORS)
              || init_pair(nextPair, textColor.color, background) == ERR)
            {
              warnAboutColors = true;
            }
          else
            {
              textPair = nextPair++;
            }
        }

      if (rectangleColor.specified && rectangleColor.color != -1)
        {
          if ((rectangleColor.color >= 0 && rectangleColor.color >= COLORS)
              || init_pair(nextPair, rectangleColor.color, background) == ERR)
            {
              warnAboutColors = true;
            }
          else
            {
              rectanglePair = nextPair++;
            }
        }

      if (authorColor.specified && authorColor.color != -1)
        {
          if ((authorColor.color >= 0 && authorColor.color >= COLORS)
              || init_pair(nextPair, authorColor.color, background) == ERR)
            {
              warnAboutColors = true;
            }
          else
            {
              authorPair = nextPair++;
            }
        }
    }
  else if (textColor.specified || rectangleColor.specified || authorColor.specified)
    {
      warnAboutColors = true;
    }

  int totalCols = getmaxx(stdscr);
  int wrapWidth = totalCols - 6;
  if (wrapWidth < 1)
    {
      wrapWidth = totalCols - 2;
      if (wrapWidth < 1)
        {
          wrapWidth = totalCols;
        }
    }

  size_t segmentCount = 0;
  QuoteSegment *segments = SplitQuoteIntoSegments(wideQuote, wrapWidth, &segmentCount);

  RenderQuoteWithFrame(segments, segmentCount, wideAuthor, authorLength, textPair,
                       rectanglePair, authorPair);

  refresh();
  if (waitForKey)
    {
      getch();
    }
  endwin();

  if (warnAboutColors)
    {
      fprintf(stderr,
              "Warning: some color options could not be applied with this terminal.\n");
    }

  FreeSegments(segments);
  free(wideAuthor);
  free(wideQuote);
  free(quote);
  FreeCLIOptions(&options);

  return 0;
}
