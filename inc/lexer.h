#pragma once

#include "stream.h"

typedef enum {
    tokenUndefined,
    tokenOther,
    tokenEOF,
    tokenKeyword,
    tokenPunct,
    tokenIdent,
    tokenInt,
    tokenStr,
    tokenChar
} tokenTag;

typedef enum {
    keywordUndefined,
    keywordUsing,
    keywordIf, keywordElse, keywordWhile, keywordDo, keywordFor,
    keywordReturn, keywordBreak, keywordContinue,
    keywordSizeof,
    keywordConst,
    keywordAuto, keywordStatic, keywordExtern, keywordTypedef,
    keywordStruct, keywordUnion, keywordEnum,
    keywordVoid, keywordBool, keywordChar, keywordInt,
    keywordTrue, keywordFalse
} keywordTag;

typedef enum {
    punctUndefined,

    punctLBrace,
    punctRBrace,
    punctLParen,
    punctRParen,
    punctLBracket,
    punctRBracket,
    punctSemicolon,
    punctPeriod, punctEllipsis,
    punctComma,

    punctAssign, punctEqual,
    punctLogicalNot, punctNotEqual,
    punctGreater, punctGreaterEqual, punctShr, punctShrAssign,
    punctLess, punctLessEqual, punctShl, punctShlAssign,

    punctQuestion,
    punctColon,

    punctBitwiseAnd, punctBitwiseAndAssign, punctLogicalAnd,
    punctBitwiseOr, punctBitwiseOrAssign, punctLogicalOr,
    punctBitwiseXor, punctBitwiseXorAssign,
    punctBitwiseNot,

    punctPlus, punctPlusAssign, punctPlusPlus,
    punctMinus, punctMinusAssign, punctMinusMinus, punctArrow,
    punctTimes, punctTimesAssign,
    punctDivide, punctDivideAssign,
    punctModulo, punctModuloAssign,
} punctTag;

typedef struct {
    int line;
    int lineChar;
} tokenLocation;

typedef struct  {
    streamCtx* stream;

    tokenTag token;
    keywordTag keyword;
    punctTag punct;

    char* buffer;
    int bufferSize;
    int length;
} lexerCtx;

lexerCtx* lexerInit (FILE* File, const char* filename);
void lexerEnd (lexerCtx* ctx);

tokenLocation lexerNext (lexerCtx* ctx);

const char* keywordTagGetStr (keywordTag tag);
const char* punctTagGetStr (punctTag tag);
