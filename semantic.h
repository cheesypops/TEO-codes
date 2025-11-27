#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* Inicia el análisis semántico sobre el árbol comenzando en 'root' */
void semantic_analysis(ASTNode *root);

/* Retorna el número de errores encontrados */
int get_semantic_errors();

#endif