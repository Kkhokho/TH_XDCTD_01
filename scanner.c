#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "charcode.h"
#include "token.h"
#include "error.h"

extern int lineNo;
extern int colNo;
extern int currentChar;

extern CharCode charCodes[];

/***************************************************************/

void skipBlank()
{
  while (currentChar != -1 && charCodes[currentChar] == CHAR_SPACE)
    readChar();
}

void skipComment() {
  while (1) {
    // Read next character
    readChar();

    if (currentChar == -1) { // End of File
      error(ERR_ENDOFCOMMENT, lineNo, colNo); // Comment end with EOF
    } else if (charCodes[currentChar] == CHAR_TIMES) { // Next is asterick character
      readChar(); // Get next character

      if (currentChar == -1) { // End of File
        error(ERR_ENDOFCOMMENT, lineNo, colNo); // Comment end with EOF
      } else if (charCodes[currentChar] == CHAR_RPAR) { // Next is right parenthesis
        // End of comment
        readChar();
        return;
      }
    }
  }
}

void skipLineComment() {
  while (currentChar != EOF && currentChar != '\n') {
    readChar();
  }
  if (currentChar != EOF) {
    readChar(); // Skip the newline character
  }
}

Token *readIdentKeyword(void)
{
  Token *token = makeToken(TK_IDENT, lineNo, colNo);
  int count = 0;

  while ((currentChar != EOF) &&
         ((charCodes[currentChar] == CHAR_LETTER) ||
          (charCodes[currentChar] == CHAR_DIGIT)))
  {

    if (count <= MAX_IDENT_LEN) {
      token->string[count++] = (char)currentChar;
    }
    readChar();
  }
  // End string
  token->string[count] = '\0';

  if (count > MAX_IDENT_LEN)
  {
    error(ERR_IDENTTOOLONG, token->lineNo, token->colNo);
    return token;
  }
  else
  {
    TokenType type = checkKeyword(token->string);
    if (type != TK_NONE)
    {
      token->tokenType = type;
    }
  }

  return token;
}

Token* readNumber(void) {
    int count = 0;
    Token *token = makeToken(TK_NUMBER, lineNo, colNo);

    while ((currentChar != EOF) && (charCodes[currentChar] == CHAR_DIGIT)) {
        // Check for number length overflow
        if (count > MAX_IDENT_LEN) {
            error(ERR_NUMBERTOOLONG, token->lineNo, token->colNo);
            break;
        }

        token->string[count] = (char)currentChar;
        count++;
        readChar();
    }

    token->string[count] = '\0';

    long value = atol(token->string);
    token->value = (int)value;

    return token;
}

Token* readConstChar(void) {
  int ln = lineNo;
  int cn = colNo;
  Token* token = makeToken(TK_CHAR, ln, cn);

  // Check if it's a string literal
  if (charCodes[currentChar] == CHAR_DOUBLEQUOTE) {
    token->tokenType = TK_STRING;
    readChar(); // Skip opening quote
    int count = 0;
    
    while (currentChar != EOF && charCodes[currentChar] != CHAR_DOUBLEQUOTE) {
      if (count < MAX_IDENT_LEN) {
        token->string[count++] = currentChar;
      }
      readChar();
    }
    
    if (currentChar == EOF) {
      error(ERR_INVALIDCHARCONSTANT, ln, cn);
    } else {
      token->string[count] = '\0';
      readChar(); // Skip closing quote
    }
    return token;
  }

  // Handle character literal
  readChar(); // Skip opening quote
  
  if (currentChar == EOF) {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
    return token;
  }

  // Handle single quote escape
  if (charCodes[currentChar] == CHAR_SINGLEQUOTE) {
    readChar();
    if (currentChar != EOF && charCodes[currentChar] == CHAR_SINGLEQUOTE) {
      token->string[0] = '\'';
      token->string[1] = '\0';
      readChar();
      if (charCodes[currentChar] == CHAR_SINGLEQUOTE) {
        readChar();
        return token;
      }
    }
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
    return token;
  }

  // Regular character
  token->string[0] = currentChar;
  token->string[1] = '\0';
  readChar();
  
  if (currentChar == EOF || charCodes[currentChar] != CHAR_SINGLEQUOTE) {
    error(ERR_INVALIDCHARCONSTANT, ln, cn);
    return token;
  }
  
  readChar(); // Skip closing quote
  return token;
}

Token *getToken(void)
{
  Token *token;
  // int ln, cn;

  if (currentChar == EOF)
    return makeToken(TK_EOF, lineNo, colNo);

  switch (charCodes[currentChar])
  {
  case CHAR_SPACE:
    skipBlank();
    return getToken();
  case CHAR_LETTER:
    return readIdentKeyword();
  case CHAR_DIGIT:
    return readNumber();
  case CHAR_PLUS:
    token = makeToken(SB_PLUS, lineNo, colNo);
    readChar();
    return token;
  case CHAR_MINUS:
    // Token Minus
    token = makeToken(SB_MINUS, lineNo, colNo);
    readChar();
    return token;
  case CHAR_TIMES:
    // Token Times
    token = makeToken(SB_TIMES, lineNo, colNo);
    readChar();
    return token;
  case CHAR_SLASH:
    readChar();
    if (currentChar == '/') {
      skipLineComment();
      return getToken();
    } else {
      token = makeToken(SB_SLASH, lineNo, colNo-1);
      return token;
    }
  case CHAR_MOD:
    token = makeToken(SB_MOD, lineNo, colNo);
    readChar(); 
    return token;
  case CHAR_EQ:
    // Token Equal
    token = makeToken(SB_EQ, lineNo, colNo);
    readChar();
    return token;
  case CHAR_COMMA:
    // Token Comma
    token = makeToken(SB_COMMA, lineNo, colNo);
    readChar();
    return token;
  case CHAR_SEMICOLON:
    // Token Semicolon
    token = makeToken(SB_SEMICOLON, lineNo, colNo);
    readChar();
    return token;
  case CHAR_RPAR:
    // Token Right Parenthesis
    token = makeToken(SB_RPAR, lineNo, colNo);
    readChar();
    return token;
  case CHAR_LPAR:
    token = makeToken(TK_NONE, lineNo, colNo);
    readChar();

    switch (charCodes[currentChar]) {
        case CHAR_PERIOD:
            token->tokenType = SB_LSEL;
            readChar();
            return token;
        case CHAR_TIMES:
            free(token);
            skipComment();
            return getToken();
        case CHAR_SPACE:
            token->tokenType = SB_LPAR;
            return token;
        default:
            token->tokenType = SB_LPAR;
            return token;
    }
  case CHAR_GT:
    // Token Greater
    token = makeToken(SB_GT, lineNo, colNo);

    // If next character is '='
    readChar();
    if (charCodes[currentChar] == CHAR_EQ)
    {
      // Token is Greater Than
      token->tokenType = SB_GE;
      readChar();
    }
    return token;
  case CHAR_LT:
    // Empty token
    token = makeToken(TK_NONE, lineNo, colNo);

    // Check next character
    readChar();
    switch (charCodes[currentChar])
    {
    case CHAR_EQ:
      // Token Lest Than or Equal
      token->tokenType = SB_LE;
      readChar();
      return token;
    default:
      // Token Lest Than
      token->tokenType = SB_LT;
      return token;
    }
  case CHAR_EXCLAIMATION:
    // Make empty token
    token = makeToken(TK_NONE, lineNo, colNo);

    // If next character is not '='
    readChar();
    if (charCodes[currentChar] != CHAR_EQ)
    {
      // it is an invalid token
      error(ERR_INVALIDSYMBOL, token->lineNo, token->colNo);
    }
    else
    {
      // else, it's token Not Equal
      token->tokenType = SB_NEQ;
      readChar();
    }
    return token;
  case CHAR_PERIOD:
    // Token Period
    token = makeToken(SB_PERIOD, lineNo, colNo);

    // If next character is Right Parenthesis
    readChar();
    if (charCodes[currentChar] == CHAR_RPAR)
    {
      // it is token Right Parenthesis
      token->tokenType = SB_RSEL;
      readChar();
    }
    return token;
  case CHAR_COLON:
    // Token Semicolon
    token = makeToken(SB_COLON, lineNo, colNo);

    // If next character is Equal
    readChar();
    if (charCodes[currentChar] == CHAR_EQ)
    {
      // it is token Assignment
      token->tokenType = SB_ASSIGN;
      readChar();
    }
    return token;
  case CHAR_SINGLEQUOTE:
    return readConstChar();
  case CHAR_DOUBLEQUOTE:
    return readConstChar();
  default:
    token = makeToken(TK_NONE, lineNo, colNo);
    error(ERR_INVALIDSYMBOL, lineNo, colNo);
    readChar();
    return token;
  }
}

/******************************************************************/

void printToken(Token *token)
{

  printf("%d-%d:", token->lineNo, token->colNo);

  switch (token->tokenType)
  {
  case TK_NONE:
    printf("TK_NONE\n");
    break;
  case TK_IDENT:
    printf("TK_IDENT(%s)\n", token->string);
    break;
  case TK_NUMBER:
    printf("TK_NUMBER(%d)\n", token->value);
    break;
  case TK_STRING:
    printf("TK_STRING(%s)\n", token->string);
    break;
  case TK_CHAR:
    printf("TK_CHAR(\'%s\')\n", token->string);
    break;
  case TK_EOF:
    printf("TK_EOF\n");
    break;

  case KW_PROGRAM:
    printf("KW_PROGRAM\n");
    break;
  case KW_CONST:
    printf("KW_CONST\n");
    break;
  case KW_TYPE:
    printf("KW_TYPE\n");
    break;
  case KW_VAR:
    printf("KW_VAR\n");
    break;
  case KW_INTEGER:
    printf("KW_INTEGER\n");
    break;
  case KW_CHAR:
    printf("KW_CHAR\n");
    break;
  case KW_ARRAY:
    printf("KW_ARRAY\n");
    break;
  case KW_OF:
    printf("KW_OF\n");
    break;
  case KW_FUNCTION:
    printf("KW_FUNCTION\n");
    break;
  case KW_PROCEDURE:
    printf("KW_PROCEDURE\n");
    break;
  case KW_BEGIN:
    printf("KW_BEGIN\n");
    break;
  case KW_END:
    printf("KW_END\n");
    break;
  case KW_CALL:
    printf("KW_CALL\n");
    break;
  case KW_IF:
    printf("KW_IF\n");
    break;
  case KW_THEN:
    printf("KW_THEN\n");
    break;
  case KW_ELSE:
    printf("KW_ELSE\n");
    break;
  case KW_WHILE:
    printf("KW_WHILE\n");
    break;
  case KW_DO:
    printf("KW_DO\n");
    break;
  case KW_FOR:
    printf("KW_FOR\n");
    break;
  case KW_TO:
    printf("KW_TO\n");
    break;
  case KW_STRING:
    printf("KW_STRING\n");
    break;

  case SB_SEMICOLON:
    printf("SB_SEMICOLON\n");
    break;
  case SB_COLON:
    printf("SB_COLON\n");
    break;
  case SB_PERIOD:
    printf("SB_PERIOD\n");
    break;
  case SB_COMMA:
    printf("SB_COMMA\n");
    break;
  case SB_ASSIGN:
    printf("SB_ASSIGN\n");
    break;
  case SB_EQ:
    printf("SB_EQ\n");
    break;
  case SB_NEQ:
    printf("SB_NEQ\n");
    break;
  case SB_LT:
    printf("SB_LT\n");
    break;
  case SB_LE:
    printf("SB_LE\n");
    break;
  case SB_GT:
    printf("SB_GT\n");
    break;
  case SB_GE:
    printf("SB_GE\n");
    break;
  case SB_PLUS:
    printf("SB_PLUS\n");
    break;
  case SB_MINUS:
    printf("SB_MINUS\n");
    break;
  case SB_TIMES:
    printf("SB_TIMES\n");
    break;
  case SB_SLASH:
    printf("SB_SLASH\n");
    break;
  case SB_LPAR:
    printf("SB_LPAR\n");
    break;
  case SB_RPAR:
    printf("SB_RPAR\n");
    break;
  case SB_LSEL:
    printf("SB_LSEL\n");
    break;
  case SB_RSEL:
    printf("SB_RSEL\n");
    break;
  case SB_MOD:
    printf("SB_MOD\n");
    break;
  }
}

int scan(char *fileName)
{
  Token *token;

  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  token = getToken();
  while (token->tokenType != TK_EOF)
  {
    printToken(token);
    free(token);
    token = getToken();
  }

  free(token);
  closeInputStream();
  return IO_SUCCESS;
}

/******************************************************************/

int main(int argc, char *argv[])
{
  if (argc <= 1)
  {
    printf("scanner: no input file.\n");
    return -1;
  }

  if (scan(argv[1]) == IO_ERROR)
  {
    printf("Can\'t read input file!\n");
    return -1;
  }

  return 0;
}