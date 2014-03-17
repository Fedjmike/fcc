#pragma once

#include "stream.h"

typedef enum tokenTag {
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

typedef enum keywordTag {
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

typedef enum punctTag {
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
    punctModulo, punctModuloAssign
} punctTag;

typedef struct lexerCtx {
    streamCtx* stream;
    int line, lineChar;

    tokenTag token;
    keywordTag keyword;
    punctTag punct;

    char* buffer;
    int bufferSize;
    int length;
} lexerCtx;

lexerCtx* lexerInit (FILE* file);
void lexerEnd (lexerCtx* ctx);

void lexerNext (lexerCtx* ctx);

const char* keywordTagGetStr (keywordTag tag);
const char* punctTagGetStr (punctTag tag);
