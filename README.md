à faire : codegen.h et codegen.c
Traverser l'AST produit par structfe.y et produire du STRUCIT-backend.
1. Aplatir les expressions en tmp _t1, _t2
2. Convertir les if-else en goto
3. Convertir les for-while en goto