//
// Created by serpentspirale on 13/01/2022.
//

#ifndef UNTITLED_SHADERCONV_H
#define UNTITLED_SHADERCONV_H

#include "../shader.h"

void set_uniforms_default_value(GLuint program, uniforms_declarations uniformVector, int uniformCount);
char* process_uniform_declarations(char* glslCode, uniforms_declarations uniformVector, int* uniformCount);
char* ConvertShaderVgpu(struct shader_s* shader_source);

char * GLSLHeader(char* source);
char * RemoveConstInsideBlocks(char* source, int * sourceLength);
char * ForceIntegerArrayAccess(char* source, int * sourceLength);
char * CoerceIntToFloat(char * source, int * sourceLength);
char * ReplaceModOperator(char * source, int * sourceLength);
char * WrapIvecFunctions(char * source, int * sourceLength);
char * WrapFunction(char * source, int * sourceLength, char * functionName, char * wrapperFunctionName, char * wrapperFunction);
int FindPositionAfterDirectives(char * source);
int FindPositionAfterVersion(char * source);
char * ReplaceGLFragData(char * source, int * sourceLength);
char * ReplaceGLFragColor(char * source, int * sourceLength);
char * ReplaceVariableName(char * source, int * sourceLength, char * initialName, char* newName);
char * ReplaceFunctionName(char * source, int * sourceLength, char * initialName, char * finalName);
char * RemoveUnsupportedExtensions(char * source);
int doesShaderVersionContainsES(const char * source);
char *ReplacePrecisionQualifiers(char *source, int *sourceLength, int isVertex);
char * GetClosingTokens(char openingToken);
int isOpeningToken(char openingToken);
int GetClosingTokenPosition(const char * source, int initialTokenPosition);
int GetClosingTokenPositionTokenOverride(const char * source, int initialTokenPosition, char initialToken);
int GetNextTokenPosition(const char * source, int initialPosition, char token, const char * acceptedChars);
void GetNextWord(char *source, int startPoint, int * startWord, int * endWord);
unsigned long strstrPos(const char * haystack, const char * needle);
char * insertIntAtFunctionCall(char * source, int * sourceSize, const char * functionName, int argumentPosition);
char * InsertExtension(char * source, int * sourceLength, int insertPoint, const char * extension);
char * InsertExtensions(char *source, int *sourceLength);
int GetShaderVersion(const char * source);
char * ReplaceFragmentOut(char * source, int *sourceLength);
char * BackportConstArrays(char *source, int * sourceLength);
char * ExtractString(char * source, int startString, int endString);
char* ProcessSwitchCases(char* source, int* length);


char* GetOperandFromOperator(char* source, int operatorIndex, int rightOperand, int * limit);

#endif //UNTITLED_SHADERCONV_H
